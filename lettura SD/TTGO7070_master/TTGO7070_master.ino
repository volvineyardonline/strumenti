#include "SPIFFS.h"

#define BLOCK_SIZE 512  // blocco di lettura in RAM
uint8_t block[BLOCK_SIZE];  // buffer temporaneo

void getFileToSPIFFS(String filename, size_t fileSize) {
  if(!SPIFFS.begin(true)) {
    Serial.println("Errore inizializzazione SPIFFS");
    return;
  }

  File f = SPIFFS.open("/" + filename, FILE_WRITE);
  if (!f) {
    Serial.println("Errore apertura file nella SPIFFS");
    return;
  }

  Serial.println("GET " + filename);  // invio comando allo Slave

  size_t bytesReceived = 0;
  size_t lastPercent = 0;
  String marker = "";

  while (true) {
    while (Serial.available()) {
      int c = Serial.read();
      if (c == -1) continue;

      marker += (char)c;

      // verifica END_OF_FILE
      if (marker.endsWith("END_OF_FILE")) {
        marker.remove(marker.length() - 11);
        for (size_t i = 0; i < marker.length(); i++) {
          f.write(marker[i]);
          bytesReceived++;
        }
        f.close();
        Serial.println("\nFile trasferito completamente nella SPIFFS!");
        printSPIFFSContents();
        return;
      }

      // scrivi blocchi man mano
      if (marker.length() >= BLOCK_SIZE) {
        for (size_t i = 0; i < BLOCK_SIZE; i++) {
          f.write(marker[i]);
          bytesReceived++;
        }
        marker.remove(0, BLOCK_SIZE);
      }

      // aggiorna percentuale ogni 5%
      if (fileSize > 0) {
        size_t percent = (bytesReceived * 100) / fileSize;
        if (percent >= lastPercent + 5) {
          Serial.print("Trasferimento: ");
          Serial.print(percent);
          Serial.println("%");
          lastPercent = percent;
        }
      }
    }
  }
}

void printSPIFFSContents() {
  Serial.println("\nContenuto SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
    Serial.print("FILE: ");
    Serial.print(file.name());
    Serial.print(" | Dimensione: ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);  // UART0 collegata allo Slave
  delay(1000);

  size_t estimatedFileSize = 500000; // se non conosci la dimensione esatta, metti una stima
  Serial.println("Richiedo file...");
  getFileToSPIFFS("photo_0m20s_1_4259.jpg", estimatedFileSize);
}

void loop() {}
