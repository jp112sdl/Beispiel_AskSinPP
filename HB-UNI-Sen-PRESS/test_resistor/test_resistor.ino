#define ZERO 110
void setup() {
Serial.begin(57600);
pinMode(A0, INPUT);
  // put your setup code here, to run once:

}

void loop() {
  delay(1000);
  int a = analogRead(A0);
  int y = map(a, ZERO, 1023, 0, 1200);
  float val = y / 100.0;
  //val = val * 0.75;
  Serial.println("Analog: "+String(a)+", map: "+String(val,2));
  // put your main code here, to run repeatedly:

}
