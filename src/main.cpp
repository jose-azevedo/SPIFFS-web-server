#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

AsyncWebServer server(80);

int i = 0;

const char* ssid = "Azevedo";
const char* password = "familiaea";

void setup(){

  WiFi.begin(ssid,password);

  Serial.begin(115200);

  if (SPIFFS.begin()){
    Serial.println("SPIFFS iniciado");
  } else {
    Serial.println("SPIFFS não iniciado");
    return;
  }

  while(WiFi.status()!=WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Adress: ");
  Serial.println(WiFi.localIP());

  if(!MDNS.begin("monitoracaogedae")) {
    Serial.println("MDNS falhou");
    return;  
  }
  
  MDNS.addService("http", "tcp", 80);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/index.html", "text/html");});
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/style.css", "text/css");});
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/script.js", "text/javascript");});
  server.on("/gedae.png", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/gedae.png", "image/png");});

  server.begin();

}

void loop(void) {}