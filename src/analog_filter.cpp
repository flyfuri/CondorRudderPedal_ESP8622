//analog filter.cpp

#include "analog_filter.h"
#include "Arduino.h"

namespace ANFLTR{

//Base class implementations/////////////////////////////////////////////////////////////////////////

CFilterAnalogBase::CFilterAnalogBase(){ //Constructor
    m_init(1000);
}

CFilterAnalogBase::CFilterAnalogBase(unsigned int buffersize){ //Constructor
    m_init(buffersize);
}

void CFilterAnalogBase::m_init(unsigned int buffersize){ //Constructor
  if (buffersize < 2)
    m__bf_length = 2;
  else if (buffersize > 10000)
    m__bf_length = 10000;
  else
    m__bf_length = buffersize;

  m__bf = new m__rawMeas[m__bf_length];

  reset();    
}

int CFilterAnalogBase::reset(){
  for(unsigned int i = 0; i < m__bf_length; i++){
      m__bf[i].value = 0;    
      m__bf[i].tstamp = 0;
  }

  m__PntrNewest = m__bf; 
  m__PntrOldest = m__bf;
  
  m__total = 0;
  m__nbr_meas = 0;
  m__fcycdone = false;

  m__max = 0;
  m__min = 0;

  return 0;
}

void CFilterAnalogBase::m__add(int &rawvalue, unsigned long &tstampNow){
    m__PntrNewest->value = rawvalue;
    m__PntrNewest->tstamp = tstampNow;
    m__total += m__PntrNewest->value;
    m__nbr_meas++;
    if (++m__PntrNewest - m__bf >= m__bf_length) { m__PntrNewest = m__bf; }
} 

void CFilterAnalogBase::m__remove(){
  m__total -= m__PntrOldest->value;
  m__PntrOldest->value = 0;
  m__PntrOldest->tstamp = 0;
  m__nbr_meas--;
  if (++m__PntrOldest - m__bf >= m__bf_length) { 
    m__PntrOldest = m__bf; 
  }
}

int CFilterAnalogBase::calcMinMax (bool return_max){
    m__max = 0;
    m__min = 0;
    m__rawMeas* tmpPtr = m__PntrNewest - 1;
    if(tmpPtr - m__bf < 0)
      tmpPtr = m__bf + m__bf_length - 1;

    m__max = tmpPtr->value;
    m__min = tmpPtr->value;
    for(unsigned int i = 0; i < m__nbr_meas; i++){
      m__max = tmpPtr->value > m__max? tmpPtr->value : m__max;
      m__min = tmpPtr->value < m__min? tmpPtr->value : m__min;
      if(--tmpPtr - m__bf < 0)
        tmpPtr = m__bf + m__bf_length - 1;
    }

    if(return_max)
      return m__max;
    else
      return m__min;
}

int CFilterAnalogBase::getMin(){
  return m__min;
}

int CFilterAnalogBase::getMax(){
  return m__max;
}

unsigned int CFilterAnalogBase::getNbrMeas(){
  if (m__nbr_meas >= m__bf_length -2)
    return m__nbr_meas;// * -1;
  return m__nbr_meas;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//time based class implementation (filtered over time)/////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFilterAnalogOverTime::CFilterAnalogOverTime(){ //Constructor
    m__filtert_micros = 10000;
}

CFilterAnalogOverTime::CFilterAnalogOverTime(unsigned long targfilttime_micros) : CFilterAnalogBase(1000U){ //Constructor
    m__filtert_micros = targfilttime_micros;
}

CFilterAnalogOverTime::CFilterAnalogOverTime(unsigned int buffersize, unsigned long targfilttime_micros) : CFilterAnalogBase(buffersize){ //Constructor
    m__filtert_micros = targfilttime_micros;
}


int CFilterAnalogOverTime::measurement(int &measure){
  
  unsigned long tstamp = micros(); //timestamp of that particular measurement

  while (m__nbr_meas >= m__bf_length -1){
      m__remove();
  }

  m__add(measure, tstamp);

  while(tstamp  - m__PntrOldest->tstamp > m__filtert_micros){  //remove all measures which are older than filter time
    m__remove();
  }

  if ((m__total * 1000) / (m__nbr_meas*1000) % 1000 >= 500 )  //take 3 digits after period to round
    return m__total / m__nbr_meas  + 1;
  else
    return m__total / m__nbr_meas;
}

unsigned long CFilterAnalogOverTime::setgetTargfiltT_micros (unsigned long targfilttime_micros){
    if (targfilttime_micros > 0){
        m__filtert_micros = targfilttime_micros;
    }
    return m__filtert_micros;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//record based class implementation (filtered over a number of records (measurements)) ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFilterAnalogOverMeasures::CFilterAnalogOverMeasures(){ //Constructor
    m__filterNbrMeasures = 10;
}

CFilterAnalogOverMeasures::CFilterAnalogOverMeasures(unsigned int buffersize) : CFilterAnalogBase(buffersize){ //Constructor
    m__filterNbrMeasures = buffersize;
}

CFilterAnalogOverMeasures::CFilterAnalogOverMeasures(unsigned int buffersize, int targMeasNbrs) : CFilterAnalogBase(buffersize){ //Constructor
    m__filterNbrMeasures = (unsigned)targMeasNbrs > buffersize ? targMeasNbrs : (unsigned)buffersize;
}

int CFilterAnalogOverMeasures::measurement(int &measure){
  
  unsigned long tstamp = micros(); //timestamp of that particular measurement

  while (m__nbr_meas >= m__bf_length -1){
      m__remove();
  }

  m__add(measure, tstamp);

   while(m__nbr_meas >  m__filterNbrMeasures){  //remove all measures which are older than filter time
    m__remove();
  }

  if ((m__total * 1000) / (m__nbr_meas*1000) % 1000 >= 500 )  //take 3 digits after period to round
    return m__total / m__nbr_meas  + 1;
  else
    return m__total / m__nbr_meas;
}

int CFilterAnalogOverMeasures::setgetTargetMeasures (unsigned int targMeasNbrs){
    if (targMeasNbrs > 0){
        m__filterNbrMeasures = targMeasNbrs;
    }
    return m__filterNbrMeasures;
}
}