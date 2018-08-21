void setup()
{
  delay(5000);
  Serial.begin(115200);
  Serial3.begin(115200);
  pinMode(2,OUTPUT);
  digitalWrite(2, HIGH);
}

void loop()
{
  /* send everything received from the hardware uart to usb serial & vv */
  if (Serial.available() > 0) {
    char ch = Serial.read();
    Serial3.print(ch);
  }
  if (Serial3.available() > 0) {
    char ch = Serial3.read();
    Serial.print(ch);
  }
}
