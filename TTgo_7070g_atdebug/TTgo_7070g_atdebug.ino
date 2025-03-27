/*
  Situazione: FUNZIONA
  FILE: ATdebug.ino
  AUTHOR: Koby Hale
  PURPOSE: test AT commands
  List of SIM7000 AT commands can be found here
  http://www.microchip.ua/simcom/LTE/SIM7000/SIM7000%20Series_AT%20Command%20Manual_V1.05.pdf

23/12/24
Bosio L.
aggiunto funzione per impostare modalità LTE-M: startLTE()

22/12/24
  Bosio L.
  aggiunto esp.restart per eliminare il problema della connessione
ATTENZIONE
1) con deep sleep non esegue un hard reset (pulsante reset)
2) esp.restart: non mantiene in memoria il valore RTC_DATA_ATTR e quindi rieseguirebbe il ciclo per sempre
*/

#include <Arduino.h>

#define SerialAT Serial1

/*
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
*/
#define PIN_DTR     32
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)
#define SerialAT  Serial1

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1        /* Time ESP32 will go to sleep (in seconds) */


RTC_DATA_ATTR bool startModem = false;
bool reply = false;



uint32_t dectModemBaud()
{
    static uint32_t rates[] = {115200, 57600,  38400, 19200, 9600,  74400, 74880,
                               230400, 460800, 2400,  4800,  14400, 28800
                              };

    for (uint8_t i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
        uint32_t rate = rates[i];
        Serial.printf("[INFO]:Trying baud rate:%d\n", rate);
        SerialAT.updateBaudRate(rate);
        delay(10);
        
        for (int j = 0; j < 10; j++) {
            SerialAT.print("AT\r\n");
            String input = SerialAT.readString();
            
            
            if (input.indexOf("OK") >= 0) {
                Serial.printf("[INFO]:Modem responded at rate:%d\n", rate);
                //NON FUNZIONA con Sim7070G:
                //SerialAT.println("AT+IPREX=115200");
                return rate;
            }
           //22/12/24
           
            if ((rate < 115200) && (!startModem)) {
              startModem = true;
              Serial.println("RICORDARSI che la prima volta si riavvia altrimenti il modulo sim non dialoga con la scheda");
              delay(100);
              esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
              delay(100);
              esp_deep_sleep_start();
              Serial.println("Deep sleep non avviato !"); 
            }
            
            
            //FINE
        }
    }
    SerialAT.updateBaudRate(115200);
    Serial.println("[ERROR]:Modem is not online!!!");
    return 0;
}


void modem_on()
{
    // Set-up modem  power pin
    Serial.println("\nStarting Up Modem...");
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    // Starting the machine requires at least 1 second of low level, and with a level conversion, the levels are opposite
    //delay(1000);
    delay(1500);
    digitalWrite(PWR_PIN, LOW);
    //delay(10000);                 //Wait for the SIM7000 communication to be normal, and will quit when receiving OK
    delay(3000);
    int i = 10;

    if(dectModemBaud() == 0){
        Serial.println("Unable to communicate with modem.");
        while(1);
    }

    Serial.println("\nTesting Modem Response...\n");
    Serial.println("****");
    while (i) {
        SerialAT.println("AT");
        delay(500);
        if (SerialAT.available()) {
            String r = SerialAT.readString();
            Serial.println(r);
            if ( r.indexOf("OK") >= 0 ) {
                reply = true;
                break;;
            }
        }
        delay(500);
        i--;
    }
    Serial.println("****\n");
}

//23/12/24
//INIZIO: startLTE
void startLTE()
{
    //i delay sono necessari per eseguire i comandi, soprattutto il cambio di banda
    //avvia connessione alla rete mobile
    SerialMon.println("1) connessione rete mobile");
    SerialAT.println("AT+CFUN=0");
    delay (5000);
    SerialMon.println("2) connessione rete mobile");
    SerialAT.println("AT+CFUN=1");
    delay (10000);
    //il risultato di questa sequenza di comandi (messaggio su serial.println) si ottiene solo col loop
    //dove è eseguito Serial.write(SerialAT.read())
    //SerialMon.println("Imposta solo LTE-M");
    //SerialAT.println("AT+CNMP=38");
    SerialMon.println("Imposta solo GSM");
    SerialAT.println("AT+CNMP=13");
    
    SerialMon.println("Imposta LTE-M invece di NB-IOT");
    SerialAT.println("AT+CMNB=1");
    
    
    SerialMon.println("Imposta banda per LTE-M");
    SerialAT.println("AT+CBANDCFG=\"CAT-M\",1,2,3,4,5,8,12,13,14,18,19,20,25,26,27,28,66,85");
    delay(20000);
    SerialMon.println("1) Verifica banda");
    SerialAT.println("AT+COPS?");
/*    delay(20000);

    SerialMon.println("Imposta automatica (manterrà comunque la rete scelta, GSM o LTE-M)");
    SerialAT.println("AT+CNMP=2");
    delay(20000);
    SerialMon.println("2) Verifica banda");
    SerialAT.println("AT+COPS?");
 */   
    
}


//FINE: startLTE

void setup()
{
    Serial.begin(115200); // Set console baud rate
    SerialAT.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
    delay(100);
Serial.println(F("connessione al modem in corso. \n "));
    modem_on();
    if (reply) {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" You can now send AT commands"));
        Serial.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
        Serial.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
        Serial.println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
        Serial.println(F(" can have undesired consiquinces..."));
        Serial.println(F("***********************************************************\n"));
    } else {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" Failed to connect to the modem! Check the baud and try again."));
        Serial.println(F("***********************************************************\n"));
    }
//23/12/24
//imposta modalità modem a LTE
//startLTE();
/*
      2 Automatic
      13 GSM only
      38 LTE only
      51 GSM and LTE only
    * * * */

   /*
      1 CAT-M
      2 NB-Iot
      3 CAT-M and NB-IoT
    * * */

}

void loop()
{
    
    while (SerialAT.available()) {
        Serial.write(SerialAT.read());
    }
    
    while (Serial.available()) {
        SerialAT.write(Serial.read());
    }
    delay(1);
}