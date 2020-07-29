#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include "SPI.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#define RXD2 16
#define TXD2 17
#define CS_PIN 4
#define httpPort 8008

AsyncWebServer server(httpPort);
HTTPClient http;

bool saveFlag = false, endFlag = false, startFlag = false;
char myChar;
String lineBuffer = "", fileToUpdate = "";
File config, dayFile;
int i = 0;

StaticJsonDocument<475> jsonConfig;

String apiKey, token, clientId, clientSecret;

IPAddress staticIP(192, 168, 15, 47);
IPAddress gateway(192, 168, 15, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 15, 1);
IPAddress dns2(200, 175, 89, 139);

IPAddress localhost(192, 168, 15, 111);

String makeFileName(String rawName) {
  String finalName = rawName.substring(9, 12) + "-" + rawName.substring(12, 14) + "-" + rawName.substring(14);
  finalName.toUpperCase();
  return finalName;
}

void refreshToken() {
  http.begin("https://oauth2.googleapis.com/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String ReqBody = "client_id=" + clientId;
  ReqBody += "&client_secret=" + clientSecret;
  ReqBody += "&refresh_token=" + token;
  ReqBody += "&grant_type=refresh_token";
 
  int httpResponseCode = http.POST(ReqBody);
  
  if(httpResponseCode>0){
    String rawResponse = http.getString();
  
    Serial.print("Requisição para renovação do token de acesso enviada.\nCódigo de resposta: ");
    Serial.println(httpResponseCode);
    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima
    if(httpResponseCode == 200) {
      const size_t capacity = JSON_OBJECT_SIZE(4) + 350;
      DynamicJsonDocument parsedResponse(capacity);
      deserializeJson(parsedResponse, rawResponse);
      
      const char* tokenChar = parsedResponse["access_token"];
      token = String(tokenChar);
      
      jsonConfig["token"] = tokenChar;
      String rawConfig = "";
      serializeJsonPretty(jsonConfig, rawConfig);

      config = SPIFFS.open("/config.txt", "w+");
      if(config){
        config.print(rawConfig);
        config.close();
        Serial.println("Token de acesso renovado");
      }

    } else {
      StaticJsonDocument<105> parsedResponse;
      deserializeJson(parsedResponse, rawResponse);

      const char* error = parsedResponse["error_description"];
      Serial.print("Erro: ");
      Serial.println(error);
    }
  }else{
    Serial.print("Erro ao enviar a requisição POST: ");
    Serial.println(httpResponseCode);
  }
}

void getErrorMessage(String rawResponse){
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 170;
  DynamicJsonDocument parsedResponse(capacity);
  deserializeJson(parsedResponse, rawResponse);
  JsonObject error = parsedResponse["error"];
  const char* error_message = error["message"];
  Serial.print("Mensagem de erro: ");
  Serial.println(error_message);
}

void updateFileOnGoogleDrive(String id, String data) {
  http.begin("https://sheets.googleapis.com/v4/spreadsheets/" + id + "/values/A1%3AJ1:append?valueInputOption=USER_ENTERED&key=" + apiKey);
  http.addHeader("Authorization", "Bearer " + token);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  
  String JSONReqBody = "{\"values\": [[" + data + "]],\"range\": \"A1:J1\"}";
 
  int httpResponseCode = http.POST(JSONReqBody);
  
  if(httpResponseCode>0){
    String rawResponse = http.getString();
  
    Serial.print("Requisição para atualização do arquivo no Google Drive enviada.\nCódigo de resposta: ");
    Serial.println(httpResponseCode);
    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima

    if(httpResponseCode == 200){
      Serial.println("Arquivo atualizado com sucesso!\n");
    } else {
      getErrorMessage(rawResponse);
    }
  }else{
    Serial.print("Erro ao enviar a requisição POST: ");
    Serial.println(httpResponseCode);
  }
}

void createFileOnGoogleDrive(String name, String data) {
  http.begin("https://www.googleapis.com/drive/v3/files?key=" + apiKey);
  http.addHeader("Authorization", "Bearer " + token);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  String JSONReqBody = "{\"name\": \""+name+"\",\"mimeType\": \"application/vnd.google-apps.spreadsheet\"}";
  
  int httpResponseCode = http.POST(JSONReqBody);
  
  if(httpResponseCode>0){
    String rawResponse = http.getString();

    Serial.println("Criação do arquivo " + name + " solicitada.\nCódigo de resposta: " + httpResponseCode);
    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima

    if(httpResponseCode == 200){
      Serial.println("Arquivo criado com sucesso!\n");

      const size_t capacity = JSON_OBJECT_SIZE(4) + 150;
      DynamicJsonDocument parsedResponse(capacity);
      deserializeJson(parsedResponse, rawResponse);
 
      String fileId = parsedResponse["id"];
      updateFileOnGoogleDrive(fileId, data);
    } else {
      getErrorMessage(rawResponse);
    }
  }else{
    Serial.print("Erro ao enviar a requisição POST: ");
    Serial.println(httpResponseCode);
  }
}

void searchFileOnGoogleDrive(String name, String dataToAppend) {
  http.begin("https://www.googleapis.com/drive/v3/files?pageSize=10&q=name%3D'" + name + "'&fields=nextPageToken%2C%20files(id%2C%20name)&key=" + apiKey);
  http.addHeader("Authorization", "Bearer " + token);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  int httpResponseCode = http.GET();

  if(httpResponseCode>0){
    String rawResponse = http.getString();

    Serial.println("Busca pelo arquivo " + name + " solicitada.\nCódigo de resposta: " + httpResponseCode);
    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima

    if(httpResponseCode == 200){
      const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 90;
      DynamicJsonDocument parsedResponse(capacity);
      deserializeJson(parsedResponse, rawResponse);
      
      const char* foundFileChar = parsedResponse["files"][0]["name"];
      const char* foundIdChar = parsedResponse["files"][0]["id"];

      String foundFile = String(foundFileChar);
      String foundId = String(foundIdChar);

      if(foundFile.length() > 0){
        Serial.println("Arquivo e ID encontrados:");
        Serial.println(foundFile);
        Serial.println(foundId + "\n");

        String timeStamp = dataToAppend.substring(0, 8);
        timeStamp = "\"" + timeStamp + "\"";

        String values = dataToAppend.substring(8);
        values.replace(",",".");
        values.replace(";",",");

        dataToAppend = timeStamp + values;

        Serial.println(dataToAppend);

        updateFileOnGoogleDrive(foundId, dataToAppend);
      } else {
        Serial.println("Nenhum arquivo encontrado\n");

        String HeaderAndTime = dataToAppend.substring(0, 69);
        HeaderAndTime.replace(";","\",\"");
        HeaderAndTime.replace("\n","\"],[\"");
        HeaderAndTime = "\"" + HeaderAndTime + "\"";

        String values = dataToAppend.substring(69);
        values.replace(",",".");
        values.replace(";",",");

        dataToAppend = HeaderAndTime + values;
        Serial.println(dataToAppend);
        createFileOnGoogleDrive(name, dataToAppend);
      }
    } else {
      getErrorMessage(rawResponse);
    }
  } else {
    Serial.print("Erro ao enviar a requisição POST: ");
    Serial.println(httpResponseCode);
  }
}

void setup(){
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  SPI.begin(18, 19, 23);
  
  if (SPIFFS.begin()){
    Serial.println("SPIFFS inicializado");
  } else {
    Serial.println("SPIFFS não inicializado");
    return;
  }
  
  if (SD.begin(CS_PIN)) Serial.println("Cartão SD inicializado");
  else  Serial.println("Inicialização do cartão SD falhou");

  config = SPIFFS.open("/config.txt", "r");
  String rawConfig = "";
  if(config){
    while(config.available()){
      rawConfig += char(config.read());
    }
    config.close();
  }

  deserializeJson(jsonConfig, rawConfig);

  // Credenciais da rede WiFi, nome da rede e senha
  const char* ssid = jsonConfig["ssid"];
  const char* password = jsonConfig["password"];

  // Inicialização de recursos, WiFi, comunicação serial e sistema de armazenamento de memória flash interna
  if (WiFi.config(staticIP, gateway, subnet, dns1, dns2) == false) {
    Serial.println("Configuration failed.");
  }
  WiFi.begin(ssid, password);

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
    request->send(SD, fname, "text/csv", true);
  });

  server.on("/google/auth", HTTP_GET, [](AsyncWebServerRequest *request) {
    String code = request->arg("code");
    
    request->send(200, "text/plain", "Código de autorização: " + code);
  
  });

  
  const char* apiKeyChar = jsonConfig["api_key"];
  apiKey = String(apiKeyChar);
  
  const char* tokenChar = jsonConfig["token"];
  token = String(tokenChar);

  const char* clientIdChar = jsonConfig["client_id"];
  clientId = String(clientIdChar);
  
  const char* clientSecretChar = jsonConfig["client_secret"];
  clientSecret = String(clientSecretChar);

  server.begin();
  //Serial.println("API KEY: " + apiKey + "\nTOKEN: " + token + "\nCLIENT ID: " + clientId + "\nCLIENT SECRET: " + clientSecret);
  refreshToken();
}

void loop(void) {
  while(Serial1.available() > 0){

    myChar = char(Serial1.read());

    if (myChar == '>') {
      fileToUpdate = makeFileName(fileToUpdate);
      dayFile = SD.open("/dados/" + fileToUpdate, "a");

      if(dayFile){
        dayFile.println(lineBuffer);
        dayFile.close();
        Serial.println("Arquivo atualizado: " + fileToUpdate);
        Serial.println("Conteúdo adicionado:\n" + lineBuffer + "\n");
      } else {
        Serial.print("Arquivo não abriu");
      }

      searchFileOnGoogleDrive(fileToUpdate, lineBuffer);

      lineBuffer = "";
      saveFlag = false;
      endFlag = true;
      Serial.println("\n------------------------------------------------------------------\n");
    } 
    else if (myChar == '|'){
      fileToUpdate =  lineBuffer;
      lineBuffer = "";
    }
    else if (saveFlag) {
      lineBuffer += myChar;
    }  
    else if (myChar == '<') {
      saveFlag = true;
    }
  }
}