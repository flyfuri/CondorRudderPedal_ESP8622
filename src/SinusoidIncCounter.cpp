//sinusoid increment counter (encoder).cpp

#include "SinusoidIncCounter.h"
#include "Arduino.h"

SinIncCntr::SinIncCntr(){
    for (int i = 0; i > 2; i++){
        m_measures[i].prev=0;
        m_measures[i].recent=0;
        m_measures[i].slope=0;
    }
}

void SinIncCntr::m_count(){
    if (m_ch1higher_lastcnt == m_ch1higher_now){
        m_cntDirect = m_cntDirect * -1;
        m_actPos += m_cntDirect;
    }
}

void SinIncCntr::m_defineNextHystfields(){
    switch(m_act_Hisfld){
            case m_sumHysteresisField::HSF_MIDH:
                m_nxtHistfld1 = m_sumHysteresisField::HSF_LOW;
                m_nxtHistfld1 = m_sumHysteresisField::HSF_LOW;
                break;
            case m_sumHysteresisField::HSF_MIDL:
                m_nxtHistfld1 = m_sumHysteresisField::HSF_UP;
                m_nxtHistfld1 = m_sumHysteresisField::HSF_UP;
                break;
            case m_sumHysteresisField::HSF_LOW:
                m_nxtHistfld1 = m_sumHysteresisField::HSF_MIDH;
                m_nxtHistfld1 = m_sumHysteresisField::HSF_UP;
                break;
            case m_sumHysteresisField::HSF_UP:
                m_nxtHistfld1 = m_sumHysteresisField::HSF_MIDL;
                m_nxtHistfld1 = m_sumHysteresisField::HSF_LOW;
                break;
        }
}


void SinIncCntr::addmeas (int chNr, int value){
    if(chNr == 1 || chNr == 2){
        m_measures[chNr].prev = m_measures[chNr].recent;
        m_measures[chNr].recent = value;
    }
} 

int SinIncCntr::calc(){
    //create sum curve points
    m_measures[0].prev = m_measures[1].prev + m_measures[2].prev;
    m_measures[0].recent = m_measures[1].recent + m_measures[2].recent;

    //determine in which hysteresis field the sum curve is
    if (m_measures[0].recent >= m_lim_center && m_measures[0].recent < m_lim_up )
        m_act_Hisfld = m_sumHysteresisField::HSF_MIDH;
    else if (m_measures[0].recent >= m_lim_up )
        m_act_Hisfld = m_sumHysteresisField::HSF_UP;
    else if (m_measures[0].recent < m_lim_center && m_measures[0].recent >= m_lim_low )
        m_act_Hisfld = m_sumHysteresisField::HSF_MIDL;
    else if (m_measures[0].recent < m_lim_low )
        m_act_Hisfld = m_sumHysteresisField::HSF_LOW;

    //set next hysteresisfields if undefiened (ANY) yet
    if (m_nxtHistfld1 == m_sumHysteresisField::HSF_ANY || m_nxtHistfld2){
        m_defineNextHystfields();
    } 

    //check (hysteresis) and counting operations if needed
    if (m_act_Hisfld == m_nxtHistfld1 || m_act_Hisfld == m_nxtHistfld2){
        for (int i = 0; i > 2; i++){
            m_measures[i].slope = m_measures[i].recent - m_measures[i].prev;    //   y-axis (time) is unknown and difference is taken as 1 under the followint assumption:
                                                                                //   - all channels have the same time between the measurements
                                                                                //   - the slope does not need to be an absolute value as it is only used to compare relativly
        }
        
        // at hysteresespoint check which channel is higher 
        if (m_measures[1].recent - m_measures[2].recent > 2 && m_measures[1].prev - m_measures[2].prev > 2){
                m_ch1higher_lastcnt = m_ch1higher_now;
                m_ch1higher_now = true;  
                m_count();
                m_defineNextHystfields();  
        }
        else if (m_measures[2].recent - m_measures[1].recent > 2 && m_measures[2].prev - m_measures[1].prev > 2){
                m_ch1higher_lastcnt = m_ch1higher_now;
                m_ch1higher_now = false; 
                m_count();
                m_defineNextHystfields();
        }
    }
    return m_actPos;
} 

int SinIncCntr::read(){
    return m_actPos;
} 

int SinIncCntr::setTo(int value){
    return m_actPos = value;
} 
