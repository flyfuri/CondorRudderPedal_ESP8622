/* this class serves to memorize mins and maxs of the curve along the toothed rail
    how the memory is organized
                   *
                 *   *
               *      *
    *         *        *
     *       *          *
       *   *              *   *
         *                  *
    -1 ->|<- 0   ->|<- 1  ->|<-  2      half-tooth enumeration
         |         |        |
      min(-1)    max(0)   min(1)        odd min, even max, 
      min(0)     max(1)   min(2)        even min, odd max,
       **+0      **+1     **+2          index in memory array, ** = NBR_TEETH_ON_RACK (array is double the size)
*/

#pragma once
#include <TeethMemory.h>

template <typename T> CTeethMemory<T>::CTeethMemory(){
    for(int i = 0; i < NBR_TEETH_ON_RACK  * 2; i++){
        for(int ii = 0; ii < NBR_TO_AVR_IND_TOOTH; ii++){
            teethrack[i].halft_min[ii]=0;
            teethrack[i].halft_max[ii]=0;
        }
        teethrack[i].minAv=0;
        teethrack[i].maxAv=0;
        teethrack[i].halftMin_index=0;
        teethrack[i].halftMax_index=0;
    }
}

template <typename T> bool CTeethMemory<T>::calcActIndexTeethrack(int halftooth, int &calculatedIndex){
    int tempTeethindex = NBR_TEETH_ON_RACK + halftooth;
    if(tempTeethindex < 0){
        calculatedIndex = 0;
        return false;
    } 
    else if (tempTeethindex >= NBR_TEETH_ON_RACK * 2){
        calculatedIndex = (NBR_TEETH_ON_RACK * 2) -1;
        return false;
    }
    else{
        calculatedIndex = tempTeethindex;
        return true;
    }
}

template <typename T> T CTeethMemory<T>::addCalcMinAv(int halftooth, T valueToAdd){ //add and calc average Min for given half-tooth
    int tempTeethIndex;
    if(calcActIndexTeethrack(halftooth, tempTeethIndex)){ 
        teethrack[tempTeethIndex].halft_min[teethrack[tempTeethIndex].halftMin_index] = valueToAdd;
        if (++(teethrack[tempTeethIndex].halftMin_index) >= NBR_TO_AVR_IND_TOOTH ) //move index for next measure to replace
            teethrack[tempTeethIndex].halftMin_index = 0;
    }
    
    T temptotal = 0;
    int tempNbrsToAv = NBR_TO_AVR_IND_TOOTH;
    for(int i = 0; i < NBR_TO_AVR_IND_TOOTH; i++){  //calc average
        if(teethrack[tempTeethIndex].halft_min[i] > 0){
            temptotal += teethrack[tempTeethIndex].halft_min[i];
        }
        else if (tempNbrsToAv > 1){ //don't go lower than 1 to avoid later DIV0
            tempNbrsToAv--;
        }
    }
    if (temptotal > 0){
        return teethrack[tempTeethIndex].minAv = temptotal / tempNbrsToAv;
    }
    else{
        return teethrack[tempTeethIndex].minAv;
    }
}

template <typename T> T CTeethMemory<T>::addCalcMaxAv(int halftooth, T valueToAdd){ //add and calc average Max for given half-tooth 
    int tempTeethIndex;
     if(calcActIndexTeethrack(halftooth, tempTeethIndex)){ 
        teethrack[tempTeethIndex].halft_max[teethrack[tempTeethIndex].halftMax_index] = valueToAdd;
        if (++(teethrack[tempTeethIndex].halftMax_index) >= NBR_TO_AVR_IND_TOOTH ) //move index for next measure to replace
            teethrack[tempTeethIndex].halftMax_index = 0;
    }
    
    T temptotal = 0;
    int tempNbrsToAv = NBR_TO_AVR_IND_TOOTH;
    for(int i = 0; i < NBR_TO_AVR_IND_TOOTH; i++){  //calc average
        if(teethrack[tempTeethIndex].halft_max[i] > 0){
            temptotal += teethrack[tempTeethIndex].halft_max[i];
        }
        else if (tempNbrsToAv > 1){ //don't go lower than 1 to avoid later DIV0
            tempNbrsToAv--;
        }
    }
    if (temptotal > 0){
        return teethrack[tempTeethIndex].maxAv = temptotal / tempNbrsToAv;
    }
    else{
        return teethrack[tempTeethIndex].maxAv;
    }
}