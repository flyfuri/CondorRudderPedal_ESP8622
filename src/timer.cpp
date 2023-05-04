//Timer.cpp

#include "timer.h"
#include "Arduino.h"

CTimer::CTimer(){
  ms_at_start=millis();
  timer_started=false;  
}

bool CTimer::evaluate(bool condition){
  if (condition){ 
    if (!timer_started){
      ms_at_start=millis();        
      timer_started=true;   
    }           
    else {
      if (ms_delay == 0){
        ms_delay=1;
      }        
      if (millis() - ms_at_start > ms_delay ){
        return true;            
      } 
    }       
  }
  else {
            ms_at_start=millis();
            timer_started=false;
  }        
  return false;  
}

void CTimer::setTime(int time){
  if (time<=0) {
    ms_delay=1;
  }
  else if (time>2000000) {
    ms_delay=2000000;
  }
  else{
    ms_delay=(unsigned long)time;
  }
}

bool CTimer::evalAndSetTime(bool condition, int time){
  setTime(time);
  bool output = evaluate(condition); 
  return output;
}

unsigned long CTimer::getDelay(){
  return ms_delay;
}

unsigned long CTimer::getElapsedTime(){
  if (timer_started){    
    return millis() - ms_at_start;
  }
  else{
    return 0;
  }
}