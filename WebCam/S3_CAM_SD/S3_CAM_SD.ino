#include "FS.h"           // Libreria per file system
#include "HTTPClient.h"   // Libreria per richieste HTTP
#include "SD.h"           // Gestione della scheda SD
#include "WiFi.h"         // Libreria WiFi
#include "config.h"       // Configurazioni hardware (pin, costanti, ecc.)
#include "esp_camera.h"   // Libreria ufficiale per camera ESP32
#include <Arduino.h>      // Libreria base Arduino
#include <WiFiAP.h>       // Gestione access point WiFi
#include <driver/i2s.h>   // Libreria per interfaccia I2S (microfono)

// 🔌 Oggetto seriale per comunicare con il modulo 4G/Modem (UART1)
HardwareSerial SerialAT(1);

// 🌍 Oggetto per fare richieste HTTP (GET/POST verso server esterni)
HTTPClient http_client;

// 📌 Prototipi delle funzioni utilizzate più avanti
void mic_init(void);
void check_sound(void);
void sd_test(void);
void wifi_scan_connect(void);
void pcie_test(void);
void camera_test(void);
void startCameraServer();

// =====================
// 🔧 FUNZIONE SETUP()
// =====================
void setup() {
  // Attiva alimentazione del modulo (tramite un pin dedicato al power-on)
  pinMode(PWR_ON_PIN, OUTPUT);
  digitalWrite(PWR_ON_PIN, HIGH);
  delay(2000); // tempo per stabilizzare

  // Avvia la comunicazione seriale di debug
  Serial.begin(115200);
 
  sd_test();    

  camera_test();
}

// =====================
// 🔁 FUNZIONE LOOP()
// =====================
#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "esp_sleep.h"

void loop() {
  Serial.println("📸 Catturo foto...");

  // Richiede un frame (immagine) dalla camera
  camera_fb_t *fb = esp_camera_fb_get();

  // Mostra la memoria disponibile (utile per debug RAM e PSRAM)
  Serial.printf("Heap: %d, Free PSRAM: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());

  // Se il frame non è stato acquisito -> errore
  if (!fb) {
    Serial.println("❌ Immagine non acquisita");
    delay(2000); // aspetta un po’ e riprova
    return;
  }

  Serial.println("✅ Immagine acquisita con successo");

  // -----------------------------
  // 💾 SALVATAGGIO SU SD
  // -----------------------------
  static int photo_id = 0;                    // contatore progressivo foto
  unsigned long seconds = millis() / 1000;    // tempo in secondi dall’avvio
  unsigned long minutes = seconds / 60;       // minuti
  unsigned long sec_only = seconds % 60;      // secondi (da 0 a 59)

  // Nome file tipo: photo_3m45s_2.jpg
  String path = "/photo_" + String(minutes) + "m" + String(sec_only) + "s_" + String(photo_id++) + ".jpg";

  // Apre file sulla SD in modalità scrittura
  File file = SD.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("❌ Impossibile aprire file su SD");
  } else {
    // Scrive i dati JPEG catturati nel file
    file.write(fb->buf, fb->len);
    file.close();
    Serial.println("✅ Foto salvata su SD: " + path);
  }

  // Rilascia il frame buffer per liberare RAM
  esp_camera_fb_return(fb);

  // Attendi un po’ prima dello scatto successivo
  delay(10000); 
}


// =====================
// 💾 TEST SCHEDA SD
// =====================
void sd_test(void) {
  // Avvio bus SPI sui pin dedicati alla SD
  SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  // Prova a montare la scheda SD
  if (!SD.begin(SD_CS_PIN, SPI)) {
    Serial.println("Card Mount Failed");
    return;
  }

  // Ottiene il tipo di scheda rilevata
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  // Stampa tipo di scheda
  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
    Serial.println("MMC");
  else if (cardType == CARD_SD)
    Serial.println("SDSC");
  else if (cardType == CARD_SDHC)
    Serial.println("SDHC");
  else
    Serial.println("UNKNOWN");

  // Mostra la capacità totale della SD in MB
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

// =====================
// 🔊 VARIABILI MICROFONO
// (non usate in questo sketch ma predisposte)
// =====================
#define BUFFER_SIZE (4 * 1024)
uint8_t buffer[BUFFER_SIZE] = { 0 };   // buffer circolare audio

// Parametri di rilevamento audio (se implementato)
const int define_max = 600;
const int define_avg = 150;
const int define_zero = 3900;
String timelong_str = "";
float val_avg = 0;
int16_t val_max = 0;
float val_avg_1 = 0;
int16_t val_max_1 = 0;

float all_val_avg = 0;
int32_t all_val_zero1 = 0;
int32_t all_val_zero2 = 0;
int32_t all_val_zero3 = 0;

int16_t val16 = 0;
uint8_t val1, val2;
uint32_t j = 0;
bool aloud = false;

// =====================
// 📸 INIZIALIZZAZIONE CAMERA
// =====================
void camera_test() {
  Serial.println("Camera init");

  // Configurazione hardware dei pin collegati al sensore camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAM_Y2_PIN;
  config.pin_d1 = CAM_Y3_PIN;
  config.pin_d2 = CAM_Y4_PIN;
  config.pin_d3 = CAM_Y5_PIN;
  config.pin_d4 = CAM_Y6_PIN;
  config.pin_d5 = CAM_Y7_PIN;
  config.pin_d6 = CAM_Y8_PIN;
  config.pin_d7 = CAM_Y9_PIN;
  config.pin_xclk = CAM_XCLK_PIN;
  config.pin_pclk = CAM_PCLK_PIN;
  config.pin_vsync = CAM_VSYNC_PIN;
  config.pin_href = CAM_HREF_PIN;
  config.pin_sccb_sda = CAM_SIOD_PIN;
  config.pin_sccb_scl = CAM_SIOC_PIN;
  config.pin_pwdn = CAM_PWDN_PIN;
  config.pin_reset = CAM_RESET_PIN;
  config.xclk_freq_hz = 10000000;          // clock esterno camera
  config.pixel_format = PIXFORMAT_JPEG;    // formato immagine (JPEG)

  // Se c’è PSRAM disponibile, usa risoluzione alta e più buffer
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;    // 1600x1200
    config.jpeg_quality = 2;               // qualità JPEG alta
    config.fb_count = 3;                   // 3 buffer per frame
    Serial.printf("UXGA");
  } else {
    // Senza PSRAM -> risoluzione più bassa
    config.frame_size = FRAMESIZE_SVGA;    // 800x600
    config.jpeg_quality = 2;
    config.fb_count = 1;                   // un solo buffer
    config.fb_location = CAMERA_FB_IN_DRAM;
    Serial.printf("SVGA");
  }

  // Inizializza la camera con i parametri
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed with error 0x%x\n", err);
    Serial.println("Controlla i pin definiti in config.h e il collegamento hardware.");
    Serial.println("Prova a ridurre la risoluzione o cambiare pixel_format.");
    return;
  } else {
    Serial.println("✅ Camera inizializzata correttamente");
  }

  // Ottiene puntatore al sensore camera per applicare regolazioni
  sensor_t *s = esp_camera_sensor_get();

  // -----------------------------
  // ⚙️ CONFIGURAZIONI IMMAGINE
  // -----------------------------
  s->set_framesize(s, FRAMESIZE_UXGA);   // imposta risoluzione
  s->set_quality(s, 2);                  // qualità JPEG
  s->set_brightness(s, 0);               // luminosità
  s->set_contrast(s, 1);                 // contrasto
  s->set_saturation(s, 1);               // saturazione
  s->set_sharpness(s, 1);                // nitidezza
  s->set_denoise(s, 5);                  // riduzione rumore
  s->set_exposure_ctrl(s, 1);            // esposizione automatica
  s->set_gainceiling(s, GAINCEILING_8X); // guadagno massimo
  s->set_whitebal(s, 1);                 // bilanciamento del bianco
  s->set_awb_gain(s, 1);                 // auto white balance gain
  s->set_aec2(s, 1);                     // auto-esposizione avanzata
  s->set_gain_ctrl(s, 1);                // controllo guadagno automatico
  s->set_lenc(s, 1);                     // correzione lente
  s->set_vflip(s, 1);                    // ribalta immagine verticalmente
  s->set_bpc(s, 1);                      // correzione pixel neri difettosi
  s->set_wpc(s, 1);                      // correzione pixel bianchi difettosi

  Serial.println("Camera configurata per foto UXGA 1600x1200");
}
