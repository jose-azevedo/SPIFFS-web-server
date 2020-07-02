#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <SD.h>
#include "SPI.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#define RXD2 16
#define TXD2 17
#define CS_PIN 4

AsyncWebServer server(80);

int i = 0;

// Credenciais da rede WiFi, nome da rede e senha
const char* ssid = "Azevedo";
const char* password = "familiaea";

IPAddress staticIP(192, 168, 15, 47);
IPAddress gateway(192, 168, 15, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 15, 1);
IPAddress dns2(200, 175, 89, 139);

void setup(){
// Inicialização de recursos, WiFi, comunicação serial e sistema de armazenamento de memória flash interna
  if (WiFi.config(staticIP, gateway, subnet, dns1, dns2) == false) {
    Serial.println("Configuration failed.");
  }
  WiFi.begin(ssid, password);

  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  SPI.begin(18, 19, 23);
  
  if (SD.begin(CS_PIN)) Serial.println("Cartão SD inicializado");
  else  Serial.println("Inicialização do cartão SD falhou");

  if (SPIFFS.begin()){
    Serial.println("SPIFFS iniciado");
  } else {
    Serial.println("SPIFFS não iniciado");
    return;
  }

// Espera até que a rede esteja conectada
  while(WiFi.status()!=WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");

// Imprime na tela o IP atribuído dinamicamente
  Serial.print("IP Adress: "); 
  Serial.println(WiFi.localIP());

// "Paths", ou rotas, a que o servidor responde ao receber uma requisição de um cliente. Na raiz "/" o servidor envia a página HTML. Como os arquivos de estilo, código JavaScript e logo estão referenciados na página HTML as requisições pedindo por esses arquivos são enviadas automaticamente e por isso é preciso haver uma resposta para cada uma delas.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/index.html", "text/html");});
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/style.css", "text/css");});
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/script.js", "text/javascript");});
  server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/logo.png", "image/png");});

// Descrito no código JavaScript, na página, ao se apertar o botão "Listar arquivos" o cliente envia uma requisição na rota "/listFiles", que executa a função a seguir
  server.on("/listFiles", HTTP_GET, [](AsyncWebServerRequest *request) {
    File dir = SD.open("/dados");
    File file = dir.openNextFile();
    String list = "";

    while (file) {
      list += file.name();
      list += "|";
      file = dir.openNextFile();
    }

    Serial.println(list);
    Serial.println("Lista enviada");
    dir.close();
    file.close();
    request->send(200, "text/plain", list);
    });

  server.on("/download", HTTP_POST, [](AsyncWebServerRequest *request) {
    String fname = "/dados/";
    fname += request->arg("filename"); // No AsyncWebServer é preciso especificar o 'name' do argumento enviado, não o índice
    Serial.print("Download requisitado: "); // O nome de um argumento ainda pode ser verificado com a sintaxe request->argName(índice)
    Serial.println(fname);
    request->send(SD, fname, "text/text", true);
    });  
    
  server.begin();

}

void loop(void) {}