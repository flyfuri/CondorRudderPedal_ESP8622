
#include <analog_filter.h>

#define NBR_TEETH_ON_RACK 10 //number of theeth on rail
#define NBR_TO_AVR_IND_TOOTH 3 //number to average individual tooth 

class CSinIncCntr{
    private: 
        //used for basic counting the theeth (counts twice per tooth on each crossing point of the 2 channel curves):
        int m__sum;  //summary of both channels
        int m__sub;   //CH1 - CH2
        int m__sumMidLine = 0; //approx middle line of summary
        short m__actSubStatus = 0;  //wich half of the difference curve we're now (-1=negative, 0 undefined(at beginning), 1=positive)
        
        // used to find the approx. middle line of summary curve
        ANFLTR::CFilterAnalogOverMeasures LastSumMinMaxs{10U, 10}; //to find initial mid line
        bool m__initialSumMidFound; //initial sum middle line has been found

        //used for interpolation beetween flank counts to increse resolution        
        struct TeethRack {
            struct HalfTooth{
                int min; //point on summary curve above crossing of both channels is taken as max of that theeth
                int max; //point on summary curve underneath crossing of both channels is taken as min of that theeth
            }individ_tooth[NBR_TO_AVR_IND_TOOTH]; 
            int act_nbr_index; //shows to the next place in the measurements where a 
            int minAv;
            int maxAv;
        }teethrack[NBR_TEETH_ON_RACK  * 2]; //learned behaviour of teethrack. Array double as long and start in middle of array since we don't know where we start at init

        

        int m__actHalfTooth;  //endresult

        int m__addCalcMinAv(int halftooth, int valueToAdd); //add and calc average Min for given half-tooth
        int m__addCalcMaxAv(int halftooth, int valueToAdd); //add and calc average Min for given half-tooth  
        int m__calcSumMid(); //add actual m_sum and calculate initial mid between min and max (if they differ a minimum amount and if min 10 measures were taken)
        int m__procedure_subRising_sumMax(); //one of 4 cases channel curves cross
        int m__procedure_subRising_sumMin(); //one of 4 cases channel curves cross
        int m__procedure_subFalling_sumMax(); //one of 4 cases channel curves cross
        int m__procedure_subFalling_sumMin(); //one of 4 cases channel curves cross
        
    public:
        CSinIncCntr();  
        int calc(int actCh1, int actCh2);
        int read();
        int setTo(int value);
};