#include "LoRaWan_APP.h"
#include "HT_SH1107Wire.h"
#include <Wire.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;
extern SH1107Wire display;

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  VextON();
  delay(200);

  if (!aht.begin()) {
    Serial.println("AHT20 non trovato!");
    while (1);
  }

  display.init();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "AHT20 pronto");
  display.display();
  delay(1000);
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  Serial.printf("Temp: %.2f °C\n", temp.temperature);
  Serial.printf("Hum: %.2f %%\n", humidity.relative_humidity);

  display.clear();
  display.drawString(0, 0, "AHT20");
  display.drawString(0, 20, "Temp: " + String(temp.temperature, 1) + " C");
  display.drawString(0, 40, "Hum:  " + String(humidity.relative_humidity, 1) + " %");
  display.display();

  delay(5000);
}
