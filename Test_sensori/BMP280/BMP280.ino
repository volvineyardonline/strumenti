#include "Arduino.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "HT_SH1107Wire.h"
#include "LoRaWan_APP.h"

extern SH1107Wire display;

#define SEALEVELPRESSURE_HPA (1013.25)

// Sensore BMP280
Adafruit_BMP280 bmp;

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW); // Vext ON
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH); // Vext OFF
}

void setup() {
  Serial.begin(115200);
  delay(500);

  VextON();
  delay(200);

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 non trovato!");
    while (1);
  }

  // Inizializza display OLED
  display.init();
  display.clear();
  display.display();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "BMP280 pronto");
  display.display();
  delay(1000);
}

void loop() {
  float temp = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;
  float altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);

  Serial.printf("Temperatura: %.2f °C\n", temp);
  Serial.printf("Pressione: %.2f hPa\n", pressure);
  Serial.printf("Altitudine: %.2f m\n", altitude);

  display.clear();
  display.drawString(0, 0, "BMP280");
  display.drawString(0, 15, "Temp: " + String(temp, 1) + " C");
  display.drawString(0, 30, "Pres: " + String(pressure, 1) + " hPa");
  display.drawString(0, 45, "Alt:  " + String(altitude, 1) + " m");
  display.display();

  delay(5000);
}
