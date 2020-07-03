#include <SD.h>
#include <SPI.h>
#include <DS3231.h>

#define intervalo 1

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
  cli();
  rtc.begin();
  Serial.begin(19200);
  //pinMode(pinCS, OUTPUT);
  //if (SD.begin()) Serial.println("Cartão SD inicializado");
  //else  Serial.println("Inicialização do cartão SD falhou");
  
  UCSR1A = 0X00;
  UCSR1B = 0x98;
  UCSR1C = 0X06;
  UBRR1H = 0x00;  //
  UBRR1L = 0x67;  // Baud Rate: 9600
  //UBRR1L = 0xCF;
  
  //UCSR1B |= (1 << RXCIE1) | (1 << RXEN1) | (1 << TXEN1); // Set interrupção no Rx e habilitar Rx e Tx
  
  //UCSR1C |= (1 << UCSZ10) | (1 << UCSZ11); // Set palavra de 8 bits
  sei();
}

ISR(USART1_RX_vect) {
  char myChar = UDR1;

  if (myChar == '>') {
    printFlag = true;
    startFlag = false;
  }
  else if (startFlag) {
    conteudo[index] = myChar;
    index++;
  }  
  else if (myChar == '<') {
    startFlag = true;
    index = 0;
  }
  else if (myChar == '|') {
    listFlag = true;
  }
  ISRcont++;
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
  if (tempo.min > wait) wait = (tempo.min/intervalo + 1)*intervalo; // Os dados só são salvos em minutos múltiplos de 5
  if (wait > tempo.min + intervalo) wait = 0; // Condição para reiniciar o ciclo quando min0 superar 55

  if (tempo.min == wait){
    Serial.println("Entrou no laço");
    String fileNameString = String(tempo.date) + "-" + String(tempo.mon) + "-" + String(tempo.year-2000) + ".csv";
    char fileName[15] = {0};
    size_t len = fileNameString.length();
    fileNameString.toCharArray(fileName, len+1);

    while ((UCSR1A & (1 << UDRE1)) == 0) {};
    UDR1 = '<';
    for (i = 0; i < len; i++) {
      while ((UCSR1A & (1 << UDRE1)) == 0) {};
      UDR1 = byte(fileName[i]);
    }
    while ((UCSR1A & (1 << UDRE1)) == 0) {};
    UDR1 = '|';
    
    String fileDataString = String(rtc.getTimeStr()) + " ; I DC ; I DC rms ; V DC ; V DC rms ; P DC ; I AC rms ; V AC rms ; S ; FP ";
    char fileData[100] = {0};
    len = fileDataString.length();
    fileDataString.toCharArray(fileData, len+1);
    
    for (i = 0; i < len-1; i++) {
      while ((UCSR1A & (1 << UDRE1)) == 0) {};
      UDR1 = byte(fileData[i]);
    }
    while ((UCSR1A & (1 << UDRE1)) == 0) {};
    UDR1 = '>';
    
    wait = wait + intervalo;
  } 
}
