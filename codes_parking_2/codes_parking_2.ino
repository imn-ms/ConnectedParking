#define BLYNK_TEMPLATE_ID "TMPL5QUa5AsTx"
#define BLYNK_TEMPLATE_NAME "Parking Intelligent"
#define BLYNK_AUTH_TOKEN "ptO45GUuljAa5dfZevREonY8CFQ-44tP"
#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <Servo.h>

// ===================== BARRIERE =====================
Servo monServomoteur;
const int servoPin = 9;

// Capteurs ultrason 1 fil (SIG)
const int capteurAvantPin = 8;
const int capteurApresPin = A7;

const float SEUIL_BARRIERE_CM = 6.0;
const unsigned long OUVERTURE_MIN_MS = 2000;

bool barriereOuverte = false;
bool ouvertureManuelle = false;
unsigned long dernierInstantDetection = 0;

// ===================== PARKING =====================
const int trigPin1 = 2;
const int echoPin1 = 3;
const int LedRPin1 = A0;
const int LedVPin1 = A1;

const int trigPin2 = 5;
const int echoPin2 = 4;
const int LedRPin2 = A2;
const int LedVPin2 = A3;

const int trigPin3 = 6;
const int echoPin3 = 7;
const int LedRPin3 = A4;
const int LedVPin3 = A5;

const int SEUIL_PLACE_CM = 5;

int distance1 = -1, distance2 = -1, distance3 = -1;
int placesLibres = 0;

// ===================== TIMERS =====================
unsigned long tCapteursBarriere = 0;
unsigned long tCapteursPlaces   = 0;
unsigned long tSerial           = 0;

const unsigned long PERIODE_BARRIERE_MS = 100;
const unsigned long PERIODE_PLACES_MS   = 200;
const unsigned long PERIODE_SERIAL_MS   = 500;

BlynkTimer timer;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

bool ethernetOK = false;
bool blynkOK = false;

// ===================== FONCTIONS ULTRASON =====================

unsigned long readEchoDuration_1wire(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(2);
  digitalWrite(pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pin, LOW);

  pinMode(pin, INPUT);
  return pulseIn(pin, HIGH, 30000UL);
}

float durationToCm(unsigned long us) {
  if (us == 0) return -1.0;
  return us / 58.0;
}

int measureDistanceCm_2wire(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duree = pulseIn(echoPin, HIGH, 30000UL);
  if (duree == 0) return -1;

  return (int)(duree * 0.034 / 2.0);
}

// ===================== METIER =====================

void updateBarriere(unsigned long now) {
  if (ouvertureManuelle) {
    if (!barriereOuverte) {
      barriereOuverte = true;
      monServomoteur.write(90);
    }
    return;
  }

  unsigned long tAvant = readEchoDuration_1wire(capteurAvantPin);
  unsigned long tApres = readEchoDuration_1wire(capteurApresPin);

  float dAvant = durationToCm(tAvant);
  float dApres = durationToCm(tApres);

  bool detectAvant = (dAvant > 0 && dAvant < SEUIL_BARRIERE_CM);
  bool detectApres = (dApres > 0 && dApres < SEUIL_BARRIERE_CM);
  bool detect = detectAvant || detectApres;

  if (detect) {
    dernierInstantDetection = now;
    if (!barriereOuverte) {
      barriereOuverte = true;
      monServomoteur.write(90);
    }
  }

  if (barriereOuverte && (now - dernierInstantDetection >= OUVERTURE_MIN_MS)) {
    barriereOuverte = false;
    monServomoteur.write(180);
  }
}

void updatePlacesEtLeds() {
  distance1 = measureDistanceCm_2wire(trigPin1, echoPin1);
  distance2 = measureDistanceCm_2wire(trigPin2, echoPin2);
  distance3 = measureDistanceCm_2wire(trigPin3, echoPin3);

  bool occ1 = (distance1 > 0 && distance1 < SEUIL_PLACE_CM);
  bool occ2 = (distance2 > 0 && distance2 < SEUIL_PLACE_CM);
  bool occ3 = (distance3 > 0 && distance3 < SEUIL_PLACE_CM);

  digitalWrite(LedRPin1, occ1 ? HIGH : LOW);
  digitalWrite(LedVPin1, occ1 ? LOW  : HIGH);

  digitalWrite(LedRPin2, occ2 ? HIGH : LOW);
  digitalWrite(LedVPin2, occ2 ? LOW  : HIGH);

  digitalWrite(LedRPin3, occ3 ? HIGH : LOW);
  digitalWrite(LedVPin3, occ3 ? LOW  : HIGH);

  placesLibres = 0;
  if (!occ1) placesLibres++;
  if (!occ2) placesLibres++;
  if (!occ3) placesLibres++;
}

void printStatus() {
  Serial.print("IP=");
  Serial.print(Ethernet.localIP());
  Serial.print(" | D1=");
  Serial.print(distance1);
  Serial.print(" D2=");
  Serial.print(distance2);
  Serial.print(" D3=");
  Serial.print(distance3);
  Serial.print(" | Libres=");
  Serial.print(placesLibres);
  Serial.print(" | Barriere=");
  Serial.print(barriereOuverte ? "OUVERTE" : "FERMEE");
  Serial.print(" | Ethernet=");
  Serial.print(ethernetOK ? "OK" : "NON");
  Serial.print(" | BlynkConnect=");
  Serial.print(blynkOK ? "OK" : "NON");
  Serial.print(" | BlynkRun=");
  Serial.println(Blynk.connected() ? "OK" : "NON");
}

void sendToBlynk() {
  if (!Blynk.connected()) return;

  bool occ1 = (distance1 > 0 && distance1 < SEUIL_PLACE_CM);
  bool occ2 = (distance2 > 0 && distance2 < SEUIL_PLACE_CM);
  bool occ3 = (distance3 > 0 && distance3 < SEUIL_PLACE_CM);

  Blynk.virtualWrite(V0, placesLibres);
  Blynk.virtualWrite(V1, barriereOuverte ? "OUVERTE" : "FERMEE");
  Blynk.virtualWrite(V2, distance1);
  Blynk.virtualWrite(V3, distance2);
  Blynk.virtualWrite(V4, distance3);
  Blynk.virtualWrite(V5, occ1 ? "Occupee" : "Libre");
  Blynk.virtualWrite(V6, occ2 ? "Occupee" : "Libre");
  Blynk.virtualWrite(V7, occ3 ? "Occupee" : "Libre");
}

BLYNK_WRITE(V8) {
  int etat = param.asInt();
  ouvertureManuelle = (etat == 1);

  if (ouvertureManuelle) {
    barriereOuverte = true;
    monServomoteur.write(90);
  } else {
    barriereOuverte = false;
    monServomoteur.write(180);
  }
}

BLYNK_CONNECTED() {
  Serial.println("Blynk connecte");
  Blynk.syncVirtual(V8);
}

// ===================== SETUP =====================

void setup() {
  Serial.begin(9600);
  delay(1000);

  Serial.println("=== DEMARRAGE PROGRAMME ===");

  monServomoteur.attach(servoPin);
  monServomoteur.write(180);
  Serial.println("Servo initialise");

  pinMode(trigPin1, OUTPUT); pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT); pinMode(echoPin2, INPUT);
  pinMode(trigPin3, OUTPUT); pinMode(echoPin3, INPUT);

  pinMode(LedRPin1, OUTPUT); pinMode(LedVPin1, OUTPUT);
  pinMode(LedRPin2, OUTPUT); pinMode(LedVPin2, OUTPUT);
  pinMode(LedRPin3, OUTPUT); pinMode(LedVPin3, OUTPUT);

  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  Serial.println("Pins initialisees");

  IPAddress ip(172, 30, 1, 200);
  IPAddress dns(172, 30, 1, 254);
  IPAddress gateway(172, 30, 1, 254);
  IPAddress subnet(255, 255, 255, 0);

  Serial.println("Initialisation Ethernet IP fixe...");
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  delay(1000);

  Serial.print("IP = ");
  Serial.println(Ethernet.localIP());

  if (Ethernet.localIP()[0] == 0) {
    Serial.println("ECHEC Ethernet");
    ethernetOK = false;
  } else {
    Serial.println("Ethernet OK");
    ethernetOK = true;
  }

   if (ethernetOK) {
    Serial.println("Configuration Blynk...");
    Blynk.config(BLYNK_AUTH_TOKEN);

    Serial.println("Tentative connexion Blynk (10 s max)...");
    blynkOK = Blynk.connect(10000);

    if (blynkOK) {
      Serial.println("BLYNK CONNECTE AVEC SUCCES");
    } else {
      Serial.println("ECHEC CONNEXION BLYNK");
    }
  }
}

// ===================== LOOP =====================

void loop() {
  if (ethernetOK) {
    Blynk.run();
  }

  timer.run();

  unsigned long now = millis();

  if (now - tCapteursBarriere >= PERIODE_BARRIERE_MS) {
    tCapteursBarriere = now;
    updateBarriere(now);
  }

  if (now - tCapteursPlaces >= PERIODE_PLACES_MS) {
    tCapteursPlaces = now;
    updatePlacesEtLeds();
  }

  if (now - tSerial >= PERIODE_SERIAL_MS) {
    tSerial = now;
    printStatus();
  }
}