//sinusoid increment counter (encoder).cpp

#include "SinusoidIncCounter.h"
#include "math.h"
#include "Arduino.h"

#define _USE_MATH_DEFINES

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

CSinIncCntr::CSinIncCntr(){
    m__sumOnLastCrossing = -9999; 
    m__sumMidLine = 0;
    m__actStatusSUB = 0;  
    m__actStatusSUM = 0; 
    m__sumAtPowerON = -9999; 
    m__offset = -9999;
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
    SumCurveLastMaxs.reset();
    SumCurveLastMins.reset();
}

bool CSinIncCntr::m__calcActIndexTeethrack(){
    int tempTeethindex = NBR_TEETH_ON_RACK + m__actHalfTooth;
    if(tempTeethindex < 0){
        m__actIndexTeethrack = 0;
        return false;
    } 
    else if (tempTeethindex >= NBR_TEETH_ON_RACK * 2){
        m__actIndexTeethrack = (NBR_TEETH_ON_RACK * 2) -1;
        return false;
    }
    else{
        m__actIndexTeethrack = tempTeethindex;
        return true;
    }
}

int CSinIncCntr::m__calcSumMid(){
    if(m__sumMidLine == 0 || m__actStatusSUM == 0){ //initial search for middle line of summary curve
        if(m__actStatusSUM == 0)
        {  
            int tempCrsDiff = m__sum - m__sumOnLastCrossing;
            if(m__sumOnLastCrossing == -9999){  //only the case at very first crossing measured
                m__sumOnLastCrossing = m__sum;
            }
            else if (tempCrsDiff <= INIT_MIN_DIST_SUM_MINMAX * -1){ //act sum (last crossing) is MIN of curve
                m__actStatusSUM = -1;
                
                //interpolate startup position to calculate offset later
                /*dbugprintln(";");    
                dbugprint("Sum negative; ");      
                dbugprint(";min:");   
                dbugprint(m__sum);      
                dbugprint(";max:");   
                dbugprint(m__sumOnLastCrossing);        
                dbugprint(";startpos:");   
                dbugprint(m__sumAtPowerON);  */
                m__sumAtPowerON = m__SinInterpolMinMax(m__sum, m__sumOnLastCrossing, m__sumAtPowerON, INTPOLRES);      
                /*dbugprint(";intpol:");   
                dbugprint(m__sumAtPowerON);  
                dbugprintln(";");*/    

                //return line which decides whether sum curve is MIN or MAX
                return m__sumMidLine = m__sum + SUM_MIDLINE_ABOVE_MIN;  
            }
            else if (tempCrsDiff >= INIT_MIN_DIST_SUM_MINMAX){ //old sum (this crossing) is MIN of curve
                m__actStatusSUM = 1;
                /*if (m__actStatusSUB = -1){
                    m__actHalfTooth--;
                }  TODO*/ 

                //interpolate startup position to calculate offset later
                /*dbugprintln(";");    
                dbugprint("Sum positive; ");      
                dbugprint(";min:");   
                dbugprint(m__sum);      
                dbugprint(";max:");   
                dbugprint(m__sumOnLastCrossing);        
                dbugprint(";startpos:");   
                dbugprint(m__sumAtPowerON); */ 
                m__sumAtPowerON = m__SinInterpolMinMax(m__sumOnLastCrossing, m__sum, m__sumAtPowerON, INTPOLRES);  
                /*dbugprint(";intpol:");   
                dbugprint(m__sumAtPowerON);  
                dbugprintln(";"); */   
                
                //return line which decides whether sum curve is MIN or MAX
                return m__sumMidLine = m__sumOnLastCrossing + SUM_MIDLINE_ABOVE_MIN;
            }
        } 
        return 0;
    }
    else {
        if(m__sum < m__sumMidLine){
            SumCurveLastMins.measurement(m__sum);
            if (SumCurveLastMins.getNbrMeas() > 4){
                m__sumMidLine = SumCurveLastMins.getAverage() + SUM_MIDLINE_ABOVE_MIN;
            }
        }

        if(m__sum >= m__sumMidLine){
            SumCurveLastMaxs.measurement(m__sum);
            //if (SumCurveLastMaxs.getNbrMeas() > 4){
            //    m__sumMidLine = SumCurveLastMaxs.getAverage() + SUM_MIDLINE_ABOVE_MIN;
            //}
        }

        if (m__sum >= m__sumMidLine){
           m__actStatusSUM = 1; 
        }
        else{
            m__actStatusSUM = 1;
        }

        return m__sumMidLine;
    }
    return 0;
}

int CSinIncCntr::m__addCalcMinAv(int halftooth, int valueToAdd){ //add and calc average Min for given half-tooth
    if(m__calcActIndexTeethrack()){ 
        teethrack[m__actIndexTeethrack].halft_min[teethrack[m__actIndexTeethrack].halftMin_index] = valueToAdd;
        if (++(teethrack[m__actIndexTeethrack].halftMin_index) >= NBR_TO_AVR_IND_TOOTH ) //move index for next measure to replace
            teethrack[m__actIndexTeethrack].halftMin_index = 0;
    }
    
    int temptotal = 0;
    int tempNbrsToAv = NBR_TO_AVR_IND_TOOTH;
    for(int i = 0; i < NBR_TO_AVR_IND_TOOTH; i++){  //calc average
        if(teethrack[m__actIndexTeethrack].halft_min[i] > 0){
            temptotal += teethrack[m__actIndexTeethrack].halft_min[i];
        }
        else if (tempNbrsToAv > 1){ //don't go lower than 1 to avoid later DIV0
            tempNbrsToAv--;
        }
    }
    if (temptotal > 0){
        return teethrack[m__actIndexTeethrack].minAv = temptotal / tempNbrsToAv;
    }
    else{
        return teethrack[m__actIndexTeethrack].minAv;
    }
}

int CSinIncCntr::m__addCalcMaxAv(int halftooth, int valueToAdd){ //add and calc average Max for given half-tooth  
     if(m__calcActIndexTeethrack()){ 
        teethrack[m__actIndexTeethrack].halft_max[teethrack[m__actIndexTeethrack].halftMax_index] = valueToAdd;
        if (++(teethrack[m__actIndexTeethrack].halftMax_index) >= NBR_TO_AVR_IND_TOOTH ) //move index for next measure to replace
            teethrack[m__actIndexTeethrack].halftMax_index = 0;
    }
    
    int temptotal = 0;
    int tempNbrsToAv = NBR_TO_AVR_IND_TOOTH;
    for(int i = 0; i < NBR_TO_AVR_IND_TOOTH; i++){  //calc average
        if(teethrack[m__actIndexTeethrack].halft_max[i] > 0){
            temptotal += teethrack[m__actIndexTeethrack].halft_max[i];
        }
        else if (tempNbrsToAv > 1){ //don't go lower than 1 to avoid later DIV0
            tempNbrsToAv--;
        }
    }
    if (temptotal > 0){
        return teethrack[m__actIndexTeethrack].maxAv = temptotal / tempNbrsToAv;
    }
    else{
        return teethrack[m__actIndexTeethrack].maxAv;
    }
}

int CSinIncCntr::m__SinInterpolMinMax(int min, int max, int actval, int resolution){

   if(max <= 50 && SumCurveLastMaxs.getNbrMeas() > 4){
        max = SumCurveLastMaxs.getAverage();
    }
    if(min <= 10 && SumCurveLastMins.getNbrMeas() > 4){
        min = SumCurveLastMins.getAverage();
    }

    if(max > 0 && min > 0){ //do only interpolate if some min and max is memorised already
        if(actval < min){
            return 0;
        }
        else if(actval >= max){
            return resolution;
        }
        else{
            double tmpresult = (resolution / PI) * ((asin(((2 / ((float)max - (float)min)) * ((float)actval - (float)min)) - 1)) + PI / 2);

            /*double tmpmax = max -min;  
            double tmpactval = actval - min;
            double tmpresult = (resolution / PI) * ((asin(((2 / tmpmax) * tmpactval)-1)) + PI/2);
            //double tmpresult = (resolution/(PI/2)) * sin(((PI/2)/tmpmax) * tmpactval);*/
            return (int)(tmpresult * 1000) % 1000 >= 500 ? (int)tmpresult + 1 : (int)tmpresult;  //take 3 digits after period to round
        }
    }
    else{
        return 0;
    }
}

int CSinIncCntr::calc(int actCh1, int actCh2){
    //TODO provisionaly amplifying ch2
    actCh2 = (actCh2 - 75) * 2 + 75;

    //calculate sum of both channels the help determine counting direction
    m__sum = actCh1 + actCh2;   

    //calculate difference (subtraction) of the channels to see where they cross (nullpoint of difference)
    m__sub = actCh1 - actCh2;


    //init difference curve half status at beginning
    if(m__actStatusSUB == 0){
        if(m__sub >=0){
            m__actStatusSUB = 1;
        }
        else{
            m__actStatusSUB = -1;
        } 
        m__sumAtPowerON = m__sum;  //memorize actual sum to do interpolation of very first position later
    }

    if(m__actStatusSUB > 0 && m__sub < 0){ //when difference is crossing Null-line from positive to negative
        if (m__calcSumMid() != 0){
            if(m__sum >= m__sumMidLine) //sub FALLING sum at MAX  (channel lines are crossing: sub-curve crossing nullpoint FALLING with summary at MAX)
            {   
                if(m__sumOnLastCrossing < m__sumMidLine){    
                    m__addCalcMinAv(m__actHalfTooth, m__sumOnLastCrossing);
                }
                m__actHalfTooth--;
                if(m__calcActIndexTeethrack()){
                    m__intpolMax = m__addCalcMaxAv(m__actHalfTooth, m__sum);
                    m__intpolMin = teethrack[m__actIndexTeethrack].minAv;
                }
            }
            else //sub FALLING sum at MIN (channel lines are crossing: sub-curve crossing nullpoint FALLING with summary at MIN)
            {
                if(m__sumOnLastCrossing >= m__sumMidLine){    
                        m__addCalcMaxAv(m__actHalfTooth, m__sumOnLastCrossing);
                }
                m__actHalfTooth++;
                if(m__calcActIndexTeethrack()){
                    m__intpolMin = m__addCalcMinAv(m__actHalfTooth, m__sum);
                    m__intpolMax = teethrack[m__actIndexTeethrack].maxAv;
                }
            } 
        }
        m__sumOnLastCrossing = m__sum;
        m__actStatusSUB = -1;  //always to do when sub crossing zero line!
    }  
    else if(m__actStatusSUB < 0 && m__sub >= 0){//when difference is crossing Null-line from negative to positive
        if (m__calcSumMid() != 0){
            if(m__sum >= m__sumMidLine)//sub RISING sum at MAX  (channel lines are crossing: sub-curve crossing nullpoint RISING with summary at MAX)
            {   
                if(m__sumOnLastCrossing < m__sumMidLine){    
                    m__addCalcMinAv(m__actHalfTooth, m__sumOnLastCrossing);
                }
                m__actHalfTooth++;
                m__intpolMax = m__addCalcMaxAv(m__actHalfTooth, m__sum);
                m__intpolMin = teethrack[m__actIndexTeethrack].minAv;
            }
            else //sub RISING sum at MIN  (channel lines are crossing: sub-curve crossing nullpoint RISING with summary at MIN)
            {
                if(m__sumOnLastCrossing >= m__sumMidLine){    
                        m__addCalcMaxAv(m__actHalfTooth, m__sumOnLastCrossing);
                }
                m__actHalfTooth--;
                m__intpolMin = m__addCalcMinAv(m__actHalfTooth, m__sum);
                m__intpolMax = teethrack[m__actIndexTeethrack].maxAv;
            }
        }
        m__sumOnLastCrossing = m__sum;
        m__actStatusSUB = 1; //always to do when sub crossing zero line!
    }
    /*else{
        m__intpolMax = teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].maxAv;
        m__intpolMin = teethrack[NBR_TEETH_ON_RACK + m__actHalfTooth].minAv;
    } */
    if (m__sumMidLine != 0){
        int tempIntpol = m__SinInterpolMinMax(m__intpolMin, m__intpolMax, m__sum, INTPOLRES);

        int tmpActPos;
        if (m__actStatusSUB < 0){
            tmpActPos = m__actHalfTooth * INTPOLRES + tempIntpol;
            if(m__offset == -9999){ //only the case at the very beginning
                m__offset = tmpActPos - (m__sumAtPowerON) ;    
                /*dbugprintln(";");    
                dbugprint("Sub negative; ");    
                dbugprint(m__sumAtPowerON);     
                dbugprint(";");   
                dbugprint(tmpActPos);  
                dbugprint(";");     
                dbugprint(m__offset);   
                dbugprintln(";");   */  
            }
        }
        else if (m__actStatusSUB > 0){
            tmpActPos = m__actHalfTooth * INTPOLRES + (INTPOLRES - tempIntpol);
            if(m__offset == -9999){ //only the case at the very beginning
                m__offset = tmpActPos - (INTPOLRES - m__sumAtPowerON);   
                /*dbugprintln(";");    
                dbugprint("Sub positive; ");    
                dbugprint(m__sumAtPowerON);     
                dbugprint(";");   
                dbugprint(tmpActPos);  
                dbugprint(";");     
                dbugprint(m__offset);   
                dbugprintln(";"); */    
            }
        }
        else{
            tmpActPos = m__actHalfTooth * INTPOLRES;
        }

        m__actPos = tmpActPos + m__offset;
    }


    dbugprint(m__actPos);     
    dbugprint(";");
    /*dbugprint(m__intpolMin);      
    dbugprint(";");
    dbugprint(m__intpolMax);    
    dbugprint(";");
    dbugprint(teethrack[m__actIndexTeethrack].halftMax_index);      
    dbugprint(";");
    dbugprint(teethrack[m__actIndexTeethrack].halftMin_index);      
    dbugprint(";");*/
    dbugprint(m__actHalfTooth);      
    dbugprint(";");
    dbugprint(m__offset);     
    dbugprint(";");
    dbugprint(m__sumAtPowerON);     
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
    dbugprint(m__actIndexTeethrack);
    dbugprint(";");

    return m__actPos;
} 

int CSinIncCntr::read(){
    return m__actPos;
} 

int CSinIncCntr::setTo(int value){
    return m__actHalfTooth = value;
} 
