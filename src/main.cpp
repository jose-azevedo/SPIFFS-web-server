#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "SPIFFS.h"

WebServer server;

int i = 0;

uint8_t pin_led = 5;
const char* ssid = "Azevedo";
const char* password = "familiaea";

void serveIndexFile()
{
  File file = SPIFFS.open("/index.html", "r");
  if(file)
  {
    Serial.println("Serving index.html");
    server.streamFile(file, "text/html");
    file.close();
  }
}

void serveStyleFile()
{
  File file = SPIFFS.open("/style.css", "r");
  if(file)
  {
    Serial.println("Serving style.css");
    server.streamFile(file, "text/css");
    file.close();
  }
}

void serveScriptFile()
{
  File file = SPIFFS.open("/script.js", "r");
  if(file)
  {
    Serial.println("Serving script.js");
    server.streamFile(file, "text/javascript");
    file.close();
  }
}

void listFiles() {

// alguma coisa a ver o com o método openNextFile() do objeto File

}

void setup(){

  pinMode(pin_led, OUTPUT);
  WiFi.begin(ssid,password);
  Serial.begin(115200);
  if (SPIFFS.begin()){
    Serial.println("SPIFFS iniciado");
  } else {
    Serial.println("SPIFFS não iniciado");
    return;
  }
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Adress: ");
  Serial.println(WiFi.localIP());

  if(!MDNS.begin("monitoracaogedae"))
  {
    Serial.println("MDNS falhou");
    return;  
  }
  
  MDNS.addService("http", "tcp", 80);
  
  server.on("/", serveIndexFile);
  server.on("/style.css", serveStyleFile);
  server.on("/script.js", serveScriptFile);
  server.begin();

}

void loop(void)
{
server.handleClient();
}