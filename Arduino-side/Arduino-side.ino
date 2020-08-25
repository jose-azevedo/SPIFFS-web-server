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
void clearVariables(void);
void clearAverageVariables(void);

int timeToSaveA = 0, timeToSaveB = 0;
int acc = 0;

char C = 'C';
int x = 0;
int bitInput = 0;
int sample = 0;
int media = 0;
int i = 0;

unsigned int ADCH_16 = 0;
unsigned int ADCL_16 = 0;

//Vetores com os valores de três ciclos de corrente e tensão
int I[N] = {0}, V[N] = {0};

//Soma dos valores do vetor para cálculo da média
double I_DC[2] = {0}, V_DC[2] = {0}, I_AC[2] = {0}, V_AC[2] = {0};

//Média de dez aquisições de valores médios de corrente e tensão
double avgI_DC[2] = {0}, avgV_DC[2] = {0};

//Soma dos quadrados dos valores dos vetores
double sumI_DC[2] = {0}, sumV_DC[2] = {0}, sumI_AC[2] = {0}, sumV_AC[2] = {0};

//Soma dos produtos de corrente e tensão
double sumP_AC[2] = {0};

//Valores eficazes de corrente e tensão
double rmsI_DC[2] = {0}, rmsV_DC[2] = {0}, rmsI_AC[2] = {0}, rmsV_AC[2] = {0};

//Média de dez aquisições dos valores eficazes de corrente e tensão
double avg_rmsI_AC[2] = {0}, avg_rmsV_AC[2] = {0};

//Potências ativas
double P_DC[2] = {0}, P_AC[2] = {0};

//Potências aparentes
double S[2] = {0};

//Soma dos fatores de potência de dez aquisições e média destes
double FP[2] = {0};

//Variáveis acumuladoras para armazenamento local
double acc_avgI_DC[2] = {0}, acc_avgV_DC[2] = {0}, acc_rmsI_DC[2] = {0}, acc_rmsV_DC[2] = {0};
double acc_rmsI_AC[2] = {0}, acc_rmsV_AC[2] = {0};
double accP_DC[2] = {0}, accP_AC[2] = {0};
double accS[2] = {0};
double accFP[2] = {0};


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
    case 0x63: // ADC2: Corrente DC 2
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
    C = 'C';
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
void next(){
  if (x==0 && C=='C') {
    x = 1;
    ADMUX = 0x62;
  }
  else {
    if (x==1 && C=='C') {
      x = 0;
      C = 'A';
      ADMUX = 0x64;
    }
    else {
      if (x==0 && C=='A') {
        x = 1;
        ADMUX = 0x66;
      }
      else {
        if (x==1 && C=='A') {
          x = 0;
          C = 'C';
          ADMUX = 0x60;
        }
      }
    }
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

  const String header = "Hora;I DC;I DC rms;V DC;V DC rms;P DC;I AC rms;V AC rms;S;FP";
  
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

  String fileData = String(rtc.getTimeStr()) + ";" + String(acc_avgI_DC[i]/acc) + ";" + String(acc_rmsI_DC[i]/acc) + ";" + String(acc_avgV_DC[i]/acc) + ";" + String(acc_rmsV_DC[i]/acc) + ";" + String(accP_DC[i]/acc) + ";" + String(acc_rmsI_AC[i]/acc) + ";" + String(acc_rmsV_AC[i]/acc) + ";" + String(accS[i]/acc) + ";" + String(accFP[i]/acc);
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
  for (i=0; i<=1; i++) {  
    acc_avgI_DC[i] += avgI_DC[i];
    acc_avgV_DC[i] += avgV_DC[i];
    acc_rmsI_DC[i] += rmsI_DC[i];
    acc_rmsV_DC[i] += rmsV_DC[i];
    acc_rmsI_AC[i] += avg_rmsI_AC[i];
    acc_rmsV_AC[i] += avg_rmsV_AC[i];
    accP_DC[i] += P_DC[i];
    accP_AC[i] += P_AC[i]; 
    accS[i] += S[i];
    accFP[i] += FP[i];
  }
  acc++;
}

/*
  Função limpadora de variáveis instantâneas
  Para receber novos dados do conversor AD, as seguintes variáveis são zeradas.
*/
void clearVariables(){
  sample = 0;
  for (i=0; i<=1; i++) {
    I_DC[i] = 0;
    V_DC[i] = 0;
    I_AC[i] = 0;
    V_AC[i] = 0;
    
    sumI_AC[i] = 0;
    sumV_AC[i] = 0;
    
    sumP_AC[i] = 0;
  }
}

/*
  Função limpadora de variáveis de média
  Para se calcular novos valores de médias, as seguintes variáveis são zeradas.
*/
void clearAverageVariables(){
  for (i=0; i<=1; i++) {
    avgI_DC[i] = 0;
    avgV_DC[i] = 0;
    rmsI_DC[i] = 0;
    rmsV_DC[i] = 0;
    
    avg_rmsI_AC[i] = 0;
    avg_rmsV_AC[i] = 0;
    
    FP[i] = 0;
  }
}

/*
  Função limpadora de variáveis acumuladoras
  A fim de se iniciar outra coleta de dados para outra integralização, as seguintes variáveis são zeradas.
*/
void clearCumulativeVariables(){
  for (i=0; i<=1; i++) {  
    acc_avgI_DC[i] = 0;
    acc_avgV_DC[i] = 0;
    acc_rmsI_DC[i] = 0;
    acc_rmsV_DC[i] = 0;
    acc_rmsI_AC[i] = 0;
    acc_rmsV_AC[i] = 0;
    accP_DC[i] = 0;
    accP_AC[i] = 0; 
    accS[i] = 0;
    accFP[i] = 0; 
  }
  acc = 0;
}

void loop() {

/*
  Loop principal de cálculos de parâmetros DC
  Após a coleta de 384 amostras para os vetores I[N] e V[N] e C = 'C', este laço é iniciado.
  Ele é executado duas vezes seguidas, uma para dados de cada sistema, estes que são selecionados
  a partir da variável x, alterada pela função next() ao fim do laço.
*/
  if (sample>=N && C=='C') {
    cli();
    
    for(i=0; i<N ;i++){
      // Soma para média
      I_DC[x] += I[i]*stepADC;
      V_DC[x] += V[i]*stepADC;
    }
    
    I_DC[x] = I_DC[x]/N;
    V_DC[x] = V_DC[x]/N;
    
    // Soma das médias para cálculo da média
    avgI_DC[x] += I_DC[x];
    avgV_DC[x] += V_DC[x];
    
    for(i=0; i<N ;i++){
      // Soma dos quadrados
      sumI_DC[x] += pow(I[i]*stepADC, 2);
      sumV_DC[x] += pow(V[i]*stepADC, 2);
    }
    
    rmsI_DC[x] += sqrt(sumI_DC[x]/N);
    rmsV_DC[x] += sqrt(sumV_DC[x]/N);
    
    media++;
    
    if (media >= M){
    
      avgI_DC[x] = avgI_DC[x]/M;
      avgV_DC[x] = avgV_DC[x]/M;
      rmsI_DC[x] = rmsI_DC[x]/M;
      rmsV_DC[x] = rmsV_DC[x]/M;
      
      // Curvas de calibração
    
      if (x == 0){
        // Corrente DC 1
        if (avgI_DC[x] <= 3.574){
          avgI_DC[x] = avgI_DC[x]*8.3227 - 28.349;   //Curva para correntes abaixo de 1A
          rmsI_DC[x] = rmsI_DC[x]*8.3227 - 28.349;  
        }
        else{
          avgI_DC[x] = avgI_DC[x]*9.5957 - 32.781;   //Curva para correntes acima de 1A
          rmsI_DC[x] = rmsI_DC[x]*9.5957 - 32.781;  
        }
      
        // Tensão DC 1
        avgV_DC[x] = avgV_DC[x]*8.226 + 0.3816;
        rmsV_DC[x] = rmsV_DC[x]*8.226 + 0.3816;
      }
    
      if (x == 1){
        // Corrente DC 2
        if (avgI_DC[x] <= 3.574){
          avgI_DC[x] = avgI_DC[x]*9.7144 - 33.469;   //Curva para correntes abaixo de 1A
          rmsI_DC[x] = rmsI_DC[x]*9.7144 - 33.469;
        }
        else{
          avgI_DC[x] = avgI_DC[x]*9.5777 - 33.01;    //Curva para correntes acima de 1A
          rmsI_DC[x] = rmsI_DC[x]*9.5777 - 33.01;   
        }
      
        // Tensão DC 2
        avgV_DC[x] = avgV_DC[x]*8.3735 + 0.4027;
        rmsV_DC[x] = rmsV_DC[x]*8.3735 + 0.4027;    
      }
    
      
      P_DC[x] = rmsI_DC[x]*rmsV_DC[x];
    
      digitalWrite(LED_BUILTIN, LOW); // Acionamento do LED embutido para aferir o bom funcionamento do programa
      next();
      media = 0;
    }
    clearVariables();
    sei();
  }
  
  /*
    Loop principal de cálculos de parâmetros AC
    Após a coleta de 384 amostras para os vetores I[N] e V[N] e C = 'A', este laço é iniciado.
    Ele é executado duas vezes seguidas, uma para dados de cada sistema, estes que são selecionados
    a partir da variável x, alterada pela função next() ao fim do laço. Ao fim, também são operadas
    as variáveis de temporização de armazenamento e são chamadas as funções que salvam os dados no cartão SD.
  */
  if (sample>=N && C=='A') {
    cli();
    
    for(i=0; i<N ;i++){
      // Soma para média
      I_AC[x] += I[i]*stepADC;
      V_AC[x] += V[i]*stepADC;
    }
    
    I_AC[x] = I_AC[x]/N;
    V_AC[x] = V_AC[x]/N;
    
    for(i=0; i<N ;i++){
      // Soma dos quadrados
      sumI_AC[x] += pow(I[i]*stepADC - I_AC[x], 2);
      sumV_AC[x] += pow(V[i]*stepADC - V_AC[x], 2);
      
      // Soma dos produtos de corrente e tensão
      sumP_AC[x] += (I[i]*stepADC - I_AC[x])*(V[i]*stepADC - V_AC[x]);
    }
    
    // Valores eficazes
    rmsI_AC[x] = sqrt(sumI_AC[x]/N);
    rmsV_AC[x] = sqrt(sumV_AC[x]/N);
    
    // Soma dos valores eficazes para cálculo da média
    avg_rmsI_AC[x] += rmsI_AC[x];
    avg_rmsV_AC[x] += rmsV_AC[x];
    
    // Potência ativa
    P_AC[x] = sumP_AC[x]/N;
    
    // Soma dos valores de fator de potência para cálculo da média
    FP[x] += P_AC[x]/(rmsI_AC[x]*rmsV_AC[x]); 
    
    media++;
    
    if (media >= M){
    
      avg_rmsI_AC[x] = avg_rmsI_AC[x]/M;
      avg_rmsV_AC[x] = avg_rmsV_AC[x]/M;
    
          // Curvas de calibração
      if (x == 0){
        // Corrente AC 1
        if (avg_rmsI_AC[x] <= 0.076)  avg_rmsI_AC[x] = avg_rmsI_AC[x]*13.116 - 0.0161;   //Curva para correntes abaixo de 1A
        else  avg_rmsI_AC[x] = avg_rmsI_AC[x]*13.556 - 0.0279;                           //Curva para correntes acima de 1A
        
        // Tensão AC 1
        avg_rmsV_AC[x] = avg_rmsV_AC[x]*154.74 - 24.544;
      }
    
      if (x == 1){
        // Corrente AC 2
        if (avg_rmsI_AC[x] <= 0.079)  avg_rmsI_AC[x] = avg_rmsI_AC[x]*13.189 - 0.0182;   //Curva para correntes abaixo de 1A
        else  avg_rmsI_AC[x] = avg_rmsI_AC[x]*13.621301 - 0.032166;                      //Curva para correntes acima de 1A
      
        // Tensão AC 2
        avg_rmsV_AC[x] = avg_rmsV_AC[x]*154.080718 - 29.486292;
      }
      
      // Condição para não se ter valores irreais quando não houver tensão
      if (avg_rmsV_AC[x] < 180) avg_rmsV_AC[x] = 0; 
    
      // Potência
      FP[x] = FP[x]/M;
      S[x] = avg_rmsI_AC[x]*avg_rmsV_AC[x];
      P_AC[x] = S[x]*FP[x];
      
      if (x == 1){
        accumulate();
        clearAverageVariables();
      }
      
      tempo = rtc.getTime(); // Variável recebedora da informação de tempo
      if (tempo.min > timeToSaveA) timeToSaveA = (tempo.min/interval + 1)*interval; // Os dados só são salvos em minutos múltiplos de 5
      if (timeToSaveA > tempo.min + interval) timeToSaveA = 0; // Condição para reiniciar o ciclo quando tempo.min superar 55
      if (tempo.min == timeToSaveA && x == 1){ // Salvar os dados após o tempo de espera e somente quando após os cálculos de AC2
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
      if (tempo.min == timeToSaveB && x == 1){
        sendToESP32();
        Serial.println("\n----------------------------------------------------------------\n");
        timeToSaveB = timeToSaveB + interval;
      }
           
      digitalWrite(LED_BUILTIN, HIGH); // Acionamento do LED embutido para aferir o bom funcionamento do programa
      next();
      media = 0;
    }
    clearVariables();
    sei();
  }
}
