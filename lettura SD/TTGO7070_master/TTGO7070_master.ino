#define BAUD 115200

void setup() {
  Serial.begin(115200); // USB debug sul TTGO


  delay(1000);
  Serial.println("TTGO Master pronto. Richiedo lista file...");
  Serial.println("LIST"); // invia comando alla CAM
}

void loop() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      Serial.println("Ricevuto: " + line);
      if (line == "END_OF_LIST") {
        delay(5000);
        Serial.println("LIST"); // richiesta successiva
      }
    }
  }
  delay(5000);
}
