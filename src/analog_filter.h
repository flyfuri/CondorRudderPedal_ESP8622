namespace ANFLTR{
  #define BUFFSIZE 1000
}
class CFilterAnalog {
  private:
      struct m__rawMeas{
      int value;
      signed long tstamp;
    } m__fb[BUFFSIZE];
    m__rawMeas* m__PntrNewest = m__fb; //rolling pointer newest valid value in buffer
    m__rawMeas* m__PntrOldest = m__fb; //rolling pointer oldest valid value in buffer
    long m__total; //total to calc average
    unsigned int m__nbr_meas; //number of measurements added in total to avarage
    bool m__fcycdone; //buffer filled up cycle done

    unsigned long m__micros_start;
    unsigned long m__filtert_micros;
  
    void m_init();
    void m__add(int &rawvalue, unsigned long &tstampNow); //add one measurement to total and buffer
    void m__remove(); //remove a expired measurements from total and buffer
  
  public:
    CFilterAnalog(); 
    CFilterAnalog(unsigned long targfilttime_micros);      
    int measurement(int &measure);
    unsigned int getNbrMeas();
    unsigned long setgetTargfiltT_micros (unsigned long targfilttime_micros);
};

