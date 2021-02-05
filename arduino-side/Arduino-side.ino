#include <SD.h> // Bibliotecas para módulo SD
#include <SPI.h>
#include <DS3231.h> // Biblioteca RTC

#define N 384      // Número de amostras
#define stepADC 0.00481640625 // 4,932/1024
#define M 10 // Total de valores para cálculo da média de valores instantâneos
#define pinCS 53
#define interval 5

File data;

DS3231 rtc(SDA, SCL); // Inicialização RTC
Time tempo;

char filePath[21] = {0};
String linesToAppend = "";

void createFile(void);
void storeData(void);
void accumulate(void);
void clearCumulativeVariables(void);
void clearAverageVariables(void);

int timeToSaveA = 0, timeToSaveB = 0;

char step = 'D';
int x = 0;
int bitInput = 0;
int sample = 0;
int iterations = 0;
int i = 0;
int counter = 0;

unsigned int ADCH_16 = 0;
unsigned int ADCL_16 = 0;

//Vetores com os valores de três ciclos de corrente e tensão
int I[N] = {0}, V[N] = {0};

struct parameters {
  double avg = 0;
  double rms = 0;
  double avg_rms = 0;

  void clearValues() {
    avg = 0;
    rms = 0;
    avg_rms = 0;
  }
} IrawDC[2], VrawDC[2], IrawAC[2], VrawAC[2],  I_DC[2], V_DC[2], I_AC[2], V_AC[2];

struct cumulative_parameters {
  double I_DCavg = 0;
  double I_DCrms = 0;
  double V_DCavg = 0;
  double V_DCrms = 0;
  double P_DC = 0;
  double I_ACrms = 0;
  double V_ACrms = 0;
  double P_AC = 0;
  double S = 0;
  double FP = 0;

  void clearValues() {
    I_DCavg = 0;
    I_DCrms = 0;
    V_DCavg = 0;
    V_DCrms = 0;
    P_DC = 0;
    I_ACrms = 0;
    V_ACrms = 0;
    P_AC = 0;
    S = 0;
    FP = 0;
  }
} acc[2], acc_raw[2];

double P_DC[2] = {0}, P_AC[2] = {0}, S[2] = {0}, FP[2] = {0};

void setup() {
  cli(); // Desabilitar interrupções
    
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 1040;     // 16MHz / (prescaler*15360Hz) - 1
  
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (0 << CS12) | (0 << CS11) |(1 << CS10); //prescaler de 1
  TIFR1 |= (0 << OCF1B);
  
  ADCSRA = 0;
  ADCSRB = 0;
  ADMUX |= (1 << REFS0); // Tensão de referência AVcc (5 V)
  ADMUX |= (0 << REFS1); // Tensão de referência AVcc (5 V)
  ADMUX |= (1 << ADLAR); // Resultado da conversão AD alinhado à esquerda
  ADCSRA |= (0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);// setar clock do ADC com prescaler= 16
  
  ADCSRA |= (1 << ADATE); //ADC auto-trigger
  ADCSRB |= (1 << ADTS2) | (0 << ADTS1) | (1 << ADTS0); // Fonte do trigger: Timer1 Compare Match B
  ADCSRA |= (1 << ADIE); // Habilitar interrupção quando terminar a conversão AD
  ADCSRA |= (1 << ADEN); // Habilitar o ADC
  ADCSRA |= (1 << ADSC); // Iniciar a conversão AD
  
  Serial.begin(19200); // Inicializar comunicação serial a 9600 bps
  Serial1.begin(9600);
  rtc.begin();
  pinMode(pinCS, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Inicialização do cartão SD
  if (SD.begin()){
    Serial.println("Cartão SD inicializado");
  }
  else  {
    Serial.println("Falha ao inicializar o cartão SD");
    return;
  }
  sei(); // Habilitar interrupções
}

// Interrupção para aquisição de dados
ISR(ADC_vect) { // Quando um novo resultado AD estiver pronto
// Calcular o valor de tensão na entrada analógica:
  ADCH_16 = (ADCH << 2);
  ADCL_16 = (ADCH >> 6);
  bitInput = ADCH_16|ADCL_16;
  
  switch(ADMUX){ // Incrementar canal para próxima aquisição de dados:
    case 0x60: // ADC0: Corrente DC 1
    I[sample] = bitInput;
    ADMUX = 0x61;
    break;
    case 0x61: // ADC1: Tensão DC 1
    V[sample] = bitInput;
    ADMUX = 0x60;
    sample++;
    break;
    
    case 0x62: // ADC2: Corrente DC 2
    I[sample] = bitInput;
    ADMUX = 0x63;
    break;
    case 0x63: // ADC2: Tensão DC 2
    V[sample] = bitInput;
    ADMUX = 0x62;
    sample++;
    break;
    
    case 0x64: // ADC4: Corrente AC 1
    I[sample] = bitInput;
    ADMUX = 0x65;
    break;
    case 0x65: // ADC5: Tensão AC 1
    V[sample] = bitInput;
    ADMUX = 0x64;
    sample++;
    break;
    
    case 0x66: // ADC6: Corrente AC 2
    I[sample] = bitInput;
    ADMUX = 0x67;
    break;
    case 0x67: // ADC7: Tensão AC 2
    V[sample] = bitInput;
    ADMUX = 0x66;
    sample++;
    break;
    
    default:
    ADMUX = 0x60;
    step = 'D';
    break;
  }
  TIFR1 |= (0 << OCF1B); // Limpar a flag de interrupção do Timer, habilitando outra borda de subida
}


/* 
 Função para iniciar o ciclo de cálculos seguinte do programa.
 A função altera o valor das variáveis presentes nos dois laços If do loop principal
 selecionando se serão calculados parâmetros DC ou AC, a partir da variável char C,
 bem como de qual sistema está calculando, com auxílio da variável int x.
 É também alterado o canal de aquisição do conversor analógico/digital pela variável ADMUX
 Esta função faz com que o loop principal do programa execute apenas um subciclo a cada iteração.
 Ou seja, a cada vez que o loop principal é executado, são executados somente os cálculos
 de parâmetros ou DC1 ou DC2 ou AC1 ou AC2, nunca mais de um destes e sempre nesta ordem.
*/

void nextStep(){
  switch(step) {
    case 'D':
      if(x == 0){
        x = 1;
        ADMUX = 0x62;
      } else {
        x = 0;
        ADMUX = 0x64;
        step='A';
      }
    break;
    case 'A':
      if(x == 0){
        x = 1;
        ADMUX = 0x66;
      } else {
        x = 0;
        ADMUX = 0x60;
        step='S';
      }
    break;
    case 'S':
      step='D';
    break;
  }
}

/*
  Função de verificação de existência e criação de arquivo
  É gerado um vetor de char com o presente dia e uma letra identificadora do sistema.
  Posteriormente é verificado se um arquivo com este nome já existe. Em caso positivo,
  nada acontece. Do contrário o arquivo é criado e um cabeçalho é escrito na primeira linha.
*/
void createFile(){
  char generator = {0};
  char directory[9] = {0};
  if(i == 0) generator = 'A';
  if(i == 1) generator = 'B';

  // Geração do nome e da rota do arquivo em vetores de caracteres

  switch (tempo.mon) {
    case 1:
    sprintf(directory, "%d/JAN/", tempo.year);
    sprintf(filePath, "%d/JAN/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 2:
    sprintf(directory, "%d/FEV", tempo.year);
    sprintf(filePath, "%d/FEV/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 3:
    sprintf(directory, "%d/MAR", tempo.year);
    sprintf(filePath, "%d/MAR/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 4:
    sprintf(directory, "%d/ABR", tempo.year);
    sprintf(filePath, "%d/ABR/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 5:
    sprintf(directory, "%d/MAI", tempo.year);
    sprintf(filePath, "%d/MAI/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 6:
    sprintf(directory, "%d/JUN", tempo.year);
    sprintf(filePath, "%d/JUN/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 7:
    sprintf(directory, "%d/JUL", tempo.year);
    sprintf(filePath, "%d/JUL/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 8:
    sprintf(directory, "%d/AGO", tempo.year);
    sprintf(filePath, "%d/AGO/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 9:
    sprintf(directory, "%d/SET", tempo.year);
    sprintf(filePath, "%d/SET/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 10:
    sprintf(directory, "%d/OUT", tempo.year);
    sprintf(filePath, "%d/OUT/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 11:
    sprintf(directory, "%d/NOV", tempo.year);
    sprintf(filePath, "%d/NOV/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
    case 12:
    sprintf(directory, "%d/DEZ", tempo.year);
    sprintf(filePath, "%d/DEZ/%02d%02d%02d%c.csv", tempo.year, tempo.date, tempo.mon, tempo.year-2000, generator);
    break;
  }

  const String header = "Hora;Transdutor I DC;Transdutor V DC;Transdutor I AC;Transdutor V AC rms;I DC;I DC rms;V DC;V DC rms;P DC;I AC rms;V AC rms;P AC;FP";
  
  // Cabeçalho do arquivo
  if(SD.mkdir(directory)){
    Serial.print("Diretório aberto: ");
    Serial.println(directory);
    if (!SD.exists(filePath)) {
      data = SD.open (filePath, FILE_WRITE);
      if (data) {
        data.println(header);
        data.close();

        linesToAppend += header + "\n";

        Serial.print("Novo arquivo criado: ");
        Serial.println(filePath);
      } else {
        Serial.println("Não foi possível criar o novo arquivo");
      }
    }
  } else {
    Serial.println("Não criou diretório");
  }
}

/*
  Função para armazenamento dos dados no cartão SD
  São concatenadas em uma string as variáveis acumuladoras divididas pela contagem de acumulações
  a fim de integralizar os dados. O processo é feito para cada sistema e por isso é dependente
  da variável x.
*/
void storeData(){

  String fileData = String(rtc.getTimeStr()) + ";" + String(acc_raw[i].I_DCavg/counter) + ";" + String(acc_raw[i].V_DCavg/counter) + ";" + String(acc_raw[i].I_ACrms/counter) + ";" + String(acc_raw[i].V_ACrms/counter);
  
  fileData = fileData + ";" + String(acc[i].I_DCavg/counter) + ";" + String(acc[i].I_DCrms/counter) + ";" + String(acc[i].V_DCavg/counter) + ";" + String(acc[i].V_DCrms/counter) + ";" + String(acc[i].P_DC/counter) + ";" + String(acc[i].I_ACrms/counter) + ";" + String(acc[i].V_ACrms/counter) + ";" + String(acc[i].P_AC/counter) + ";" + String(acc[i].FP/counter);
  
  fileData.replace(".",",");
  
  data = SD.open(filePath, FILE_WRITE);
    if (data) {
  data.println(fileData);
  data.close();
  Serial.print("Arquivo atualizado: ");
  Serial.println(filePath);
  linesToAppend += fileData;
  }
}

void sendToESP32(){
  String messageBuffer = "<" + String(filePath) + "|" + linesToAppend + ">";
  Serial1.print(messageBuffer);
  linesToAppend = "";
  Serial.println("Dados enviados para o ESP32:");
  Serial.println(messageBuffer);
}

/*
  Função acumuladora.
  Para posteriormente se integralizar os dados coletados, estes são somados às variáveis abaixo
  e no momento do armazenamento, divididos pelo número de acumulações.
*/
void accumulate(){
  for (int i=0; i<=1; i++) {  
    acc[i].I_DCavg += I_DC[i].avg;
    acc[i].I_DCrms += I_DC[i].rms;
    acc[i].V_DCavg += V_DC[i].avg;
    acc[i].V_DCrms += V_DC[i].rms;
    acc[i].P_DC += P_DC[i];
    acc[i].I_ACrms += I_AC[i].rms;
    acc[i].V_ACrms += V_AC[i].rms;
    acc[i].P_AC += P_AC[i]; 
    acc[i].S += S[i];
    acc[i].FP += FP[i];


    acc_raw[i].I_DCavg += IrawDC[i].avg;
    acc_raw[i].V_DCavg += VrawDC[i].avg;
    acc_raw[i].I_ACrms += IrawAC[i].rms;
    acc_raw[i].V_ACrms += VrawAC[i].rms;  

  }
  counter++;
}

/*
  Função limpadora de variáveis de média
  Para se calcular novos valores de médias, as seguintes variáveis são zeradas.
*/
void clearAverageVariables(){
  for (int i=0; i<=1; i++) {
    I_DC[i].avg = 0;
    I_DC[i].rms = 0;
    V_DC[i].avg = 0;
    V_DC[i].rms = 0;
    
    I_AC[i].avg_rms = 0;
    V_AC[i].avg_rms = 0;
    P_AC[i] = 0;
    
    FP[i] = 0;

    IrawDC[i].clearValues();
    VrawDC[i].clearValues();
    IrawAC[i].clearValues();
    VrawAC[i].clearValues();

  }
}

/*
  Função limpadora de variáveis acumuladoras
  A fim de se iniciar outra coleta de dados para outra integralização, as seguintes variáveis são zeradas.
*/
void clearCumulativeVariables(){
  for (int i=0; i<=1; i++) {  
    acc[i].I_DCavg = 0;
    acc[i].I_DCrms = 0;
    acc[i].V_DCavg = 0;
    acc[i].V_DCrms = 0;
    acc[i].P_DC = 0;
    acc[i].I_ACrms = 0;
    acc[i].V_ACrms = 0;
    acc[i].P_AC = 0; 
    acc[i].S = 0;
    acc[i].FP = 0;

    acc_raw[i].clearValues();
  }
  counter = 0;
}

double getAverageValue(int param[]) {
  double sum = 0;
  for(i=0; i < N; i++){
    sum += param[i]*stepADC;
  }
  
  double avg = sum/N;
  return avg;
}

double getRMSValueDC(int param[]) {
  double sum = 0;
  for(i=0; i < N; i++){
    sum += pow(param[i]*stepADC, 2);
  }
  
  double rms = sqrt(sum/N);
  return rms;
}

double getRMSValueAC(int param[], double avg) {
  double sum = 0;
  for(i=0; i < N; i++){
    sum += pow(param[i]*stepADC - avg, 2);
  }
  
  double rms = sqrt(sum/N);
  return rms;
}

void calibrate(double* param, double a, double b) {
  *param = *param*a + b;
}

void loop() {

/*
  Loop principal de cálculos de parâmetros DC
  Após a coleta de 384 amostras para os vetores I[N] e V[N] e C = 'C', este laço é iniciado.
  Ele é executado duas vezes seguidas, uma para dados de cada sistema, estes que são selecionados
  a partir da variável x, alterada pela função nextStep() ao fim do laço.
*/
  if (sample>=N && step=='D') {
    cli();
    
    // Soma das médias para cálculo da média
    IrawDC[x].avg += getAverageValue(I);
    VrawDC[x].avg += getAverageValue(V);
    
    IrawDC[x].rms += getRMSValueDC(I);
    VrawDC[x].rms += getRMSValueDC(V);

    iterations++;
    if (iterations >= M){
    
      IrawDC[x].avg /= M;
      VrawDC[x].avg /= M;

      IrawDC[x].rms /= M;
      VrawDC[x].rms /= M;
      
      I_DC[x] = IrawDC[x];
      V_DC[x] = VrawDC[x];


      // Curvas de calibração
    
      if (x == 0){
        // Corrente DC 1
        if (I_DC[x].avg <= 0.62){
          calibrate(&I_DC[x].avg, 2.2035, -0.3637);   //Curva para correntes abaixo de 1A
          calibrate(&I_DC[x].rms, 2.2035, -0.3637);   
        } else {
          calibrate(&I_DC[x].avg, 2.176, -0.3514);   //Curva para correntes acima de 1A
          calibrate(&I_DC[x].rms, 2.176, -0.3514);
        }
      
        // Tensão DC 1
        calibrate(&V_DC[x].avg, 8.73, 1.5027);
        calibrate(&V_DC[x].rms, 8.73, 1.5027);
      }
    
      if (x == 1){
        // Corrente DC 2
        if (I_DC[x].avg <= 3.42){
          calibrate(&I_DC[x].avg, 2.2648, -0.5329);    //Curva para correntes abaixo de 1A
          calibrate(&I_DC[x].rms, 2.2648, -0.5329);
        } else {
          calibrate(&I_DC[x].avg, 2.2294, -0.5155);   //Curva para correntes acima de 1A
          calibrate(&I_DC[x].rms, 2.2294, -0.5155);
        }
      
        // Tensão DC 2
        calibrate(&V_DC[x].avg, 8.5996, 1.3735);
        calibrate(&V_DC[x].rms, 8.5996, 1.3735);
      }

      if (I_DC[x].avg < 0 || I_DC[x].avg < 0) {
          I_DC[x].avg = 0;
          I_DC[x].rms = 0;
      }
      
      P_DC[x] = I_DC[x].rms*V_DC[x].rms;
    
      digitalWrite(LED_BUILTIN, LOW); // Acionamento do LED embutido para aferir o bom funcionamento do programa
      if(x == 0) Serial.println("I DC 1 ; V DC 1      I DC 2 ; V DC 2      I AC 1 ; V AC 1      I AC 2 ; V AC 2");
      Serial.print(IrawDC[x].avg, 4);
      Serial.print(" ; ");
      Serial.print(V_DC[x].avg, 4);
      Serial.print("      ");
      nextStep();
      iterations = 0;
    }
    sample = 0;
    sei();
  }
  
  /*
    Loop principal de cálculos de parâmetros AC
    Após a coleta de 384 amostras para os vetores I[N] e V[N] e C = 'A', este laço é iniciado.
    Ele é executado duas vezes seguidas, uma para dados de cada sistema, estes que são selecionados
    a partir da variável x, alterada pela função nextStep() ao fim do laço.
  */
  if (sample>=N && step=='A') {
    cli();    
    
    IrawAC[x].avg = getAverageValue(I);
    VrawAC[x].avg = getAverageValue(V);
    
    // Valores eficazes
    IrawAC[x].rms = getRMSValueAC(I, IrawAC[x].avg);
    VrawAC[x].rms = getRMSValueAC(V, VrawAC[x].avg);

    IrawAC[x].avg_rms += IrawAC[x].rms;
    VrawAC[x].avg_rms += VrawAC[x].rms;
    
    for(i=0; i < N ;i++){
      // Soma dos produtos de corrente e tensão
      P_AC[x] += (I[i]*stepADC - IrawAC[x].avg)*(V[i]*stepADC - VrawAC[x].avg);
    }
    
    // Potência ativa
    P_AC[x] /= N;
    
    // Soma dos valores de fator de potência para cálculo da média
    FP[x] += P_AC[x] / (IrawAC[x].rms*VrawAC[x].rms); 
    
    iterations++;
    if (iterations >= M) {
    
      IrawAC[x].rms = IrawAC[x].avg_rms/M;
      VrawAC[x].rms = VrawAC[x].avg_rms/M;

      I_AC[x] = IrawAC[x];
      V_AC[x] = VrawAC[x];
    
      // Curvas de calibração
      if (x == 0){
        // Corrente AC 1
        calibrate(&I_AC[x].rms, 0.9933, -0.0094);
        
        // Tensão AC 1
        calibrate(&V_AC[x].rms, 137.01, -2.901);
      }
    
      if (x == 1){
        // Corrente AC 2
        calibrate(&I_AC[x].rms, 1.0236, -0.0128);
          
        // Tensão AC 2
        calibrate(&V_AC[x].rms, 140.4, -9.0167);
      }

      if (I_AC[x].avg < 0 || I_AC[x].avg < 0) {             // Condição para não haver correntes negativas
          I_AC[x].avg = 0;
          I_AC[x].rms = 0;
      }

//      if (I_DC[x].avg == 0) {                               // Condição para zerar a corrente no lado AC caso não haja corrente no lado DC
//          I_AC[x].avg = 0;
//          I_AC[x].rms = 0;
//      }
      
      // Condição para não se ter valores irreais quando não houver tensão
      if (V_AC[x].rms < 180) V_AC[x].rms = 0; 
    
      // Potência
      FP[x] /= M;
      S[x] = I_AC[x].rms*V_AC[x].rms;
      P_AC[x] = S[x]*FP[x];

      Serial.print(I_AC[x].rms, 4);
      Serial.print(" ; ");
      Serial.print(V_AC[x].rms, 4);
      if (x == 0) Serial.print("      ");
      else Serial.println("\n------------------------------------------------------------------------------\n");
      nextStep();
      iterations = 0;
    }
    sample = 0;
    sei();
  }

  if (step=='S') {
    cli();
    accumulate();
    clearAverageVariables();    
    
    tempo = rtc.getTime();                                                        // Variável recebedora da informação de tempo
    if (tempo.min > timeToSaveA) timeToSaveA = (tempo.min/interval + 1)*interval; // Os dados só são salvos em minutos múltiplos de 5
    if (timeToSaveA > tempo.min + interval) timeToSaveA = 0;                      // Condição para reiniciar o ciclo quando tempo.min superar 55
    if (tempo.min == timeToSaveA){  // Salvar os dados após o tempo de espera e somente quando após os cálculos de AC2
      Serial.println("Salvando dados...");
      i = 0;
      createFile();  
      storeData();
      sendToESP32();
      Serial.println("\n----------------------------------------------------------------\n");
      
      i = 1;
      createFile();  
      storeData();
      
      clearCumulativeVariables();
      timeToSaveB = timeToSaveA + 1;
      timeToSaveA = timeToSaveA + interval;
    }
    if (tempo.min == timeToSaveB){
      sendToESP32();
      Serial.println("\n----------------------------------------------------------------\n");
      timeToSaveB = timeToSaveB + interval;
    }
          
    digitalWrite(LED_BUILTIN, HIGH); // Acionamento do LED embutido para aferir o bom funcionamento do programa
    nextStep();
    sei();
  }
}
