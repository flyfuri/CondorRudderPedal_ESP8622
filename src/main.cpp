#include <Arduino.h>
#include <timer.h>
#include <analog_filter.h>
#include <SinusoidIncCounter.h>

#define DEBGCH 0 //debug chanel mode: 0=normal(mux) 1=only CH1; 2=only CH2; 3=only CH3; 2=only CH4; 5=long mux (3s each channel)  
#define DEBGOUT -1 //debug chanel mode: 0=normal(hitire) : 1=value 2=both channels raw 3=both channels filtered 4=CH1 filtered with MeasNbr 99=debug-print
#define SCALEM 2 //Scalemode:  0= +/-180  1= 0..256(128 in middle) 2=unscaled

#if DEBGOUT == 99
  #define dbugprint(x) Serial.print(x)
  #define dbugprintln(x) Serial.println(x)
#else
  #define dbugprint(x) 
  #define dbugprintln(x) 
#endif

const int ACT_MUX_CH1 = GPIO_ID_PIN(4);
const int ACT_MUX_CH2 = GPIO_ID_PIN(5);
const int ACT_MUX_CH3 = GPIO_ID_PIN(12);
const int ACT_MUX_CH4 = GPIO_ID_PIN(13);
const int IR_LEDS = GPIO_ID_PIN(14);

int act_Mux_Channel = 0; //which MUX-Channel to activate (0 = none, 1,2,3,4)
int i_clk; //counters: loopclock,measures CH1 measures CH2
int resetADC;
int daylightDist[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc (measure daylight desturbance by measure without LED activated)
int analogRaw[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc  
float filtValue[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc
float sumCh1u2; //summary of Channel 1 and 2 
unsigned long t_lastcycl, t_now; //measure cycle time
int outputRudder;
uint8_t out8BitRudder;

CFilterAnalog filterCH[5]; //1=Ch1, 2=Ch2
CTimer TimerInitLeft, TimerInitRigth, TimerBlink, TimerMux;
CSinIncCntr encoderL; //encoder Left pedal
float minLPedal, maxLPedal, minRPedal, maxRPedal;
float tempLPedal, tempRPedal;
bool tLres, tRres, blkFlag; 
bool bMuxDelay; //wait for mux delay
bool bIR_LED_on; //IR LED is on

const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0

int sensorValue = 0;  // value read from the pot

// hatire struct read
typedef struct  {
  int16_t  Begin  ;   // 2  Debut
  uint16_t Cpt ;      // 2  Compteur trame or Code
  float    gyro[3];   // 12 [Y, P, R]    gyro
  float    acc[3];    // 12 [x, y, z]    Acc
  int16_t  End ;      // 2  Fin
} tdef_hat;

tdef_hat hat;



//procedure read channel with daylight filter
void procDayLightFilter(short chNr, bool bLEDisON){
  if (chNr > 0 && chNr < 5){
    if (bLEDisON){
      analogRaw[chNr] = analogRead(analogInPin) - daylightDist[chNr]; //A1: right pedal 772..118
      filtValue[chNr] = filterCH[chNr].measurement(analogRaw[chNr]);
    }
    else{
      daylightDist[chNr] =  analogRead(analogInPin);
    }
  }
}

//procedure read channel without daylight filter
void readChannel(short chNr, bool bLEDisON){
  if (chNr > 0 && chNr < 5){
    if (bLEDisON){
      analogRaw[chNr] = analogRead(analogInPin); //A1: right pedal 772..118
      filtValue[chNr] = filterCH[chNr].measurement(analogRaw[chNr]);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  act_Mux_Channel = 0;
  minLPedal=-1;
  maxLPedal=-1;
  minRPedal=-1;
  maxRPedal=-1;
  i_clk=0;
  sumCh1u2=0;
  bMuxDelay = false;
  encoderL.setTo(0);
  //analogReference(DEFAULT);
  pinMode(ACT_MUX_CH1, OUTPUT); 
  pinMode(ACT_MUX_CH2, OUTPUT); 
  pinMode(ACT_MUX_CH3, OUTPUT); 
  pinMode(ACT_MUX_CH4, OUTPUT); 
  pinMode(IR_LEDS, OUTPUT); 
  digitalWrite(ACT_MUX_CH1, LOW);
  digitalWrite(ACT_MUX_CH2, LOW);
  digitalWrite(ACT_MUX_CH3, LOW);
  digitalWrite(ACT_MUX_CH4, LOW);
  bIR_LED_on = false;
  digitalWrite(IR_LEDS, bIR_LED_on ? HIGH : LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  //only arduiono: resetADC=analogRead(A0); //read A0 pinned to ground to reset ADC capacitor;
  TimerBlink.setTime(1000);
  #if DEBGCH == 5
    TimerMux.setTime(3000);
  #else 
    TimerMux.setTime(1);
  #endif
  Serial.begin(460800);//(115200);

  //initialize hatire
  hat.Begin=0xAAAA; // header frame 
  hat.Cpt=0; // Frame Number or Error code 
  hat.End=0x5555; // footer frame
  hat.gyro[0] = 0;
  hat.gyro[1] = 0;
  hat.gyro[2] = 0;
  hat.acc[0] = 0;
  hat.acc[1] = 0;
  hat.acc[2] = 0;
}

void loop() {
  
    dbugprint(bMuxDelay == true ? "True" : "False" );
    dbugprint("   ");
  if(TimerMux.evaluate(bMuxDelay)){
    bMuxDelay = false;
    TimerMux.evaluate(false);
    dbugprint(" ");
    dbugprint(bMuxDelay == true ? "TRUE" : "" );
    dbugprint(" ");
  }
  if(!bMuxDelay){
    switch(act_Mux_Channel){
        case 1:   readChannel(1, bIR_LED_on);
                  encoderL.addmeas(1,(int)filtValue[1]);
                  act_Mux_Channel = 2;
                  break;
        case 2:   readChannel(2, bIR_LED_on);
                  encoderL.addmeas(2,(int)filtValue[2]);
                  sumCh1u2 = filtValue[1] + filtValue[2];
                  act_Mux_Channel = 3;
                  break;
        case 3:   readChannel(3, bIR_LED_on);
                  act_Mux_Channel = 4;
                  break;
        case 4:   readChannel(4, bIR_LED_on);
                  act_Mux_Channel = 5;
                  break;
    }
  }
  if(i_clk == 0) {
      bIR_LED_on = true;
      digitalWrite(IR_LEDS, bIR_LED_on ? HIGH : LOW);
      act_Mux_Channel = 0;
      bMuxDelay = false;
      i_clk++;
  } 
  /*else if (i_clk == 10) {
      act_Mux_Channel = 1;  //trigger 1 measure for all channels
        i_clk++;
  }
  else if (i_clk == 11){
      if (act_Mux_Channel == 5){
        i_clk++;
      }
  } 
  else if (i_clk == 10) {
    bIR_LED_on = true;
    digitalWrite(IR_LEDS, bIR_LED_on ? HIGH : LOW);
    i_clk++;
  } */
  else if (i_clk == 10){ 
      act_Mux_Channel = 1; //trigger 1 measure for all channels
      i_clk++;
  }
  else if (i_clk == 11){
    if (act_Mux_Channel == 5){
       Serial.print("  ");
       Serial.print(sumCh1u2);
       Serial.print(encoderL.calc());
      //Serial.print("  ");
      
      /*bIR_LED_on = true;
      digitalWrite(IR_LEDS, bIR_LED_on ? HIGH : LOW);  */ 
      #if DEBGOUT == 2
        Serial.print(analogRaw1);
        Serial.print("  ");
        Serial.println(analogRaw2);
      #elif DEBGOUT == 3
        Serial.print(filtValue[1]);
        Serial.print("  ");
        Serial.print(filtValue[2]);
        Serial.print("  ");
        Serial.print(sumCh1u2);
      #elif DEBGOUT == 4
        Serial.print(filtValue[1]);
        Serial.print("  ");
        Serial.print(filterA.getNbrMeas());
      #endif
    
      #if DEBGOUT != 0
        t_lastcycl = t_now;
        t_now = millis();
        //Serial.print("  ");
        //Serial.println(t_now - t_lastcycl);
      #endif

      i_clk=10;
    
      if (minLPedal >= 0 && minRPedal >= 0 && maxLPedal >= 0 && maxRPedal >= 0){ 
        #if SCALEM == 0
          filtValue[1] = map(filtValue[1], minRPedal, maxRPedal, 0 , -180);
          filtValue[2] = map(filtValue[2], minLPedal, maxLPedal, 0 , 180); 
        #elif SCALEM == 1
          filtValue[1] = map(filtValue[1], minRPedal, maxRPedal, 128 , 0);
          filtValue[2] = map(filtValue[2], minLPedal, maxLPedal, 0 , 128); 
        #elif SCALEM == 2
        #endif
        //outputRudder = filtValue[1] + filtValue[2];
        //out8BitRudder = outputRudder;  //old output for UNO_Joy

        #if DEBGOUT == 0
          if(abs(hat.gyro[0] - (filtValue[1] + filtValue[2])) > 1){
            hat.gyro[0] = filtValue[1] + filtValue[2];

              // Send HAT  Trame to  PC
              Serial.write((byte*)&hat,30);
              hat.Cpt++;
              if (hat.Cpt>999) {
                  hat.Cpt=0;
              }
          }
        #elif DEBGOUT == 1 
          Serial.println(outputRudder);
        #endif

        TimerBlink.setTime(50);         
      } 
      else{  
        if (minLPedal < 0 ){
          TimerInitLeft.setTime(4000);
          if (abs(tempLPedal - filtValue[2]) < 5 && filtValue[2] < 400){ //value stable and over certain level
          tLres=TimerInitLeft.evaluate(true);              
            if (tLres) {
              minLPedal=filtValue[2];
              //Serial.println("minL calibrated");          
            }
          }
          else {
            tempLPedal = filtValue[2];
            TimerInitLeft.evaluate(false);
          }
        }            
        else if (maxLPedal < 0){
          if (abs(tempLPedal - filtValue[2]) < 5 && filtValue[2] > 650){  //value stable and over certain level
            tLres=TimerInitLeft.evaluate(true);     
            if (tLres){
              maxLPedal=filtValue[2];            
              //Serial.println("maxL calibrated");
              TimerBlink.setTime(750);                       
            }
          }
          else{
            tempLPedal = filtValue[2];
            TimerInitLeft.evaluate(false);
          }
        } 

        if (minRPedal < 0 ){
          TimerInitRigth.setTime(4000);
          if ((abs(tempRPedal - filtValue[1])) < 5 && (filtValue[1] < 400)) {      
            tRres=TimerInitRigth.evaluate(true);
            if (tRres==0){
              minRPedal=filtValue[1];
              //Serial.println("minR calibrated");           
            }
          }
          else {
            tempRPedal = filtValue[1];
            TimerInitRigth.evaluate(false);
          }
        }            
        else if (maxRPedal < 0){
          if (abs(tempRPedal - filtValue[1]) < 5 && filtValue[1] > 650){      
            tRres=TimerInitRigth.evaluate(true);
            if (tRres){
              maxRPedal=filtValue[1];
              //Serial.println("maxR calibrated"); 
              TimerBlink.setTime(500);                      
            }
          }
          else{
            tempRPedal = filtValue[1];
            TimerInitRigth.evaluate(false);
          }
        } 
      }        
    }
  }
  else{
    if (!bMuxDelay && (act_Mux_Channel == 0 || act_Mux_Channel == 5)){
      i_clk++; 
    }
  }
  //Led Blink 
  if (TimerBlink.evaluate(true)) {
    if (blkFlag){
      digitalWrite(LED_BUILTIN, LOW);
      blkFlag=false;
    } 
    else{
        digitalWrite(LED_BUILTIN, HIGH);
        blkFlag=true;       
    }    
    TimerBlink.evaluate(false);  
  } 
  //MuxOuts 
  #if DEBGCH == 0 || DEBGCH == 5
    if(!bMuxDelay){
      switch(act_Mux_Channel){
        case 0: 
        case 5: 
          digitalWrite(ACT_MUX_CH1, LOW);
          digitalWrite(ACT_MUX_CH2, LOW);
          digitalWrite(ACT_MUX_CH3, LOW);
          digitalWrite(ACT_MUX_CH4, LOW);
          break;
        case 1: 
          digitalWrite(ACT_MUX_CH2, LOW);
          digitalWrite(ACT_MUX_CH3, LOW);
          digitalWrite(ACT_MUX_CH4, LOW);
          digitalWrite(ACT_MUX_CH1, HIGH);
          bMuxDelay = true;
          break;
        case 2: 
          digitalWrite(ACT_MUX_CH1, LOW);
          digitalWrite(ACT_MUX_CH3, LOW);
          digitalWrite(ACT_MUX_CH4, LOW);
          digitalWrite(ACT_MUX_CH2, HIGH);
          bMuxDelay = true;
          break;
        case 3: 
          digitalWrite(ACT_MUX_CH1, LOW);
          digitalWrite(ACT_MUX_CH2, LOW);
          digitalWrite(ACT_MUX_CH4, LOW);
          digitalWrite(ACT_MUX_CH3, HIGH);
          bMuxDelay = true;
          break;
        case 4: 
          digitalWrite(ACT_MUX_CH1, LOW);
          digitalWrite(ACT_MUX_CH2, LOW);
          digitalWrite(ACT_MUX_CH3, LOW);
          digitalWrite(ACT_MUX_CH4, HIGH);
          bMuxDelay = true;
          break;
      }  
    }     
  #elif DEBGCH == 1
    digitalWrite(ACT_MUX_CH2, LOW);
    digitalWrite(ACT_MUX_CH1, HIGH);
  #elif DEBGCH == 2
    digitalWrite(ACT_MUX_CH1, LOW);
    digitalWrite(ACT_MUX_CH2, HIGH);
  #endif
  dbugprint(act_Mux_Channel);
  dbugprint(" ");
  dbugprint(i_clk);
  dbugprint(" ");
  dbugprint(bMuxDelay == true ? "true" : "false" );
  dbugprint(" ");
  dbugprint(TimerMux.getDelay());
  dbugprint(" ");
  dbugprintln(TimerMux.getElapsedTime());
}