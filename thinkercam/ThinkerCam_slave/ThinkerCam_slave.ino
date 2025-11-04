// ============================================================================
// Librerie necessarie
// ============================================================================
#include "esp_camera.h"     // Gestione della fotocamera OV2640 (ESP32-CAM)
#include "FS.h"             // FileSystem astratto (necessario per SD_MMC)
#include "SD_MMC.h"         // Gestione della scheda SD tramite MMC (ESP32)
#include "soc/soc.h"        // Accesso ai registri hardware
#include "soc/rtc_cntl_reg.h" // Registro per disabilitare il brownout (reset tensione bassa)
#include "driver/rtc_io.h"  // Controllo dei pin RTC (opzionale)

// Dimensione dei blocchi di dati che vengono inviati via seriale
#define BLOCK_SIZE 512


// ============================================================================
// Pin configuration — ESP32-CAM modello AI THINKER
// Ogni pin collega il chip ESP32 con il sensore OV2640
// ============================================================================
#define PWDN_GPIO_NUM     32  // Power down camera (HIGH spegne la camera)
#define RESET_GPIO_NUM    -1  // Pin reset camera (non usato)
#define XCLK_GPIO_NUM      0  // Clock esterno verso la camera
#define SIOD_GPIO_NUM     26  // I2C SDA (configurazione camera)
#define SIOC_GPIO_NUM     27  // I2C SCL (configurazione camera)

#define Y9_GPIO_NUM       35  // Segnali immagine (bus dati) D0–D7
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25  // Segnale di sincronizzazione verticale (nuovo frame)
#define HREF_GPIO_NUM     23  // Indica riga valida
#define PCLK_GPIO_NUM     22  // Pixel clock (ogni impulso = pixel valido)

#define LED_GPIO_NUM       4  // LED FLASH della ESP32-CAM (HIGH = ON)


// ============================================================================
// Variabili globali
// ============================================================================
camera_config_t config;        // Configurazione fotocamera
String lastPhotoFilename = ""; // Nome file dell'ultima foto salvata



// ============================================================================
// Inizializzazione camera OV2640
// ============================================================================
void initCamera(){

  // Configurazione parametri del driver fotocamera
  config.ledc_channel = LEDC_CHANNEL_0;      // Canale PWM usato per XCLK
  config.ledc_timer = LEDC_TIMER_0;          // Timer PWM

  // Mappatura pin dati e controllo
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;           // Power down
  config.pin_reset = RESET_GPIO_NUM;         // Reset (non usato)

  config.xclk_freq_hz = 20000000;            // Frequenza XCLK → 20MHz
  config.pixel_format = PIXFORMAT_JPEG;      // La fotocamera invierà JPEG

  config.grab_mode = CAMERA_GRAB_LATEST;     // Usa sempre l’ultimo frame catturato
  config.fb_location = CAMERA_FB_IN_PSRAM;   // Frame buffer in PSRAM (più memoria)
  config.frame_size = FRAMESIZE_VGA;         // Default → 640x480
  config.jpeg_quality = 12;                  // Qualità JPEG (0 = migliore, 63 = peggiore)
  config.fb_count = 2;                       // Due frame buffer (più fluido)

  // Se la scheda ha PSRAM, abilita risoluzione e qualità maggiori
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;      // 800x600
    config.jpeg_quality = 15;
    config.fb_count = 2;
  } 
  // Se NON c’è PSRAM → riduci risoluzione per evitare crash
  else {
    config.frame_size = FRAMESIZE_QVGA;      // 320x240
    config.jpeg_quality = 20;
    config.fb_count = 1;
  }

  // Inizializza fotocamera
  if(esp_camera_init(&config) != ESP_OK){
    return; // Se fallisce, esce senza bloccare il programma
  }

  // Recupera puntatore al sensore per modificare parametri manuali
  sensor_t *s = esp_camera_sensor_get();

  // Regolazioni dell’immagine
  s->set_brightness(s, 0);     // Luminosità: da -2 a +2
  s->set_contrast(s, 1);       // Contrasto
  s->set_saturation(s, 1);     // Saturazione colore
  s->set_whitebal(s, 1);       // White balance automatico
  s->set_gain_ctrl(s, 1);      // Gain automatico
  s->set_exposure_ctrl(s, 1);  // Esposizione automatica
  s->set_hmirror(s, 0);        // No mirror orizzontale
  s->set_vflip(s, 0);          // No capovolgimento verticale
  s->set_lenc(s, 1);           // Correzione lente

  // Imposta pin del LED flash
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW); // Spegni LED all'avvio
}



// ============================================================================
// Inizializzazione scheda SD
// ============================================================================
bool initSD(){
  if(!SD_MMC.begin())      // Monta la SD in modalità 1-bit via MMC
    return false;

  if(SD_MMC.cardType() == CARD_NONE)
    return false;

  return true;
}



// ============================================================================
// Ritorna un nome fisso per la foto (puoi personalizzarlo)
// ============================================================================
String getPhotoFilename() {
  return "/photo.jpg";  // Puoi cambiare se vuoi timestamp
}



// ============================================================================
// Cattura una foto e la salva su SD
// ============================================================================
bool takePhoto(){

  // Scarta i primi 15 frame per evitare immagini sfocate
  for (int i = 0; i < 15; i++) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) continue;
    esp_camera_fb_return(fb);
    delay(100);
  }

  // Cattura frame finale
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb) return false;

  lastPhotoFilename = getPhotoFilename();  // Nome file da salvare

  File file = SD_MMC.open(lastPhotoFilename.c_str(), FILE_WRITE);
  if(!file){
    esp_camera_fb_return(fb);
    return false;
  }

  // Scrivi i dati JPEG dentro il file
  file.write(fb->buf, fb->len);
  file.close();

  esp_camera_fb_return(fb); // Restituisce frame buffer alla camera

  return true;
}



// ============================================================================
// Invia l'ultima foto salvata via seriale (in blocchi)
// ============================================================================
void sendLastPhoto() {

  if (lastPhotoFilename == "") {
    Serial.println("ERROR_NO_PHOTO");
    return;
  }

  File f = SD_MMC.open(lastPhotoFilename.c_str(), FILE_READ);
  if (!f) {
    Serial.println("ERROR_FILE_NOT_FOUND");
    return;
  }

  size_t fileSize = f.size();           // Calcola dimensione immagine
  Serial.printf("START_FILE:%u\n", (unsigned)fileSize);  // Intestazione al master
  Serial.flush();
  delay(50);

  uint8_t buffer[BLOCK_SIZE];

  while (f.available()) {               // Finché ci sono dati
    size_t len = f.read(buffer, BLOCK_SIZE); // Leggi blocco
    Serial.write(buffer, len);          // Invia blocco via seriale
    delay(2);
  }

  f.close();
  Serial.flush();
}



// ============================================================================
// SETUP — eseguito una volta all'avvio
// ============================================================================
void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disattiva brownout (evita reset)

  Serial.begin(115200);
  delay(2000);

  initCamera();                        // Inizializza fotocamera

  while(!initSD()){                    // Attesa finché SD non è inizializzata
    delay(2000);
  }
}



// ============================================================================
// LOOP — attesa comandi seriali
// ============================================================================
void loop(){

  if(Serial.available()){               // Se ricevo un comando via seriale…
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // Comando ricevuto dal master
    if(cmd == "SCATTA"){

      if(takePhoto()){                 // Se scatto riuscito…
        Serial.println("INVIO");
        Serial.flush();
        sendLastPhoto();               // Invia immagine
        Serial.println("FINE");        // Segnale fine invio
      }
      else {
        Serial.println("ERRORE_SCATTO");
      }
    }
  }
}
