namespace TIMER{

class CTimer {
  protected:
    unsigned long ms_at_start;
    unsigned long ms_delay;
    bool timer_started;
    virtual unsigned long m__getTimestamp() = 0;
    
  public:
    CTimer();      
    bool evaluate(bool condition);
    void setTime(int time);
    bool evalAndSetTime(bool condition, int time);
    unsigned long getDelay();
    unsigned long getElapsedTime();
};

class CTimerMillis : public CTimer{
  private:
    unsigned long m__getTimestamp();

  public:
  CTimerMillis();
};

class CTimerMicros : public CTimer{
  private:
    unsigned long m__getTimestamp();

  public:
  CTimerMicros();
};

}