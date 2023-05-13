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
}

int CSinIncCntr::m__calcInitialSumMid(){
    if(m__sumMidLine == 0){ //initial search for middle line of summary curve
        LastSumMinMaxs.measurement(m__sum);
        int tempMidLine = (LastSumMinMaxs.calcMinMax(true) + LastSumMinMaxs.getMin()) / 2;
        if (LastSumMinMaxs.getMax() - LastSumMinMaxs.getMin() > 60 
        && LastSumMinMaxs.getNbrMeas() >= 9
        && std::abs(LastSumMinMaxs.getAverage() - tempMidLine) < 50){
            return m__sumMidLine = tempMidLine;
        }
        else return 0;
    }
    else return m__sumMidLine;
}

int CSinIncCntr::m__addCalcMinAv(int halftooth, int valueToAdd){ //add and calc average Min for given half-tooth
    int tempTeethindex = NBR_TEETH_ON_RACK + halftooth;
    if(tempTeethindex > 0 && tempTeethindex < NBR_TEETH_ON_RACK * 2){ 
        teethrack[tempTeethindex].individ_tooth[teethrack[tempTeethindex].act_nbr_index].min = valueToAdd;
        if (++teethrack[tempTeethindex].act_nbr_index >= NBR_TO_AVR_IND_TOOTH ) //move index for next measure to replace
            teethrack[tempTeethindex].act_nbr_index = 0;
    }
    else{ //if more theeth than array records use halfteeth 0
        tempTeethindex = NBR_TEETH_ON_RACK;
    }
    
    int temptotal = 0;
    for(int i = 0; i < NBR_TO_AVR_IND_TOOTH; i++){  //calc average
        temptotal += teethrack[tempTeethindex].individ_tooth[i].min;
    }
    return teethrack[tempTeethindex].minAv = temptotal / NBR_TO_AVR_IND_TOOTH;
}
int CSinIncCntr::m__addCalcMaxAv(int halftooth, int valueToAdd){ //add and calc average Max for given half-tooth 
    int tempTeethindex = NBR_TEETH_ON_RACK + halftooth;
    if(tempTeethindex > 0 && tempTeethindex < NBR_TEETH_ON_RACK * 2){ 
        teethrack[tempTeethindex].individ_tooth[teethrack[tempTeethindex].act_nbr_index].max = valueToAdd;
        if (++teethrack[tempTeethindex].act_nbr_index >= NBR_TO_AVR_IND_TOOTH ) //move index for next measure to replace
            teethrack[tempTeethindex].act_nbr_index = 0;
    }
    else{ //if more theeth than array records use halfteeth 0
        tempTeethindex = NBR_TEETH_ON_RACK;
    }
    
    int temptotal = 0;
    for(int i = 0; i < NBR_TO_AVR_IND_TOOTH; i++){  //calc average
        temptotal += teethrack[tempTeethindex].individ_tooth[i].max;
    }
    return teethrack[tempTeethindex].maxAv = temptotal / NBR_TO_AVR_IND_TOOTH;
}

int CSinIncCntr::m__SinInterpolMinMax(int min, int max, int actval, int resolution){
    float tmpmax = max - min;
    float tmpact = actval -min; 
    float tmpactval = tmpact > tmpmax ? tmpmax : tmpact;
    float tmpresult = (resolution/(PI/2)) * sin(((PI/2)/tmpmax) * tmpactval);
    return (int)(tmpresult * 1000) % 1000 >= 500 ? (int)tmpresult + 1 : (int)tmpresult;  //take 3 digits after period to round
}

int CSinIncCntr::calc(int actCh1, int actCh2){
    //calculate sum of both channels the help determen counting direction
    m__sum = actCh1 + actCh2;

    //calculate difference of the channels to see where they cross (nullpoint of difference)
    m__sub = actCh1 - actCh2;


    //init differende half at beginning
    if(m__actSubStatus == 0){
        if(m__sub >=0){
            m__actSubStatus = 1;
        }
        else{
            m__actSubStatus = -1;
        } 
    }

    if(m__actSubStatus > 0 && m__sub < 0){ //when difference is crossing Null-line from positive to negative
        if (m__calcInitialSumMid() != 0){
            if(m__sum >= m__sumMidLine) //sub FALLING sum at MAX  (channel lines are crossing: sub-curve crossing nullpoint FALLING with summary at MAX)
            {
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
        if (m__calcInitialSumMid() != 0){
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
                m__intpolMax = teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].maxAv;
            }
            m__sumOnLastCrossing = m__sum;
        }
        m__actSubStatus = 1; //always to do when sub crossing zero line!
    }   
    if (m__calcInitialSumMid() != 0){
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


    dbugprint(m__actPos);      
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
    dbugprint(LastSumMinMaxs.getAverage());
    dbugprint(";");
    dbugprint(LastSumMinMaxs.getMax());
    dbugprint(";");
    dbugprint(LastSumMinMaxs.getMin());
    dbugprint(";");
    dbugprint(LastSumMinMaxs.getNbrMeas());
    dbugprint(";");

    return m__actHalfTooth;
} 

int CSinIncCntr::read(){
    return m__actHalfTooth;
} 

int CSinIncCntr::setTo(int value){
    return m__actHalfTooth = value;
} 
