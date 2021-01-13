#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include "SPI.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#define RXD2 26
#define TXD2 27
#define CS_PIN 22
#define MOSI_PIN 21
#define SCK_PIN 19
#define MISO_PIN 23
#define HTTP_PORT 80
#define MAXIMUM_ATTEMPTS 2

int i = 0, tryAgain = 0;
char receivedChar;
bool saveFlag = false, endFlag = false, startFlag = false;

String messageBuffer = "", fileToUpdate = "";
String apiKey, accessToken;

File config, dayFile;

StaticJsonDocument<650> jsonConfig;

AsyncWebServer server(HTTP_PORT);

IPAddress staticIP(192, 168, 0, 47);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(8, 8, 4, 4);

String createDirectory(String rawFileName) {
  String year = rawFileName.substring(0, 4);
  String month = rawFileName.substring(5, 8);
  String directory = "/" + year + "/" + month + "/";
  if(SD.mkdir("/" + year)) {
    if(SD.mkdir("/" + year + "/" + month)) {
      Serial.println("Diretório " + directory + " criado com sucesso");
    } else {
      Serial.println("Erro ao criar diretório de mês\n");
    }
  } else {
    Serial.printf("Erro ao criar diretório de ano\n");
  }
  return directory;
}

String formatFileName(String rawFileName) {
  String finalName = rawFileName.substring(9, 11) + "-" + rawFileName.substring(11, 13) + "-20" + rawFileName.substring(13, 15) + "-M2" + rawFileName.substring(15, 16);
  return finalName;
}

void formatData(String* data) {
  String rawValues = *data;
  rawValues.replace(",",".");
  rawValues.replace(";","\",\"");
  rawValues.replace("\n","\"],[\"");
  rawValues = "{\"values\":[[\"" + rawValues + "\"]]}";

  StaticJsonDocument<600> parsedValues;
  deserializeJson(parsedValues, rawValues);
  int numberOfRows = parsedValues["values"].size();

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
    HeaderAndTime = HeaderAndTime.substring(0, 72);
    HeaderAndTime.replace(";","\",\"");
    HeaderAndTime.replace("\n","\"],[\"");
    HeaderAndTime = "\"" + HeaderAndTime + "\"";

    String values = *data;
    values = values.substring(72);
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
  if(httpResponseCode > 0){
    
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
void getErrorMessage(const String& rawResponse){
  StaticJsonDocument<310> parsedResponse;
  deserializeJson(parsedResponse, rawResponse);
  JsonObject error = parsedResponse["error"];
  const char* error_message = error["message"];
  Serial.print("Mensagem de erro: ");
  Serial.println(error_message);
}

void renewAccessToken(HTTPClient& http) {
  const char* clientId = jsonConfig["client_id"];
  const char* clientSecret = jsonConfig["client_secret"];
  const char* refreshToken = jsonConfig["refresh_token"];  

  http.begin("https://oauth2.googleapis.com/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  http.setTimeout(15000);
  
  String reqBody = "client_id=";
  reqBody += clientId;
  reqBody += "&client_secret=";
  reqBody += clientSecret;
  reqBody += "&refresh_token=";
  reqBody += refreshToken;
  reqBody += "&grant_type=refresh_token";
 
  int httpResponseCode = http.POST(reqBody);
  String rawResponse = http.getString();

  http.end();
  if(httpResponseCode > 0){
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
    if(tryAgain <= MAXIMUM_ATTEMPTS) {
      Serial.println("Erro ao enviar a requisição. Tentando novamente...\n");
      delay(3000);
      renewAccessToken(http);
      tryAgain = 0;
    } else {
      Serial.println("Impossível realizar operação, continuando fluxo do programa.\n");
    }
  }
}

void updateFileOnGoogleDrive(const String& fileId, const String& dataToSend, HTTPClient& http) {
  http.begin("https://sheets.googleapis.com/v4/spreadsheets/" + fileId + "/values/A1%3AJ1:append?valueInputOption=USER_ENTERED&key=" + apiKey);
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  
  http.setTimeout(15000);
  
  String JSONReqBody = "{\"values\": " + dataToSend + ",\"range\": \"A1:J1\"}";
  int httpResponseCode = http.POST(JSONReqBody);
  
  Serial.printf("Requisição para atualização do arquivo no Google Drive enviada.\nCódigo de resposta: %i\n", httpResponseCode);
  String rawResponse = http.getString();

  http.end();
  if(httpResponseCode > 0){
    tryAgain = 0;

    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima

    if(httpResponseCode == 200){
      Serial.println("Arquivo atualizado com sucesso!\n");
    } else if (httpResponseCode == 401) {
      Serial.println("Token expirado. Renovando token.\n");
      renewAccessToken(http);
      updateFileOnGoogleDrive(fileId, dataToSend, http);
    } else {
      getErrorMessage(rawResponse);
    }
  } else {
    tryAgain++;
    if(tryAgain <= MAXIMUM_ATTEMPTS) {
      Serial.println("Erro ao enviar a requisição. Tentando novamente...\n");
      delay(3000);
      updateFileOnGoogleDrive(fileId, dataToSend, http);
      tryAgain = 0;
    } else {
      Serial.println("Impossível realizar operação, continuando fluxo do programa.\n");
    }
  }
}

void createFileOnGoogleDrive(const String& fileName, const String& dataToSend, HTTPClient& http) {
  http.begin("https://www.googleapis.com/drive/v3/files?key=" + apiKey);
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  http.setTimeout(15000);

  String JSONReqBody = "{\"name\": \""+fileName+"\",\"mimeType\": \"application/vnd.google-apps.spreadsheet\", \"parents\":[\"12laC6FeSPaeqy3mrEDzfnW2x3Aq8GqdJ\"]}";
  int httpResponseCode = http.POST(JSONReqBody);

  Serial.println("Criação do arquivo " + fileName + " solicitada.\nCódigo de resposta: " + httpResponseCode);

  String rawResponse = http.getString();

  http.end();
  if(httpResponseCode > 0){
    tryAgain = 0;
    // Serial.println("Resposta recebida:\n" + rawResponse);
    // Para receber o objeto JSON completo de resposta "descomentar" a linha acima

    if(httpResponseCode == 200){
      Serial.println("Arquivo criado com sucesso!\n");

      StaticJsonDocument<194> parsedResponse;
      deserializeJson(parsedResponse, rawResponse);
 
      String fileId = parsedResponse["id"];
      updateFileOnGoogleDrive(fileId, dataToSend, http);
    } else if (httpResponseCode == 401) {
      Serial.println("Token expirado. Renovando token.\n");
      renewAccessToken(http);
      createFileOnGoogleDrive(fileName, dataToSend, http);
    } else {
      getErrorMessage(rawResponse);
    }
  } else {
    tryAgain++;
    if(tryAgain <= MAXIMUM_ATTEMPTS) {
      Serial.println("Erro ao enviar a requisição. Tentando novamente...\n");
      delay(3000);
      createFileOnGoogleDrive(fileName, dataToSend, http);
      tryAgain = 0;
    } else {
      Serial.println("Impossível realizar operação, continuando fluxo do programa.\n");
    }
  }
}

void searchFileOnGoogleDrive(const String& fileName, String dataToSend, HTTPClient& http) {
  http.begin("https://www.googleapis.com/drive/v3/files?pageSize=10&q=name%3D'" + fileName + "'&fields=nextPageToken%2C%20files(id%2C%20name)&key=" + apiKey);
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  http.setTimeout(15000);

  int httpResponseCode = http.GET();

  Serial.println("Busca pelo arquivo " + fileName + " solicitada.");
  Serial.printf("Código de resposta: %i\n", httpResponseCode);

  String rawResponse = http.getString();
  
  http.end();
  if(httpResponseCode > 0){
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
      formatData(&dataToSend);
      // Serial.println("\nDados como serão enviados ao Google Sheets: \n" + dataToSend + "\n");

      if(foundFile.length() > 0){
        Serial.println("Arquivo encontrado - " + foundFile);
        Serial.println("ID do arquivo - " + foundId + "\n");

        updateFileOnGoogleDrive(foundId, dataToSend, http);
      } else {
        Serial.println("Nenhum arquivo encontrado\n");

        createFileOnGoogleDrive(fileName, dataToSend, http);
      }
    } else if (httpResponseCode == 401) {
      Serial.println("Token expirado. Renovando token.\n");
      renewAccessToken(http);
      searchFileOnGoogleDrive(fileName, dataToSend, http);
    } else {
      getErrorMessage(rawResponse);
    }
  } else {
    tryAgain++;
    if(tryAgain <= MAXIMUM_ATTEMPTS) {
      Serial.println("Erro ao enviar a requisição. Tentando novamente...\n");
      delay(3000);
      searchFileOnGoogleDrive(fileName, dataToSend, http);
      tryAgain = 0;
    } else {
      Serial.println("Impossível realizar operação, continuando fluxo do programa.\n");
    }
  }
}

void setup(){
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);
  
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
  int connectionAttempts = 0;
  while(WiFi.status() != WL_CONNECTED && connectionAttempts <= MAXIMUM_ATTEMPTS+2) {
    Serial.print(".");
    delay(1000);
    connectionAttempts++;
  }

  if(!MDNS.begin("monitoramento-m2")) {
    Serial.println("MDNS falhou");
    return;  
  } else {
    Serial.print("\nPágina do servidor disponível em http://monitoramento-m2.local ou http://");
    Serial.println(WiFi.localIP());
    Serial.println("\n------------------------------------------------------------------\n");
  }

  MDNS.addService("http", "tcp", 80);

// "Paths", ou rotas, a que o servidor responde ao receber uma requisição de um cliente. Na raiz "/" o servidor envia a página HTML. Como os arquivos de estilo, código JavaScript e logo estão referenciados na página HTML as requisições pedindo por esses arquivos são enviadas automaticamente e por isso é preciso haver uma resposta para cada uma delas.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/index.html", "text/html");});
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/style.css", "text/css");});
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/script.js", "text/javascript");});
  server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request) {request->send(SPIFFS, "/logo.png", "image/png");});

// Descrito no código JavaScript, na página, ao se apertar o botão "Listar arquivos" o cliente envia uma requisição na rota "/listFiles", que executa a função a seguir
  server.on("/listFilesInDirectory", HTTP_GET, [](AsyncWebServerRequest *request) {
    String path = request->arg("path");
    File dir = SD.open(path);
    File file = dir.openNextFile();
    String filesList = "{\"files\":[";

    while (file) {
      filesList += "\"";
      filesList += file.name();
      filesList += "\"";
      file = dir.openNextFile();
      if (file) filesList += ",";
    }
    filesList += "]}";

    dir.close();
    request->send(200, "application/json", filesList);
  });

  server.on("/downloadFile", HTTP_GET, [](AsyncWebServerRequest *request) {
    String filePath = request->arg("filePath"); // No AsyncWebServer é preciso especificar o 'name' do argumento enviado, não o índice
    Serial.print("Download requisitado: "); // O nome de um argumento ainda pode ser verificado com a sintaxe request->argName(índice)
    Serial.println(filePath);
    request->send(SD, filePath, "text/csv", true);
  });
  
  const char* apiKeyChar = jsonConfig["api_key"];
  apiKey = String(apiKeyChar);
  
  const char* accessTokenChar = jsonConfig["access_token"];
  accessToken = String(accessTokenChar);

  server.begin();
}

void loop(void) {
  while(Serial1.available() > 0){

    receivedChar = char(Serial1.read());

    if (receivedChar == '>') {
      String directory = createDirectory(fileToUpdate);
      fileToUpdate = formatFileName(fileToUpdate);
      dayFile = SD.open(directory + fileToUpdate + ".csv", "a");

      if(dayFile){
        dayFile.println(messageBuffer);
        dayFile.close();
        Serial.println("Arquivo local atualizado: " + directory + fileToUpdate);
        Serial.println("Conteúdo adicionado:\n" + messageBuffer + "\n");
      } else {
        Serial.println("Arquivo local não abriu\n");
      }

      HTTPClient http;
      Serial.println("Enviando dados para o Google Drive.\n");
      searchFileOnGoogleDrive(fileToUpdate, messageBuffer, http);

      messageBuffer = "";
      saveFlag = false;
      endFlag = true;
      Serial.println("------------------------------------------------------------------\n");
    } 
    else if (receivedChar == '|'){
      fileToUpdate =  messageBuffer;
      messageBuffer = "";
    }
    else if (saveFlag) {
      messageBuffer += receivedChar;
    }  
    else if (receivedChar == '<') {
      saveFlag = true;
    }
  }
}