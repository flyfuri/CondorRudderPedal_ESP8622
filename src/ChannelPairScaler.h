#pragma once

#include <TeethMemory.h>
#include <analog_filter.h>

template <typename T>
class CSinCosScaler{
    private:
        //calculate ongoing derivate to find mins and maxs of channels
        ANFLTR::CFilterAnalogOverMeasures<double> FirstDerivCH1 = {5, 5};  //filter to calculate the derivate
        ANFLTR::CFilterAnalogOverMeasures<double> FirstDerivCH2 = {5, 5};  //filter to calculate the derivate
        T prevDerivCH1, prevDerivCH2, lastDerivCH1, lastDerivCH2; //dirivates of previous and last measures

        //detect 0-line-crossings of derivates
        enum ENUM_DRVCROSSING(UNKNOWN, CH1UP, CH2UP, CH1DOWN, CH2DOWN); //type of derivate crossing 0 (curve changing direction)
        ENUM_DRVCROSSING typePreviousDrvCrossing = UNKNOWN;
        ENUM_DRVCROSSING typeLastDrvCrossing = UNKNOWN;

        //used to memorize the different min and max of individual teeth
        T actCH1_Min, actCH2_Min, actCH1_Max, actCH2_Max ;
        CTeethMemory<T> TeethMem_CH1, TeethMem_CH2; 
        int m__actHalfToothCH1, m__actHalfToothCH2;
        
        //normalized size (target)
        T targetMIN, targetMAX; //Target min or max value after channel has been scaled
        //used to scale the input channels (equalize the amplitudes)
        float m__Ch1Factor = 1; //factor to scale the channel
        float m__Ch2Factor = 1; //factor to scale the channel
        float m__Ch1MinLevel = 0;  //level of minimum(above 0) 
        float m__Ch2MinLevel = 0;  //level of minimum(above 0)  

    public:
        CSinCosScaler(T scaleTargetMIN, T scaleTargetMAX); //set target value for min max after scaling
        bool calculate (T CH1, T CH2, T &scaledCH1, T &scaledCH2);
};
