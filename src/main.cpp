#include <Arduino.h>
#include <timer.h>
#include <analog_filter.h>
#include <SinusoidIncCounter.h>
#include <IO_wiring.h>
#include <ESP8266WiFi.h>

#if DEBGOUT == 99
  #define dbugprint(x) Serial.print(x)
  #define dbugprintln(x) Serial.println(x)
#else
  #define dbugprint(x) 
  #define dbugprintln(x) 
#endif

int act_Mux_Channel = 0; //which MUX-Channel to activate (0 = none, 1,2,3,4)
int i_clk; //counters: loopclock,measures CH1 measures CH2
int resetADC;
int daylightDist[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc (measure daylight desturbance by measure without LED activated)
int analogRaw[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc  
int filtValue[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc
int encoderL_Result; //encoder LeftPedal Channel 1 and 2 
int encoderR_Result; //encoder RightPedal Channel 3 and 4 
unsigned long t_lastcycl, t_now; //measure cycle time
int outputRudder;
uint8_t out8BitRudder;
int cnt0swtch = 0; //counter to "filter" zero switch

ANFLTR::CFilterAnalogOverTime<int> filterCH[5] = {{1, 1}, {1000, 4000}, {1000, 4000}, {1000, 4000}, {1000, 4000} }; //0 = not used, 1=Ch1, 2=Ch2, ...
ANFLTR::CFilterAnalogOverMeasures<double> derivativesCH[5] = {{1, 1}, {6, 6}, {6, 6}, {6, 6}, {6, 6} }; //0 = not used, 1=Ch1, 2=Ch2, ...
TIMER::CTimerMillis TimerInitLeft, TimerInitRigth, TimerBlink;
TIMER::CTimerMicros TimerMux, TimerIRonOff;
CSinIncCntr encoderL, encoderR; //encoder Left pedal and Right pedal
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
      double tmpvalue = filterCH[chNr].getAverageDbl();
      derivativesCH[chNr].measurementIfMinChange(tmpvalue, 0.8);
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
  encoderL_Result=0;
  bMuxDelay = false;
  bIR_LED_on = false;
  encoderL.setTo(0);
  encoderL.setScalings(1,1,0,0);
  encoderR.setTo(0);
  encoderR.setScalings(1,1,0,0);
  //analogReference(DEFAULT);
  pinMode(A0, INPUT);
  pinMode(INP_0SWITCH_PULLUP,INPUT_PULLUP);
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
  #if SER2TXONLY == 0
    pinMode(LED_BUILTIN, OUTPUT); //same as GPOI2 and also Serial1!
    digitalWrite(LED_BUILTIN, LOW);
  #endif
  #if PWMON == 1 && SER2TXONLY == 0
    pinMode(PIN_PWM_OUT, OUTPUT);
    digitalWrite(PIN_PWM_OUT, LOW);
  #endif
  #if  SER2TXONLY ==1 && PWMON == 0
    Serial1.begin(57600, SERIAL_8N1, SERIAL_TX_ONLY, PIN_SER1TX_2);  //CONNECTION TO ARDUINO (to use with UNO JOY)
  #endif

  #if DEBGCH == 5
    TimerMux.setTime(3000000);
  #else 
    TimerMux.setTime(50);
  #endif
  TimerIRonOff.setTime(100);

  #if DEBGOUT != 0 //hitire max on 115200
    Serial.begin(460800, SERIAL_8N1, SERIAL_FULL);//;(256000);//(230400);//(460800);//(115200);
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
        case 2:   procDayLightFilter(2, bIR_LED_on);
                  act_Mux_Channel = 3;
                  break;
        case 3:   procDayLightFilter(3, bIR_LED_on);
                  act_Mux_Channel = 4;
                  break;
        case 4:   procDayLightFilter(4, bIR_LED_on);
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

      dbugprint(derivativesCH[1].deriv1overLastNbr(2, 0.1)); //dx is used as a scaling factor 10
      dbugprint(";");
      dbugprint(derivativesCH[1].deriv2overLastNbr(3, 1.0) * 10);
      dbugprint(";");
      
      dbugprint(derivativesCH[2].deriv1overLastNbr(2, 0.1)); //dx is used as a scaling factor 10
      dbugprint(";");
      dbugprint(derivativesCH[2].deriv2overLastNbr(3, 1.0) * 10);
      dbugprint(";");

      encoderL_Result = encoderL.calc((int)filtValue[1], (int)filtValue[2]);
      encoderR_Result = encoderR.calc((int)filtValue[3], (int)filtValue[4]);
  
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

      minRPedal = -250; //TODO   -1000;
      maxRPedal = 250; //TODO   1000;
      #if PWMON == 1 && SER2TXONLY == 0
        int pwmscaled = constrain(map(encoderL_Result, minRPedal, maxRPedal, 10 , 245), 10, 245);
        digitalWrite(PIN_PWM_OUT, pwmscaled);
        Serial.print("  ");
        Serial.println(pwmscaled);
        //digitalWrite(PIN_PWM_OUT, map(encoderL_Result, minRPedal, maxRPedal, 10 , 245));
      #endif
      #if SER2TXONLY ==1 && PWMON == 0
        //TODO: int serscaled = constrain(map((encoderL_Result + encoderR_Result), minRPedal, maxRPedal, 1 , 255), 1, 255);
        int serscaled = constrain(map((encoderL_Result), minRPedal, maxRPedal, 1 , 255), 1, 255);
        Serial1.write(serscaled);
      #endif
      

      #if SCALEM == 0
        encoderL_Result = map(encoderL_Result, minRPedal, maxRPedal, 0 , -180);  //TODO:
      #elif SCALEM == 1
        encoderL_Result = map(encoderL_Result, minRPedal, maxRPedal, 128 , 0);  //TODO:
      #elif SCALEM == 2
      #endif
      //outputRudder = filtValue[1] + filtValue[2];
      //out8BitRudder = outputRudder;  //old output for UNO_Joy

      #if DEBGOUT == 0
        if(abs(hat.gyro[0] - (float)encoderL_Result > 1)){
          hat.gyro[0] = (float)encoderL_Result;

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
      Serial.print(encoderL_Result);
      #endif

      if(digitalRead(INP_0SWITCH_PULLUP) == LOW){ //inverted due to pullup
        if(cnt0swtch < 10000){
          cnt0swtch++;
          if(cnt0swtch > 200){  //min 200 cycles on(aprox. 2ms each, results in aprox. 350-500ms )
            encoderL.setTo(0);
            encoderR.setTo(0);
            cnt0swtch = 10001;
          }
        }
      }
      else{
        cnt0swtch = 0;
      }

      #if DEBGOUT != 0
        t_lastcycl = t_now;
        t_now = micros();
        //Serial.print(";");
        Serial.print(serscaled);
        Serial.print(";");
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