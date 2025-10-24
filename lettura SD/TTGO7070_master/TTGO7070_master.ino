#include <WiFi.h>
#include "SPIFFS.h"

#define BLOCK_SIZE 512

const char* ssid = "TP-Link_3FAE";
const char* password = "63502497";

WiFiServer server(80);
String latestPhotoPath = "/latest.jpg";

void getPhotoFromSlave() {
  Serial.println("Invio comando GET_PHOTO allo slave...");
  Serial.println("GET_PHOTO");  // comando verso lo slave

  // Aspetta la riga START_FILE:<size>\n (timeout generoso)
  unsigned long start = millis();
  String header = "";
  while (millis() - start < 10000) {  // 10s di timeout per il header
    if (Serial.available()) {
      header = Serial.readStringUntil('\n');
      header.trim();
      if (header.length() > 0) break;
    } else {
      delay(2);
    }
  }

  if (!header.startsWith("START_FILE:")) {
    Serial.printf("Errore: header non ricevuto o non valido: '%s'\n", header.c_str());
    return;
  }

  size_t fileSize = (size_t)header.substring(11).toInt();
  if (fileSize == 0) {
    Serial.println("Errore: fileSize = 0");
    return;
  }
  Serial.printf("Inizio ricezione, dimensione attesa: %u bytes\n", (unsigned)fileSize);

  File f = SPIFFS.open(latestPhotoPath, FILE_WRITE);
  if (!f) {
    Serial.println("Errore apertura file su SPIFFS");
    return;
  }

  uint8_t buffer[BLOCK_SIZE];
  size_t received = 0;
  unsigned long lastDataTime = millis();

  while (received < fileSize) {
    // Se ci sono dati disponibili, leggili (solo la quantità utile)
    if (Serial.available()) {
      size_t avail = Serial.available();
      size_t toRead = min((size_t)avail, min((size_t)BLOCK_SIZE, fileSize - received));
      size_t len = Serial.readBytes(buffer, toRead);  // non blocca perchè toRead <= avail
      if (len > 0) {
        f.write(buffer, len);
        received += len;
        lastDataTime = millis();
        // stampa progress occasionalmente
        static size_t lastPct = 0;
        size_t pct = (received * 100) / fileSize;
        if (pct >= lastPct + 5) {
          Serial.printf("Ricevuto ~%d%% (%u bytes)\n", (int)pct, (unsigned)received);
          lastPct = pct;
        }
      }
    } else {
      // timeout di inattività (es. slave fermo)
      if (millis() - lastDataTime > 15000) {  // 15s
        Serial.println("Timeout di ricezione: nessun dato ricevuto per 15s");
        break;
      }
      delay(2);
    }
  }

  f.close();

  if (received < fileSize) {
    Serial.printf("Attenzione: trasferimento incompleto: ricevuti %u / %u bytes\n", (unsigned)received, (unsigned)fileSize);
    // prova a pulire il buffer e leggere eventuale END_FILE
    if (Serial.available()) {
      String rest = Serial.readString();
      Serial.printf("Rimanenti nel buffer: %u bytes (visualizzazione come stringa): %s\n", rest.length(), rest.c_str());
    }
    return;
  }

  // Legge la riga END_FILE (dovrebbe essere sul flusso dopo i fileSize byte)
  unsigned long t0 = millis();
  String endLine = "";
  while (millis() - t0 < 5000) {  // 5s di attesa per l'END_FILE
    if (Serial.available()) {
      endLine = Serial.readStringUntil('\n');
      endLine.trim();
      if (endLine.length() > 0) break;
    } else {
      delay(2);
    }
  }
  if (endLine != "END_FILE") {
    Serial.printf("Attenzione: fine file non riconosciuta correttamente: '%s'\n", endLine.c_str());
  } else {
    Serial.println("Ricezione completata correttamente (END_FILE).");
  }

  Serial.printf("Ricezione terminata. Totale: %u bytes\n", (unsigned)received);
}

void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;
  String request = client.readStringUntil('\r');
  client.flush();  // Mostra l’ultima foto scattata
  if (request.indexOf("GET / ") >= 0 || request.indexOf("GET /index") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<html><head><meta http-equiv='refresh' content='5'></head><body>");
    client.println("<h2>Ultima foto scattata</h2>");
    client.println("<img src='/photo.jpg' width='640'><br>");
    client.println("<a href='/photo.jpg'>Scarica foto</a>");
    client.println("</body></html>");
  } else if (request.indexOf("GET /photo.jpg") >= 0) {
    File f = SPIFFS.open(latestPhotoPath, FILE_READ);
    if (!f) {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("File non trovato");
      client.stop();
      return;
    }
    size_t fileSize = f.size();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/jpeg");
    client.println("Content-Length: " + String(fileSize));
    client.println("Connection: close");
    client.println();
    uint8_t buffer[BLOCK_SIZE];
    size_t sent = 0;
    while (f.available()) {
      size_t len = f.read(buffer, BLOCK_SIZE);
      client.write(buffer, len);
      sent += len;
      size_t percent = (sent * 100) / fileSize;
      static size_t lastPercent = 0;
      if (percent >= lastPercent + 10) {
        Serial.printf("Invio HTTP %d%%\n", percent);
        lastPercent = percent;
      }
    }
    f.close();
    Serial.printf("Trasferimento HTTP completato: %d bytes inviati.\n", fileSize);
  }
  client.stop();
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  if (!SPIFFS.begin(true)) {
    Serial.println("Errore inizializzazione SPIFFS");
  }
  WiFi.begin(ssid, password);
  Serial.println("Connessione WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnesso!");
  Serial.print("IP locale: ");
  Serial.println(WiFi.localIP());

  delay(5000);
  getPhotoFromSlave();

  server.begin();
  Serial.println("Server web avviato.");
}

void loop() {
  handleClient();
}
