#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "SPIFFS.h"

WebServer server;

int i = 0;

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

void serveLogo()
{
  File file = SPIFFS.open("/gedae.png", "r");
  if(file)
  {
    Serial.println("Serving gedae.png");
    server.streamFile(file, "image/png");
    file.close();
  }
}

void listFiles() {
File dir = SPIFFS.open("/");
File file = dir.openNextFile();
String list = "";

while (file) {
  list += file.name();
  list += "|";
  file = dir.openNextFile();
}

Serial.println("Lista enviada");
dir.close();
file.close();
server.send(200, "text/plain", list);
}

void download() {
  File file = SPIFFS.open("/dados/" + server.arg(0), "r");
  if (file) {
    server.sendHeader("Content-Disposition", "attachment; filename=" + server.arg(0));
    server.sendHeader("Connection", "close");
    server.streamFile(file, "text/text");
    file.close();
  } 
}

void setup(){

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
  server.on("/script.js", serveScriptFile);
  server.on("/style.css", serveStyleFile);
  server.on("/gedae.png", serveLogo);
  server.on("/listFiles", listFiles);
  server.on("/download", download);
  server.begin();

}

void loop(void) 
{
server.handleClient();
}