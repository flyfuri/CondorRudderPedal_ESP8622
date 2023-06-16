#include <ChannelPairScaler.h>

template <typename T> CSinCosScaler<T>::CSinCosScaler(T scaleTargetMIN, T scaleTargetMAX){
    actCH1_Min = 0;
    actCH1_Max = 0;
    actCH2_Min = 0;
    actCH2_Max = 0;
    m__actHalfToothCH1 = 0;
    m__actHalfToothCH2 = 0;
    targetMIN = scaleTargetMIN;
    targetMAX = scaleTargetMAX;

    lastDerivCH1 = 0;
    lastDerivCH2 = 0;
}

template <typename T> bool CSinCosScaler<T>::calculate (T CH1, T CH2, T &scaledCH1, T &scaledCH2){
    if(CH1 > actCH1_Max){actCH1_Max=CH1;}
    if(CH1 < actCH1_Min){actCH1_Max=CH1;}
    if(CH2 > actCH2_Max){actCH2_Max=CH2;}
    if(CH2 < actCH2_Min){actCH2_Min=CH2;}

    FirstDerivCH1.measurementIfMinChange(CH1, 0);
    FirstDerivCH2.measurementIfMinChange(CH2, 0);

    if(FirstDerivCH1.getNbrMeas() >= 2 && FirstDerivCH2.getNbrMeas() >= 2){
        //calc derivates
        prevDerivCH1 = lastDerivCH1;
        prevDerivCH2 = lastDerivCH2;
        lastDerivCH1 = FirstDerivCH1.deriv1overLastNbr(2, 0.1);   
        lastDerivCH2 = FirstDerivCH2.deriv1overLastNbr(2, 0.1);

        //search for crossings of any derivate to the 0 line (find min, max or just direction changes)
        if(prevDerivCH1 < 0 && lastDerivCH1 > 0){
            typePreviousDrvCrossing = typeLastDrvCrossing;
            typeLastDrvCrossing = CH1UP;
            if (typePreviousDrvCrossing == CH2DOWN){
                TeethMem_CH1.addCalcMinAv(m__actHalfToothCH1);
                m__actHalfToothCH1++;               
                TeethMem_CH1.addCalcMinAv(m__actHalfToothCH1);
            }
            else if (typePreviousDrvCrossing == CH2UP){
                TeethMem_CH1.addCalcMinAv(m__actHalfToothCH1);
                m__actHalfToothCH1--;               
                TeethMem_CH1.addCalcMinAv(m__actHalfToothCH1);
            }
        }
        else if(prevDerivCH2 < 0 && lastDerivCH2 > 0){
            typePreviousDrvCrossing = typeLastDrvCrossing;
            typeLastDrvCrossing = CH2UP;
            if (typePreviousDrvCrossing == CH1UP){
                
            }
        }
        else if(prevDerivCH1 > 0 && lastDerivCH1 < 0){
            typePreviousDrvCrossing = typeLastDrvCrossing;
            typeLastDrvCrossing = CH1DOWN;
            if (typePreviousDrvCrossing == CH2UP){
                
            }
        }
        else if(prevDerivCH2 > 0 && lastDerivCH2 < 0){   
            typePreviousDrvCrossing = typeLastDrvCrossing;
            typeLastDrvCrossing = CH2DOWN;
            if (typePreviousDrvCrossing == CH1DOWN){
                
            }
        }
    }

    return false;
}