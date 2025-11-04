// ========================================================================
// PROGRAMMA DI TEST PER ESP32-CAM (AI Thinker)
// - Si connette al WiFi
// - Ottiene data/ora dal server NTP
// - Inizializza la fotocamera e la SD
// - Scatta una foto salvandola su SD con nome basato su data/ora
// - Invia la foto via seriale quando riceve il comando "GET_PHOTO"
// ========================================================================


#include "esp_camera.h"     // Libreria per usare la fotocamera ESP32-CAM
#include "FS.h"             // Libreria per astrarre l'accesso ai filesystem
#include "SD_MMC.h"         // Libreria per usare SD tramite interfaccia MMC
#include "soc/soc.h"        // Libreria per funzioni hardware interne
#include "soc/rtc_cntl_reg.h"// Registro per brownout (reset per bassa tensione)
#include "driver/rtc_io.h"  // Funzioni per controllare i pin RTC (deep sleep)
#include <WiFi.h>           // Libreria WiFi per connessione access point
#include "time.h"           // Libreria per gestire data/ora tramite NTP


#define BLOCK_SIZE 512  // Numero di byte inviati alla volta via seriale


// 🔒 Credenziali WiFi (riempire con proprie password)
const char* ssid = "***";      
const char* password = "****"; 


// 🕒 Impostazione fuso orario (Europa occidentale)
String myTimezone = "WET0WEST,M3.5.0/1,M10.5.0";


// ------------------------------------------------------------------------
// Configurazione pin modulo ESP32-CAM AI Thinker
// Ogni pin è collegato fisicamente tra ESP32 e sensore OV2640.
// ------------------------------------------------------------------------
#define PWDN_GPIO_NUM     32    // Power-Down camera (32 = GPIO)
#define RESET_GPIO_NUM    -1    // Reset camera (-1 = non usato)
#define XCLK_GPIO_NUM      0    // Clock per sensore (obbligatorio)
#define SIOD_GPIO_NUM     26    // SDA I2C verso sensore (registri camera)
#define SIOC_GPIO_NUM     27    // SCL I2C verso sensore
#define Y9_GPIO_NUM       35    // Pin dati camera Y9
#define Y8_GPIO_NUM       34    // Pin dati camera Y8
#define Y7_GPIO_NUM       39    // Pin dati camera Y7
#define Y6_GPIO_NUM       36    // Pin dati camera Y6
#define Y5_GPIO_NUM       21    // Pin dati camera Y5
#define Y4_GPIO_NUM       19    // Pin dati camera Y4
#define Y3_GPIO_NUM       18    // Pin dati camera Y3
#define Y2_GPIO_NUM        5    // Pin dati camera Y2
#define VSYNC_GPIO_NUM    25    // Segnale sincronizzazione verticale (nuovo frame)
#define HREF_GPIO_NUM     23    // Segnale indicazione riga valida
#define PCLK_GPIO_NUM     22    // Pixel clock


camera_config_t config;         // Struttura di configurazione della fotocamera
String lastPhotoFilename = "";  // Nome dell’ultima foto salvata


// ------------------------------------------------------------------------
// 🔧 INIZIALIZZAZIONE FOTOCAMERA
// Configura tutti i pin e parametri della fotocamera OV2640
// ------------------------------------------------------------------------
void initCamera(){

  config.ledc_channel = LEDC_CHANNEL_0;       // Canale PWM (clock esterno)
  config.ledc_timer = LEDC_TIMER_0;           // Timer PWM per generare clock XCLK
  config.pin_d0 = Y2_GPIO_NUM;                // Mappa pin sensore → pin ESP
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;            // Clock per OV2640
  config.pin_pclk = PCLK_GPIO_NUM;            // Pixel clock
  config.pin_vsync = VSYNC_GPIO_NUM;          // Sincronizzazione frame
  config.pin_href = HREF_GPIO_NUM;            // Sync orizzontale
  config.pin_sccb_sda = SIOD_GPIO_NUM;        // SDA bus controllo camera
  config.pin_sccb_scl = SIOC_GPIO_NUM;        // SCL bus controllo camera
  config.pin_pwdn = PWDN_GPIO_NUM;            // Power-down camera
  config.pin_reset = RESET_GPIO_NUM;          // Reset (non usato)
  config.xclk_freq_hz = 20000000;             // Clock a 20 MHz per il sensore
  config.pixel_format = PIXFORMAT_JPEG;       // Formato immagine: JPEG
  config.grab_mode = CAMERA_GRAB_LATEST;      // Usa l'ultimo frame disponibile

  // Se la scheda ha PSRAM → qualità migliore
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;        // Risoluzione 640x480
    config.jpeg_quality = 15;                 // Qualità JPEG più alta (meno compressione)
    config.fb_count = 1;                      // Un buffer immagine
  } else {
    config.frame_size = FRAMESIZE_QVGA;       // Risoluzione 320x240
    config.jpeg_quality = 20;                 // Qualità inferiore (file più piccolo)
    config.fb_count = 1;
  }

  // Inizializza la fotocamera con la configurazione sopra
  if(esp_camera_init(&config) != ESP_OK){
    Serial.println("Camera init failed");     // Se qualcosa va male → errore
  } else {
    // Ottiene oggetto "sensore" per poter modificare parametri immagine
    sensor_t * s = esp_camera_sensor_get();

    // Regolazioni per migliorare l'immagine
    s->set_brightness(s, 0);   // Luminosità (-2 a 2)
    s->set_contrast(s, 1);     // Contrasto
    s->set_saturation(s, 1);   // Saturazione colori
    s->set_whitebal(s, 1);     // Bilanciamento bianco automatico
    s->set_gain_ctrl(s, 1);    // Controllo automatico guadagno
    s->set_hmirror(s, 0);      // Disabilita specchio orizzontale
    s->set_vflip(s, 0);        // Disabilita flip verticale
    s->set_lenc(s, 1);         // Correzione lente (distorsione)
    Serial.println("Camera OK");
  }
}


// ------------------------------------------------------------------------
// 🌐 Connessione al WiFi
// ------------------------------------------------------------------------
void initWiFi(){
  WiFi.begin(ssid, password);      // Avvia connessione access point
  Serial.print("Connecting to WiFi");

  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");             // Stampiamo puntini finché non si connette
    delay(500);
  }
  Serial.println(" Connected!");
}


// ------------------------------------------------------------------------
// 🕒 Ottiene data e ora da server NTP
// ------------------------------------------------------------------------
void initTime(){
  configTime(0, 0, "pool.ntp.org");     // Richiede ora al server NTP
  setenv("TZ", myTimezone.c_str(), 1);  // Applica il fuso orario scelto
  tzset();                              // Aggiorna configurazione interna
}


// ------------------------------------------------------------------------
// 📌 Genera nome file della foto usando data/ora attuale
// ------------------------------------------------------------------------
String getPhotoFilename(){
  struct tm timeinfo;

  if(!getLocalTime(&timeinfo)){     // Richiede data/ora memorizzata
    Serial.println("Failed to get time");
    return "/photo.jpg";            // Se fallisce, salva nome fisso
  }

  char buffer[30];
  sprintf(buffer, "/photo_%04d-%02d-%02d_%02d-%02d-%02d.jpg",
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  return String(buffer);            // Restituisce filename
}


// ------------------------------------------------------------------------
// 💾 Inizializza SD
// ------------------------------------------------------------------------
bool initSD(){
  if(!SD_MMC.begin()){              // Avvia SD in modalità MMC
    Serial.println("SD init failed");
    return false;
  }
  if(SD_MMC.cardType() == CARD_NONE){
    Serial.println("No SD card found");
    return false;
  }

  Serial.println("SD initialized");
  return true;
}


// ------------------------------------------------------------------------
// 📷 Scatta foto e la salva su SD
// ------------------------------------------------------------------------
bool takePhoto(){

  camera_fb_t * fb = esp_camera_fb_get();  // Ottiene frame da fotocamera

  if(!fb){                                 // Se nulla → errore
    Serial.println("Camera capture failed");
    return false;
  }

  lastPhotoFilename = getPhotoFilename();  // Genera nome file per salvarlo

  File file = SD_MMC.open(lastPhotoFilename, FILE_WRITE); // Crea file su SD

  if(!file){
    Serial.println("Failed to open file");
    esp_camera_fb_return(fb);
    return false;
  }

  file.write(fb->buf, fb->len);   // Scrive immagine JPEG dentro file
  file.close();                   // Chiude file
  esp_camera_fb_return(fb);       // Libera frame buffer

  Serial.printf("Saved photo: %s\n", lastPhotoFilename.c_str());
  return true;
}


// ------------------------------------------------------------------------
// 📤 Invia l'ultima foto salvata via seriale in blocchi da 512 byte
// ------------------------------------------------------------------------
void sendLastPhoto(){

  if(lastPhotoFilename == ""){
    Serial.println("No photo available");
    return;
  }

  File f = SD_MMC.open(lastPhotoFilename, FILE_READ);

  if(!f){
    Serial.println("ERROR_FILE_NOT_FOUND");
    return;
  }

  uint8_t buffer[BLOCK_SIZE];     // Buffer temporaneo per seriale

  while(f.available()){
    size_t len = f.read(buffer, BLOCK_SIZE); // Legge max 512 byte
    Serial.write(buffer, len);               // Invia blocco su seriale
    delay(1);
  }

  f.close();
  Serial.println("END_OF_FILE");       // Segnale fine trasmissione
}


// ========================================================================
// SETUP — eseguito una sola volta all’accensione
// ========================================================================
void setup(){

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disabilita protezione brownout

  Serial.begin(115200);   // Avvia seriale (debug)
  delay(2000);

  initWiFi();             // Connessione WiFi
  initTime();             // Sincronizzazione data/ora
  initCamera();           // Fotocamera

  while(!initSD()){       // Finché la SD non viene inizializzata
    Serial.println("Retrying SD init in 2s...");
    delay(2000);
  }

  while(!takePhoto()){    // Scatta una foto all'avvio
    Serial.println("Retrying photo in 2s...");
    delay(2000);
  }

  Serial.println("Ready to send last photo. Awaiting GET_PHOTO command...");
}


// ========================================================================
// LOOP — viene eseguito continuamente
// ------------------------------------------------------------------------
// Rimane in attesa del comando "GET_PHOTO" via seriale
// ========================================================================
void loop(){

  if(Serial.available()){                 // Se arrivano dati dalla seriale
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();                           // Rimuove caratteri extra

    if(cmd == "GET_PHOTO"){               // Se comando richiesto è GET_PHOTO
      sendLastPhoto();                    // Invio immagine via seriale
    }
  }
}
