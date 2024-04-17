/* ADC0.c
 * Students put your names here
 * Modified: put the date here
 * 12-bit ADC input on ADC0 channel 5, PB18
 */
#include <ti/devices/msp/msp.h>
#include "../inc/Clock.h"
#define ADCVREF_VDDA 0x000
#define ADCVREF_INT  0x200



void ADC2init(void){
      ADC0->ULLMEM.GPRCM.RSTCTL = 0xB1000003; // 1) reset
      ADC0->ULLMEM.GPRCM.PWREN = 0x26000001;  // 2) activate
      Clock_Delay(24);                        // 3) wait
      ADC0->ULLMEM.GPRCM.CLKCFG = 0xA9000000; // 4) ULPCLK
      ADC0->ULLMEM.CLKFREQ = 7;               // 5) 40-48 MHz
      ADC0->ULLMEM.CTL0 = 0x03010000;         // 6) divide by 8
      ADC0->ULLMEM.CTL1 = 0x00000000;         // 7) mode
      ADC0->ULLMEM.CTL2 = 0x00000000;         // 8) MEMRES
      ADC0->ULLMEM.MEMCTL[0] = 5;             // 9) channel 5 is PB24
      ADC0->ULLMEM.SCOMP0 = 0;                // 10) 8 sample clocks
      ADC0->ULLMEM.CPU_INT.IMASK = 0;         // 11) no interrupt
// write code to initialize ADC0 channel 5, PB18
// Your measurement will be connected to PB18
// 12-bit mode, 0 to 3.3V, right justified
// software trigger, no averaging
  
}
uint32_t ADC2in(void){
      ADC0->ULLMEM.CTL0 |= 0x00000001;             // 1) enable conversions
      ADC0->ULLMEM.CTL1 |= 0x00000100;             // 2) start ADC
      uint32_t volatile delay=ADC0->ULLMEM.STATUS; // 3) time to let ADC start
      while((ADC0->ULLMEM.STATUS&0x01)==0x01){}    // 4) wait for completion
      return ADC0->ULLMEM.MEMRES[0];               // 5) 12-bit result
}

// your function to convert ADC sample to distance (0.001cm)
// use main2 to calibrate the system fill in 5 points in Calibration.xls
//    determine constants k1 k2 to fit Position=(k1*Data + k2)>>12
uint32_t Convert2(uint32_t input){
    int32_t result = ((1918 * input)>>12) + 121;
    if(result < 0) {
        result = 0;
    }

    //int32_t result = ((32 * 16 * (input - 2048))/2048);
    result = (result - 1000);

    if(result < 0) {
        result = 0;
    }

    if(result > 0) {
        result = 1;
    }

  return result; // replace this with a linear function
}

// do not use this function
// it is added just to show you how SLOW floating point in on a Cortex M0+
float FloatConvert2(uint32_t input){
  return 0.00048828125*input -0.0001812345;
}

