/*
 * Switch.c
 *
 *  Created on: Nov 5, 2023
 *      Author:
 */
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
// LaunchPad.h defines all the indices into the PINCM table
void Switch_Init(void){
    IOMUX->SECCFG.PINCM[PB20INDEX] = 0x00040081;
    IOMUX->SECCFG.PINCM[PB16INDEX] = 0x00040081;
    IOMUX->SECCFG.PINCM[PB17INDEX] = 0x00040081;
    IOMUX->SECCFG.PINCM[PB19INDEX] = 0x00040081;
    // write this
  
}
// return current state of Switch A
uint32_t Switch_InA(void){
    // write this
    return ((GPIOB->DIN31_0 & (1<<16)) >> 16);
}

// return current state of switch B
uint32_t Switch_InB(void){
    // write this
    return ((GPIOB->DIN31_0 & (1<<17)) >> 17);
}

// return current state of switch C
uint32_t Switch_InC(void){
    // write this
    return ((GPIOB->DIN31_0 & (1<<19)) >> 19);
}

// return current state of switch D
uint32_t Switch_InD(void){
    // write this
    return ((GPIOB->DIN31_0 & (1<<20)) >> 20);
}
