#pragma once
#define NBR_TEETH_ON_RACK 10 //number of theeth on rail
#define NBR_TO_AVR_IND_TOOTH 5 //number to average individual tooth 

class CTeethMemory{
public:
    struct TeethRack {
                int halft_min[NBR_TO_AVR_IND_TOOTH]; //point on summary curve above crossing of both channels is taken as max of that theeth
                int halft_max[NBR_TO_AVR_IND_TOOTH]; //point on summary curve underneath crossing of both channels is taken as min of that theeth
                int halftMin_index; // 
                int halftMax_index; // 
                int minAv;
                int maxAv;
            }teethrack[NBR_TEETH_ON_RACK  * 2]; //learned behaviour of teethrack. Array double as long and start in middle of array since we don't know where we start at init

    CTeethMemory();
    bool calcActIndexTeethrack(int halftooth, int &calculatedIndex); //calculate actual index to find learned halfteeth min max (returns false when out of range)     
    int addCalcMinAv(int halftooth, int valueToAdd); //add and calc average Min for given half-tooth
    int addCalcMaxAv(int halftooth, int valueToAdd); //add and calc average Min for given half-tooth  

};
       