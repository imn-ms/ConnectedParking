/*
  Fait le 16/01/2026
  Code qui allume les LEDs rouge ou verte en fonction de deux capteurs
  (présence d'un objet à une certaine distance ou non)
*/

// ===== Capteur 1 =====
const int trigPin1 = 2;
const int echoPin1 = 3;
const int LedRPin1 = A0;
const int LedVPin1 = A1;

// ===== Capteur 2 =====
const int trigPin2 = 4;
const int echoPin2 = 5;
const int LedRPin2 = A2;
const int LedVPin2 = A3;

long duree1, duree2;
int distance1, distance2;
int compteur = 0;

void setup() {
  // Capteur 1
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(LedRPin1, OUTPUT);
  pinMode(LedVPin1, OUTPUT);

  // Capteur 2
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(LedRPin2, OUTPUT);
  pinMode(LedVPin2, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  // ===== Mesure capteur 1 =====
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);

  duree1 = pulseIn(echoPin1, HIGH);
  distance1 = duree1 * 0.034 / 2;

  // ===== Mesure capteur 2 =====
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  duree2 = pulseIn(echoPin2, HIGH);
  distance2 = duree2 * 0.034 / 2;

  // ===== LEDs capteur 1 =====
  if (distance1 < 30) {
    digitalWrite(LedVPin1, LOW);
    digitalWrite(LedRPin1, HIGH);
  } else {
    digitalWrite(LedRPin1, LOW);
    digitalWrite(LedVPin1, HIGH);
  }

  // ===== LEDs capteur 2 =====
  if (distance2 < 30) {
    digitalWrite(LedVPin2, LOW);
    digitalWrite(LedRPin2, HIGH);
  } else {
    digitalWrite(LedRPin2, LOW);
    digitalWrite(LedVPin2, HIGH);
  }

  // ===== Compteur (places libres) =====
  compteur = 0;
  if (distance1 >= 30) compteur++;
  if (distance2 >= 30) compteur++;

  // ===== Affichage série =====
  Serial.print("Distance1 : ");
  Serial.print(distance1);
  Serial.print(" cm | Distance2 : ");
  Serial.print(distance2);
  Serial.print(" cm | Places libres : ");
  Serial.println(compteur);

  delay(200);
}
