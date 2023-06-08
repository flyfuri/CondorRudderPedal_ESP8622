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
    int measurementIfMinChange(int &measureToAdd, int minChange); //add a measurement only when differs minimal to last measurement and return average
    int getAverage(); //just average (rounded integer)
    double getAverageDbl(); //get average in double precicion
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
    int measurement(int &measureToAdd); //add a measurement and return average over time
    unsigned long setgetTargfiltT_micros (unsigned long targfilttime_micros); //set 
};

class CFilterAnalogOverMeasures : public CFilterAnalogBase {  //class filtering over given time
  private:
    unsigned int m__filterNbrMeasures; //number of measures to include in the results
  
  public:
    CFilterAnalogOverMeasures();  
    CFilterAnalogOverMeasures(unsigned int buffersize); //targMeasNbrs will be set to same value
    CFilterAnalogOverMeasures(unsigned int buffersize, int targMeasNbrs);     
    int measurement(int &measureToAdd);  //add a measurement and return average over target number of measurements
    int setgetTargetMeasures (unsigned int targMeasNbrs); //set over how many measures the average shall be taken (if 0, just de actual setting is returned)
    double deriv1overLast4(double dy=1); //first derivative over last 5 measures f_x = (-2*f[i-3]+9*f[i-2]-18*f[i-1]+11*f[i+0])/(6*1.0*h**1) from https://web.media.mit.edu/~crtaylor/calculator.html
    double deriv2overLast4(double dy=1); //second derivaive over last 5 measures f_xx = (-1*f[i-3]+4*f[i-2]-5*f[i-1]+2*f[i+0])/(1*1.0*h**2) from https://web.media.mit.edu/~crtaylor/calculator.html
};
}