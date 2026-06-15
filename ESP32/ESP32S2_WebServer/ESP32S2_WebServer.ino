/*
 Ce code permet de :
 - Générer un wifi sur adressE IP unique,
 - Héberger un site web
 - Détecter l'appui d'un bouton sur le site,
 - Mettre à jour des valeurs en float sur le site hébergé,
 - Gérer la communication UART entre l'ESP32S2 et le DSP dans les deux sens ainsi qu'au traitement des données reçues.

  Options de compilation Arduino
  1. esp32 dev module
  2. USB CDC On Boot Enable
  3. Erase All Flash Before Sketch Upload Enable
  upload speed 115200
  cpu speed 240 mhz
  flash speed 80 mhz
  flash mode qio
  flash size 4mb
  partition scheme default

 Auteur : Thomas Freyche - 2025
*/

#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include <stdlib.h>  // Pour atof()
#include <string.h>  // Pour strtok()

#include "WebPage.h"

// Nom et mot de passe du wifi crée par l'esp32
#define AP_SSID "ESP32-S2-WIFI"
#define AP_PASS "TFE-TB2025-HEIG"

// Configuration de la LED RGB
#define LED_PIN         1          // Pin de données 
#define LED_ENABLE_PIN  2          // Pin d'activation de l'alimentation (sur le transistor)
#define NUM_LEDS        1          // Une seule LED RGB sur le board

// Congiguration of UART pins
HardwareSerial MySerial(1);
#define BAUDRATE 115200
#define TX1 17
#define RX1 18
#define UART_BUFFER 128 

// Variable globale
int count_once = 1;
uint16_t RxIndex = 0;
uint32_t SensorUpdate = 0;
bool btn_state = 0; 
float pos_ind1 = 0.0, pos_ind2 = 0.0, pos_ind3 = 0.0, pos_ind4 = 0.0;
float cur_ind1 = 0.0, cur_ind2 = 0.0, cur_ind3 = 0.0, cur_ind4 = 0.0;

// Declare our FastLED strip object:
CRGB leds[NUM_LEDS];

// the XML array size needs to be bigger that your maximum expected size. 2048 is way too big for this example
char XML[2048];
// just some buffer holder for char operations
char buf[32];

// Uart Receive
char buffer[UART_BUFFER] = {0};
float floatValues[8];  

// Adresse IP du point d'accès
IPAddress Actual_IP;
IPAddress PageIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Create the server
WebServer server(80); 

void setup() {
  Serial.begin(BAUDRATE);  
  // while(!Serial) Empêche l'execution du code si arduino n'est pas lancé !
	delay(1000);
  Serial.println("=== GroundStudio Carbon S2 - Hosting WebPage ===");

  // Configuration de la LED RGB
  pinMode(LED_ENABLE_PIN, OUTPUT);
  digitalWrite(LED_ENABLE_PIN, LOW);  // Active l'alimentation
  
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  leds[0] = CRGB::Black;  // Éteind la LED
  FastLED.show();
  
  Serial.println("FastLED initialisé");
  
  // Configuration de l'UART
  MySerial.begin(BAUDRATE, SERIAL_8N1, RX1, TX1, false);
  delay(1000);
  Serial.println("UART initialisé");

  // Test de la LED au démarrage
  Serial.println("Test de la LED...");
  leds[0] = CRGB::Red;
  FastLED.show();
  delay(1000);
  leds[0] = CRGB::Green;
  FastLED.show();
  delay(1000);
  leds[0] = CRGB::Blue;
  FastLED.show();
  delay(1000);
  leds[0] = CRGB::Black;
  FastLED.show();
  Serial.println("Test LED terminé");

  // if your web page or XML are large, you may not get a call back from the web page
  // and the ESP will think something has locked up and reboot the ESP
  // not sure I like this feature, actually I kinda hate it
  // disable watch dog timer 0
  disableCore0WDT();

  // Configuration du point d'accès WiFi
  Serial.println("Configuration du point d'accès WiFi...");
  
  WiFi.softAP(AP_SSID, AP_PASS);
  delay(100);
  WiFi.softAPConfig(PageIP, gateway, subnet);
  delay(100);
  WiFi.setSleep(false);
  Actual_IP = WiFi.softAPIP();
  Serial.print("IP address: "); Serial.println(Actual_IP);
  
  // Configuration des routes du serveur web
	// these calls will handle data coming back from your web page
  // this one is a page request, upon ESP getting / string the web page will be sent
  server.on("/", SendWebsite);
  server.on("/xml", SendXML);
  server.on("/BUTTON_START", ProcessButton_START);
  server.on("/BUTTON_STOP", ProcessButton_STOP);
  
  // Démarrer le serveur
  server.begin();
  Serial.println("Serveur web démarré");
  Serial.println("Connectez-vous au WiFi 'ESP32-S2-WIFI' puis allez à l'IP correspondant");
}

void loop() {
	if(count_once == 1){
		Serial.println("Loop start");
		count_once = 2;
	}

  // Test avec compteur envoyé à la page web chaque seconde
//  if ((millis() - SensorUpdate) >= 100) { // If >= x seconds changed -> have to also change in .h : setInterval(process, x);
//     SensorUpdate = millis();

//     // Update only when the start button has been pressed
//     if (btn_state == 1){
//       updateValues();
//     }
//   } 

// Test de l'UART en reliant la pin Tx et Rx de l'ESP32-S2
// Fonctionne grâce au bouton (btn_state)
  // if (MySerial.available() > 2) {
  //   char c = MySerial.read();
  //  if (c == '$'){
  //     while (!MySerial.available()); // Attend la prochaine donnée
  //     char data = MySerial.read();
  //     if (data == '0') {
  //       leds[0] = CRGB::Green;
  //     }
  //     else if (data == '1') {      
  //       leds[0] = CRGB::Red;
  //     }
  //       FastLED.show();
  //       taskYIELD(); // Permet au WiFi de reprendre la main
  //   }
  // } 

// Réception de l'UART
  // Réception de l'UART (Limité à 128 caractères par boucle pour ne pas bloquer le WebServer)
  int bytesToRead = 128;
  while (MySerial.available() > 0 && bytesToRead > 0) {
  uint8_t c = MySerial.read();
  
  if (RxIndex < UART_BUFFER) {
    buffer[RxIndex++] = c;
  } else {
    RxIndex = 0;
    Serial.println("Buffer plein, réinitialisation");
  }
  bytesToRead--;
}
// Envoi Hello World!
  // if (RxIndex > 0) {
  //   Serial.print("Message reçu : ");
  //   // for (int i = 0; i < RxIndex; i++) {
  //     Serial.println(buffer); // [i]
  //   // }
  //   Serial.println();
  //   RxIndex = 0; 
  // }

// Gérer les floats
  // Traitement des données une fois toute la trame reçue
  if (RxIndex > 0) {
    // Ajout d'un caractère de fin 
    buffer[RxIndex] = '\0';

    // Vérification de la forme de la trame
    if (buffer[0] == 0x02 && buffer[RxIndex - 1] == 0x03) {
      char* token = strtok(&buffer[1], ",");  // Découpe la chaîne après '\x02' et avant la première virgule de séparation
      int i = 0;

      // Conversion des token en floats
      while (token != NULL && i < 8) {
      floatValues[i++] = atof(token);
      token = strtok(NULL, ",");  
      }

      // Afficher les floats reçus (facultatif)
      // Serial.print("Valeurs reçues : ");
      // for (int i = 0; i < 4; i++) {
      //   Serial.print(floatValues[i]);
      //   Serial.print(" ");
      // }
      // Serial.println();

      // Mise à jour des valeurs de position pour la page web
      updateValues();

      // Réinitialiser l'index du buffer après traitement
      RxIndex = 0;
    } 
    // Si la trame ne commence pas par \x02 ou ne se termine pas par \x03, ignorer
    else {
      Serial.println("Trame invalide");
      RxIndex = 0;
    }
  }
  
  server.handleClient(); // Absolutely necessary 
}

// Mise à jour des valeurs de position sur la page web
void updateValues() {
// Simulation avec des compteurs
  // pos_ind1 += 1.0;
  // if (pos_ind1 >= 100.0) pos_ind1 = 0.0;
  
  // pos_ind2 += 0.5;
  // if (pos_ind2 >= 50.0) pos_ind2 = 0.0;
  
  // pos_ind3 += 0.5;
  // if (pos_ind3 >= 20.0) pos_ind3 = 0.0;
  
  // pos_ind4 += 0.1;
  // if (pos_ind4 >= 10.0) pos_ind4 = 0.0;

// Mise à jour des position via l'uart
  pos_ind1 = floatValues[0];
  pos_ind2 = floatValues[1];
  pos_ind3 = floatValues[2];
  pos_ind4 = floatValues[3];

// Mise à jour des courants via l'uart
  cur_ind1 = floatValues[4];
  cur_ind2 = floatValues[5];
  cur_ind3 = floatValues[6];
  cur_ind4 = floatValues[7];
}

// Fonction de status pour le démarrage de la sustentation
void ProcessButton_START() {

	Serial.println("Bouton start appuyé");
  btn_state = 1;
  MySerial.print('\x02');
  MySerial.print(btn_state);
  MySerial.print('\x03');

  // LED Rouge pour START 
  leds[0] = CRGB::Red;
  FastLED.show();
  taskYIELD(); // Permet au WiFi de reprendre la main

  // regardless if you want to send stuff back to client or not
  // you must have the send line--as it keeps the page running
  // if you don't want feedback from the MCU--or let the XML manage
  // sending feeback

  // option 1 -- keep page live but dont send any thing
  // here i don't need to send and immediate status, any status
  // like the illumination status will be send in the main XML page update
  // code
  server.send(200, "text/plain", ""); //Send web page
}

// Fonction de status pour l'arrêt de la sustentation 
void ProcessButton_STOP() {

	Serial.println("Bouton stop appuyé");
  btn_state = 0;
  MySerial.print('\x02');
  MySerial.print(btn_state);
  MySerial.print('\x03');
  
   // LED Verte pour STOP 
  leds[0] = CRGB::Green;
  FastLED.show();
  taskYIELD(); // Permet au WiFi de reprendre la main

  // Keep the page live by sending something back
  server.send(200, "text/plain", ""); //Send web page
}

void SendXML() {

  // Serial.println("sending xml");

  strcpy(XML, "<?xml version = '1.0'?>\n<Data>\n");

  // send the inductor 1 position
  sprintf(buf, "<valInd1>%f</valInd1>\n", pos_ind1);
  strcat(XML, buf);

  // send the inductor 2 position
  sprintf(buf, "<valInd2>%f</valInd2>\n", pos_ind2);
  strcat(XML, buf);

  // send the inductor 3 position
  sprintf(buf, "<valInd3>%f</valInd3>\n", pos_ind3);
  strcat(XML, buf);

  // send the inductor 4 position
  sprintf(buf, "<valInd4>%f</valInd4>\n", pos_ind4);
  strcat(XML, buf);

  // send the inductor currents
  sprintf(buf, "<curInd1>%f</curInd1>\n", cur_ind1);
  strcat(XML, buf);
  sprintf(buf, "<curInd2>%f</curInd2>\n", cur_ind2);
  strcat(XML, buf);
  sprintf(buf, "<curInd3>%f</curInd3>\n", cur_ind3);
  strcat(XML, buf);
  sprintf(buf, "<curInd4>%f</curInd4>\n", cur_ind4);
  strcat(XML, buf);

  strcat(XML, "</Data>\n");

  // wanna see what the XML code looks like?
  // actually print it to the serial monitor and use some text editor to get the size
  // then pad and adjust char XML[2048]; above
  // Serial.println(XML);

  // Keep the page live 
  server.send(200, "text/xml", XML);
}

// code to send the main web page
// PAGE_MAIN is a large char defined in WebPage.h
void SendWebsite() {

  Serial.println("sending web page");

  // Keep the page live 
  server.send(200, "text/html", PAGE_MAIN);
}
