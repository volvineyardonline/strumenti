#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);  // Attendi avvio seriale

  Serial.println("🔍 Controllo PSRAM...");

  if (psramFound()) {
    Serial.println("✅ PSRAM presente!");
    Serial.print("📦 Dimensione PSRAM: ");
    Serial.print(ESP.getFreePsram());
    Serial.println(" byte disponibili");
  } else {
    Serial.println("❌ PSRAM NON presente.");
  }
}

void loop() {
  // Non serve loop continuo
}
