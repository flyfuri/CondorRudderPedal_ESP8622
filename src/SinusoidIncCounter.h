class CSinIncCntr{
    private: 
        int m__sum;  //summary of both channels
        int m__sub;   //CH1 - CH2

        int m__sumMidLine = 90; //approx middle line of summary

        short m__actSubStatus = 0;  //wich half of the difference curve where now (-1=negative, 0 undefined(at beginning), 1=positive)
        
        int m__actPos;
        
    public:
        CSinIncCntr();  
        int calc(int actCh1, int actCh2);
        int read();
        int setTo(int value);
};