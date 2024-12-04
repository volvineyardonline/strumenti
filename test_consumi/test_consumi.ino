#include <Arduino.h>

// Durate in millisecondi
#define ACTIVE_TIME 120000     // 2 minuti (attivo)
#define LIGHT_SLEEP_TIME 120000 // 2 minuti (light sleep)
#define DEEP_SLEEP_TIME 120000  // 2 minuti (deep sleep)

// Funzione per entrare in Light Sleep
void enterLightSleep() {
  Serial.println("Entrando in Light Sleep...");
  esp_sleep_enable_timer_wakeup(LIGHT_SLEEP_TIME * 1000); // Configura la durata del light sleep
  esp_light_sleep_start(); // Entra in Light Sleep
  Serial.println("Sveglio dal Light Sleep.");
}

// Funzione per entrare in Deep Sleep
void enterDeepSleep() {
  Serial.println("Entrando in Deep Sleep...");
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME * 1000); // Configura la durata del deep sleep
  esp_deep_sleep_start(); // Entra in Deep Sleep
}

// Setup iniziale
void setup() {
  Serial.begin(115200); // Avvia la comunicazione seriale
  delay(1000);          // Aspetta un attimo per stabilire la connessione
}

// Loop principale
void loop() {
  // Stato Attivo
  Serial.println("Stato: Attivo per 2 minuti.");
  delay(ACTIVE_TIME);

  // Stato Light Sleep
  enterLightSleep();

  // Stato Deep Sleep
  enterDeepSleep();

  // Non si ritorna qui dopo il Deep Sleep
}