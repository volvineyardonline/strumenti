#ifndef __CONFIG_H__
#define __CONFIG_H__

#define WIFI_SSID        "TIM-23461291"
#define WIFI_PASSWORD    "whtaZW2SVVtvzyOSlwCzYVys"

#define WIFI_AP_SSID     "T-SIMCAM-"
#define WIFI_AP_PASSWORD "12345678"

// Corresponding version of board screen printing
//#define USE_SIM_CAM_V1_2
#define USE_SIM_CAM_V1_3    //Add IR Filter


// PIN
#define SD_MISO_PIN      40
#define SD_MOSI_PIN      38
#define SD_SCLK_PIN      39
#define SD_CS_PIN        47
#define PCIE_PWR_PIN     48
#define PCIE_TX_PIN      45
#define PCIE_RX_PIN      46
#define PCIE_LED_PIN     21
#define MIC_IIS_WS_PIN   42
#define MIC_IIS_SCK_PIN  41
#define MIC_IIS_DATA_PIN 2
#define CAM_PWDN_PIN     -1
#define CAM_RESET_PIN    -1
#define CAM_XCLK_PIN     14
#define CAM_SIOD_PIN     4
#define CAM_SIOC_PIN     5
#define CAM_Y9_PIN       15
#define CAM_Y8_PIN       16
#define CAM_Y7_PIN       17
#define CAM_Y6_PIN       12
#define CAM_Y5_PIN       10
#define CAM_Y4_PIN       8
#define CAM_Y3_PIN       9
#define CAM_Y2_PIN       11
#define CAM_VSYNC_PIN    6
#define CAM_HREF_PIN     7
#define CAM_PCLK_PIN     13
#define BUTTON_PIN       0
#define PWR_ON_PIN       1
#define SERIAL_RX_PIN    44
#define SERIAL_TX_PIN    43
#define BAT_VOLT_PIN     -1

#if  defined(USE_SIM_CAM_V1_3)
#define CAM_IR_PIN       18
#define CAM_RESET_PIN    -1
#warning "Currently using V1.3 Pinmap"
#else
#define CAM_RESET_PIN    18
#warning "Currently using V1.2 Pinmap"
#endif



#endif