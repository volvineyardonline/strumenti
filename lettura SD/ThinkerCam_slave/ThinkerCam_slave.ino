#include "FS.h"
#include "SD_MMC.h"

#define BLOCK_SIZE 512

void sendFile(String filename) {
  File f = SD_MMC.open("/" + filename, FILE_READ);
  if (!f) {
    Serial.println("ERROR_FILE_NOT_FOUND");
    return;
  }

  uint8_t buffer[BLOCK_SIZE];
  while (f.available()) {
    size_t len = f.read(buffer, BLOCK_SIZE);
    Serial.write(buffer, len);  // invio dati binari
  }
  f.close();
  Serial.println("END_OF_FILE"); // marker fine
}

void setup() {
  Serial.begin(115200); // UART0
  SD_MMC.begin("/sdcard", true);
  Serial.println("Slave pronto per invio file");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("GET ")) {
      String filename = cmd.substring(4);
      sendFile(filename);
    }
  }
}
