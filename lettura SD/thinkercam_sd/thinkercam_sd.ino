#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include "time.h"

#define BLOCK_SIZE 512

// WiFi credentials
const char* ssid = "TP-Link_3FAE";
const char* password = "63502497";

// Timezone
String myTimezone ="WET0WEST,M3.5.0/1,M10.5.0";

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

  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 20;
    config.fb_count = 1;
  }

  if(esp_camera_init(&config) != ESP_OK){
    Serial.println("Camera init failed");
  } else {
    // Miglioramento colori
    sensor_t * s = esp_camera_sensor_get();
    s->set_brightness(s, 0);   // -2 a 2
    s->set_contrast(s, 1);     // -2 a 2
    s->set_saturation(s, 1);   // -2 a 2
    s->set_whitebal(s, 1);     // auto WB
    s->set_gain_ctrl(s, 1);
    s->set_hmirror(s, 0);
    s->set_vflip(s, 0);
    s->set_lenc(s, 1);         // correzione lente
    Serial.println("Camera OK");
  }
}

// Initialize WiFi
void initWiFi(){
  WiFi.begin(ssid,password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected");
}

// Initialize time
void initTime(){
  configTime(0,0,"pool.ntp.org");
  setenv("TZ",myTimezone.c_str(),1);
  tzset();
}

// Generate filename with timestamp
String getPhotoFilename(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to get time");
    return "/photo.jpg";
  }
  char buffer[30];
  strftime(buffer,sizeof(buffer),"/photo_%Y-%m-%d_%H-%M-%S.jpg",&timeinfo);
  return String(buffer);
}

// Initialize SD
bool initSD(){
  if(!SD_MMC.begin()){
    Serial.println("SD init failed");
    return false;
  }
  if(SD_MMC.cardType() == CARD_NONE){
    Serial.println("No SD card");
    return false;
  }
  Serial.println("SD initialized");
  return true;
}

// Take photo and save
bool takePhoto(){
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed");
    return false;
  }
  lastPhotoFilename = getPhotoFilename();
  File file = SD_MMC.open(lastPhotoFilename.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    esp_camera_fb_return(fb);
    return false;
  }
  file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
  Serial.printf("Saved photo: %s\n", lastPhotoFilename.c_str());
  return true;
}

// Send last photo over Serial
void sendLastPhoto(){
  if(lastPhotoFilename == ""){
    Serial.println("No photo available");
    return;
  }
  File f = SD_MMC.open(lastPhotoFilename.c_str(), FILE_READ);
  if(!f){
    Serial.println("ERROR_FILE_NOT_FOUND");
    return;
  }
  uint8_t buffer[BLOCK_SIZE];
  while(f.available()){
    size_t len = f.read(buffer, BLOCK_SIZE);
    Serial.write(buffer,len);
    delay(1);
  }
  f.close();
  Serial.println("END_OF_FILE");
}

void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(2000);

  initWiFi();
  initTime();
  initCamera();

  while(!initSD()){
    Serial.println("Retrying SD init in 2s...");
    delay(2000);
  }

  // Take initial photo
  while(!takePhoto()){
    Serial.println("Retrying photo in 2s...");
    delay(2000);
  }

  Serial.println("Ready to send last photo. Awaiting GET_PHOTO command...");
}

void loop(){
  if(Serial.available()){
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if(cmd == "GET_PHOTO"){
      sendLastPhoto();
    }
  }
}
