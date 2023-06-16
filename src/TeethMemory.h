#pragma once
#define NBR_TEETH_ON_RACK 10 //number of theeth on rail
#define NBR_TO_AVR_IND_TOOTH 5 //number to average individual tooth 

template <typename T>
class CTeethMemory{
public:
    struct TeethRack {
                T halft_min[NBR_TO_AVR_IND_TOOTH]; //point on summary curve above crossing of both channels is taken as max of that theeth
                T halft_max[NBR_TO_AVR_IND_TOOTH]; //point on summary curve underneath crossing of both channels is taken as min of that theeth
                int halftMin_index; // index 
                int halftMax_index; // 
                T minAv;
                T maxAv;
            }teethrack[NBR_TEETH_ON_RACK  * 2]; //learned behaviour of teethrack. Array double as long and start in middle of array since we don't know where we start at init

    CTeethMemory();
    bool calcActIndexTeethrack(int halftooth, int &calculatedIndex); //calculate actual index to find learned halfteeth min max (returns false when out of range)     
    T addCalcMinAv(int halftooth, T valueToAdd); //add and calc average Min for given half-tooth
    T addCalcMaxAv(int halftooth, T valueToAdd); //add and calc average Min for given half-tooth  

};
       