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
int tryAgain = 0;
bool saveFlag = false, endFlag = false, startFlag = false;
char myChar;
String lineBuffer = "", fileToUpdate = "";
File config, dayFile;
int i = 0;

StaticJsonDocument<636> jsonConfig;

String apiKey, accessToken;

IPAddress staticIP(192, 168, 15, 47);
IPAddress gateway(192, 168, 15, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 15, 1);
IPAddress dns2(200, 175, 89, 139);

IPAddress localhost(192, 168, 15, 111);

String makeFileName(String rawName) {
  String finalName = rawName.substring(9, 12) + "-" + rawName.substring(12, 14) + "-20" + rawName.substring(14, 16);
  finalName.toUpperCase();
  return finalName;
}

void parseRecievedData(String* data) {
  // Serial.println("\nDados como recebidos do Arduino: \n" + data + "\n");

  String rawValues = *data;
  rawValues.replace(",",".");
  rawValues.replace(";","\",\"");
  rawValues.replace("\n","\"],[\"");
  rawValues = "{\"values\":[[\"" + rawValues + "\"]]}";

  // Serial.println("Dados em JSON serializados: \n" + rawValues + "\n");

  StaticJsonDocument<600> parsedValues;
  deserializeJson(parsedValues, rawValues);
  int numberOfRows = parsedValues["values"].size();
  // Serial.print("O array tem a quantidade de elementos: ");
  // Serial.println(numberOfRows);
  if(numberOfRows == 1) {
    String timeStamp = *data;
    timeStamp = timeStamp.substring(0, 8);
    timeStamp = "\"" + timeStamp + "\"";

    String values = *data;
    values = values.substring(8);
    values.replace(",",".");
    values.replace(";",",");
    
    *data = "[[" + timeStamp + values + "]]";
  } else {
    String HeaderAndTime = *data;
    HeaderAndTime = HeaderAndTime.substring(0, 69);
    HeaderAndTime.replace(";","\",\"");
    HeaderAndTime.replace("\n","\"],[\"");
    HeaderAndTime = "\"" + HeaderAndTime + "\"";

    String values = *data;
    values = values.substring(69);
    values.replace(",",".");
    values.replace(";",",");

    *data = "[[" + HeaderAndTime + values + "]]";
  }
}
/*
void getRefreshToken() {
  const char* clientId = jsonConfig["client_id"];
  const char* clientSecret = jsonConfig["client_secret"];

  http.begin("https://oauth2.googleapis.com/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Adicionar um código para conseguir um novo refresh token
  String ReqBody = "code=";
  ReqBody += "&client_id=";
  ReqBody += clientId;
  ReqBody += "&client_secret=";
  ReqBody += clientSecret;
  ReqBody += "&redirect_uri=http%3A//localhost:5000/google/auth";
  ReqBody += "&grant_type=authorization_code";
  ReqBody += "&prompt=consent";
 
  int httpResponseCode = http.POST(ReqBody);
  String rawResponse = http.getString();

  http.end();
  if(httpResponseCode>0){
    
    Serial.print("Requisição para adquirir tokens de renovação e de acesso enviada.\nCódigo de resposta: ");
    Serial.println(httpResponseCode);
    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima
    if(httpResponseCode == 200) {

      // Salvar o refresh token no arquivo json de configuração

    } else {
      StaticJsonDocument<128> parsedResponse;
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
*/
void renewAccessToken(HTTPClient& http) {
  const char* clientId = jsonConfig["client_id"];
  const char* clientSecret = jsonConfig["client_secret"];
  const char* refreshToken = jsonConfig["refresh_token"];  

  http.begin("https://oauth2.googleapis.com/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  http.setTimeout(15000);
  
  String ReqBody = "client_id=";
  ReqBody += clientId;
  ReqBody += "&client_secret=";
  ReqBody += clientSecret;
  ReqBody += "&refresh_token=";
  ReqBody += refreshToken;
  ReqBody += "&grant_type=refresh_token";
 
  int httpResponseCode = http.POST(ReqBody);
  String rawResponse = http.getString();

  http.end();
  if(httpResponseCode>0){
    tryAgain = 0;

    Serial.print("Requisição para renovação do token de acesso enviada.\nCódigo de resposta: ");
    Serial.println(httpResponseCode);
    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima
    if(httpResponseCode == 200) {
      StaticJsonDocument<480> parsedResponse;
      deserializeJson(parsedResponse, rawResponse);
      
      const char* accessTokenChar = parsedResponse["access_token"];
      accessToken = String(accessTokenChar);
      
      jsonConfig["access_token"] = accessTokenChar;
      String rawConfig = "";
      serializeJsonPretty(jsonConfig, rawConfig);

      config = SPIFFS.open("/config.json", "w+");
      if(config){
        config.print(rawConfig);
        config.close();
        Serial.println("Token de acesso renovado!\n");
      }

    } else {
      StaticJsonDocument<128> parsedResponse;
      deserializeJson(parsedResponse, rawResponse);

      const char* error = parsedResponse["error_description"];
      Serial.print("Erro: ");
      Serial.println(error);
    }
  } else {
    tryAgain++;
    if(tryAgain <= 1) {
      Serial.println("Erro ao enviar a requisição. Tentando novamente...\n");
      renewAccessToken(http);
      tryAgain = 0;
    } else {
      Serial.println("Impossível realizar operação, continuando fluxo do programa.\n");
    }
  }
}

void getErrorMessage(const String& rawResponse){
  StaticJsonDocument<310> parsedResponse;
  deserializeJson(parsedResponse, rawResponse);
  JsonObject error = parsedResponse["error"];
  const char* error_message = error["message"];
  Serial.print("Mensagem de erro: ");
  Serial.println(error_message);
}

void updateFileOnGoogleDrive(const String& id, const String& data, HTTPClient& http) {
  http.begin("https://sheets.googleapis.com/v4/spreadsheets/" + id + "/values/A1%3AJ1:append?valueInputOption=USER_ENTERED&key=" + apiKey);
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  
  http.setTimeout(15000);
  
  String JSONReqBody = "{\"values\": " + data + ",\"range\": \"A1:J1\"}";
  int httpResponseCode = http.POST(JSONReqBody);
  
  Serial.printf("Requisição para atualização do arquivo no Google Drive enviada.\nCódigo de resposta: %i\n", httpResponseCode);
  String rawResponse = http.getString();

  http.end();
  if(httpResponseCode>0){
    tryAgain = 0;

    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima

    if(httpResponseCode == 200){
      Serial.println("Arquivo atualizado com sucesso!\n");
    } else if (httpResponseCode == 401) {
      Serial.println("Token expirado. Renovando token.\n");
      renewAccessToken(http);
      updateFileOnGoogleDrive(id, data, http);
    } else {
      getErrorMessage(rawResponse);
    }
  } else {
    tryAgain++;
    if(tryAgain <= 1) {
      Serial.println("Erro ao enviar a requisição. Tentando novamente...\n");
      updateFileOnGoogleDrive(id, data, http);
      tryAgain = 0;
    } else {
      Serial.println("Impossível realizar operação, continuando fluxo do programa.\n");
    }
  }
}

void createFileOnGoogleDrive(const String& name, const String& data, HTTPClient& http) {
  http.begin("https://www.googleapis.com/drive/v3/files?key=" + apiKey);
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  http.setTimeout(15000);

  String JSONReqBody = "{\"name\": \""+name+"\",\"mimeType\": \"application/vnd.google-apps.spreadsheet\"}";
  int httpResponseCode = http.POST(JSONReqBody);

  Serial.println("Criação do arquivo " + name + " solicitada.\nCódigo de resposta: " + httpResponseCode);

  String rawResponse = http.getString();

  http.end();
  if(httpResponseCode>0){
    tryAgain = 0;
    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima

    if(httpResponseCode == 200){
      Serial.println("Arquivo criado com sucesso!\n");

      StaticJsonDocument<194> parsedResponse;
      deserializeJson(parsedResponse, rawResponse);
 
      String fileId = parsedResponse["id"];
      updateFileOnGoogleDrive(fileId, data, http);
    } else if (httpResponseCode == 401) {
      Serial.println("Token expirado. Renovando token.\n");
      renewAccessToken(http);
      createFileOnGoogleDrive(name, data, http);
    } else {
      getErrorMessage(rawResponse);
    }
  } else {
    tryAgain++;
    if(tryAgain <= 1) {
      Serial.println("Erro ao enviar a requisição. Tentando novamente...\n");
      createFileOnGoogleDrive(name, data, http);
      tryAgain = 0;
    } else {
      Serial.println("Impossível realizar operação, continuando fluxo do programa.\n");
    }
  }
}

void searchFileOnGoogleDrive(const String& name, String dataToAppend, HTTPClient& http) {
  http.begin("https://www.googleapis.com/drive/v3/files?pageSize=10&q=name%3D'" + name + "'&fields=nextPageToken%2C%20files(id%2C%20name)&key=" + apiKey);
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  http.setTimeout(15000);

  int httpResponseCode = http.GET();

  Serial.println("Busca pelo arquivo " + name + " solicitada.");
  Serial.printf("Código de resposta: %i\n", httpResponseCode);

  String rawResponse = http.getString();
  
  http.end();
  if(httpResponseCode>0){
    tryAgain = 0;
    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima

    if(httpResponseCode == 200){
      
      StaticJsonDocument<150> parsedResponse;
      deserializeJson(parsedResponse, rawResponse);
      
      const char* foundFileChar = parsedResponse["files"][0]["name"];
      const char* foundIdChar = parsedResponse["files"][0]["id"];

      String foundFile = String(foundFileChar);
      String foundId = String(foundIdChar);
      parseRecievedData(&dataToAppend);
      // Serial.println("\nDados como serão enviados ao Google Sheets: \n" + dataToAppend + "\n");

      if(foundFile.length() > 0){
        Serial.println("Arquivo encontrado - " + foundFile);
        Serial.println("ID do arquivo - " + foundId + "\n");

        updateFileOnGoogleDrive(foundId, dataToAppend, http);
      } else {
        Serial.println("Nenhum arquivo encontrado\n");

        createFileOnGoogleDrive(name, dataToAppend, http);
      }
    } else if (httpResponseCode == 401) {
      Serial.println("Token expirado. Renovando token.\n");
      renewAccessToken(http);
      searchFileOnGoogleDrive(name, dataToAppend, http);
    } else {
      getErrorMessage(rawResponse);
    }
  } else {
    tryAgain++;
    if(tryAgain <= 1) {
      Serial.println("Erro ao enviar a requisição. Tentando novamente...\n");
      searchFileOnGoogleDrive(name, dataToAppend, http);
      tryAgain = 0;
    } else {
      Serial.println("Impossível realizar operação, continuando fluxo do programa.\n");
    }
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

  config = SPIFFS.open("/config.json", "r");
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
    Serial.println("Configuração de rede falhou");
  }
  WiFi.begin(ssid, password);

// Espera até que a rede esteja conectada
  while(WiFi.status()!=WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

// Imprime na tela o IP atribuído dinamicamente
  Serial.print("\nEndereço de IP: ");
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
  
  const char* accessTokenChar = jsonConfig["access_token"];
  accessToken = String(accessTokenChar);

  server.begin();
}

void loop(void) {
  while(Serial1.available() > 0){

    myChar = char(Serial1.read());

    if (myChar == '>') {
      fileToUpdate = makeFileName(fileToUpdate);
      dayFile = SD.open("/dados/" + fileToUpdate + ".csv", "a");

      if(dayFile){
        dayFile.println(lineBuffer);
        dayFile.close();
        Serial.println("Arquivo local atualizado: " + fileToUpdate);
        Serial.println("Conteúdo adicionado:\n" + lineBuffer + "\n");
      } else {
        Serial.println("Arquivo local não abriu\n");
      }

      HTTPClient http;
      Serial.println("Enviando dados para o Google Drive.\n");
      searchFileOnGoogleDrive(fileToUpdate, lineBuffer, http);

      lineBuffer = "";
      saveFlag = false;
      endFlag = true;
      Serial.println("------------------------------------------------------------------\n");
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