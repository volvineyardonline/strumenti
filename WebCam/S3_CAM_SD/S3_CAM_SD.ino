#include <Arduino.h>
#include "SD.h"
#include "SPI.h"
#include "config.h"
#include "esp_camera.h"
#include "esp_sleep.h"

// 🔌 Oggetto seriale per comunicare con il modulo 4G/Modem (UART1)
HardwareSerial SerialAT(1);

// 📌 Prototipi
void sd_test(void);
void camera_test(void);

// =====================
// 🔧 SETUP (eseguito una sola volta)
// =====================
void setup() {
  pinMode(PWR_ON_PIN, OUTPUT);
  digitalWrite(PWR_ON_PIN, HIGH);
  pinMode(CAM_IR_PIN, OUTPUT);

  delay(3000); 
  Serial.begin(115200);

  // Inizializzazione SD e Camera
  sd_test();    
  camera_test();

  randomSeed(analogRead(0));
  digitalWrite(CAM_IR_PIN, HIGH);

  // ================================
  // 📸 Cattura e salvataggio foto (una sola volta)
  // ================================
  delay(3000);
  Serial.println("📸 Catturo foto...");

  camera_fb_t *fb = esp_camera_fb_get();
  Serial.printf("Heap: %d, Free PSRAM: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());

  if (!fb) {
    Serial.println("❌ Immagine non acquisita");
  } else {
    Serial.println("✅ Immagine acquisita con successo");

    static int photo_id = 0;
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long sec_only = seconds % 60;
    int randomNum = random(1000, 9999);

    String path = "/photo_" + String(minutes) + "m" + String(sec_only) + "s_" +
                  String(photo_id++) + "_" + String(randomNum) + ".jpg";

    File file = SD.open(path.c_str(), FILE_WRITE);
    if (!file) {
      Serial.println("❌ Impossibile aprire file su SD");
    } else {
      file.write(fb->buf, fb->len);
      file.close();
      Serial.println("✅ Foto salvata su SD: " + path);
    }

    esp_camera_fb_return(fb);
  }

  Serial.println("✅ Programma eseguito una volta. Fine.");
}

// =====================
// 🔁 LOOP (vuoto)
// =====================
void loop() {
  // Non fare nulla: il codice viene eseguito solo nel setup
}



// =====================
// 💾 TEST SCHEDA SD
// =====================
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

// =====================
// 📸 INIZIALIZZAZIONE CAMERA
// =====================
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
  config.xclk_freq_hz = 8000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 4;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 20;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("✅ Camera inizializzata correttamente");

  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_UXGA);
  s->set_quality(s, 4);
  s->set_brightness(s, 0);
  s->set_contrast(s, 1);
  s->set_saturation(s, 1);
  s->set_sharpness(s, 1);
  s->set_denoise(s, 5);
  s->set_exposure_ctrl(s, 1);
  s->set_gainceiling(s, GAINCEILING_8X);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_aec2(s, 1);
  s->set_gain_ctrl(s, 1);
  s->set_lenc(s, 1);
  s->set_vflip(s, 1);
  s->set_bpc(s, 1);
  s->set_wpc(s, 1);
}
