#include <Arduino.h>
#include <timer.h>
#include <analog_filter.h>
#include <SinusoidIncCounter.h>
#include <AutoscalePair.h> 
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
//double filtValue[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc
int encoderL_Result; //encoder LeftPedal Channel 1 and 2 
int encoderR_Result; //encoder RightPedal Channel 3 and 4 
unsigned long t_lastcycl, t_now, t_cycletime; //measure cycle time
int outputRudder;
uint8_t out8BitRudder;
int cnt0swtch = 0; //counter to "filter" zero switch


ANFLTR::CFilterAnalogOverMeasures<float> filterCH[5] = {{1, 1}, {5, 5}, {5, 5}, {5, 5}, {5, 5} }; //0 = not used, 1=Ch1, 2=Ch2, ...
CAutoScalePair<float> AutoscalerLeft = {0, 100}; //{targMin, targMax}
CAutoScalePair<float> AutoscalerRight = {0, 100}; //{targMin, targMax}
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


//procedure read channel with daylight filter
void procDayLightFilter(short chNr, bool bLEDisON){
  if (chNr > 0 && chNr < 5){
    if (bLEDisON){
      analogRaw[chNr] = analogRead(analogInPin) - daylightDist[chNr]; 
      filterCH[chNr].measurement(analogRaw[chNr]); 
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
      analogRaw[chNr] = analogRead(analogInPin); 
      //filtValue[chNr] = filterCH[chNr].measurement(analogRaw[chNr]);
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
  
  Serial1.begin(57600, SERIAL_8N1, SERIAL_TX_ONLY, PIN_SER1TX_2);  //CONNECTION TO ARDUINO (to use with UNO JOY)

  #if DEBGCH == 5
    TimerMux.setTime(3000000);
  #else 
    TimerMux.setTime(40);
  #endif
  TimerIRonOff.setTime(70);

  Serial.begin(460800, SERIAL_8N1, SERIAL_FULL);//;(256000);//(230400);//(460800);//(115200);
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

      AutoscalerLeft.update(filteredValues + 1, filteredValues + 2, scaledValues + 1, scaledValues + 2);
      AutoscalerRight.update(filteredValues + 3, filteredValues + 4, scaledValues + 3, scaledValues + 4);

      encoderL_Result = encoderL.calc(round(scaledValues[1]), round(scaledValues[2]));
      encoderR_Result = encoderR.calc(round(scaledValues[3]), round(scaledValues[4]));
      
      int serscaled = constrain(map((encoderL_Result - encoderR_Result), minRPedal, maxRPedal, 1 , 255), 1, 255);
      Serial1.write(serscaled);
     
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
        do{
          t_now = micros();
          t_cycletime = t_now - t_lastcycl;
        }while(t_cycletime < 2000);
        //Serial.print(";");
        Serial.print(serscaled);
        Serial.print(";");
        Serial.println(t_cycletime);
      #endif       
    }        
  }
  
  //MuxOuts 
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