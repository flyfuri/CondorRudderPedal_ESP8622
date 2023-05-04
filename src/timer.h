class CTimer {
  private:
    unsigned long ms_at_start;
    unsigned long ms_delay;
    bool timer_started;
    
  public:
    CTimer();      
    bool evaluate(bool condition);
    void setTime(int time);
    bool evalAndSetTime(bool condition, int time);
    unsigned long getDelay();
    unsigned long getElapsedTime();
};