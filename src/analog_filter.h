#pragma once
namespace ANFLTR{
class CFilterAnalogBase {  //base class
  protected:
    struct m__rawMeas{
      int value;
      signed long tstamp;
    }; 
    m__rawMeas* m__bf ; //buffer
    m__rawMeas* m__PntrNewest = m__bf; //rolling pointer newest valid value in buffer
    m__rawMeas* m__PntrOldest = m__bf; //rolling pointer oldest valid value in buffer
    unsigned int m__bf_length = 1000;  //buffer length (records)
    long m__total; //total to calc average
    unsigned int m__nbr_meas; //number of measurements added in total to avarage
    bool m__fcycdone; //buffer filled up cycle done
    int m__min, m__max; //highest and lowest value in the buffer 

    
    void m_init(unsigned int buffersize);
    void m__add(int &rawvalue, unsigned long &tstampNow); //add one measurement to total and buffer
    void m__remove(); //remove a expired measurements from total and buffer
    int m__average(); //calculate average and round
  
  public:
    CFilterAnalogBase(); 
    CFilterAnalogBase(unsigned int buffersize); 
    virtual ~CFilterAnalogBase();
    int reset(); //reset fiter to 0    
    virtual int measurement(int &measureToAdd) = 0; //adds a measurement to buffer and returns average
    int getAverage(); //just average
    unsigned int getNbrMeas();
    int calcMinMax (bool return_max = false); //calculate maximum and minimum and return the choosen (default: false = minimum) 
    int getMin(); //minima calculated (must be called after calcMinMax, otherwise potentially outdated)
    int getMax(); //get maxima calculated (must be called after calcMinMax, otherwise potentially outdated)
};


class CFilterAnalogOverTime : public CFilterAnalogBase {  //class filtering over given time
  private:
    unsigned long m__micros_start; //timestamp start
    unsigned long m__filtert_micros; //filtertime
  
  public:
    CFilterAnalogOverTime();  
    CFilterAnalogOverTime(unsigned long targfilttime_micros); //buffer size set to 1000
    CFilterAnalogOverTime(unsigned int buffersize, unsigned long targfilttime_micros);     
    int measurement(int &measureToAdd);
    unsigned long setgetTargfiltT_micros (unsigned long targfilttime_micros);
};

class CFilterAnalogOverMeasures : public CFilterAnalogBase {  //class filtering over given time
  private:
    unsigned int m__filterNbrMeasures; //number of measures to include in the results
  
  public:
    CFilterAnalogOverMeasures();  
    CFilterAnalogOverMeasures(unsigned int buffersize); //targMeasNbrs will be set to same value
    CFilterAnalogOverMeasures(unsigned int buffersize, int targMeasNbrs);     
    int measurement(int &measureToAdd);
    int setgetTargetMeasures (unsigned int targMeasNbrs);
};
}