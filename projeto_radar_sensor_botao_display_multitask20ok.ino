/*Projeto Radar Ultrassonico
Programa: Varredura bidirecional control + Sensor
Autor: Luiz C M Oliveira
Data: 23.11.2018
Versão: 2.0 - Multitarefas com millis()
Funcionamento: Varredura do servomotor - controle de velocidade e amplitude - mostra distancia no monitor serial

*/
//Bibliotecas
#include <Servo.h>
#include <Ultrasonic.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

//Pinos de I/O
#define pot_vel_pin 0         //pino A0 - potenciometro velocidade
#define pot_ampl_pin 1        //pino A1 - potenciomentro amplitude
#define bot_pin 4             //pino botao
#define servo_pin 9           //pino pwm servo
#define TRIGGER_PIN  11
#define ECHO_PIN     12

//Objetos
Servo servo_radar; // Cria objeto servo
Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN); //cria objeto sensor ultrasonico
LiquidCrystal_I2C lcd(0x27,16,2);  // INSIRA o endereço do SEU display LCD obtido com o I2Cscanner

// variaveis de temporização
unsigned long previousMillisServo=0;
unsigned long previousMillisLED=0;
unsigned long previousMillisMede=0;
unsigned long previousMillisLCD=0;
unsigned long previousMillisSTA=0;
unsigned long previousMillisPOT=0;
unsigned long previousMillisMSG=0;
unsigned long previousMillisBOT=0;
unsigned long lastDebounceTime=0;
unsigned long previousMillisMSGinit=0;

int intervalServo = 20;               //posicao atualizada a cada 10ms
int intervalLED = 200;                //Led pisca a cada 200ms
int intervalMedeDist = intervalServo; //Distancia medida a cada mudanca de posicao do servo
int intervalLCD = 100;                //Atualiza LCD a cada 300ms
int intervalSTA = intervalServo;      //Atualiza STATUS a cada movimento
int intervalPOT = 5;                  //Atualiza valor potenciometros
int intervalMSG = 5;                  //Verifica se houve intenção de mudança de mensagem no display a cada 5ms
int intervalBOT = 5;                  //Verifica se botao foi pressionado a cada 5ms

// Controle movimento servo - inicializações
const int ang_mid = 90;       //posição central servo
int vel = 20;                 //velocidade de varredura 20 (10-30)
int ang_ampl = 180;           //amplitude de varredura (180-0)
int ang_max = 180;
int ang_min = 0;
int ang = ang_min;            // angulo inicial de movimento

//Valores das leituras das entradas analógicas dos potenciometros
int leitura_pot1_vel = 0;
int leitura_pot2_ampl = 0;

//inicializações - leitor sensor ultrasonico
float distancia_cm;

//inicialização - variaveis de string para display
char *angs = malloc(4);
String stat[3]={"DETEC!","PROX!!","SCAN.."};

//inicializações - flags (variáveis binárias) e strings
String status = "SCAN";
int updown = LOW;       // contagem crescente e decrescente - LOW -> Crescente - HIGH ->decrescente
int display_msg = LOW;  //flag mensagem a ser mostrada no display - LOW (padrão) - HIGH - vel e ang_ampl
int botao_press = LOW;  //flag indica se botao foi pressionado - HIGH - (SIM), LOW (Não)

void setup() {
   pinMode(bot_pin, INPUT_PULLUP);
   lcd.init();                      // inicializa o LCD
   lcd.backlight();                 // acende a luz de fundo
   msginit_display();               // mostra mensagens iniciais 
   servo_radar.attach(servo_pin);
   servo_radar.write(ang_mid);      //varredura começa no meio do curso
}

void loop() {
   // Loop - Multitarefa - Multitasking
   // obtem o tempo atual - millis()
   // Cada evento precisa de uma declaraçao if dentro do intervalo de tempo especificado
   
   unsigned long currentMillis = millis();
   intervalServo = vel;
   
   // Movimente Servomotor
   if ((unsigned long)(currentMillis - previousMillisServo) >= intervalServo){//Já passou o tempo para movimentação do servo? - Se sim execute, se não ignore
      if(updown == LOW){ //se não chegou no fim de curso - incrementa angulo
        ang++;
      }
      else if (updown == HIGH)
      {
        ang--; //se chegou - decremente angulo
      }; 
      servo_radar.write(ang);//posiciona radar - escreva angulo no servo
     
      if ((updown==LOW && ang>ang_max) || (updown==HIGH && ang < ang_min)) //Verifica se é momento de inverter o movimento do servo
        {                                                                  //inverte a direção se: a) (Ang++ e ang>ang_max) OU (Ang-- e ang<ang_min)
          updown=!updown;
        }
      // atualiza o temmpo de movimentação do servo em previousMillisServo
      previousMillisServo = currentMillis;
      }
   
   // Meça distância 
   if ((unsigned long)(currentMillis - previousMillisMede) >= intervalMedeDist) {//Já passou o tempo para medição da distancia? Se sim execute, se não ignore
      long microsec = ultrasonic.timing(); //emissão do sinal e medição do tempo de eco em microsegundos
      distancia_cm = ultrasonic.convert(microsec, Ultrasonic::CM); //conversao do tempo de eco para distância
      previousMillisMede = currentMillis;
   }

   //Atualize display - conforme mensagem necessária (display_msg)
   if ((unsigned long)(currentMillis - previousMillisLCD) >= intervalLCD) {//Já passou o tempo para atualização do display?
    if (display_msg == LOW){  //Nesta condição deverá ser apresentado no display a distancia, angulo e status
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Ang:");
          lcd.setCursor(8,0);
          lcd.print("Dis:");
          lcd.setCursor(0,1);
          lcd.print("Status: ");
          sprintf(angs, "%03d", ang);
          lcd.setCursor(4,0);
          lcd.print(angs);
          lcd.setCursor(12,0);
          lcd.print(distancia_cm,1);
          lcd.setCursor(8,1);
          lcd.print(status);
      }
    else if(display_msg == HIGH){//Nesta condição deverá ser apresentado - vel e ang de varredura
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("*** Settings ***");
          lcd.setCursor(0,1);
          lcd.print("vel:");
          lcd.setCursor(4,1);
          lcd.print(vel);
          lcd.setCursor(8,1);
          lcd.print("ampl:");
          lcd.setCursor(13,1);
          lcd.print(ang_ampl);
          }
          previousMillisLCD = currentMillis;
   }
   
  //Atualize o STATUS de detecção - SCAN, PROX ou DETEC
    if ((unsigned long)(currentMillis - previousMillisSTA) >= intervalSTA){//Já é tempo de atualizar o status?
      if (distancia_cm > 20){
        status = stat[2]; //"SCAN.."
      }
      else if(distancia_cm <=20 && distancia_cm>10){
        status = stat[1]; //"PROX"
        }
      else {
        status = stat[0]; //"DETEC"
      }
    previousMillisSTA = currentMillis;
    }

    //Ler os potenciômetros - atualização dos valores de velocidade e amplitude
    if ((unsigned long)(currentMillis - previousMillisPOT) >= intervalPOT){//Já passou o tempo para verificação dos potenciômetros?
      //Leitura das entradas analógicas - potenciometros
      leitura_pot1_vel = analogRead(pot_vel_pin);
      leitura_pot2_ampl = analogRead(pot_ampl_pin);
      
      //Conversão para os valores lidos para os do projeto - velocidade 10 a 30, e amplitude 0 a 180
      vel = map(leitura_pot1_vel,0,1023,10,30);
      ang_ampl = map(leitura_pot2_ampl,0,1023,0,180);

      //Atualizar os valores de angulo máximo e mínimo
      ang_max = ang_mid + ang_ampl/2;
      ang_min = ang_mid - ang_ampl/2;
      previousMillisPOT = currentMillis;
}

 //Atualizar a mensagem
    if ((unsigned long)(currentMillis - previousMillisMSG) >= intervalMSG){//Já passou o tempo para atualização da mensagem no display?
      if(botao_press == HIGH){
          display_msg = !display_msg;
      }    
    previousMillisMSG = currentMillis;
  }

  //Verificar se botão foi pressionado
    if ((unsigned long)(currentMillis - previousMillisBOT) >= intervalBOT){//Já passou o tempo para verificar se o botão foi pressionado?
      if (digitalRead(bot_pin)==LOW)
      {
      botao_press = HIGH;
      }
        else
      {
      botao_press = LOW;
      }
      previousMillisBOT = currentMillis;
    }     
}//loop

//Rotina para mostrar mensagens iniciais no display
void msginit_display(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("** Projeto Radar **");
  lcd.setCursor(0,1);
  lcd.print("MIC 2019");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("** Projeto Radar **");
  lcd.setCursor(0,1);
  lcd.print("**** Autor ****");
  delay(1000);  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("** Projeto Radar **");
  lcd.setCursor(0,1);
  lcd.print("Luiz Marangoni");
  delay(1000);
}
    

