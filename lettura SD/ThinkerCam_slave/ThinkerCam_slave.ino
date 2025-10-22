#include "FS.h"
#include "SD_MMC.h"  // usa SD_MMC per la SD integrata
#include "esp_camera.h"

// --- Configurazione Camera AI Thinker ---
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_GPIO_NUM       4

void listDir(fs::FS &fs, const char * dirname) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.println("ERRORE: impossibile aprire la directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("DIR: ");
      Serial.println(file.name());
    } else {
      Serial.print("FILE: ");
      Serial.println(file.name());
    }
    file = root.openNextFile();
  }

  Serial.println("END_OF_LIST");
}

void setup() {
  // UART0 su GPIO1/3 (default)
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-CAM Slave avvio...");

  // Inizializza SD_MMC
  if (!SD_MMC.begin("/sdcard", true)) { // true = 1-bit mode
    Serial.println("SD_MMC mount fallita!");
  } else {
    Serial.println("SD_MMC montata correttamente.");
  }
/*
  // (facoltativo) Inizializza la camera, anche se non usata in questa demo
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  esp_camera_init(&config);
*/
  Serial.println("Pronto a ricevere comandi via UART...");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "LIST") {
      Serial.println("LISTA FILE SD:");
      listDir(SD_MMC, "/");
    }
  }
  delay(50);
}
