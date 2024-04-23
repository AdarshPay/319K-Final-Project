// Sound.c

// Runs on MSPM0

// Sound assets in sounds/sounds.h

// Jonathan Valvano

// 11/15/2021 

#include <stdint.h>

#include <ti/devices/msp/msp.h>

#include "Sound.h"

#include "sounds/sounds.h"

#include "../inc/DAC5.h"

#include "../inc/Timer.h"



volatile uint8_t *daPointer;

volatile uint32_t ArrayCount;

void SysTick_IntArm(uint32_t period, uint32_t priority){

  // write this

    SysTick->CTRL = 0x00; //disable during init

    SysTick->LOAD = period-1;

    SCB->SHP[1] = (SCB->SHP[1]&(~0xC0000000))|priority<<30;

    SysTick->VAL = 0;

    SysTick->CTRL = 0x07;//enable



}

// initialize a 11kHz SysTick, however no sound should be started

// initialize any global variables

// Initialize the 5 bit DAC

void Sound_Init(void){

// write this

    SysTick_IntArm(9090,2);

    DAC5_Init();

    SysTick->CTRL = 0;

  

}

void SysTick_Handler(void){ // called at 11 kHz

  // output one value to DAC if a sound is active



    if(ArrayCount){

        ArrayCount--;

        DAC5_Out (*daPointer);

        daPointer++;

    }

    else{

        SysTick->CTRL = 0;

    }

    

}



//******* Sound_Start ************

// This function does not output to the DAC. 

// Rather, it sets a pointer and counter, and then enables the SysTick interrupt.

// It starts the sound, and the SysTick ISR does the output

// feel free to change the parameters

// Sound should play once and stop

// Input: pt is a pointer to an array of DAC outputs

//        count is the length of the array

// Output: none

// special cases: as you wish to implement

void Sound_Start(const uint8_t *pt, uint32_t count){

// write this

    daPointer = pt;

    ArrayCount = count;

    SysTick->VAL = 0;

    SysTick->CTRL = 0x07;

  

}



void Sound_Shoot(void){

// write this

    GPIOB->DOUTSET31_0 = (1<<27);

    Sound_Start(penisbutt, 2300);

    GPIOB->DOUTCLR31_0 = (1<<27);

}

void Sound_Killed(void){

// write this

  Sound_Start(buttpenis, 2600);

}

void Sound_Explosion(void){

// write this

 

}



void Sound_Fastinvader1(void){

  

}

void Sound_Fastinvader2(void){

  

}

void Sound_Fastinvader3(void){

  

}

void Sound_Fastinvader4(void){

  

}

void Sound_Highpitch(void){

  

}
