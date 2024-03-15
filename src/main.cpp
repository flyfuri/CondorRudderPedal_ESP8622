#include <Arduino.h>
#include <timer.h>
#include <analog_filter.h>
#include <SinusoidIncCounter.h>
#include <AutoscalePair.h> 
#include <IO_wiring.h>
#include <ESP8266WiFi.h>

#define DEBGOUT 1 //usually 1  (0: don't print anything, 1: print debug function stuff, >1: special, often temporary)

#if DEBGOUT > 0  
  #define dbugprint(x) Serial.print(x)
  #define dbugprintln(x) Serial.println(x)
#else
  #define dbugprint(x) 
  #define dbugprintln(x) 
#endif

int act_Mux_Channel = 0; //which MUX-Channel to activate (0 = none, 1,2,3,4)
int i_clk; //counters: loopclock,measures CH1 measures CH2
int daylightDist[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc (measure daylight desturbance by measure without LED activated)
float analogRaw[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc  
float filteredValues[5] = {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc
float scaledValues[5]= {0,0,0,0,0}; //1=Ch1, 2=Ch2, etc
int encoderL_Result; //encoder LeftPedal Channel 1 and 2 
int encoderR_Result; //encoder RightPedal Channel 3 and 4 
unsigned long t_lastcycl, t_now, t_cycletime; //measure cycle time
int outputRudder;
int cnt0swtch = 0; //counter to "filter" zero switch

//log settings
#define LOG_OPTIONS_SIZE 140
String log_separator = ",";
String log_command = "";
bool log_options[LOG_OPTIONS_SIZE];
float log_tempmem[LOG_OPTIONS_SIZE];
String logdescr[LOG_OPTIONS_SIZE];


ANFLTR::CFilterAnalogOverMeasures<float> filterCH[5] = {{1, 1}, {5, 5}, {5, 5}, {5, 5}, {5, 5} }; //0 = not used, 1=Ch1, 2=Ch2, ...
CAutoScalePair<float> AutoscalerLeft = {0, 100}; //{targMin, targMax}
CAutoScalePair<float> AutoscalerRight = {0, 100}; //{targMin, targMax}
TIMER::CTimerMillis TimerInitLeft, TimerInitRigth, TimerBlink;
TIMER::CTimerMicros TimerMux, TimerIRonOff;
CSinIncCntr encoderL, encoderR; //encoder Left pedal and Right pedal
int minPedal = 0, maxPedal = 0;
int posBackPedal_L = 0, posBackPedal_R = 0;
int travelPedal_L = 0, travelPedal_R = 0;
bool bMuxDelay, bIRonoffDelay; //wait for mux delay
bool bIR_LED_on; //IR LED is on

const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0

int serscaled;  //value to HID

ANFLTR::CFilterAnalogOverTime<int> Final_filter(15, 10000); //unsigned int(15), unsigned long(10000));
void debuglogs();

//procedure read channel with daylight filter
void procDayLightFilter(short chNr, bool bLEDisON){
  if (chNr > 0 && chNr < 5){
    if (bLEDisON){
      analogRaw[chNr] = analogRead(analogInPin) - daylightDist[chNr]; 
      filteredValues[chNr] = filterCH[chNr].measurement(analogRaw[chNr]); 
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
  minPedal=-250;
  maxPedal=250;
  posBackPedal_L = 0;
  posBackPedal_R = 0;
  travelPedal_L = 0;
  travelPedal_R = 0;
  i_clk=0;
  encoderL_Result=0;
  bMuxDelay = false;
  bIR_LED_on = false;
  encoderL.setTo(0);
  
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

  encoderL.setSumMidLine(80);
  encoderR.setSumMidLine(80);

  Serial.begin(460800, SERIAL_8N1, SERIAL_FULL);//;(256000);//(230400);//(460800);//(115200); debug info sent here
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

      //calculate pedal position and send it to arduino HID--------------------------------------------------------------------

      AutoscalerLeft.update(filteredValues + 1, filteredValues + 2, scaledValues + 1, scaledValues + 2);
      AutoscalerRight.update(filteredValues + 3, filteredValues + 4, scaledValues + 3, scaledValues + 4);

      encoderL_Result = encoderL.calc(round(scaledValues[1]), round(scaledValues[2]));
      encoderR_Result = encoderR.calc(round(scaledValues[3]), round(scaledValues[4]));
      
      serscaled = constrain(map((encoderR_Result - encoderL_Result), minPedal, maxPedal, 1 , 255), 1, 255);
      outputRudder = constrain(Final_filter.measurement(serscaled), 1,255);

      Serial1.write(outputRudder);


      //debug info -----------------------------------------------------------------------------------------------------------

      while(Serial.available() > 0){
        dbugprintln(Serial.available());
        log_command=Serial.readString();
        log_command.trim();
      }
      
      debuglogs();

     
      //set pedals to 0 with button -----------------------------------------------------------------------------------------------------------
      //to initialize the pedals the switch must be pressed with the pedals once in full right and once in full left position
      if(digitalRead(INP_0SWITCH_PULLUP) == LOW){ //inverted due to pullup
        if(cnt0swtch < 10000){
          cnt0swtch++;
          if(cnt0swtch > 200){  //min 200 cycles on(aprox. 2ms each, results in aprox. 350-500ms )
            if(encoderL_Result - encoderR_Result > 100){  //Left is greater (to be set to 0)
              if(posBackPedal_L != 0){
                travelPedal_L = encoderL_Result - posBackPedal_L;  //calc difference when point furtest back was memorized before
              }
              encoderL.setTo(0);
              if(posBackPedal_R == 0){
                posBackPedal_R = encoderR_Result; //memorize point furtest back
                travelPedal_R = posBackPedal_R;
              }
            }
            else if(encoderR_Result - encoderL_Result > 100){ //Right is greater (to be set to 0)
              if(posBackPedal_R != 0){
                travelPedal_R = encoderR_Result - posBackPedal_R;  //calc difference when point furtest back was memorized before
              }
              encoderR.setTo(0);
              if(posBackPedal_L == 0){
                posBackPedal_L = encoderL_Result; //memorize point furtest back
                travelPedal_L = posBackPedal_L;
              }
            }
            if (travelPedal_L != 0 && travelPedal_R != 0){
                int tmphalf = (abs(travelPedal_L) < abs(travelPedal_R) ? abs(travelPedal_L) : abs(travelPedal_R)) * 0.98; //factor 0.98 ensures to reach max short before mechanical stop
                minPedal = tmphalf * -1;
                maxPedal = tmphalf;
                posBackPedal_L = 0;
                posBackPedal_R = 0;
                travelPedal_L = 0;
                travelPedal_R = 0;
              }
            cnt0swtch = 10001;
          }
        }
      }
      else{
        cnt0swtch = 0;
      }
      //------------------------------------------------------------------------------------------------------------------------------------

      //print cycletime if chosen and wait to make cycle time to a fix value

      t_lastcycl = t_now;
      t_now = micros();
      t_cycletime = t_now - t_lastcycl;
      if (log_options[0]){dbugprintln(t_cycletime);}
      else{dbugprintln("");}
      while(t_cycletime < 2000){
        t_now = micros();
        t_cycletime = t_now - t_lastcycl;
      }      
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
}



//------------------------------------------------------------------------------------------------------------------------------------
// send debug information to analyze signals 
// commands can be sent to the ESP to activate/diactivate certain debug informations to be sent
// unfortunately the monitor filter sentOnEnter seems not to work in VSCode use another terminal like CoolTerminal 
// commands:
//   t    activate/disactivate cycletime (should be lower than 2000 micros)
//   r    deactivate all debug infos
//   ?    list acivated infos (not implemented completly)
//   1..  any positive number activates the corresponding information to be sent (options check code below)
//   -1.. any negative number disactivates the corresponding information 

void debuglogs(){
  int intcmd = log_command.toInt();

  if(intcmd > 0 && intcmd < LOG_OPTIONS_SIZE){
    log_options[intcmd] = true;
  }
  else if(intcmd < 0 && abs(intcmd) < LOG_OPTIONS_SIZE){
    log_options[abs(intcmd)] = false;
  }
  else if(log_command == "?"){
    for(int i = 1; i < LOG_OPTIONS_SIZE; i++){
      if(log_options[i]){
        dbugprint(i);
        dbugprint(log_separator);
      }
    }
    if(log_options[0]){
      dbugprint(" ");
      dbugprintln(log_separator);
    }
  }
  else if(log_command == "r"){
    for(int i = 1; i < LOG_OPTIONS_SIZE; i++){
      log_options[i]=false;
    }
  }
  else if(log_command == "t"){
    log_options[0] = !log_options[0];
  }
  else if (log_command == ";" || log_command == ","){
    log_separator = log_command;
  }else if (isSpace(log_command[0])){
    log_separator = "  ";
  }
  
  log_command = "";

  //send choosen options

  String lastReadTmp = "";
  for(int i = 0; i < LOG_OPTIONS_SIZE; i++){
      if(log_options[i]){
        switch(i){
          case 1: 
            dbugprint((int)analogRaw[1]);
            break;
          case 2: 
            dbugprint((int)analogRaw[2]);
            break;
          case 3: 
            dbugprint((int)analogRaw[3]);
            break;
          case 4: 
            dbugprint((int)analogRaw[4]);
            break;
          case 5: 
            dbugprint((int)scaledValues[1]);
            break;
          case 6: 
            dbugprint((int)scaledValues[2]);
            break;
          case 7: 
            dbugprint((int)scaledValues[3]);
            break;
          case 8: 
            dbugprint((int)scaledValues[4]);
            break;
          case 11:
          case 12:
          case 13:
          case 14:
          case 15:
          case 16:
          case 17:
          case 18:
          case 19:
          case 20:
          case 21:
          case 22: 
          case 23:
          case 24:
          case 25:
          case 26:
            if(lastReadTmp != "PedalLeftChA"){
              AutoscalerLeft.debugChA(log_tempmem+11,16);
              lastReadTmp = "PedalLeftChA";
            }
            dbugprint(log_tempmem[i]);
            break;
          case 30:
          case 32:
          case 33:
          case 34:
          case 35:
          case 36:
          case 37:
          case 38:
          case 39:
          case 40:
          case 41:
          case 42:  
          case 43:
          case 44:
          case 45:
            if(lastReadTmp != "PedalLeftChB"){
              AutoscalerLeft.debugChB(log_tempmem+30,16);
              lastReadTmp = "PedalLeftChB";
            }
            dbugprint(log_tempmem[i]);
            break;
          case 50:
          case 51:
          case 52:
          case 53:
          case 54:
          case 55:
          case 56:
          case 57:
          case 58:
          case 59:
          case 60:
          case 61:
          case 62:  
          case 63:
          case 64:
          case 65:
            if(lastReadTmp != "PedalRightChA"){
              AutoscalerRight.debugChA(log_tempmem+50,16);
              lastReadTmp = "PedalRightChA";
            }
            dbugprint(log_tempmem[i]);
            break;
          case 70:
          case 71:
          case 72:
          case 73:
          case 74:
          case 75:
          case 76:
          case 77:
          case 78:
          case 79:
          case 80:
          case 81:
          case 82:  
          case 83:
          case 84:
          case 85:
          case 86:
            if(lastReadTmp != "PedalRightChB"){
              AutoscalerRight.debugChB(log_tempmem+70,16);
              lastReadTmp = "PedalRightChB";
            }
            dbugprint(log_tempmem[i]);
            break;     
          case 90:
            dbugprint(serscaled);
            break;
          case 91:
            dbugprint(encoderL_Result);
            break;
          case 92:
            dbugprint(encoderR_Result);
            break;
          case 93:
            dbugprint(posBackPedal_L);
            break;
          case 94:
            dbugprint(posBackPedal_R);
            break;
          case 95:
            dbugprint(travelPedal_L);
            break;
          case 96:
            dbugprint(travelPedal_R);
            break;
          case 97:
            dbugprint(minPedal);
            break;
          case 98:
            dbugprint(maxPedal);
            break;
          case 101:
          case 102:
          case 103:
          case 104:
          case 105:
          case 106:
          case 107:
          case 108:
          case 109:
          case 110:
          case 111:
          case 112: 
          case 113: 
          case 114: 
          case 115: 
            if(lastReadTmp != "PedalLeftCounter"){
              encoderL.debug(log_tempmem+101,15);
              lastReadTmp = "PedalLeftCounter";
            }
            dbugprint(log_tempmem[i]);
            break;
          case 121:
          case 122:
          case 123:
          case 124:
          case 125:
          case 126:
          case 127:
          case 128:
          case 129:
          case 130:
          case 131:
          case 132: 
          case 133: 
          case 134: 
          case 135: 
            if(lastReadTmp != "PedalRightCounter"){
              encoderR.debug(log_tempmem+121,15);
              lastReadTmp = "PedalRightCounter";
            }
            dbugprint(log_tempmem[i]);
            break;
        }
        dbugprint(log_separator);
      }
    }
    
}

