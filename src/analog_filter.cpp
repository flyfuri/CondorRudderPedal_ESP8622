//analog filter.cpp

#include "analog_filter.h"
#include "Arduino.h"

CFilterAnalog::CFilterAnalog(unsigned long targfilttime_micros){ //Constructor
    for(int i = 0; i < BUFFSIZE; i++){
        m__fb[i].value = 0;    
        m__fb[i].tstamp = 0;
    }

    m__PntrNewest = m__fb; 
    m__PntrOldest = m__fb;
    
    m__total = 0;
    m__nbr_meas = 0;
    m__fcycdone = false;

    m__filtert_micros = targfilttime_micros;
  }

void CFilterAnalog::m__add(int &rawvalue, unsigned long &tstampNow){
    m__PntrNewest->value = rawvalue;
    m__PntrNewest->tstamp = tstampNow;
    m__total += m__PntrNewest->value;
    m__nbr_meas++;
    if (++m__PntrNewest - m__fb >= BUFFSIZE) { m__PntrNewest = m__fb; }
} 

void CFilterAnalog::m__remove(){
  m__total -= m__PntrOldest->value;
  m__PntrOldest->value = 0;
  m__PntrOldest->tstamp = 0;
  m__nbr_meas--;
  if (++m__PntrOldest - m__fb >= BUFFSIZE) { 
    m__PntrOldest = m__fb; 
  }
}

int CFilterAnalog::measurement(int &measure){
  
  unsigned long tstamp = micros(); //timestamp of that particular measurement

  while (m__nbr_meas >= BUFFSIZE -1){
      m__remove();
  }

  m__add(measure, tstamp);

  while(tstamp  - m__PntrOldest->tstamp > m__filtert_micros){
    m__remove();
  }

  if ((m__total * 1000) / (m__nbr_meas*1000) % 1000 >= 500 )  //take 3 digits after period to round
    return m__total / m__nbr_meas  + 1;
  else
    return m__total / m__nbr_meas;
}

unsigned long CFilterAnalog::setgetTargfiltT_micros (unsigned long targfilttime_micros){
    if (targfilttime_micros > 0){
        m__filtert_micros = targfilttime_micros;
    }
    return m__filtert_micros;
}

unsigned int CFilterAnalog::getNbrMeas(){
  if (m__nbr_meas >= BUFFSIZE -2)
    return m__nbr_meas;// * -1;
  return m__nbr_meas;
}

