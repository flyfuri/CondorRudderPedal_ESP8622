#include <ChannelPairScaler.h>
#include "Arduino.h"

#ifndef DEBGOUT
    #define DEBGOUT 99  //if this is 99 dbugprints will be active
#endif

#if DEBGOUT == 99
    #ifndef dbugprint
        #define dbugprint(x) Serial.print(x)
    #endif
    #ifndef dbugprintln
        #define dbugprintln(x) Serial.println(x)
    #endif
#else
    #ifndef dbugprint
        #define dbugprint(x)
    #endif
    #ifndef dbugprintln
        #define dbugprintln(x)
    #endif
#endif

template <typename T> CSinCosScaler<T>::CSinCosScaler(T scaleTargetMIN, T scaleTargetMAX){
    m__actCH1_Min = 0;
    m__actCH1_Max = 0;
    m__actCH2_Min = 0;
    m__actCH2_Max = 0;
    m__actHalfToothCH1 = 0;
    m__actHalfToothCH2 = 0;
    targetMIN = scaleTargetMIN;
    targetMAX = scaleTargetMAX;

    m__lastDerivCH1 = 0;
    m__lastDerivCH2 = 0;
}

template <typename T> bool CSinCosScaler<T>::calculate(T CH1, T CH2){
    if(CH1 > m__actCH1_Max){m__actCH1_Max=CH1;}
    if(CH1 < m__actCH1_Min){m__actCH1_Max=CH1;}
    if(CH2 > m__actCH2_Max){m__actCH2_Max=CH2;}
    if(CH2 < m__actCH2_Min){m__actCH2_Min=CH2;}

    double inCH1 = CH1;
    double inCH2 = CH2;

    FirstDerivCH1.measurementIfMinChange(inCH1, 1);
    FirstDerivCH2.measurementIfMinChange(inCH2, 1);

    if(FirstDerivCH1.getNbrMeas() >= 3 && FirstDerivCH2.getNbrMeas() >= 3){
        //calc derivates
        double actDerivCHx = FirstDerivCH1.deriv1overLastNbr(3, 0.1);
        if (actDerivCHx != 0){
            m__prevDerivCH1 = m__lastDerivCH1;
            m__lastDerivCH1 =  actDerivCHx;
        }

        actDerivCHx = FirstDerivCH2.deriv1overLastNbr(3, 0.1);
        if (actDerivCHx != 0){
            m__prevDerivCH2 = m__lastDerivCH2; 
            m__lastDerivCH2 = actDerivCHx;
        }

        //search for crossings of any derivate to the 0 line (find min, max or just direction changes)
        if(m__prevDerivCH1 < 0 && m__lastDerivCH1 > 0){
            typePreviousDrvCrossing = typeLastDrvCrossing;
            typeLastDrvCrossing = CH1UP;
            if (typePreviousDrvCrossing == CH2DOWN){
                TeethMem_CH1.addCalcMinAv(m__actHalfToothCH1, m__actCH1_Min);
                m__actHalfToothCH1++;         
            }
            else if (typePreviousDrvCrossing == CH2UP){
                TeethMem_CH1.addCalcMinAv(m__actHalfToothCH1, m__actCH1_Min);
                m__actHalfToothCH1--;       
            }
        }
        else if(m__prevDerivCH2 < 0 && m__lastDerivCH2 > 0){
            typePreviousDrvCrossing = typeLastDrvCrossing;
            typeLastDrvCrossing = CH2UP;
            if (typePreviousDrvCrossing == CH1UP){
                TeethMem_CH2.addCalcMinAv(m__actHalfToothCH1, m__actCH2_Min);
                m__actHalfToothCH2++;         
            }
            else if (typePreviousDrvCrossing == CH1DOWN){
                TeethMem_CH2.addCalcMinAv(m__actHalfToothCH1, m__actCH2_Min);
                m__actHalfToothCH2--;       
            }
        }
        else if(m__prevDerivCH1 > 0 && m__lastDerivCH1 < 0){
            typePreviousDrvCrossing = typeLastDrvCrossing;
            typeLastDrvCrossing = CH1DOWN;
            if (typePreviousDrvCrossing == CH2UP){
                TeethMem_CH1.addCalcMaxAv(m__actHalfToothCH1, m__actCH1_Max);
                m__actHalfToothCH1++;         
            }
            else if (typePreviousDrvCrossing == CH2DOWN){
                TeethMem_CH1.addCalcMaxAv(m__actHalfToothCH1, m__actCH1_Max);
                m__actHalfToothCH1--;       
            }
        }
        else if(m__prevDerivCH2 > 0 && m__lastDerivCH2 < 0){   
            typePreviousDrvCrossing = typeLastDrvCrossing;
            typeLastDrvCrossing = CH2DOWN;
            if (typePreviousDrvCrossing == CH1DOWN){
                TeethMem_CH2.addCalcMaxAv(m__actHalfToothCH1, m__actCH2_Max);
                m__actHalfToothCH2++;         
            }
            else if (typePreviousDrvCrossing == CH1UP){
                TeethMem_CH2.addCalcMaxAv(m__actHalfToothCH1, m__actCH2_Max);
                m__actHalfToothCH2--;       
            }
        }
    }

    dbugprint(inCH1);     
    dbugprint(";");
    dbugprint(inCH2);   
    dbugprint(";");
    dbugprint(inCH1 - inCH2);   
    dbugprint(";");
    dbugprint(m__lastDerivCH1);     
    dbugprint(";");
    dbugprint(m__lastDerivCH2);    
    dbugprint(";");
    dbugprint(m__actHalfToothCH1);
    dbugprint(";");
    dbugprint(m__actHalfToothCH2);
    dbugprint(";");
    

    return false;
}

template class CSinCosScaler<int>;
template class CSinCosScaler<float>;
template class CSinCosScaler<double>;