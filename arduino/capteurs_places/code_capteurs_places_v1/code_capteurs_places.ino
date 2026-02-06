/*
  Fait le 16/01/2026
  Code qui allume les LEDs rouge ou verte en fonction ds capteurs (présence d'un objet à une certaine distance ou non)
*/
const int trigPin=10;
const int echoPin=11;
const int LedRPin=5;
const int LedVPin=4;

long duree;
int distance;

void setup() {
  pinMode(LedRPin, OUTPUT);
  pinMode(LedVPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(9600);

}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duree = pulseIn(echoPin,HIGH);
  distance= duree*0.034/2;

  if (distance<30){
    digitalWrite(LedVPin, LOW);
    digitalWrite(LedRPin, HIGH);
  }
  else{
    digitalWrite(LedRPin, LOW);
    digitalWrite(LedVPin, HIGH);
  }
}
