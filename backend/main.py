from datetime import datetime
from typing import Optional

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from sqlmodel import SQLModel, Field, Session, create_engine, select

app = FastAPI(title="Parking Connecté API + DB")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# --- Base SQLite (un fichier parking.db sera créé dans backend/) ---
engine = create_engine("sqlite:///parking.db")


# --- Tables ---
class Slot(SQLModel, table=True):
    id: int = Field(primary_key=True)  # 1..N
    occupied: bool = False
    last_update: datetime = Field(default_factory=datetime.utcnow)


class ParkingConfig(SQLModel, table=True):
    id: int = Field(default=1, primary_key=True)
    total_spots: int = 4
    price_per_minute: float = 0.05


class ParkingSession(SQLModel, table=True):
    id: Optional[int] = Field(default=None, primary_key=True)
    plate: str
    slot_id: Optional[int] = None
    start_time: datetime
    end_time: Optional[datetime] = None
    paid_amount: Optional[float] = None

class EventLog(SQLModel, table=True):
    id: Optional[int] = Field(default=None, primary_key=True)
    event_type: str
    payload: str
    ts: datetime = Field(default_factory=datetime.utcnow)


def init_db():
    SQLModel.metadata.create_all(engine)
    with Session(engine) as s:
        # config par défaut
        cfg = s.get(ParkingConfig, 1)
        if cfg is None:
            cfg = ParkingConfig(total_spots=4, price_per_minute=0.05)
            s.add(cfg)
            s.commit()

        # slots 1..total_spots
        for i in range(1, cfg.total_spots + 1):
            if s.get(Slot, i) is None:
                s.add(Slot(id=i, occupied=False))
        s.commit()


@app.on_event("startup")
def on_startup():
    init_db()


def compute_amount(start: datetime, end: datetime, ppm: float) -> float:
    minutes = (end - start).total_seconds() / 60
    return round(max(minutes, 0) * ppm, 2)


# --- API pour la PWA / supervision ---
@app.get("/status")
def get_status():
    with Session(engine) as s:
        cfg = s.get(ParkingConfig, 1)
        slots = s.exec(select(Slot).order_by(Slot.id)).all()
        occupied = sum(1 for sl in slots if sl.occupied)
        return {
            "total_spots": cfg.total_spots,
            "occupied": occupied,
            "available": max(cfg.total_spots - occupied, 0),
            "price_per_minute": cfg.price_per_minute,
            "slots": [{"id": sl.id, "occupied": sl.occupied} for sl in slots],
        }


# --- Endpoints appelés par l'Arduino en Ethernet (HTTP) ---
@app.post("/iot/slot")
def update_slot(slot_id: int, occupied: bool, plate: Optional[str] = None):
    """
    Arduino appelle ça quand l'ultrason change d'état.
    - occupied=true  => une voiture vient d'occuper la place
    - occupied=false => la voiture a quitté la place

    plate est optionnel (si vous avez la plaque). Sinon on met UNKNOWN.
    """
    with Session(engine) as s:
        slot = s.get(Slot, slot_id)
        if slot is None:
            raise HTTPException(404, "Slot introuvable")

        old = slot.occupied
        if old == occupied:
            # rien n'a changé, mais on peut quand même logguer si tu veux
            s.add(EventLog(event_type="slot_no_change", payload=f"slot={slot_id}, occupied={occupied}"))
            s.commit()
            return {"ok": True, "slot_id": slot_id, "occupied": occupied, "changed": False}

        # 1) update slot
        slot.occupied = occupied
        slot.last_update = datetime.utcnow()
        s.add(slot)

        # 2) log
        s.add(EventLog(
            event_type="slot_update",
            payload=f"slot={slot_id}, old={old}, new={occupied}, plate={plate or 'UNKNOWN'}"
        ))

        # 3) session auto (simple et efficace)
        if occupied is True:
            # création session si aucune session active sur ce slot
            active = s.exec(
                select(ParkingSession).where(
                    ParkingSession.slot_id == slot_id,
                    ParkingSession.end_time.is_(None)
                )
            ).first()
            if active is None:
                sess = ParkingSession(
                    plate=plate or "UNKNOWN",
                    slot_id=slot_id,
                    start_time=datetime.utcnow()
                )
                s.add(sess)

        else:
            # fermeture session active de ce slot
            active = s.exec(
                select(ParkingSession).where(
                    ParkingSession.slot_id == slot_id,
                    ParkingSession.end_time.is_(None)
                )
            ).first()
            if active is not None:
                active.end_time = datetime.utcnow()
                s.add(active)

        s.commit()
        return {"ok": True, "slot_id": slot_id, "occupied": occupied, "changed": True}

@app.get("/events")
def last_events(limit: int = 20):
    with Session(engine) as s:
        rows = s.exec(select(EventLog).order_by(EventLog.id.desc()).limit(limit)).all()
        return [{"id": e.id, "type": e.event_type, "payload": e.payload, "ts": e.ts} for e in rows]



@app.post("/iot/plate")
def plate_detected(plate: str, slot_id: Optional[int] = None):
    # Arduino / module plaque envoie la plaque détectée (entrée)
    with Session(engine) as s:
        cfg = s.get(ParkingConfig, 1)
        slots = s.exec(select(Slot)).all()
        if sum(1 for sl in slots if sl.occupied) >= cfg.total_spots:
            raise HTTPException(409, "Parking complet")

        sess = ParkingSession(plate=plate, slot_id=slot_id, start_time=datetime.utcnow())
        s.add(sess)
        s.commit()
        s.refresh(sess)
        return {"session_id": sess.id}


@app.post("/sessions/stop")
def stop_session(session_id: int):
    with Session(engine) as s:
        sess = s.get(ParkingSession, session_id)
        if sess is None:
            raise HTTPException(404, "Session introuvable")
        if sess.end_time is not None:
            raise HTTPException(409, "Session déjà terminée")
        sess.end_time = datetime.utcnow()
        s.add(sess)
        s.commit()
        return {"ok": True}


@app.post("/sessions/pay")
def pay_session(session_id: int):
    with Session(engine) as s:
        cfg = s.get(ParkingConfig, 1)
        sess = s.get(ParkingSession, session_id)
        if sess is None:
            raise HTTPException(404, "Session introuvable")
        if sess.end_time is None:
            raise HTTPException(409, "Stop d’abord la session")
        if sess.paid_amount is not None:
            raise HTTPException(409, "Déjà payé")

        amount = compute_amount(sess.start_time, sess.end_time, cfg.price_per_minute)
        sess.paid_amount = amount
        s.add(sess)
        s.commit()
        return {"status": "paid", "amount": amount}

