/* =============================================================================
   ESP32 MASTER - Web Server + Gestione trasferimento foto da ESP32-CAM (slave)
   ---------------------------------------------------------------------------
   Funzionamento:
   - La ESP32 (MASTER) si connette al WiFi e apre un server web sulla porta 80.
   - Ogni 40 secondi invia via Serial il comando "SCATTA" allo slave (ESP32-CAM).
   - Lo slave scatta una foto e la invia in blocchi attraverso la seriale.
   - La foto viene salvata in SPIFFS con nome "latest.jpg".
   - Collegandosi all’indirizzo IP della ESP32 (master) tramite browser:
        → viene mostrata l’ultima foto ricevuta
        → è possibile scaricare il file JPEG
   - Il trasferimento include protocollo di controllo:
        MASTER → "SCATTA"
        SLAVE  → "INVIO"
        SLAVE  → "START_FILE:<dimensione>"
        SLAVE  → invio dati immagine a blocchi
        SLAVE  → "FINE"
============================================================================= */

// ============================================================================
// Librerie necessarie
// ============================================================================
#include <WiFi.h>        // Gestione connessione WiFi
#include "SPIFFS.h"      // File system interno della ESP32 (memoria Flash)

#define BLOCK_SIZE 512   // Dimensione blocco usato per ricevere/trasmettere dati via Serial


// ============================================================================
// Dati di connessione WiFi (ESP32 Master si collega al router)
// ============================================================================
const char* ssid = "TP-Link_3FAE";
const char* password = "63502497";

WiFiServer server(80);     // Avvia un server web sulla porta 80 (HTTP standard)


// Percorso dove verrà salvata l’ultima foto ricevuta dalla ESP32-CAM
String latestPhotoPath = "/latest.jpg";

// Variabile per gestire scatti automatici (timer)
unsigned long lastShotTime = 0;


// ============================================================================
// Funzione: invia comando allo SLAVE per scattare una foto e riceve immagine
// ============================================================================
void getPhotoFromSlave() {

  Serial.println("Invio comando SCATTA allo slave...");
  Serial.println("SCATTA");    // Invia comando allo slave via Serial

  // Attesa risposta "INVIO" dallo slave
  unsigned long start = millis();
  String line = "";

  while (millis() - start < 5000) {      // Timeout = 5 secondi
    if (Serial.available()) {
      line = Serial.readStringUntil('\n');  // Legge una riga dalla seriale
      line.trim();
      if (line == "INVIO") break;          // Comando di conferma ricevuto
    }
    delay(2);
  }

  if (line != "INVIO") {
    Serial.println("Errore: INVIO non ricevuto");
    return;
  }

  Serial.println("INVIO ricevuto, inizio ricezione foto...");


  // ========================================================================
  // Attende intestazione: START_FILE:<dimensione_in_bytes>
  // Lo slave invia questo messaggio prima di mandare i dati della foto
  // ========================================================================
  start = millis();
  String header = "";

  while (millis() - start < 10000) {     // Timeout = 10 secondi
    if (Serial.available()) {
      header = Serial.readStringUntil('\n');
      header.trim();
      if (header.startsWith("START_FILE:")) break; // Header corretto
    }
    delay(2);
  }

  if (!header.startsWith("START_FILE:")) {
    Serial.printf("Errore: header non valido: '%s'\n", header.c_str());
    return;
  }

  // Estrae la dimensione del file dall’header
  size_t fileSize = (size_t)header.substring(11).toInt();
  if (fileSize == 0) {
    Serial.println("Errore: fileSize = 0");
    return;
  }

  Serial.printf("Dimensione attesa: %u bytes\n", (unsigned)fileSize);


  // ========================================================================
  // Apre file su SPIFFS (flash interno ESP32) per salvare la foto ricevuta
  // ========================================================================
  File f = SPIFFS.open(latestPhotoPath, FILE_WRITE);
  if (!f) {
    Serial.println("Errore apertura file su SPIFFS");
    return;
  }

  uint8_t buffer[BLOCK_SIZE];
  size_t received = 0;
  unsigned long lastDataTime = millis();


  // ========================================================================
  // Ciclo di ricezione della foto
  // Legge byte dalla seriale e li scrive nella memoria SPIFFS
  // ========================================================================
  while (received < fileSize) {

    if (Serial.available()) {
      size_t availableBytes = Serial.available();

      size_t toRead = min((size_t)availableBytes,
                      min((size_t)BLOCK_SIZE,
                      (size_t)(fileSize - received)));

      size_t len = Serial.readBytes(buffer, toRead);

      if (len > 0) {
        f.write(buffer, len);     // Scrive dati nel file
        received += len;
        lastDataTime = millis();  // Reset timer timeout
      }
    }
    else {
      // Esce se non riceve dati per più di 15 secondi (connessione persa)
      if (millis() - lastDataTime > 15000) {
        Serial.println("Timeout: nessun dato ricevuto");
        break;
      }
      delay(2);
    }
  }

  f.close(); // Chiude file una volta completata la ricezione


  // ========================================================================
  // Riceve dallo slave messaggio "FINE" che conferma fine trasmissione
  // ========================================================================
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

  if (line != "FINE")
    Serial.println("Attenzione: FINE non ricevuto correttamente");
  else
    Serial.println("Ricezione completata correttamente (FINE).");
}



// ============================================================================
// Gestione delle richieste HTTP al server Web
// ============================================================================
void handleClient() {

  WiFiClient client = server.available();  // Verifica se un client si collega
  if (!client) return;

  String request = client.readStringUntil('\r'); // Legge richiesta HTTP
  client.flush();


  // ============================== PAGINA PRINCIPALE =========================
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

  // ============================= RICHIESTA FOTO =============================
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
      client.write(buffer, len); // Invia immagine
    }

    f.close();
  }

  client.stop(); // Chiude connessione con il browser
}



// ============================================================================
// SETUP – eseguito una sola volta all'avvio
// ============================================================================
void setup() {

  Serial.begin(115200);    // Inizializza comunicazione Serial
  delay(2000);

  if (!SPIFFS.begin(true)) {
    Serial.println("Errore inizializzazione SPIFFS");
  }

  // Connessione WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnesso al WiFi!");
  Serial.print("Indirizzo IP: ");
  Serial.println(WiFi.localIP());

  delay(2000);

  getPhotoFromSlave();   // Richiede subito una foto allo slave

  server.begin();        // Avvia il server Web
}



// ============================================================================
// LOOP – eseguito continuamente
// ============================================================================
void loop() {

  handleClient(); // Gestisce richieste web (web server sempre attivo)

  // Richiede una nuova foto automaticamente ogni 40 secondi
  if (millis() - lastShotTime >= 40000) {
    lastShotTime = millis();
    getPhotoFromSlave();
  }
}
