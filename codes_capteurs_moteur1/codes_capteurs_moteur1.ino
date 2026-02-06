#include <Servo.h>

// ===================== BARRIERE =====================
Servo monServomoteur;
const int servoPin = 12;

// Capteurs ultrason 1 fil (SIG)
const int capteurAvantPin = 11;
const int capteurApresPin = 10;

// Paramètres barrière
const float SEUIL_BARRIERE_CM = 6.0;          // détection véhicule
const unsigned long OUVERTURE_MIN_MS = 500;  // maintien ouvert après détection

// Etat barrière
bool barriereOuverte = false;
unsigned long dernierInstantDetection = 0;

// ===================== PARKING (3 CAPTEURS + LEDS) =====================
// Capteur 1
const int trigPin1 = 2;
const int echoPin1 = 3;
const int LedRPin1 = A0;
const int LedVPin1 = A1;

// Capteur 2
const int trigPin2 = 4;
const int echoPin2 = 5;
const int LedRPin2 = A2;
const int LedVPin2 = A3;

// Capteur 3
const int trigPin3 = 6;
const int echoPin3 = 7;
const int LedRPin3 = A4;
const int LedVPin3 = A5;

const int SEUIL_PLACE_CM = 5; // comme ton code

int distance1 = -1, distance2 = -1, distance3 = -1;
int placesLibres = 0;

// ===================== TIMERS (sans bloquer) =====================
unsigned long tCapteursBarriere = 0;
unsigned long tCapteursPlaces   = 0;
unsigned long tSerial           = 0;

const unsigned long PERIODE_BARRIERE_MS = 100; // lecture rapide
const unsigned long PERIODE_PLACES_MS   = 200; // comme ton code
const unsigned long PERIODE_SERIAL_MS   = 200; // affichage

// ===================== FONCTIONS ULTRASON =====================

// 1 fil (SIG)
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

// 2 fils (Trig/Echo)
int measureDistanceCm_2wire(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duree = pulseIn(echoPin, HIGH, 30000UL);
  if (duree == 0) return -1;

  // même formule que ton code : duree * 0.034 / 2
  // (0.034 cm/us)
  int dist = (int)(duree * 0.034 / 2.0);
  return dist;
}

// ===================== FONCTIONS METIER =====================

void updateBarriere(unsigned long now) {
  // Lire les 2 capteurs SIG
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

  // Ferme seulement si plus de détection depuis OUVERTURE_MIN_MS
  if (barriereOuverte && (now - dernierInstantDetection >= OUVERTURE_MIN_MS)) {
    barriereOuverte = false;
    monServomoteur.write(180);
  }
}

void updatePlacesEtLeds() {
  // Mesure distances
  distance1 = measureDistanceCm_2wire(trigPin1, echoPin1);
  distance2 = measureDistanceCm_2wire(trigPin2, echoPin2);
  distance3 = measureDistanceCm_2wire(trigPin3, echoPin3);

  // LEDs capteur 1
  if (distance1 > 0 && distance1 < SEUIL_PLACE_CM) {
    digitalWrite(LedVPin1, LOW);
    digitalWrite(LedRPin1, HIGH);
  } else {
    digitalWrite(LedRPin1, LOW);
    digitalWrite(LedVPin1, HIGH);
  }

  // LEDs capteur 2
  if (distance2 > 0 && distance2 < SEUIL_PLACE_CM) {
    digitalWrite(LedVPin2, LOW);
    digitalWrite(LedRPin2, HIGH);
  } else {
    digitalWrite(LedRPin2, LOW);
    digitalWrite(LedVPin2, HIGH);
  }

  // LEDs capteur 3
  if (distance3 > 0 && distance3 < SEUIL_PLACE_CM) {
    digitalWrite(LedVPin3, LOW);
    digitalWrite(LedRPin3, HIGH);
  } else {
    digitalWrite(LedRPin3, LOW);
    digitalWrite(LedVPin3, HIGH);
  }

  // Compteur places libres
  placesLibres = 0;
  if (!(distance1 > 0 && distance1 < SEUIL_PLACE_CM)) placesLibres++;
  if (!(distance2 > 0 && distance2 < SEUIL_PLACE_CM)) placesLibres++;
  if (!(distance3 > 0 && distance3 < SEUIL_PLACE_CM)) placesLibres++;
}

void printStatus() {
  Serial.print("Places: D1=");
  Serial.print(distance1);
  Serial.print("cm D2=");
  Serial.print(distance2);
  Serial.print("cm D3=");
  Serial.print(distance3);
  Serial.print("cm | libres=");
  Serial.print(placesLibres);

  Serial.print(" | Barriere=");
  Serial.print(barriereOuverte ? "OUVERTE" : "FERMEE");
  Serial.println();
}

// ===================== SETUP / LOOP =====================

void setup() {
  Serial.begin(9600);

  // Servo
  monServomoteur.attach(servoPin);
  monServomoteur.write(180);
  barriereOuverte = false;
  dernierInstantDetection = millis();

  // Capteurs parking + LEDs
  pinMode(trigPin1, OUTPUT); pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT); pinMode(echoPin2, INPUT);
  pinMode(trigPin3, OUTPUT); pinMode(echoPin3, INPUT);

  pinMode(LedRPin1, OUTPUT); pinMode(LedVPin1, OUTPUT);
  pinMode(LedRPin2, OUTPUT); pinMode(LedVPin2, OUTPUT);
  pinMode(LedRPin3, OUTPUT); pinMode(LedVPin3, OUTPUT);

  Serial.println("=== SYSTEME BARRIERE + PARKING ===");
}

void loop() {
  unsigned long now = millis();

  // Mise à jour barrière
  if (now - tCapteursBarriere >= PERIODE_BARRIERE_MS) {
    tCapteursBarriere = now;
    updateBarriere(now);
  }

  // Mise à jour places + LEDs
  if (now - tCapteursPlaces >= PERIODE_PLACES_MS) {
    tCapteursPlaces = now;
    updatePlacesEtLeds();
  }

  // Affichage série
  if (now - tSerial >= PERIODE_SERIAL_MS) {
    tSerial = now;
    printStatus();
  }
}
