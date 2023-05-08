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

    
    void m_init(unsigned int buffersize);
    void m__add(int &rawvalue, unsigned long &tstampNow); //add one measurement to total and buffer
    void m__remove(); //remove a expired measurements from total and buffer
  
  public:
    CFilterAnalogBase(); 
    CFilterAnalogBase(unsigned int buffersize);     
    virtual int measurement(int &measure) = 0; 
    unsigned int getNbrMeas();
};


class CFilterAnalogOverTime : public CFilterAnalogBase {  //class filtering over given time
  private:
    unsigned long m__micros_start; //timestamp start
    unsigned long m__filtert_micros; //filtertime
  
  public:
    CFilterAnalogOverTime();  
    CFilterAnalogOverTime(unsigned int buffersize); 
    CFilterAnalogOverTime(unsigned long targfilttime_micros); 
    CFilterAnalogOverTime(unsigned int buffersize, unsigned long targfilttime_micros);     
    int measurement(int &measure);
    unsigned long setgetTargfiltT_micros (unsigned long targfilttime_micros);
};

class CFilterAnalogOverMeasures : public CFilterAnalogBase {  //class filtering over given time
  private:
    unsigned int m__filterNbrMeasues; //number of measures to include in the results
  
  public:
    CFilterAnalogOverMeasures();  
    CFilterAnalogOverMeasures(unsigned int buffersize); 
    CFilterAnalogOverMeasures(int targMeasNbrs); 
    CFilterAnalogOverMeasures(unsigned int buffersize, int targMeasNbrs);     
    int measurement(int &measure);
    int setgetTargetMeasures (unsigned int targMeasNbrs);
};
}