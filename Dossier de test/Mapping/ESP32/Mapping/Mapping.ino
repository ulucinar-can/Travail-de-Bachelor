#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include <stdlib.h>  
#include <string.h>  

#include "WebPage.h"

#define AP_SSID "ESP32-S2-WIFI"
#define AP_PASS "TFE-TB2025-HEIG"

#define LED_PIN         1          
#define LED_ENABLE_PIN  2          
#define NUM_LEDS        1          

HardwareSerial MySerial(1);

#define BAUDRATE 921600 
#define TX1 17
#define RX1 18

CRGB leds[NUM_LEDS];

char XML[1024];
char buf[64];

bool btn_state = 0; 

// --- NOUVELLES VARIABLES GLOBALES (Allégées pour 4 positions) ---
float floatValues[4] = {0, 0, 0, 0}; 
float v_pos1 = 0, v_pos2 = 0, v_pos3 = 0, v_pos4 = 0;

// Variables pour la machine à états UART (Réception Binaire)
static uint8_t rxState = 0;
static uint8_t floatBytes[16]; // 4 floats = 16 octets
static uint8_t byteCount = 0;

IPAddress Actual_IP;
IPAddress PageIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

void setup() {
  Serial.begin(BAUDRATE);  
  delay(1000);
  
  pinMode(LED_ENABLE_PIN, OUTPUT);
  digitalWrite(LED_ENABLE_PIN, LOW);  
  
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  leds[0] = CRGB::Black;  
  FastLED.show();

  MySerial.begin(BAUDRATE, SERIAL_8N1, RX1, TX1, false);
  delay(1000);

  disableCore0WDT();

  WiFi.softAP(AP_SSID, AP_PASS);
  delay(100);
  WiFi.softAPConfig(PageIP, gateway, subnet);
  delay(100);
  WiFi.setSleep(false);
  Actual_IP = WiFi.softAPIP();
  
  server.on("/", SendWebsite);
  server.on("/xml", SendXML);
  server.on("/BUTTON_START", ProcessButton_START);
  server.on("/BUTTON_STOP", ProcessButton_STOP);
  
  server.begin();
}

void loop() {
  // Machine à états pour lire le binaire entrant très rapidement
  while (MySerial.available() > 0) {
    uint8_t c = MySerial.read();

    switch (rxState) {
      case 0: // Attente de 0xAA (Start 1)
        if (c == 0xAA) rxState = 1;
        break;

      case 1: // Attente de 0xBB (Start 2)
        if (c == 0xBB) {
          rxState = 2;
          byteCount = 0; // Prêt à lire les données
        } else if (c != 0xAA) {
          rxState = 0; // Faux départ
        }
        break;

      case 2: // Lecture des 16 octets de données
        floatBytes[byteCount++] = c;
        if (byteCount == 16) {
          rxState = 3; // Fin de lecture, attente du Stop 1
        }
        break;

      case 3: // Attente de 0xCC (Stop 1)
        if (c == 0xCC) rxState = 4;
        else rxState = 0; // Erreur de trame
        break;

      case 4: // Attente de 0xDD (Stop 2)
        if (c == 0xDD) {
          // Trame parfaite reçue ! On copie directement dans les variables float
          memcpy(floatValues, floatBytes, sizeof(floatValues));
          updateValues(); 
        }
        rxState = 0; // On boucle pour la prochaine trame
        break;
    }
  }
  
  server.handleClient(); 
}

// Mise à jour des 4 variables
void updateValues() {
  v_pos1 = floatValues[0];
  v_pos2 = floatValues[1];
  v_pos3 = floatValues[2];
  v_pos4 = floatValues[3];
}

void ProcessButton_START() {
  btn_state = 1;
  // Envoie l'ordre au C2000 de démarrer l'envoi de la télémétrie
  MySerial.print("\x02" "1" "\x03"); 
  leds[0] = CRGB::Red;
  FastLED.show();
  server.send(200, "text/plain", ""); 
}

void ProcessButton_STOP() {
  btn_state = 0;
  // Envoie l'ordre au C2000 de stopper l'envoi
  MySerial.print("\x02" "0" "\x03");
  leds[0] = CRGB::Green;
  FastLED.show();
  server.send(200, "text/plain", ""); 
}

void SendXML() {
  strcpy(XML, "<?xml version = '1.0'?>\n<Data>\n");
  
  sprintf(buf, "<p1>%f</p1>\n", v_pos1); strcat(XML, buf);
  sprintf(buf, "<p2>%f</p2>\n", v_pos2); strcat(XML, buf);
  sprintf(buf, "<p3>%f</p3>\n", v_pos3); strcat(XML, buf);
  sprintf(buf, "<p4>%f</p4>\n", v_pos4); strcat(XML, buf);

  strcat(XML, "</Data>\n");
  server.send(200, "text/xml", XML);
}

void SendWebsite() {
  server.send(200, "text/html", PAGE_MAIN);
}