/*
  Fait le 16/01/2026
  2 capteurs HC-SR04 + 2 LEDs (rouge/verte) chacun
  Rouge = place occupée (objet proche)
  Vert  = place libre

  Affiche le nombre de places disponibles (0..2)
*/

const int trigPin1 = 2;
const int echoPin1 = 3;
const int LedVPin1 = A0;
const int LedRPin1 = A1;

const int SEUIL_CM = 30;                 // distance en cm pour "occupé"
const unsigned long TIMEOUT_US = 60000;  // 60 ms ~ 5 m max, évite les blocages

long distance1 = 0;

long mesureDistanceCM(int trigPin, int echoPin) {
  // Trigger propre
  digitalWrite(trigPin, LOW);
  delayMicroseconds(60);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(60);
  digitalWrite(trigPin, LOW);

  // Mesure de l'écho avec timeout
  unsigned long duree = pulseIn(echoPin, HIGH, TIMEOUT_US);

  // Si timeout (pas d'écho), on retourne -1 (mesure invalide)
  if (duree == 0) return -1;

  // Conversion en cm (vitesse du son ~ 0.034 cm/us)
  long dist = (long)(duree * 0.034 / 2.0);
  return dist;
}

void setLedsPlace(bool occupe, int ledV, int ledR) {
  if (occupe) {
    digitalWrite(ledV, LOW);
    digitalWrite(ledR, HIGH);
  } else {
    digitalWrite(ledR, LOW);
    digitalWrite(ledV, HIGH);
  }
}

void setup() {
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(LedRPin1, OUTPUT);
  pinMode(LedVPin1, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  distance1 = mesureDistanceCM(trigPin1, echoPin1);
  delay(50);

  bool occupe1 = (distance1 != -1 && distance1 < SEUIL_CM);

  // LEDs par place
  setLedsPlace(occupe1, LedVPin1, LedRPin1);

  // Compteur = nombre de places disponibles
  int placesDisponibles = 0;
  if (!occupe1) placesDisponibles++;

  // Debug série
  Serial.print("Distance1: ");
  Serial.print(distance1);
  //Serial.print(" cm | Distance2: ");
  //Serial.print(distance2);
  Serial.print(" cm | Places dispo: ");
  Serial.println(placesDisponibles);

  delay(200);
}
