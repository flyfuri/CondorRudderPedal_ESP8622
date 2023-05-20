#include <Arduino.h>
#include <timer.h>
#include <analog_filter.h>
#include <SinusoidIncCounter.h>

#define DEBGCH 0 //debug chanel mode: 0=normal(mux) 1=only CH1; 2=only CH2; 3=only CH3; 2=only CH4; 5=long mux (3s each channel)  
#define DEBGOUT 99 //debug chanel mode: 0=normal(hatire) : 1=encoder left 2=both channels raw 3=both channels filtered 4=CH1 filtered with MeasNbr 99=debug-print
#define SCALEM 0 //Scalemode:  0= +/-180  1= 0..256(128 in middle) 2=unscaled
#define PWMON 1 // PWM output is on

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
const int PIN_PWM_OUT = GPIO_ID_PIN(2);

int act_Mux_Channel = 0; //which MUX-Channel to activate (0 = none, 1,2,3,4)
int i_clk; //counters: loopclock,measures CH1 measures CH2
int resetADC;
int daylightDist[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc (measure daylight desturbance by measure without LED activated)
int analogRaw[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc  
float filtValue[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc
int encoderResult; //summary of Channel 1 and 2 
unsigned long t_lastcycl, t_now; //measure cycle time
int outputRudder;
uint8_t out8BitRudder;

ANFLTR::CFilterAnalogOverTime filterCH[5] = {{1000, 1000}, {1000, 1000}, {1000, 1000}, {1000, 1000}, {1000, 1000} }; //1=Ch1, 2=Ch2
TIMER::CTimerMillis TimerInitLeft, TimerInitRigth, TimerBlink;
TIMER::CTimerMicros TimerMux, TimerIRonOff;
CSinIncCntr encoderL; //encoder Left pedal
float minLPedal, maxLPedal, minRPedal, maxRPedal;
float tempLPedal, tempRPedal;
bool tLres, tRres, blkFlag; 
bool bMuxDelay, bIRonoffDelay; //wait for mux delay
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
  minLPedal=-9999;
  maxLPedal=-9999;
  minRPedal=-9999;
  maxRPedal=-9999;
  i_clk=0;
  encoderResult=0;
  bMuxDelay = false;
  bIR_LED_on = false;
  encoderL.setTo(0);
  //analogReference(DEFAULT);
  pinMode(A0, INPUT);
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
  pinMode(LED_BUILTIN, OUTPUT); //same as GPOI2
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(PIN_PWM_OUT, OUTPUT);
  digitalWrite(PIN_PWM_OUT, LOW);
  //only arduiono: resetADC=analogRead(A0); //read A0 pinned to ground to reset ADC capacitor;
  TimerBlink.setTime(1000);
  #if DEBGCH == 5
    TimerMux.setTime(3000000);
  #else 
    TimerMux.setTime(20);
  #endif
  TimerIRonOff.setTime(50);

  #if DEBGOUT != 0 //hitire max on 115200
    Serial.begin(115200, SERIAL_8N1, SERIAL_FULL);//;(256000);//(230400);//(460800);//(115200);
  #else
    Serial.begin(115200, SERIAL_8N1, SERIAL_RX_ONLY); //hitire max on 115200!!!!
  
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
  #endif
}

void loop() {
  
  if(TimerMux.evaluate(bMuxDelay)){
    bMuxDelay = false;
    TimerMux.evaluate(false);
  }
  if(TimerIRonOff.evaluate(bIRonoffDelay)){
    bIRonoffDelay = false;
    TimerIRonOff.evaluate(false);
  }

  if(!bMuxDelay && !bIRonoffDelay){
    switch(act_Mux_Channel){
        case 1:   procDayLightFilter(1, bIR_LED_on);
                  act_Mux_Channel = 2;
                  break;
        case 2:   procDayLightFilter(4, bIR_LED_on);
                  act_Mux_Channel = 3;
                  break;
        case 3:   procDayLightFilter(3, bIR_LED_on);
                  act_Mux_Channel = 4;
                  break;
        case 4:   procDayLightFilter(2, bIR_LED_on);
                  act_Mux_Channel = 5;
                  break;
    }
  }
  if(i_clk == 0) {
      bIR_LED_on = false;
      digitalWrite(IR_LEDS, bIR_LED_on ? HIGH : LOW);
      bMuxDelay = false;
      bIRonoffDelay = true;
      act_Mux_Channel = 1;  //trigger 1 measure (IR off, measure disturbing light)for all channels
      i_clk = 10;
  } 
  else if (i_clk == 10){
      if (act_Mux_Channel == 5){
        bIR_LED_on = true;
        digitalWrite(IR_LEDS, bIR_LED_on ? HIGH : LOW);
        bIRonoffDelay = true;
        act_Mux_Channel = 1;  //trigger 1 measure (IR off, measure disturbing light)for all channels
        i_clk=20;
      }
  } 
  else if (i_clk == 20){
    if (act_Mux_Channel == 5){
      bIR_LED_on = false;
      digitalWrite(IR_LEDS, bIR_LED_on ? HIGH : LOW);
      bMuxDelay = false;
      bIRonoffDelay = true;
      act_Mux_Channel = 1;  //trigger 1 measure (IR off, measure disturbing light)for all channels
      i_clk = 10;

      encoderResult = encoderL.calc((int)filtValue[1], (int)filtValue[2]);
  
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
        Serial.print("  ");
        Serial.print(filtValue[1] -filtValue[2]);
        Serial.print("  ");
      #elif DEBGOUT == 4
        Serial.print(filtValue[1]);
        Serial.print("  ");
        Serial.print(filterA.getNbrMeas());
      #endif

      minRPedal = -500;
      maxRPedal = 500;
      #if PWMON == 1
        int pwmscaled = constrain(map(encoderResult, minRPedal, maxRPedal, 10 , 245), 10, 245);
        digitalWrite(PIN_PWM_OUT, pwmscaled);
        Serial.print("  ");
        Serial.println(pwmscaled);
        //digitalWrite(PIN_PWM_OUT, map(encoderResult, minRPedal, maxRPedal, 10 , 245));
      #endif

      #if SCALEM == 0
        encoderResult = map(encoderResult, minRPedal, maxRPedal, 0 , -180);  //TODO:
      #elif SCALEM == 1
        encoderResult = map(encoderResult, minRPedal, maxRPedal, 128 , 0);  //TODO:
      #elif SCALEM == 2
      #endif
      //outputRudder = filtValue[1] + filtValue[2];
      //out8BitRudder = outputRudder;  //old output for UNO_Joy

      #if DEBGOUT == 0
        if(abs(hat.gyro[0] - (float)encoderResult > 1)){
          hat.gyro[0] = (float)encoderResult;

            // Send HAT  Trame to  PC
            Serial.write((byte*)&hat,30);
            hat.Cpt++;
            if (hat.Cpt>999) {
                hat.Cpt=0;
            }
        }
      #elif DEBGOUT == 1 
        Serial.println(outputRudder);
      #elif DEBGOUT == 10
      Serial.print(encoderResult);
      #endif

      #if DEBGOUT != 0
        t_lastcycl = t_now;
        t_now = micros();
        Serial.print("  ");
        Serial.println(t_now - t_lastcycl);
      #endif       
    }        
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
          digitalWrite(ACT_MUX_CH2, LOW);
          digitalWrite(ACT_MUX_CH3, LOW);
          digitalWrite(ACT_MUX_CH4, HIGH);
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
          digitalWrite(ACT_MUX_CH4, LOW);
          digitalWrite(ACT_MUX_CH3, LOW);
          digitalWrite(ACT_MUX_CH2, HIGH);
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
  
  /*dbugprint(act_Mux_Channel);
  dbugprint(" ");
  dbugprint(i_clk);
  dbugprint(" ");
  dbugprint(bMuxDelay == true ? "true" : "false" );
  dbugprint(" ");
  dbugprint(TimerMux.getDelay());
  dbugprint(" ");
  dbugprintln(TimerMux.getElapsedTime());*/
}