//analog filter.cpp

#include "analog_filter.h"
#include "Arduino.h"

#ifndef DEBGOUT
    #define DEBGOUT 0  //if this is 99 dbugprints will be active
#endif

#if DEBGOUT == 99
    #ifndef dbugprint
        #define dbugprint(x) Serial.print(x)
    #endif
    #ifndef dbugprintln
        #define dbugprintln(x) Serial.println(x)
    #endif
#else
    #ifndef dbugprint
        #define dbugprint(x)
    #endif
    #ifndef dbugprintln
        #define dbugprintln(x)
    #endif
#endif

namespace ANFLTR{

//Base class implementations/////////////////////////////////////////////////////////////////////////
template <typename T> CFilterAnalogBase<T>::CFilterAnalogBase(){ //Constructor
    m_init(1000);
}

template <typename T> CFilterAnalogBase<T>::CFilterAnalogBase(unsigned int buffersize){ //Constructor
    m_init(buffersize);
}

template <typename T> CFilterAnalogBase<T>::~CFilterAnalogBase(){
  delete m__bf;
}

template <typename T> void CFilterAnalogBase<T>::m_init(unsigned int buffersize){ //Constructor
  if (buffersize < 2)
    m__bf_length = 2;
  else if (buffersize > 10000)
    m__bf_length = 10000;
  else
    m__bf_length = buffersize;

  m__bf = new m__rawMeas[m__bf_length];

  reset();    
}

template <typename T> int CFilterAnalogBase<T>::reset(){
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

template <typename T> void CFilterAnalogBase<T>::m__add(T &rawvalue, unsigned long &tstampNow){
    m__PntrNewest->value = rawvalue;
    m__PntrNewest->tstamp = tstampNow;
    m__total += m__PntrNewest->value;
    m__nbr_meas++;
    if (++m__PntrNewest - m__bf >= m__bf_length) { m__PntrNewest = m__bf; }
} 

template <typename T> void CFilterAnalogBase<T>::m__remove(){
  m__total -= m__PntrOldest->value;
  m__PntrOldest->value = 0;
  m__PntrOldest->tstamp = 0;
  m__nbr_meas--;
  if (++m__PntrOldest - m__bf >= m__bf_length) { 
    m__PntrOldest = m__bf; 
  }
}

template <> int CFilterAnalogBase<int>::m__average(){
  if (m__nbr_meas == 0){
    return 0;
  }
  else if ((m__total * 1000) / (m__nbr_meas * 1000) % 1000 >= 500 )  //take 3 digits after period to round
    return m__total / m__nbr_meas  + 1;
  else
    return m__total / m__nbr_meas; 
}

template <typename T> T CFilterAnalogBase<T>::m__average(){
  if (m__nbr_meas == 0){
    return 0;
  }
  else
  return m__total / m__nbr_meas; 
}

template <typename T> int CFilterAnalogBase<T>::getAverage(){ //just average
  return m__average();
}

template <typename T> double CFilterAnalogBase<T>::getAverageDbl(){ //just average
  if (m__nbr_meas == 0){
    return 0;
  }
  else{
    return (double)m__total / (double)m__nbr_meas;
  }
}

template <typename T> T CFilterAnalogBase<T>::calcMinMax (bool return_max){
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

template <typename T> T CFilterAnalogBase<T>::getMin(){
  return m__min;
}

template <typename T> T CFilterAnalogBase<T>::getMax(){
  return m__max;
}

template <typename T> T CFilterAnalogBase<T>::measurementIfMinChange(T &measureToAdd, T minChange){
  m__rawMeas* ptrlastMeas = (m__PntrNewest == m__bf? m__bf + m__bf_length : m__PntrNewest - 1);
  if(abs(ptrlastMeas->value - measureToAdd) >= minChange){
    return measurement(measureToAdd);
  }
  else{
    return m__average();
  } 
}

template <typename T> double CFilterAnalogBase<T>::calcFIRfiltered(double* fIRcoeffs, int fIRnbrOfCoeffs){ //array and its length of FIR-filter coefficents (can be driven from: http://t-filter.engineerjs.com/)
  if(m__nbr_meas >= fIRnbrOfCoeffs && m__bf_length >= fIRnbrOfCoeffs){
      double* loopcoeff = fIRcoeffs; 
      typename CFilterAnalogBase<T>::m__rawMeas* loopvalue = m__PntrOldest;
      double outvalue = 0;
      for (int i = 0; i < fIRnbrOfCoeffs; i++){
        outvalue += *loopcoeff * (double)loopvalue->value;
        /*dbugprint(*loopcoeff);
        dbugprint(";");
        dbugprint((double)loopvalue->value);
        dbugprint(";"); */       
        loopcoeff++;
        if(++loopvalue - m__bf >= m__bf_length){
          loopvalue = m__bf;
        }
      }
      return outvalue;
  }
  return 0;
}

template <typename T> unsigned int CFilterAnalogBase<T>::getNbrMeas(){
  if (m__nbr_meas >= m__bf_length -2)
    return m__nbr_meas;// * -1;
  return m__nbr_meas;
}

template class CFilterAnalogBase<int>;
template class CFilterAnalogBase<float>;
template class CFilterAnalogBase<double>;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//time based class implementation (filtered over time)/////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> CFilterAnalogOverTime<T>::CFilterAnalogOverTime(){ //Constructor
    m__filtert_micros = 10000;
}

template <typename T> CFilterAnalogOverTime<T>::CFilterAnalogOverTime(unsigned long targfilttime_micros) : CFilterAnalogBase<T>(1000U){ //Constructor
    m__filtert_micros = targfilttime_micros;
}

template <typename T> CFilterAnalogOverTime<T>::CFilterAnalogOverTime(unsigned int buffersize, unsigned long targfilttime_micros) : CFilterAnalogBase<T>(buffersize){ //Constructor
    m__filtert_micros = targfilttime_micros;
}

template <typename T> T CFilterAnalogOverTime<T>::measurement(T &measureToAdd){
  
  unsigned long tstamp = micros(); //timestamp of that particular measurement

  while (this->m__nbr_meas >= this->m__bf_length -1){
      this->m__remove();
  }

  this->m__add(measureToAdd, tstamp);

  while(tstamp  - this->m__PntrOldest->tstamp > m__filtert_micros){  //remove all measures which are older than filter time
    this->m__remove();
  }

  return this->m__average();
}

template <typename T> unsigned long CFilterAnalogOverTime<T>::setgetTargfiltT_micros (unsigned long targfilttime_micros){
    if (targfilttime_micros > 0){
        m__filtert_micros = targfilttime_micros;
    }
    return m__filtert_micros;
}

template class CFilterAnalogOverTime<int>;
template class CFilterAnalogOverTime<float>;
template class CFilterAnalogOverTime<double>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//record based class implementation (filtered over a number of records (measurements)) ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> CFilterAnalogOverMeasures<T>::CFilterAnalogOverMeasures(){ //Constructor
    m__filterNbrMeasures = 10;
}

template <typename T> CFilterAnalogOverMeasures<T>::CFilterAnalogOverMeasures(unsigned int buffersize) : CFilterAnalogBase<T>(buffersize){ //Constructor
    m__filterNbrMeasures = buffersize;
}

template <typename T> CFilterAnalogOverMeasures<T>::CFilterAnalogOverMeasures(unsigned int buffersize, int targMeasNbrs) : CFilterAnalogBase<T>(buffersize){ //Constructor
    m__filterNbrMeasures = (unsigned)targMeasNbrs > buffersize ? targMeasNbrs : (unsigned)buffersize;
}

template <typename T> T CFilterAnalogOverMeasures<T>::measurement(T &measureToAdd){
  
  unsigned long tstamp = micros(); //timestamp of that particular measurement

  while (this->m__nbr_meas >= this->m__bf_length -1){
      this->m__remove();
  }

  this->m__add(measureToAdd, tstamp);

  while(this->m__nbr_meas >  m__filterNbrMeasures){  //remove all measures which are older than filter time
    this->m__remove();
  }

  return this->m__average();
}

template <typename T> unsigned int CFilterAnalogOverMeasures<T>::setgetTargetMeasures (unsigned int targMeasNbrs){
    if (targMeasNbrs > 0){
        m__filterNbrMeasures = targMeasNbrs;
    }
    return m__filterNbrMeasures;
}

template <typename T> double CFilterAnalogOverMeasures<T>::deriv1overLastNbr(short Nbr, double dx){ //first derivative over last 2 to 5 measures, formulas from https://web.media.mit.edu/~crtaylor/calculator.html
  Nbr = Nbr < 2 ? 2: (Nbr > 4 ? 4 : Nbr);  //didn't use constrain from arduino.h here to keep this class independent
  dx = dx < 0.00001 ? 0.00001 : dx;
  if(this->m__nbr_meas >= 2){
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im0 = (this->m__PntrNewest == this->m__bf? this->m__bf + this->m__bf_length -1 : this->m__PntrNewest - 1);
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im1 = (ptr_Im0 == this->m__bf? this->m__bf + this->m__bf_length -1 : ptr_Im0 - 1);
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im2 = (ptr_Im1 == this->m__bf? this->m__bf + this->m__bf_length -1 : ptr_Im1 - 1);
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im3 = (ptr_Im2 == this->m__bf? this->m__bf + this->m__bf_length -1 : ptr_Im2 - 1); 
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im4 = (ptr_Im3 == this->m__bf? this->m__bf + this->m__bf_length -1 : ptr_Im3 - 1);
    /*dbugprint(ptr_Im0->value);
    dbugprint(";");
    dbugprint(ptr_Im1->value);
    dbugprint(";");
    dbugprint(ptr_Im2->value);
    dbugprint(";");
    dbugprint(ptr_Im3->value);
    dbugprint(";");
    dbugprint(ptr_Im4->value);
    dbugprint(";");*/
    if(Nbr == 5 && this->m__nbr_meas >= Nbr){
      return (3*(double)ptr_Im4->value -16*(double)ptr_Im3->value + 36*(double)ptr_Im2->value - 48*(double)ptr_Im1->value + 25*(double)ptr_Im0->value)/(12 * dx); //formula last 5: f_x = (3*f[i-4]-16*f[i-3]+36*f[i-2]-48*f[i-1]+25*f[i+0])/(12*1.0*dx^1)
    }
    else if (Nbr == 4 && this->m__nbr_meas >= Nbr || Nbr >= 4 && this->m__nbr_meas == 4){
      return (-2*(double)ptr_Im3->value + 9*(double)ptr_Im2->value - 18*(double)ptr_Im1->value + 11*(double)ptr_Im0->value)/(6 * dx); //formula last 4: f_x = (-2*f[i-3]+9*f[i-2]-18*f[i-1]+11*f[i+0])/(6*1.0*dx^1)
    }
    else if (Nbr == 3 && this->m__nbr_meas >= Nbr || Nbr >= 4 && this->m__nbr_meas == 3){
      return ((double)ptr_Im2->value - 4*(double)ptr_Im1->value + 3*(double)ptr_Im0->value)/(2 * dx); //formula last 3: f_x = (1*f[i-2]-4*f[i-1]+3*f[i+0])/(2*1.0*dx^1)
    }
    else if (Nbr == 2 && this->m__nbr_meas >= Nbr || Nbr >= 2 && this->m__nbr_meas == 2){
      return (-1*(double)ptr_Im1->value + 1*(double)ptr_Im0->value)/(dx); //formula last 2: f_x = (-1*f[i-1]+1*f[i+0])/(1*1.0*dx^1)   
    }
  }  
  return 0;
}

template <typename T> double CFilterAnalogOverMeasures<T>::deriv2overLastNbr(short Nbr, double dx){ //second derivative over last 2 to 5 measures, formulas from https://web.media.mit.edu/~crtaylor/calculator.html
  Nbr = Nbr < 2? 2: (Nbr > 4 ? 4 : Nbr);  //didn't use constrain from arduino.h here to keep this class independent
  dx = dx < 0.00001 ? 0.00001 : dx;
  if(this->m__nbr_meas >= 3){
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im0 = (this->m__PntrNewest == this->m__bf? this->m__bf + this->m__bf_length -1 : this->m__PntrNewest - 1);
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im1 = (ptr_Im0 == this->m__bf? this->m__bf + this->m__bf_length -1 : ptr_Im0 - 1); 
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im2 = (ptr_Im1 == this->m__bf? this->m__bf + this->m__bf_length -1 : ptr_Im1 - 1);
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im3 = (ptr_Im2 == this->m__bf? this->m__bf + this->m__bf_length -1 : ptr_Im2 - 1);
    typename CFilterAnalogOverMeasures<T>::m__rawMeas* ptr_Im4 = (ptr_Im3 == this->m__bf? this->m__bf + this->m__bf_length -1 : ptr_Im3 - 1);
    if(Nbr == 5 && this->m__nbr_meas >= Nbr){
      return (11*(double)ptr_Im4->value  - 56*(double)ptr_Im3->value + 114*(double)ptr_Im2->value - 104*(double)ptr_Im1->value + 35*(double)ptr_Im0->value)/(12 * dx * dx); // formula for last 5: f_xx = (11*f[i-4]-56*f[i-3]+114*f[i-2]-104*f[i-1]+35*f[i+0])/(12*1.0*dx^2)
    }
    else if (Nbr == 4 && this->m__nbr_meas >= Nbr || Nbr >= 4 && this->m__nbr_meas == 4){
      return (-1*(double)ptr_Im3->value + 4*(double)ptr_Im2->value - 5*(double)ptr_Im1->value + 2*(double)ptr_Im0->value)/(1 * dx * dx); // formula for last 4: f_xx = (-1*f[i-3]+4*f[i-2]-5*f[i-1]+2*f[i+0])/(1*1.0*dx^2)
    }
    else if (Nbr == 3 && this->m__nbr_meas >= Nbr || Nbr >= 4 && this->m__nbr_meas == 3){
      return (1*(double)ptr_Im2->value - 2*(double)ptr_Im1->value + 1*(double)ptr_Im0->value)/(1 * dx * dx); // formula for last 3: f_xx = (1*f[i-2]-2*f[i-1]+1*f[i+0])/(1*1.0*dx^2)
    }
  }
  return 0;
}

template class CFilterAnalogOverMeasures<int>;
template class CFilterAnalogOverMeasures<float>;
template class CFilterAnalogOverMeasures<double>;
}