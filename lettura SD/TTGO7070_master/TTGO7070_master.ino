#include <WiFi.h>       // Libreria per la connessione WiFi
#include "SPIFFS.h"     // Libreria per gestire il filesystem SPIFFS

#define BLOCK_SIZE 512  // Dimensione del blocco per leggere/scrivere dati della foto

// Dati WiFi
const char* ssid = "TP-Link_3FAE";
const char* password = "63502497";

// Server web sulla porta 80
WiFiServer server(80);

// Percorso file per salvare l'ultima foto ricevuta
String latestPhotoPath = "/latest.jpg";

// Variabile per gestire il tempo dell'ultimo scatto automatico
unsigned long lastShotTime = 0;

// Funzione per richiedere la foto allo slave
void getPhotoFromSlave() {
  Serial.println("Invio comando SCATTA allo slave...");
  Serial.println("SCATTA");  // Invio comando SCATTA via seriale

  // Attesa del segnale "INVIO" dallo slave
  unsigned long start = millis();
  String line = "";
  while (millis() - start < 5000) {  // timeout 5 secondi
    if (Serial.available()) {
      line = Serial.readStringUntil('\n');  // legge una riga dalla seriale
      line.trim();                          // rimuove spazi iniziali/finali
      if (line == "INVIO") break;           // esce se ricevuto INVIO
    }
    delay(2);
  }

  // Controlla se INVIO è stato ricevuto
  if (line != "INVIO") {
    Serial.println("Errore: INVIO non ricevuto");
    return; // termina funzione
  }

  Serial.println("INVIO ricevuto, inizio ricezione foto...");

  // Attende l'header START_FILE:<size> dalla seriale
  start = millis();
  String header = "";
  while (millis() - start < 10000) {  // timeout 10 secondi
    if (Serial.available()) {
      header = Serial.readStringUntil('\n');
      header.trim();
      if (header.startsWith("START_FILE:")) break;
    }
    delay(2);
  }

  // Controlla se header valido
  if (!header.startsWith("START_FILE:")) {
    Serial.printf("Errore: header non ricevuto o non valido: '%s'\n", header.c_str());
    return;
  }

  // Ottiene la dimensione della foto
  size_t fileSize = (size_t)header.substring(11).toInt();
  if (fileSize == 0) {
    Serial.println("Errore: fileSize = 0");
    return;
  }
  Serial.printf("Dimensione attesa: %u bytes\n", (unsigned)fileSize);

  // Apre il file su SPIFFS per scrivere la foto
  File f = SPIFFS.open(latestPhotoPath, FILE_WRITE);
  if (!f) {
    Serial.println("Errore apertura file su SPIFFS");
    return;
  }

  uint8_t buffer[BLOCK_SIZE];  // buffer per ricevere blocchi di dati
  size_t received = 0;
  unsigned long lastDataTime = millis();

  // Ciclo per ricevere la foto in blocchi
  while (received < fileSize) {
    if (Serial.available()) {
      size_t avail = Serial.available();
      // calcola quanti byte leggere (min tra buffer, bytes disponibili e rimanenti)
      size_t toRead = min((size_t)avail, min((size_t)BLOCK_SIZE, (size_t)(fileSize - received)));
      size_t len = Serial.readBytes(buffer, toRead);
      if (len > 0) {
        f.write(buffer, len);       // scrive nel file
        received += len;            // aggiorna contatore bytes ricevuti
        lastDataTime = millis();    // aggiorna timer per timeout
      }
    } else {
      // Se non arrivano dati per 15 secondi esce
      if (millis() - lastDataTime > 15000) {
        Serial.println("Timeout: nessun dato ricevuto");
        break;
      }
      delay(2);
    }
  }

  f.close(); // chiude il file

  // Attende "FINE" dallo slave per confermare la fine della trasmissione
  start = millis();
  line = "";
  while (millis() - start < 5000) {
    if (Serial.available()) {
      line = Serial.readStringUntil('\n');
      line.trim();
      if (line == "FINE") break;
    }
    delay(2);
  }

  if (line != "FINE") {
    Serial.println("Attenzione: FINE non ricevuto correttamente");
  } else {
    Serial.println("Ricezione completata correttamente (FINE).");
  }
}

// Funzione per gestire le richieste HTTP
void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  // legge la richiesta fino a \r
  String request = client.readStringUntil('\r');
  client.flush();

  // Pagina principale
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
  }
  // Richiesta diretta della foto
  else if (request.indexOf("GET /photo.jpg") >= 0) {
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
    while (f.available()) {
      size_t len = f.read(buffer, BLOCK_SIZE);
      client.write(buffer, len); // invia dati foto
    }
    f.close();
  }

  client.stop(); // chiude connessione client
}

void setup() {
  Serial.begin(115200); // inizializza seriale
  delay(2000);

  if (!SPIFFS.begin(true)) {
    Serial.println("Errore inizializzazione SPIFFS"); // inizializza filesystem
  }

  // connessione WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnesso!");
  Serial.print("IP locale: ");
  Serial.println(WiFi.localIP());

  delay(2000);

  getPhotoFromSlave(); // Richiede la prima foto appena accesa

  server.begin(); // avvia il server web
}

void loop() {
  handleClient(); // gestisce eventuali richieste web

  // SCATTA automatico ogni 40 secondi
  if (millis() - lastShotTime >= 40000) {
    lastShotTime = millis();
    getPhotoFromSlave();
  }
}
