#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"

#define BLOCK_SIZE 512  // dimensione blocco di invio seriale

// AI Thinker pin config
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
#define LED_GPIO_NUM       4   // Flash LED — spegnilo se non serve

camera_config_t config;
String lastPhotoFilename = "";

// Initialize camera
void initCamera(){
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA; 
    config.jpeg_quality = 15; 
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 20;
    config.fb_count = 1;
  }

  if(esp_camera_init(&config) != ESP_OK){
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, 0);
  s->set_contrast(s, 1);
  s->set_saturation(s, 1);
  s->set_whitebal(s, 1);
  s->set_gain_ctrl(s, 1);
  s->set_exposure_ctrl(s, 1);
  s->set_hmirror(s, 0);
  s->set_vflip(s, 0);
  s->set_lenc(s, 1);

  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);
}

bool initSD(){
  if(!SD_MMC.begin()){
    return false;
  }
  if(SD_MMC.cardType() == CARD_NONE){
    return false;
  }
  return true;
}

String getPhotoFilename() {
  return "/photo.jpg"; // semplificato, puoi aggiungere timestamp se vuoi
}

bool takePhoto(){
  for (int i = 0; i < 15; i++) { // discard first frames
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) continue;
    esp_camera_fb_return(fb);
    delay(100);
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb) return false;

  lastPhotoFilename = getPhotoFilename();
  File file = SD_MMC.open(lastPhotoFilename.c_str(), FILE_WRITE);
  if(!file){
    esp_camera_fb_return(fb);
    return false;
  }
  file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
  return true;
}

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

  size_t fileSize = f.size();
  Serial.printf("START_FILE:%u\n", (unsigned)fileSize);
  Serial.flush();
  delay(50);

  uint8_t buffer[BLOCK_SIZE];
  while (f.available()) {
    size_t len = f.read(buffer, BLOCK_SIZE);
    Serial.write(buffer, len);
    delay(2);
  }

  f.close();
  //Serial.println();
 // Serial.println("END_FILE");
  Serial.flush();
}

void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(2000);

  initCamera();

  while(!initSD()){
    delay(2000);
  }
}

void loop(){
  if(Serial.available()){
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if(cmd == "SCATTA"){
      if(takePhoto()){
        Serial.println("INVIO");
        Serial.flush();
        sendLastPhoto();
        Serial.println("FINE");
      } else {
        Serial.println("ERRORE_SCATTO");
      }
    }
  }
}
