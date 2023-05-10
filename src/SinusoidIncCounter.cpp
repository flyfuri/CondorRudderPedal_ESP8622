//sinusoid increment counter (encoder).cpp

#include "SinusoidIncCounter.h"
#include "Arduino.h"

#define DEBGOUT 99

#if DEBGOUT == 99
  #define dbugprint(x) Serial.print(x)
  #define dbugprintln(x) Serial.println(x)
#else
  #define dbugprint(x) 
  #define dbugprintln(x) 
#endif

CSinIncCntr::CSinIncCntr(){
    
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

    if(m__actSubStatus > 0 && m__sub < 0){ //when difference is crossing Nullline from positive to negative
        if(m__sum >= m__sumMidLine)
        {
            m__actPos--;
        }
        else
        {
            m__actPos++;
        }
        
        m__actSubStatus = -1;
    }  
    else if(m__actSubStatus < 0 && m__sub >= 0){//when difference is crossing Nullline from negative to positive
        if(m__sum >= m__sumMidLine)
        {
            m__actPos++;
        }
        else
        {
            m__actPos--;
        }
        
        m__actSubStatus = 1;
    }      
    dbugprint(" ");
    dbugprint(m__actPos);      
    dbugprint(" ");
    dbugprint(actCh1);     
    dbugprint(" ");
    dbugprint(actCh2);    
    dbugprint(" ");
    dbugprint(m__sum);
    dbugprint(" ");
    dbugprint(m__sub);
    dbugprint(" ");
    dbugprint(m__actSubStatus);
    dbugprint(" ");

    return m__actPos;
} 

int CSinIncCntr::read(){
    return m__actPos;
} 

int CSinIncCntr::setTo(int value){
    return m__actPos = value;
} 
