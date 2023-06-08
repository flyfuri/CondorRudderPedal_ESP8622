
#include <analog_filter.h>

#define NBR_TEETH_ON_RACK 10 //number of theeth on rail
#define NBR_TO_AVR_IND_TOOTH 5 //number to average individual tooth 
#define INTPOLRES 100 //resulution of interpolation between halfteeth
#define INIT_MIN_DIST_SUM_MINMAX 80 //initial minimal dist
#define SUM_MIDLINE_ABOVE_MIN 23 //initial minimal dist


class CSinIncCntr{
    private: 
        //used for basic counting the theeth (counts twice per tooth on each crossing point of the 2 channel curves):
        int m__sum;  //summary of both channels
        int m__sub;   //CH1 - CH2
        int m__sumOnLastCrossing = -9999; //point on sum curve when channel curves crosse last time
        int m__sumLowestMax;  //defines middle line of summary 
        int m__sumHighestMin; //defines middle line of summary 
        int m__sumMidLine = 0; //approx middle line of summary
        short m__actStatusSUB = 0;  //wich half of the difference curve we're now (-1=negative, 0 undefined(at beginning), 1=positive)
        short m__actStatusSUM = 0;  //wich half of the summary curve we're now (-1=MIN, 0 undefined(at beginning), 1=MAX)
        int m__sumAtPowerON = -9999; //take 
        int m__offset = -9999; //offset calculated after very first travel beetween to opposite crossings of the channel curve
        
        // used to find the approx. middle line of summary curve
        //ANFLTR::CFilterAnalogOverMeasures SumCurveLastMaxs{10,10};
        //ANFLTR::CFilterAnalogOverMeasures SumCurveLastMins{10,10};
        //bool m__initialSumMidFound; //initial sum middle line has been found

        //used for interpolation beetween flank counts to increse resolution        
        struct TeethRack {
            int halft_min[NBR_TO_AVR_IND_TOOTH]; //point on summary curve above crossing of both channels is taken as max of that theeth
            int halft_max[NBR_TO_AVR_IND_TOOTH]; //point on summary curve underneath crossing of both channels is taken as min of that theeth
            int halftMin_index; // 
            int halftMax_index; // 
            int minAv;
            int maxAv;
        }teethrack[NBR_TEETH_ON_RACK  * 2]; //learned behaviour of teethrack. Array double as long and start in middle of array since we don't know where we start at init

        int m__actIndexTeethrack; 
        int m__intpolMax;  //actual Max used for interpolation
        int m__intpolMin;    //actual Min used for interpolation

        //used to scale the input channels (equalize the amplitudes)
        float m__Ch1Factor = 1; //factor to scale the channel
        float m__Ch2Factor = 1; //factor to scale the channel
        float m__Ch1MinLevel = 0;  //level of minimum(above 0) 
        float m__Ch2MinLevel = 0;  //level of minimum(above 0) 

        int m__actHalfTooth;  
        int m__actPos; //endresult

        bool m__calcActIndexTeethrack(); //calculate actual index to find learned halfteeth min max for interpolation (returns false when out of range)
        int m__addCalcMinAv(int halftooth, int valueToAdd); //add and calc average Min for given half-tooth
        int m__addCalcMaxAv(int halftooth, int valueToAdd); //add and calc average Min for given half-tooth  
        int m__calcSumMid(); //add actual m_sum and calculate initial mid between min and max (if they differ a minimum amount and if min 10 measures were taken)
        int m__SinInterpolMinMax(int min, int max, int actval, int resolution);
        
    public:
        CSinIncCntr(); 
        void setScalings(float Ch1_fact, float Ch2_fact, float Ch1_minLev, float Ch2_minLev);
        int calc(int actCh1, int actCh2);
        int read();
        int setTo(int value);
};