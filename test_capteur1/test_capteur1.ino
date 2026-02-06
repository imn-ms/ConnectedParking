/*
  Fait le 16/01/2026
  Code qui allume la LED rouge ou verte en fonction d’un capteur
  (présence d'un objet à une certaine distance ou non)
*/

const int trigPin = 2;
const int echoPin = 3;
const int LedRPin = A0;
const int LedVPin = A1;

long duree;
int distance;
int compteur = 0;

void setup() {
  pinMode(LedRPin, OUTPUT);
  pinMode(LedVPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.begin(9600);
}

void loop() {
  // Déclenchement du capteur
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Lecture de l'écho
  duree = pulseIn(echoPin, HIGH);

  // Calcul de la distance
  distance = duree * 0.034 / 2;

  // Test de présence
  if (distance < 30) {   // distance inférieure à 30 cm
    digitalWrite(LedVPin, LOW);
    digitalWrite(LedRPin, HIGH);
    compteur = 0;        // place occupée
  } else {
    digitalWrite(LedRPin, LOW);
    digitalWrite(LedVPin, HIGH);
    compteur = 1;        // place libre
  }

  // Affichage série
  Serial.print("Distance : ");
  Serial.print(distance);
  Serial.print(" cm | Compteur : ");
  Serial.println(compteur);

  delay(200);
}
