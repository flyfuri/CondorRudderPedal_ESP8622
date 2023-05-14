//sinusoid increment counter (encoder).cpp

#include "SinusoidIncCounter.h"
#include "math.h"
#include "Arduino.h"

#define _USE_MATH_DEFINES

#define DEBGOUT 99

#if DEBGOUT == 99
  #define dbugprint(x) Serial.print(x)
  #define dbugprintln(x) Serial.println(x)
#else
  #define dbugprint(x) 
  #define dbugprintln(x) 
#endif

CSinIncCntr::CSinIncCntr(){
    for(int i = 0; i < NBR_TEETH_ON_RACK  * 2; i++){
        for(int ii = 0; ii < NBR_TO_AVR_IND_TOOTH; ii++){
            teethrack[i].individ_tooth[ii].max=0;
            teethrack[i].individ_tooth[ii].min=0;
        }
        teethrack[i].minAv=0;
        teethrack[i].maxAv=0;
        teethrack[i].act_nbr_index=0;
    }
    InitialSumCurveMinMaxs = new ANFLTR::CFilterAnalogOverMeasures(10U,10);
}

bool CSinIncCntr::m__calcActIndTeetrack(){
    int tempTeethindex = NBR_TEETH_ON_RACK + m__actHalfTooth;
    if(tempTeethindex < 0){
        m__actIndexTeethrack = 0;
        return false;
    } 
    else if (tempTeethindex >= NBR_TEETH_ON_RACK * 2){
        m__actIndexTeethrack = NBR_TEETH_ON_RACK * 2;
        return false;
    }
    else{
        m__actIndexTeethrack = tempTeethindex;
        return true;
    }
}

int CSinIncCntr::m__calcSumMid(){
    dbugprint(m__sumMidLine);     
    dbugprint(";");
    if(m__sumMidLine == 0){ //initial search for middle line of summary curve
        dbugprint(InitialSumCurveMinMaxs->getAverage());      
        dbugprint(";");
        dbugprint(InitialSumCurveMinMaxs->getMax());      
        dbugprint(";");
        dbugprint(InitialSumCurveMinMaxs->getMin());      
        dbugprint(";");
        dbugprint(InitialSumCurveMinMaxs->getNbrMeas());      
        dbugprint(";");
        dbugprint(m__sumMidLine);     
        dbugprint(";");

        if(InitialSumCurveMinMaxs->getMin() == 0 || InitialSumCurveMinMaxs->getMax() == 0 
        || m__sum < InitialSumCurveMinMaxs->getMin() || m__sum > InitialSumCurveMinMaxs->getMax() || InitialSumCurveMinMaxs->getNbrMeas() < 9 ){
            InitialSumCurveMinMaxs->measurement(m__sum);
            int tempMidLine = (InitialSumCurveMinMaxs->calcMinMax(true) + InitialSumCurveMinMaxs->getMin()) / 2;

            if (InitialSumCurveMinMaxs->getMax() - InitialSumCurveMinMaxs->getMin() > 60 
            && InitialSumCurveMinMaxs->getNbrMeas() >= 9
            && std::abs(InitialSumCurveMinMaxs->getAverage() - tempMidLine) < 40){
                m__sumMidLine = tempMidLine;
                delete InitialSumCurveMinMaxs;
                SumCurveLastMaxs = new ANFLTR::CFilterAnalogOverMeasures(20U,20);
                SumCurveLastMins = new ANFLTR::CFilterAnalogOverMeasures(20U,20);
                return m__sumMidLine;
            }
        }
    }
    else {
        if (m__sum >= m__sumMidLine){
            m__sumMidLine = (SumCurveLastMaxs->measurement(m__sum) + SumCurveLastMins->getAverage()) / 2;
        }
        else{
            m__sumMidLine = (SumCurveLastMins->measurement(m__sum) + SumCurveLastMaxs->getAverage()) / 2;
        }
        return m__sumMidLine;
    }
    return 0;
}

int CSinIncCntr::m__addCalcMinAv(int halftooth, int valueToAdd){ //add and calc average Min for given half-tooth
    if(m__calcActIndTeetrack()){ 
        teethrack[m__actIndexTeethrack].individ_tooth[teethrack[m__actIndexTeethrack].act_nbr_index].min = valueToAdd;
        if (++teethrack[m__actIndexTeethrack].act_nbr_index >= NBR_TO_AVR_IND_TOOTH ) //move index for next measure to replace
            teethrack[m__actIndexTeethrack].act_nbr_index = 0;
    }
    
    int temptotal = 0;
    int tempNbrsToAv = NBR_TO_AVR_IND_TOOTH;
    for(int i = 0; i < NBR_TO_AVR_IND_TOOTH; i++){  //calc average
        if(teethrack[m__actIndexTeethrack].individ_tooth[i].min > 0){
            temptotal += teethrack[m__actIndexTeethrack].individ_tooth[i].min;
        }
        else if (tempNbrsToAv > 1){ //don't go lower than 1 to avoid later DIV0
            tempNbrsToAv--;
        }
    }
    return teethrack[m__actIndexTeethrack].minAv = temptotal / tempNbrsToAv;
}

int CSinIncCntr::m__addCalcMaxAv(int halftooth, int valueToAdd){ //add and calc average Max for given half-tooth 
   if(m__calcActIndTeetrack()){ 
        teethrack[m__actIndexTeethrack].individ_tooth[teethrack[m__actIndexTeethrack].act_nbr_index].max = valueToAdd;
        if (++teethrack[m__actIndexTeethrack].act_nbr_index >= NBR_TO_AVR_IND_TOOTH ) //move index for next measure to replace
            teethrack[m__actIndexTeethrack].act_nbr_index = 0;
    }
    
    int temptotal = 0;
    int tempNbrsToAv = NBR_TO_AVR_IND_TOOTH;
    for(int i = 0; i < NBR_TO_AVR_IND_TOOTH; i++){  //calc average
        if(teethrack[m__actIndexTeethrack].individ_tooth[i].max > 0){
            temptotal += teethrack[m__actIndexTeethrack].individ_tooth[i].max;
        }
        else if (tempNbrsToAv > 1){ //don't go lower than 1 to avoid later DIV0
            tempNbrsToAv--;
        }
    }
    return teethrack[m__actIndexTeethrack].maxAv = temptotal / tempNbrsToAv;
}

int CSinIncCntr::m__SinInterpolMinMax(int min, int max, int actval, int resolution){
    float tmpmax = max -min;  //  == 0 ? 1 : max - min;  //avoid later division 0
    if (tmpmax == 0){Serial.print("Div0!");}  //TEST
    float tmpact = actval -min; 
    float tmpactval = tmpact > tmpmax ? tmpmax : tmpact;
    float tmpresult = (resolution/(PI/2)) * sin(((PI/2)/tmpmax) * tmpactval);
    return (int)(tmpresult * 1000) % 1000 >= 500 ? (int)tmpresult + 1 : (int)tmpresult;  //take 3 digits after period to round
}

int CSinIncCntr::calc(int actCh1, int actCh2){
    //calculate sum of both channels the help determine counting direction
    m__sum = actCh1 + actCh2;

    //calculate difference of the channels to see where they cross (nullpoint of difference)
    m__sub = actCh1 - actCh2;


    //init difference curve half status at beginning
    if(m__actSubStatus == 0){
        if(m__sub >=0){
            m__actSubStatus = 1;
        }
        else{
            m__actSubStatus = -1;
        } 
    }

    if(m__actSubStatus > 0 && m__sub < 0){ //when difference is crossing Null-line from positive to negative
        if (m__calcSumMid() != 0){
            if(m__sum >= m__sumMidLine) //sub FALLING sum at MAX  (channel lines are crossing: sub-curve crossing nullpoint FALLING with summary at MAX)
            {
                m__intpolMax = m__addCalcMaxAv(m__actHalfTooth, m__sum);
                m__actHalfTooth--;
                m__intpolMax = m__addCalcMaxAv(m__actHalfTooth, m__sum);
                m__intpolMin = teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].minAv;
            }
            else //sub FALLING sum at MIN (channel lines are crossing: sub-curve crossing nullpoint FALLING with summary at MIN)
            {
                m__actHalfTooth++;
                m__intpolMin = m__addCalcMinAv(m__actHalfTooth, m__sum);
                m__intpolMax = teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].maxAv;
            } 
            m__sumOnLastCrossing = m__sum;
        }
        m__actSubStatus = -1;  //always to do when sub crossing zero line!
    }  
    else if(m__actSubStatus < 0 && m__sub >= 0){//when difference is crossing Null-line from negative to positive
        if (m__calcSumMid() != 0){
            if(m__sum >= m__sumMidLine)//sub RISING sum at MAX  (channel lines are crossing: sub-curve crossing nullpoint RISING with summary at MAX)
            {
                m__actHalfTooth++;
                m__intpolMax = m__addCalcMaxAv(m__actHalfTooth, m__sum);
                m__intpolMin = teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].minAv;
            }
            else //sub RISING sum at MIN  (channel lines are crossing: sub-curve crossing nullpoint RISING with summary at MIN)
            {
                m__actHalfTooth--;
                m__intpolMin = m__addCalcMinAv(m__actHalfTooth, m__sum);
                m__intpolMax = teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].maxAv > 0 ? teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].maxAv : teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth -1].maxAv;
            }
            m__sumOnLastCrossing = m__sum;
        }
        m__actSubStatus = 1; //always to do when sub crossing zero line!
    }
    else{
        m__intpolMax = teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].maxAv;
        m__intpolMin = teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].minAv;
    } 
    if (m__sumMidLine != 0){
        int tempIntpol = m__SinInterpolMinMax(m__intpolMax, m__intpolMin, m__sum, INTPOLRES);
        if (m__actSubStatus < 0){
            m__actPos = m__actHalfTooth * INTPOLRES + tempIntpol;
        }
        else if (m__actSubStatus > 0){
            m__actPos = m__actHalfTooth * INTPOLRES + 1 - tempIntpol;
        }
        else{
            m__actPos = m__actHalfTooth * INTPOLRES;
        }

    }


    /*dbugprint(m__actPos);      
    dbugprint(";");
    dbugprint(m__actHalfTooth);      
    dbugprint(";");
    dbugprint(actCh1);     
    dbugprint(";");
    dbugprint(actCh2);    
    dbugprint(";");
    dbugprint(m__sum);
    dbugprint(";");
    dbugprint(m__sub);
    dbugprint(";");
    dbugprint(m__sumMidLine);
    dbugprint(";");
    dbugprint(m__sumOnLastCrossing);
    dbugprint(";");
    dbugprint(teethrack[m__actHalfTooth].act_nbr_index);
    dbugprint(";");*/

    return m__actHalfTooth;
} 

int CSinIncCntr::read(){
    return m__actHalfTooth;
} 

int CSinIncCntr::setTo(int value){
    return m__actHalfTooth = value;
} 
