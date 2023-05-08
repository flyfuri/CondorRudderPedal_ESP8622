class CSinIncCntr{
    private: 
        int m_lim_up = 140, m_lim_low = 110;
        int m_lim_center = (m_lim_up + m_lim_low)/2;  
        int m_cntDirect = 1; // count direction -1 = negative, 0 = undefinded, 1 = positive
        bool m_ch1higher_now = false, m_ch1higher_lastcnt = false; //1 = channel 1 is higher now and at last count, this must alternate otherwise count direction changes

        struct m_CH{    
            int prev; //previous measurement
            int recent; //most recent measurement
            float slope;  //slope caldulated of the last two measures       
        } m_measures[3];//channel 0=sum of both, 1= channel1, 2 = channel2


        enum m_sumHysteresisField  // y of the curve is devided in 4 sections to create hysteresis 
        {
            HSF_ANY, HSF_LOW, HSF_MIDL, HSF_MIDH, HSF_UP  //any is only when next field is not determened yet
        };

        m_sumHysteresisField m_act_Hisfld; //sum curve is actually in this hystfield
        m_sumHysteresisField m_nxtHistfld1, m_nxtHistfld2 ; //next hystfield(s) where counting will be done

        int m_actPos;

        void m_defineNextHystfields(); //set next hysteresisfields
        void m_count(); //determen count direction and count
        
    public:
        CSinIncCntr();  
        void addmeas (int chNr, int value); 
        int calc();
        int read();
        int setTo(int value);
};