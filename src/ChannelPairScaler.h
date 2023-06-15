#pragma once

#include <TeethMemory.h>


class CSinCosScaler{
    private:
        CTeethMemory TeethMem_CH1, TeethMem_CH2; 
        int m__actTeethIndexCH1, m__actTeethIndexCH2;

    public:
        CSinCosScaler();
        bool calculate (double CH1, double CH2, double &scaledCH1, double &scaledCH2);
};
