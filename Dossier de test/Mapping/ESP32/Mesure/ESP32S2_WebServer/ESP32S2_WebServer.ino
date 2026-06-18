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
// --- NOUVELLE VITESSE : 921600 bauds ---
#define BAUDRATE 921600 
#define TX1 17
#define RX1 18
#define UART_BUFFER 128 

CRGB leds[NUM_LEDS];
char XML[2048];
char buf[64];

int count_once = 1;
uint16_t RxIndex = 0;
bool btn_state = 0; 

char buffer[UART_BUFFER] = {0};
// --- PASSAGE À 10 VARIABLES ---
float floatValues[10]; 
float v_pos_c1 = 0, v_pos_m1 = 0, v_cur_c1 = 0, v_cur_m1 = 0;
float v_fc1f = 0, v_xr1 = 0, v_uc1 = 0, v_int_i1 = 0, v_state = 0, v_v1 = 0;

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
  int bytesToRead = 128;
  while (MySerial.available() > 0 && bytesToRead > 0) {
    uint8_t c = MySerial.read();
    if (RxIndex < UART_BUFFER) {
      buffer[RxIndex++] = c;
    } else {
      RxIndex = 0;
    }
    bytesToRead--;
  }

  if (RxIndex > 0) {
    buffer[RxIndex] = '\0';
    if (buffer[0] == 0x02 && buffer[RxIndex - 1] == 0x03) {
      char* token = strtok(&buffer[1], ",");  
      int i = 0;

      // Boucle de parsing pour 10 variables
      while (token != NULL && i < 10) {
        floatValues[i++] = atof(token);
        token = strtok(NULL, ",");  
      }
      updateValues();
      RxIndex = 0;
    } 
    else {
      RxIndex = 0;
    }
  }
  
  server.handleClient(); 
}

void updateValues() {
  v_pos_c1 = floatValues[0];
  v_pos_m1 = floatValues[1];
  v_cur_c1 = floatValues[2];
  v_cur_m1 = floatValues[3];
  v_fc1f   = floatValues[4];
  v_xr1    = floatValues[5];
  v_uc1    = floatValues[6];
  v_int_i1 = floatValues[7];
  v_state  = floatValues[8];
  v_v1     = floatValues[9]; // Ajout de la vitesse
}

// Sécurisation de l'envoi de la commande Start/Stop pour l'UART (Évite que la trame soit coupée)
void ProcessButton_START() {
  btn_state = 1;
  MySerial.print("\x02" "1" "\x03");
  leds[0] = CRGB::Red;
  FastLED.show();
  server.send(200, "text/plain", ""); 
}

void ProcessButton_STOP() {
  btn_state = 0;
  MySerial.print("\x02" "0" "\x03");
  leds[0] = CRGB::Green;
  FastLED.show();
  server.send(200, "text/plain", ""); 
}

void SendXML() {
  strcpy(XML, "<?xml version = '1.0'?>\n<Data>\n");
  
  sprintf(buf, "<posC1>%f</posC1>\n", v_pos_c1); strcat(XML, buf);
  sprintf(buf, "<posM1>%f</posM1>\n", v_pos_m1); strcat(XML, buf);
  sprintf(buf, "<curC1>%f</curC1>\n", v_cur_c1); strcat(XML, buf);
  sprintf(buf, "<curM1>%f</curM1>\n", v_cur_m1); strcat(XML, buf);
  sprintf(buf, "<fc1f>%f</fc1f>\n", v_fc1f); strcat(XML, buf);
  sprintf(buf, "<xr1>%f</xr1>\n", v_xr1); strcat(XML, buf);
  sprintf(buf, "<uc1>%f</uc1>\n", v_uc1); strcat(XML, buf);
  sprintf(buf, "<intI1>%f</intI1>\n", v_int_i1); strcat(XML, buf);
  sprintf(buf, "<state>%f</state>\n", v_state); strcat(XML, buf);
  sprintf(buf, "<v1>%f</v1>\n", v_v1); strcat(XML, buf); // Envoi de la vitesse au navigateur

  strcat(XML, "</Data>\n");
  server.send(200, "text/xml", XML);
}

void SendWebsite() {
  server.send(200, "text/html", PAGE_MAIN);
}