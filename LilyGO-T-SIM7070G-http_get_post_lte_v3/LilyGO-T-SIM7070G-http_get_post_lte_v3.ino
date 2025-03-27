/*
  situazione: FUNZIONA
  ultimo aggiornamento:
  
  Bosio L.

ATTENZIONE: 7070G funziona in maniera diversa da quanto indicato nel documento simcom7070g
per ciò che riguarda powerOn e PowerOff
non funziona init, restart, poweroff
il modulo sim si accende e spegne con la sequenza HIGH-LOW
il pinStatus non funziona ma non serve visto che si usa testAT per verificare lo stato del modem
27/3/25:
DA FARE:
#include <ESP32Time.h>
static ESP32Time rtc(0);
rtc.setTimeStruct(sim800_ftp_handler.get_time());
rtc.setTimeStruct(sim800_ftp_handler.get_time());
8/3/25:
controllo stato del modem
verifica accensione modem con testAT, esecuzione CCLK, abilitazione DBG
waitfornetwork:non riesce a collegarsi e continua a visualizzare: AT+CGREG? e AT+CEREG?
11/2/25:
aggiunto il pin STATUS (34)  

10/2/25:
  nuova procedura per power on e power off
  6/1/25
  descrizione: test collegamento rete mobile con ttgo sim7070g
  funziona con banda GSM o LTE
*/

//#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_MODEM_SIM7070
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1
#define SerialMon Serial

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

/*
   Tests enabled
*/
//precedente
//#define TINY_GSM_TEST_GPS    true
//7070g
#define TINY_GSM_TEST_GPS     false

#define TINY_GSM_TEST_GPRS    true
#define TINY_GSM_POWERDOWN    false

// set GSM PIN, if any
#define GSM_PIN ""

//necessario se voglio usare DBG; va inserito prima di chiamare la libreria TinyGsmClient.h
#define TINY_GSM_DEBUG SerialMon

// Your GPRS credentials, if any
const char apn[]  = "iot.1nce.net";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

//banda e tipo banda LTE/NB impostata, secondo la codifica
int banda = 2;
int tipoBanda = 3;

//per gestire i cicli di loop prima di eseguire deep sleep
int ciclo = 0;

//stato del modem
bool modemStatus;

#include <TinyGsmClient.h>


#include <SPI.h>
#include <SD.h>
//#include <Ticker.h>

#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

//6/1/25
//INIZIO
const char server[] = "vol.wine";
//GET
const char resource[] = "/iltuovino/test/test.txt";

//POST
const char resourcePOST[] = "/http_post/post-esp-data.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

//connessione al server
TinyGsmClient client(modem);
const int  port = 80;
//FINE

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  30          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD   115200
//#define PIN_DTR     25
//sim7070g
//11/2/25
#define PIN_STATUS  34

#define PIN_DTR     32
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4
//12/2/25


#define SD_MISO     2
#define SD_MOSI     15
#define SD_SCLK     14
#define SD_CS       13
#define LED_PIN     12

/*
//TTGO PCI-E con sim7070G
//NON FUNZIONA
//#define PIN_DTR     32
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4
#define LED_PIN     12
#define POWER_PIN               25
#define IND_PIN                 36
*/
int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input;

//11/2/25
//NON FUNZIONA
//pinMode(PIN_STATUS, OUTPUT);
String pinStatus;
//bool ledSimStatus;
    

void setup()
{
    // Set console baud rate
    Serial.begin(115200);
    delay(10);
Serial.println("Led Pin prima della definzione come OUTPUT (setup)(1): "+String(digitalRead(LED_PIN)));
    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
Serial.println("Led Pin dopo impostato come OUTPUT (setup)(2): "+String(digitalRead(LED_PIN)));    
    //delay(10000);
    
    //ATTENZIONE: con HIGH si spegne; il led è gestito poi dal 
    //12/2/25
    //tolto per test
    //digitalWrite(LED_PIN, HIGH);

    Serial.println("Led Pin dopo impostato come HIGH (setup)(3): "+String(digitalRead(LED_PIN)));    
    //delay(10000);
    //digitalWrite(LED_PIN, LOW);

//precedente
    /*
//FUNZIONA
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    // Starting the machine requires at least 1 second of low level, and with a level conversion, the levels are opposite
    delay(1000);
    digitalWrite(PWR_PIN, LOW);
    //per 7070
    delay(10000);                 //Wait for the SIM7000 communication to be normal, and will quit when receiving OK
*/
//10/2/25
//NON FUNZIONA
/*
pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    delay(100);
    digitalWrite(PWR_PIN, LOW);
delay(1100);
digitalWrite(PWR_PIN, HIGH);
delay(1500);
*/
/*
//10/2/25: v2
//FUNZIONA
pinMode(PWR_PIN, OUTPUT);
digitalWrite(PWR_PIN, LOW);
delay(1200);
digitalWrite(PWR_PIN, HIGH);
delay(300);
digitalWrite(PWR_PIN, LOW);
delay(10000);
*/
//10/2/25: v2
//FUNZIONA
Serial.println("avvio procedura modem acceso");

pinMode(PWR_PIN, OUTPUT);
//Serial.println("led Sim status[ON]: "+String(digitalRead(LED_PIN)));
//delay(10000);
     Serial.println("Led Pin alla inizio accensione (prima delay del cambio pwrpin a high): "+String(digitalRead(LED_PIN)));
    //digitalWrite(PWR_PIN, HIGH);
    //il valore di PWR_PIN cambia subito; il pin STATUS (che è quello che conta per il modulo sim)
    //cambia dopo. Il LED_PIN 
    Serial.println("Led Pin alla inizio accensione (prima delay e dopo cambio pwrpin a high): "+String(digitalRead(LED_PIN)));
    // Starting the machine requires at least 1 second of low level, and with a level conversion, the levels are opposite
    /*
    float cicloPowerOn = 0;
   
    while (digitalRead(LED_PIN)) {
      Serial.println("al secondo "+ String((cicloPowerOn++)/10) + " il modulo Sim è "+ String(digitalRead(LED_PIN)));
      delay(100);
    }
    */
    //precedente
    //delay(1500);
    Serial.println("PowerPin status alla inizio accensione (dopo delay): "+String(digitalRead(PWR_PIN)));
    Serial.println("Led Pin status alla inizio accensione (dopo delay): "+String(digitalRead(LED_PIN)));
    //digitalWrite(PWR_PIN, LOW);
     Serial.println("PowerPin status alla fine accensione (prima delay variabile): "+String(digitalRead(PWR_PIN)));
     Serial.println("Led Pin status alla fine accensione (prima delay variabile): "+String(digitalRead(LED_PIN)));
    //ciclo per verificare quando cambia lo stato
    /*
    cicloPowerOn = 0;
    while (!digitalRead(LED_PIN)) {
      Serial.println("al secondo "+ String((cicloPowerOn++)/10) + " il modulo Sim è "+ String(digitalRead(LED_PIN)));
      delay(100);
    }
    */
    //per 7070
    //il delay deve essere elevato altrimenti quando si riavvia dal deep sleep non riesce ad avviare il modem
    //  delay(3000);
    //  Serial.println("led Sim status[OFF]: "+String(digitalRead(LED_PIN)));
  Serial.println("PowerPin status alla fine accensione (dopo delay variabile): "+String(digitalRead(PWR_PIN)));
  Serial.println("LedPin status alla fine accensione (dopo delay variabile): "+String(digitalRead(LED_PIN)));
    
Serial.println("fine procedura modem acceso");
//11/2/25
    Serial.println("pinStatus da pin: "+String(digitalRead(PIN_STATUS)));

    //11/2/25
    //aggiunto un ritardo per consentire il cambio di stato del pin
    //NON FUNZIONA:
    //delay(5000);
    pinStatus = String(digitalRead(PIN_STATUS));
    Serial.println("pinStatus: "+pinStatus);

//solo per ttgo7070G
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        Serial.println("SDCard MOUNT FAIL");
    } else {
        uint32_t cardSize = SD.cardSize() / (1024 * 1024);
        String str = "SDCard Size: " + String(cardSize) + "MB";
        Serial.println(str);
    }

    Serial.println("\nWait...");

    delay(1000);


    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);


    //6/1/25
    //scegli banda


    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    //6/1/25
    //sostituito con ciclo while successivo
   /*
    Serial.println("Setup: Initializing modem...");
    Serial.println("Banda: "+ String(banda) + ", tipo banda: "+ String(tipoBanda));
    
    //precedente: restart esegue REBOOT, che ci mette 30 sec
    
    if (!modem.restart()) {
        Serial.println("Failed to restart modem, attempting to continue without restarting");
    }
    */
    
    //FINE





}

void loop()
{
//test
//DBG(" DBG funziona !");
//delay (1000);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    Serial.println("Loop: Initializing modem...");
    //11/2/25
    pinStatus = String(digitalRead(PIN_STATUS));
    Serial.println("pinStatus: "+pinStatus);
    //precedente
    //if (!modem.init()) {
    //if (!modem.init() && (modem.waitResponse(30000L) != 1)) {
    
    //NON FUNZIONA
    /*
    int avvioModem = 0;
    while (!modem.init() && avvioModem < 3) {
      modem.waitResponse(5000L);
      Serial.println("tentativo di avvio del modem n.: "+ String(avvioModem++));
      //ritardo per consentire di avviarsi
      //delay (5000);
      
    }
*/
//10/2/25
//riavvio modem
Serial.println("Loop: inizio procedura modem acceso");
//7/3/25
//test
/*
modem.sendAT("");
        
modemStatus = modem.waitResponse(10000L);
//Serial.println("modemStatus: ");
Serial.println("modemStatus: " + String(modemStatus));
    if (modemStatus == false) {
        DBG(" AT:  false ");
        Serial.println("modemStatus: è falso !");
    }


    delay(200);
    Serial.println("test comandi al comandi PRIMA di serialAT(1): AT");
*/
Serial.println("test (1): prima dell' accensione modem e il comando testAT con if");
if (!(modemStatus = modem.testAT(1000L)))
{
    DBG(" modem spento ");
    //modemPowerOn();  
    
    }
    else
    DBG(" modem acceso ");


/*
Serial.println("test (1): prima dell'accensione modem: CCLK");
modem.sendAT("+CCLK?");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" +CCLK?:  false ");
    }
    delay(200);
*/
modemPowerOn();



Serial.println("Loop: fine procedura modem acceso");
Serial.println("test (2): dopo accensione modem e il comando testAT con if: CCLK");
modem.sendAT("+CCLK?");
//anch con un tempo di 10 s non funziona   
if (modem.waitResponse(5000L) != 1) {
        DBG(" +CCLK?:  false ");
    }
    delay(200);
    
int cicloModemReady = 0;
DBG("cicloModemReady 1 - n. " + String(cicloModemReady));
/*
//FUNZIONA
Serial.println("test (3): dopo l'accensione modem e il comando testAT con while: CCLK");
while (!(modemStatus = modem.testAT(1000L)) && cicloModemReady < 10)
{
  DBG("cicloModemReady 1 - n. " + String(cicloModemReady++));
}
*/
//alternativa 1:
//FUNZIONA

Serial.println("test (3): dopo l'accensione modem e il comando testAT con if: AT");
if (!(modemStatus = modem.testAT(1000L)))
{
    DBG(" modem spento ");
    //modemPowerOn();  
    
    }
    else
    DBG(" modem acceso ");

//alternativa 2:
//FUNZIONA_50%: sendAT è false, CCLK è true
/*
Serial.println("test (3): dopo l'accensione modem e il comando sendAT con if: CCLK");
modem.sendAT("");
    if (modem.waitResponse(1000L) != 1) {
        DBG(" AT:  false ");
    }
    else
      DBG(" AT:  true ");
*/      
    //delay(200);
modem.sendAT("+CCLK?");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" +CCLK?:  false ");
    }
    else
      DBG(" +CCLK?:  true ");
    delay(200);


//NON SERVE per far funzionare il modem prima
//delay(5000);
modem.sendAT("+CCLK?");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" +CCLK?:  false ");
    }
    delay(200);
//FUNZIONA
    /*
Serial.println("test (4): dopo l'accensione modem e il comando testAT con while: AT");
cicloModemReady = 0;
DBG("cicloModemReady 2 - n. " + String(cicloModemReady));
while (!(modemStatus = modem.testAT(1000L)) && cicloModemReady < 10)
{
  DBG("cicloModemReady 2 - n. " + String(cicloModemReady++));
}
//delay(200);
modem.sendAT("+CCLK?");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" +CCLK?:  false ");
    }
    else
      DBG(" +CCLK?:  true ");
    delay(200);
    Serial.println("test (5): dopo un secondo testAT e un CCLK");
*/
Serial.println("modem avviato");
    String name = modem.getModemName();
    delay(500);
    Serial.println("Modem Name: " + name);

    String modemInfo = modem.getModemInfo();
    delay(500);
    Serial.println("Modem Info: " + modemInfo);

    // Set Modem GPS Power Control Pin to LOW ,turn off GPS power
    // Only in version 20200415 is there a function to control GPS power
    //solo per TTGO7070G
 //10/2/25
//NON FUNZIONA
/*   
    modem.sendAT("+CGPIO=0,48,1,0");
    if (modem.waitResponse(10000L) != 1) {
        DBG("Set GPS Power LOW Failed");
    }
*/
#if TINY_GSM_TEST_GPRS
    // Unlock your SIM card with a PIN if needed
    if ( GSM_PIN && modem.getSimStatus() != 3 ) {
        modem.simUnlock(GSM_PIN);
    }
#endif
//FUNZIONA
/*
modem.sendAT("");
modemStatus = modem.waitResponse(10000L);
//Serial.println("modemStatus: ");
Serial.println("modemStatus: " + String(modemStatus));
    if (modemStatus == false) {
        DBG(" AT:  false ");
        Serial.println("modemStatus: è falso !");
    }
    else
      DBG(" AT:  true ");
Serial.println("test modem acceso: (4)");
modemStatus = modem.testAT(1000L);
    Serial.print("SIM800 Test AT result (5) -> ");
    //Serial.println(res);
    if (!modemStatus)
    {
        DBG(" modem spento ");
        //modemPowerOn();  
        
    }
    else
      DBG(" modem acceso ");
*/

    modem.sendAT("+CFUN=0");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" +CFUN=0  false ");
    }

    /*
      2 Automatic
      13 GSM only
      38 LTE only
      51 GSM and LTE only
    * * * */
   Serial.println("test comandi al comandi  DOPO di CFUN=0: CCLK");
    modem.sendAT("+CCLK?");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" +CCLK?:  false ");
    }
    else
      DBG(" +CCLK?:  true ");
    delay(200);
   

    bool res = modem.setNetworkMode(banda);
    if (!res) {
        DBG("setNetworkMode  false ");
        return ;
    }
    delay(200);

    /*
      1 CAT-M
      2 NB-Iot
      3 CAT-M and NB-IoT
    * * */
    res = modem.setPreferredMode(tipoBanda);
    if (!res) {
        DBG("setPreferredMode  false ");
        return ;
    }
    else
      DBG("setPreferredMode  true ");
    delay(200);

    /*AT+CBANDCFG=<mode>,<band>[,<band>…]
     * <mode> "CAT-M"   "NB-IOT"
     * <band>  The value of <band> must is in the band list of getting from  AT+CBANDCFG=?
     * For example, my SIM card carrier "NB-iot" supports B8.  I will configure +CBANDCFG= "Nb-iot ",8
     */
    /* modem.sendAT("+CBANDCFG=\"NB-IOT\",8 ");
     if (modem.waitResponse(10000L) != 1) {
         DBG(" +CBANDCFG=\"NB-IOT\" ");
     }
     delay(200);*/

    modem.sendAT("+CFUN=1");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" +CFUN=1  false ");
    }
    else
        DBG(" +CFUN=1  true ");
    delay(200);

#if TINY_GSM_TEST_GPRS

    SerialAT.println("AT+CGDCONT?");
    delay(500);
    if (SerialAT.available()) {
        input = SerialAT.readString();
        for (int i = 0; i < input.length(); i++) {
            if (input.substring(i, i + 1) == "\n") {
                pieces[counter] = input.substring(lastIndex, i);
                lastIndex = i + 1;
                counter++;
            }
            if (i == input.length() - 1) {
                pieces[counter] = input.substring(lastIndex, i);
            }
        }
        // Reset for reuse
        input = "";
        counter = 0;
        lastIndex = 0;

        for ( int y = 0; y < numberOfPieces; y++) {
            for ( int x = 0; x < pieces[y].length(); x++) {
                char c = pieces[y][x];  //gets one byte from buffer
                if (c == ',') {
                    if (input.indexOf(": ") >= 0) {
                        String data = input.substring((input.indexOf(": ") + 1));
                        if ( data.toInt() > 0 && data.toInt() < 25) {
                            modem.sendAT("+CGDCONT=" + String(data.toInt()) + ",\"IP\",\"" + String(apn) + "\",\"0.0.0.0\",0,0,0,0");
                        }
                        input = "";
                        break;
                    }
                    // Reset for reuse
                    input = "";
                } else {
                    input += c;
                }
            }
        }
    } else {
        Serial.println("Failed to get PDP!");
    }


    Serial.println("\n\n\nWaiting for network...");
    
    //8/3/25
    //Problema: non riesce a collegarsi e continua a visualizzare: AT+CGREG? e AT+CEREG?
    //precedente
    /*
    if (!modem.waitForNetwork()) {
        delay(10000);
        return;
    }
*/
//modifica 8/3/25
//con 10 s non è talvolta sufficiente
    if (!modem.waitForNetwork(15000L)) {
       
        return;
    }

    if (modem.isNetworkConnected()) {
        Serial.println("Network connected");
    }

    Serial.println("\n---Starting GPRS TEST---\n");
    Serial.println("Connecting to: " + String(apn));
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        delay(10000);
        return;
    }

    Serial.print("GPRS status: ");
    if (modem.isGprsConnected()) {
        Serial.println("collegato alla rete mobile");
    } else {
        Serial.println("not connected");
    }

    String ccid = modem.getSimCCID();
    Serial.println("CCID: " + ccid);

    String imei = modem.getIMEI();
    Serial.println("IMEI: " + imei);

    String cop = modem.getOperator();
    Serial.println("Operator: " + cop);

    IPAddress local = modem.localIP();
    Serial.println("Local IP: " + String(local));

    int csq = modem.getSignalQuality();
    Serial.println("Signal quality: " + String(csq));

    SerialAT.println("AT+CPSI?");     //Get connection type and band
    delay(500);
    if (SerialAT.available()) {
        String r = SerialAT.readString();
        Serial.println(r);
    }

    Serial.println("\n---End of GPRS TEST---\n");
#endif

//6/1/25
//INIZIO: collegamento al server
SerialMon.print("Connecting to ");
    SerialMon.println(server);
    //non serve specificare il timeout perchè dovrebbe essere di default 75 s
    //if (!client.connect(server, port, 50)) {
    if (!client.connect(server, port)) {
        SerialMon.println(" fail");
        delay(1000);
        return;
    }
    SerialMon.println("Collegato al server");
//6/1/25: inserito delay perchè altrimenti si scollega dal gprs prima di collegarsi al server
//delay(10000);
delay(5000);
//FINE: collegamento al server


//6/1/25
//INIZIO: HTTP GET E POST
// Make a HTTP GET request:
    SerialMon.println("Performing HTTP GET request...");
    client.print(String("GET ") + resource + " HTTP/1.1\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.println();

    unsigned long timeout = millis();
//tempo di connessione necessario per eseguire la richiesta HTTP sopra indicata
//3/1/25
//la richiesta non viene sempre eseguita ed è stato aumentato il tempo
 while (client.connected() && millis() - timeout < 60000L) {
//precedente
//    while (client.connected() && millis() - timeout < 10000L) {
        //SerialMon.println(" client.connected");
        // Print available data
        while (client.available()) {
            //SerialMon.println(" client.available");
            char c = client.read();
            SerialMon.print(c);
            
            timeout = millis();
        }
    }
    SerialMon.println("HTTP GET: FINE");
    SerialMon.println();

    // Make a HTTP POST request:
    SerialMon.println("inizio HTTP POST");
SerialMon.print("Connecting to ");
    SerialMon.print(server);
    if (!client.connect(server, port)) {
        SerialMon.println(" fail");
        delay(10000);
        return;
    }
    SerialMon.println("Collegato al server per HTTP POST");
    
    //String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME291&location=Office&value1=24.75&value2=49.54&value3=1005.14&value4=banda";
    String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME291&location=7070&value1=24.75&value2=49.54&value3=1005.14";
//INIZIO
//sequenza estratta da file TTGO_Tcall_http_post_2

client.print(String("POST ") + resourcePOST + " HTTP/1.1\r\n");
      client.print(String("Host: ") + server + "\r\n");
      client.println("Connection: close");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(httpRequestData.length());
      client.println();
      client.println(httpRequestData);

//      unsigned long timeout = millis();
      while (client.connected() && millis() - timeout < 10000L) {
        // Print available data (HTTP response from server)
        while (client.available()) {
          char c = client.read();
          SerialMon.print(c);
          timeout = millis();
        }
      }
      SerialMon.println("HTTP POST - FINE");
//FINE: HTTP GET E POST

#if TINY_GSM_TEST_GPRS
    modem.gprsDisconnect();
    if (!modem.isGprsConnected()) {
        Serial.println("GPRS disconnected");
    } else {
        Serial.println("GPRS disconnect: Failed.");
    }
#endif

#if TINY_GSM_TEST_GPS
    Serial.println("\n---Starting GPS TEST---\n");
    // Set Modem GPS Power Control Pin to HIGH ,turn on GPS power
    // Only in version 20200415 is there a function to control GPS power
    modem.sendAT("+CGPIO=0,48,1,1");
    if (modem.waitResponse(10000L) != 1) {
        DBG("Set GPS Power HIGH Failed");
    }

    modem.enableGPS();

    float lat,  lon;
    while (1) {
        if (modem.getGPS(&lat, &lon)) {
            Serial.printf("lat:%f lon:%f\n", lat, lon);
            break;
        } else {
            Serial.print("getGPS ");
            Serial.println(millis());
        }
        delay(2000);
    }
    modem.disableGPS();

    // Set Modem GPS Power Control Pin to LOW ,turn off GPS power
    // Only in version 20200415 is there a function to control GPS power
    modem.sendAT("+CGPIO=0,48,1,0");
    if (modem.waitResponse(10000L) != 1) {
        DBG("Set GPS Power LOW Failed");
    }

    Serial.println("\n---End of GPRS TEST---\n");
#endif
//FINE: TEST_GPS



#if TINY_GSM_POWERDOWN
    // Try to power-off (modem may decide to restart automatically)
    // To turn off modem completely, please use Reset/Enable pins
    Serial.println("avvio power down con CPOWD");
    //NON FUNZIONA: lascia il modem acceso e al riavvio non va
    modem.sendAT("+CPOWD=1");
    if (modem.waitResponse(10000L) != 1) {
        DBG("+CPOWD=1");
    }
    Serial.println("fine power down con CPOWD");
    //questo delay serve solo per vedere se il led modem si spegne
    delay(5000);
    //equivale a +CPOWD=1
    //NON FUNZIONA: lascia il modem acceso e al riavvio non va
    modem.poweroff();
    Serial.println("Poweroff.");
#endif

//10/2/25
//power down: utilizzo di power_pin
//NON FUNZIONA
/*digitalWrite(PWR_PIN, HIGH);
delay(100);
digitalWrite(PWR_PIN, LOW);
delay(1300);
digitalWrite(PWR_PIN, HIGH);
delay(2000);
digitalWrite(PWR_PIN, LOW);
delay(100);
*/
//11/2/25
    pinStatus = String(digitalRead(PIN_STATUS));
    Serial.println("pinStatus: "+pinStatus);

//10/2/25: v2
//FUNZIONA
Serial.println("avvio Poweroff.");
Serial.println("test: prima dello spegnimento modem e il comando sendAT con if");
modem.sendAT("");
    if (modem.waitResponse(1000L) != 1) {
        DBG(" AT:  false ");
    }
    else
      DBG(" AT:  true ");
modemPowerOff();
      /*
      digitalWrite(PWR_PIN, HIGH);
delay(1500);
digitalWrite(PWR_PIN, LOW);
//incrementato delay da 2 a 3 s altrimenti non si spegneva sempre
delay(3000);
*/
Serial.println("fine Poweroff.");
//questo delay serve solo per vedere se il led modem si spegne
//delay(5000);
//11/2/25
//controllo spegnimento modem

//soluzione 1
//FUNZIONA solo se uso il delay
/*
delay(1000);
Serial.println("test: dopo lo spegnimento modem e il comando sendAT con if");
modem.sendAT("");
    if (modem.waitResponse(1000L) != 1) {
        DBG(" modem spento ");
    }
    else
      DBG(" modem acceso ");
*/
//soluzione 2
//FUNZIONA

Serial.println("test: dopo lo spegnimento modem e il comando testAT con if");
if (!(modemStatus = modem.testAT(1000L)))
{
    DBG(" modem spento ");
    //modemPowerOn();  
    
    }
    else
    DBG(" modem acceso ");


    pinStatus = String(digitalRead(PIN_STATUS));
    Serial.println("pinStatus: "+pinStatus);
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("avvio deep sleep");
    delay(200);
    Serial.println("ciclo n: "+String(ciclo++));
    if (ciclo >0)
      esp_deep_sleep_start();

    Serial.println("This will never be printed");
}

//SITUAZIONE: DA TESTARE
void scegliBanda(int band)
{
    //il risultato di questa sequenza di comandi (messaggio su serial.println) si ottiene solo col loop
    //dove è eseguito Serial.write(SerialAT.read())
    //SerialMon.println("Imposta solo LTE-M");
    //SerialAT.println("AT+CNMP=38");
    //banda = "LTE";

    SerialMon.println("Imposta: " + band);
    SerialAT.println("AT+CNMP=13");
    //banda = "GSM";
   
    //SerialMon.println("Imposta LTE-M");
    //SerialAT.println("AT+CMNB=1");
    
    
    //SerialMon.println("Imposta banda per LTE-M");
    //SerialAT.println("AT+CBANDCFG=\"CAT-M\",1,2,3,4,5,8,12,13,14,18,19,20,25,26,27,28,66,85");
    delay(10000);
    SerialMon.println("1a Verifica banda");
    SerialAT.println("AT+COPS?");
    
    
}
//FINE: scegliBanda
bool checkModem ()
{
    if (modemStatus = modem.testAT(1000L))
        DBG("il modem è acceso ");
    else
        DBG("il modem è spento");
    return modemStatus;
}

void modemPowerOn ()
{
    int cicloPowerOn = 0;
    while (!checkModem() && (cicloPowerOn++) < 3)
    {
         
            digitalWrite(PWR_PIN, HIGH);
            delay(1500);
            digitalWrite(PWR_PIN, LOW);
            //incrementato delay da 2 a 3 s altrimenti non si spegneva sempre
            delay(3000);
        

    }
    if (!modemStatus)
    {    
        DBG("il modem non si è acceso. Riavvio la scheda");
        delay(1000);
        ESP.restart();
    }
    
    
}

void modemPowerOff ()
{
    int cicloPowerOn = 0;
    while (checkModem() && (cicloPowerOn++) < 3)
    {
         
            digitalWrite(PWR_PIN, HIGH);
            delay(1500);
            digitalWrite(PWR_PIN, LOW);
            //incrementato delay da 2 a 3 s altrimenti non si spegneva sempre
            delay(3000);
        

    }
    if (modemStatus)
    {    
        DBG("il modem non si è spento");
        delay(1000);
        //ESP.restart();
    }
    

}