#include "FS.h"
#include "HTTPClient.h"
#include "SD.h"
#include "WiFi.h"
#include "config.h"
#include "esp_camera.h"
#include <Arduino.h>
#include <WiFiAP.h>
#include <driver/i2s.h>

HardwareSerial SerialAT(1);
HTTPClient http_client;

void mic_init(void);
void check_sound(void);
void sd_test(void);
void wifi_scan_connect(void);
void pcie_test(void);
void camera_test(void);
void startCameraServer();

void setup() {
  pinMode(PWR_ON_PIN, OUTPUT);
  digitalWrite(PWR_ON_PIN, HIGH);
  delay(1000);
  Serial.begin(115200);
  Serial.println("T-SIMCAM self test");


#ifdef CAM_IR_PIN
  // Test IR Filter
  pinMode(CAM_IR_PIN, OUTPUT);
  Serial.println("Test IR Filter");
  int i = 3;
  while (i--) {
    digitalWrite(CAM_IR_PIN, 1 - digitalRead(CAM_IR_PIN));
    delay(1000);
  }
#endif

  sd_test();      // monta la SD ma NON smonta
  camera_test();  // inizializza la camera

  if (psramFound()) {
    Serial.println("✅ PSRAM presente!");
  } else {
    Serial.println("❌ PSRAM NON presente.");
  }
}

void loop() {
  Serial.println("📸 Catturo foto...");

  // Richiedi frame dalla camera
  camera_fb_t *fb = esp_camera_fb_get();

  // Stampa memoria disponibile per capire se è un problema di RAM
  Serial.printf("Heap: %d, Free PSRAM: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());

  if (!fb) {
    Serial.println("❌ Immagine non acquisita");
    delay(8000);
    return;
  }

  Serial.println("✅ Immagine acquisita con successo");

  static int photo_id = 0;
  String path = "/photo" + String(photo_id++) + ".jpg";

  File file = SD.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("❌ Impossibile aprire file su SD");
  } else {
    file.write(fb->buf, fb->len);
    file.close();
    Serial.println("✅ Foto salvata su SD: " + path);
  }

  esp_camera_fb_return(fb);
  delay(8000);
}

void sd_test(void) {
  SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
    Serial.println("MMC");
  else if (cardType == CARD_SD)
    Serial.println("SDSC");
  else if (cardType == CARD_SDHC)
    Serial.println("SDHC");
  else
    Serial.println("UNKNOWN");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

}

 
#define BUFFER_SIZE (4 * 1024)
uint8_t buffer[BUFFER_SIZE] = { 0 };
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


void camera_test() {
  Serial.println("Camera init");
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
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;  // formato JPEG


  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 2;  
    config.fb_count = 3;
    Serial.printf("UXGA");
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 2;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    Serial.printf("SVGA");
  }

  esp_err_t err = esp_camera_init(&config);
if (err != ESP_OK) {
  Serial.printf("❌ Camera init failed with error 0x%x\n", err);
  Serial.println("Controlla i pin definiti in config.h e il collegamento hardware.");
  Serial.println("Prova a ridurre la risoluzione o cambiare pixel_format.");
  return;
} else {
  Serial.println("✅ Camera inizializzata correttamente");
}

  sensor_t *s = esp_camera_sensor_get();

  s->set_framesize(s, FRAMESIZE_UXGA);  // Imposta la risoluzione UXGA esplicitamente


s->set_quality(s, 2);
s->set_brightness(s, 0);
s->set_contrast(s, 0);
s->set_saturation(s, 0);
s->set_sharpness(s, 0);
s->set_denoise(s, 5);
s->set_exposure_ctrl(s, 1);        // abilita esposizione automatica
s->set_gainceiling(s, GAINCEILING_128X);
s->set_whitebal(s, 1);
s->set_awb_gain(s, 1);
s->set_aec2(s, 1);                 // nuova funzione per abilitare AEC
s->set_gain_ctrl(s, 1);            // sostituisce set_agc
s->set_lenc(s, 1);
s->set_vflip(s, 1);
s->set_bpc(s, 1);
s->set_wpc(s, 1);
  Serial.println("Camera configurata per foto UXGA 1600x1200");
}



 

