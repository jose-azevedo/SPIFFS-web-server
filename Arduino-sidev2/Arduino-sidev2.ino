#include <SD.h>
#include <SPI.h>
#include <DS3231.h>

#define interval 1

DS3231 rtc(SDA, SCL);
Time tempo;
int wait = 0;

File data;

volatile char conteudo[20] = {0};
volatile int index = 0;
volatile int ISRcont = 0;
volatile bool startFlag = false, printFlag = false, listFlag = false;

int i = 0;
int pinCS = 53;

void setup()
{
  rtc.begin();
  Serial.begin(19600);
  Serial1.begin(9600);
  pinMode(pinCS, OUTPUT);
  if (SD.begin()) Serial.println("Cartão SD inicializado");
  else  Serial.println("Inicialização do cartão SD falhou");
  
}


void loop()
{
 /* 
    if (printFlag) {
    while ((UCSR1A & (1 << UDRE1)) == 0) {};
    UDR1 = '<';
    data = SD.open(conteudo, FILE_READ);
    if (data) {
      Serial.println("Arquivo aberto");
      while(data.available()){
        while ((UCSR1A & (1 << UDRE1)) == 0) {};
        UDR1 = char(data.read());
      }
      data.close();
    }
    else {
      Serial.println("Erro ao abrir");
    }

    while ((UCSR1A & (1 << UDRE1)) == 0) {};
    UDR1 = '>';
    
    memset(&conteudo[0], NULL, 20);
    printFlag = false;
  }

  if (listFlag) {
    File root = SD.open("/");
    data = root.openNextFile();
    String listS = "";
    char list[1000] = {0};
    while (data) {
      listS += data.name();
      listS += "|";
      data = root.openNextFile();
    }
    //int len = listS.length();
    listS = listS.substring(9);
    size_t len = listS.length();
    listS.toCharArray(list, len);
  
    while ((UCSR1A & (1 << UDRE1)) == 0) {};
    UDR1 = '<';
    for (i = 0; i < len-1; i++) {
      while ((UCSR1A & (1 << UDRE1)) == 0) {};
      UDR1 = byte(list[i]);
    }
    while ((UCSR1A & (1 << UDRE1)) == 0) {};
    UDR1 = '>';
    
    Serial.println("Lista enviada");
    data.close();
    root.close();
    listFlag = false;
  }*/
  
  tempo = rtc.getTime(); // Variável recebedora da informação de tempo
  if (tempo.min > wait) wait = (tempo.min/interval + 1)*interval; // Os dados só são salvos em minutos múltiplos de 5
  if (wait > tempo.min + interval) wait = 0; // Condição para reiniciar o ciclo quando min0 superar 55

  if (tempo.min == wait){
    Serial.println("Entrou no laço");
    String fileNameA = "A" + String(tempo.date) + "-" + String(tempo.mon) + "-" + String(tempo.year-2000) + ".csv";
    String fileDataA = String(rtc.getTimeStr()) + " ; 5.43 ; 5.54 ; 26.7 ; 26.83 ; 144.98 ; 0.98 ; 222.4 ; 217.95 ; 1.0 ";
    Serial1.print("<");
    Serial1.print(fileNameA);
    Serial1.print("|");
    Serial1.print(fileDataA);
    Serial1.print(">");

    String fileNameB = "B" + String(tempo.date) + "-" + String(tempo.mon) + "-" + String(tempo.year-2000) + ".csv";
    String fileDataB = String(rtc.getTimeStr()) + " ; 2.5 ; 2.6 ; 13.35 ; 13.43 ; 78.63 ; 1.3 ; 218.7 ; 220.03 ; 0.99 ";
    Serial1.print("<");
    Serial1.print(fileNameB);
    Serial1.print("|");
    Serial1.print(fileDataB);
    Serial1.print(">");

    wait = wait + interval;
  } 
}
