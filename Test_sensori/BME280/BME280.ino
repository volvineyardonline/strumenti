#include "LoRaWan_APP.h"
#include "HT_SH1107Wire.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;
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

  if (!bme.begin(0x76)) {
    Serial.println("BME280 non trovato!");
    while (1);
  }

  display.init();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "BME280 pronto");
  display.display();
  delay(1000);
}

void loop() {
  float temp = bme.readTemperature();
  float press = bme.readPressure() / 100.0F;
  float hum = bme.readHumidity();
  float alt = bme.readAltitude(SEALEVELPRESSURE_HPA);

  Serial.printf("Temp: %.2f °C\n", temp);
  Serial.printf("Hum: %.2f %%\n", hum);
  Serial.printf("Pres: %.2f hPa\n", press);
  Serial.printf("Alt: %.2f m\n", alt);

  display.clear();
  display.drawString(0, 0, "BME280");
  display.drawString(0, 15, "Temp: " + String(temp, 1) + " C");
  display.drawString(0, 30, "Hum:  " + String(hum, 1) + " %");
  display.drawString(0, 45, "Pres: " + String(press, 1) + " hPa");
  display.display();

  delay(5000);
}
