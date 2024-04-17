// Lab9Main.c
// Runs on MSPM0G3507
// Lab 9 ECE319K
// Your name
// Last Modified: 12/31/2023

#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "../inc/DAC5.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"
// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz
void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}

uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}

int8_t lostGameFlag = 0;
int8_t updateMapFlag = 0;
int8_t updateBlueFlag = 0;
int8_t updateRedFlag = 0;
int8_t redCaptured = 0;

int32_t sliderY = 0;

uint32_t switchA = 0;
uint32_t switchB = 0;
uint32_t switchC = 0;
uint32_t switchD = 0;

//direction - 0 is up, 1 is right, 2 is down, 3 is left
struct sprite {
    int16_t characterX;
    int16_t characterY;
    const uint16_t *img;
    int16_t trailColor;
    uint8_t direction;
    uint16_t terrTopLeftX;
    uint16_t terrTopLeftY;
    uint16_t terrBotLeftX;
    uint16_t terrBotLeftY;
    uint16_t terrBotRightX;
    uint16_t terrBotRightY;
    uint16_t terrTopRightX;
    uint16_t terrTopRightY;
};
typedef struct sprite sprite_t;

sprite_t blueSprite = {111, 17, blueChar, 0x6f3f, 2, 98, 0, 98, 30, 128, 30, 128, 0};
sprite_t redSprite = {17, 17, redChar, 0xfcaf, 2, 0, 0, 0, 30, 30, 30, 30, 0};

int16_t redPossibleCaptureX[640];
int16_t redPossibleCaptureY[640];

int16_t redCapturedTerritoryX[720];
int16_t redCapturedTerritoryY[720];

int16_t redCapIndex = 0;
int redTerritoryIndex = 0;

int8_t redLeftTerritory = 0;

int8_t showRedTrail = 0;

// games  engine runs at 30Hz
void TIMG12_IRQHandler(void){
    uint32_t pos,msg;
    if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
        GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
        GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    // game engine goes here
        // 1) sample slide pot

        uint32_t ADC2input = ADC2in();
        sliderY = Convert2(ADC2input);

        switchA = Switch_InA();
        switchB = Switch_InB();
        switchC = Switch_InC();
        switchD = Switch_InD();

        if(switchA) {
            redSprite.direction = 0;
        }
        if(switchD) {
            redSprite.direction = 1;
        }
        if(switchC) {
            redSprite.direction = 2;
        }
        if(switchB) {
            redSprite.direction = 3;
        }

        switch(redSprite.direction) {
            case 0:
                if(redSprite.characterY - 1 > 7) {
                    redSprite.characterY -= 1;
                    updateRedFlag = 1;
                }
                break;
            case 1:
                if(redSprite.characterX + 1 < 121) {
                    redSprite.characterX += 1;
                    updateRedFlag = 1;
                }
                break;
            case 2:
                if(redSprite.characterY + 1 < 160) {
                    redSprite.characterY += 1;
                    updateRedFlag = 1;
                }
                break;
            case 3:
                if(redSprite.characterX - 1 > 0) {
                    redSprite.characterX -= 1;
                    updateRedFlag = 1;
                }
                break;
        }

        //if(paperiomap[(120 * redSprite.characterY) + redSprite.characterX] == 0x0000) {
        if(!(redSprite.characterX <= 38 && redSprite.characterY <= 38)) {
            redPossibleCaptureX[redCapIndex] = redSprite.characterX;
            redPossibleCaptureY[redCapIndex] = redSprite.characterY;
            redCapIndex++;
            redLeftTerritory = 1;
            showRedTrail = 1;
        }

        short colorSelected = paperiomap[(128 * 160) + 0];

        //if(colorSelected == 0x20FD) {
        if(redSprite.characterX <= redSprite.terrBotRightX && redSprite.characterX <= redSprite.terrTopRightX && redSprite.characterX >= redSprite.terrBotLeftX){
            if(redSprite.characterY <= redSprite.terrBotRightY && redSprite.characterY <= redSprite.terrBotLeftY && redSprite.characterY > redSprite.terrTopRightY){

                if(redLeftTerritory) {
                    redCaptured = 1;
                    redLeftTerritory = 0;
                }
                showRedTrail = 0;
            }
        }


        // 2) read input switches

        // 3) move sprites
        // 4) start sounds
        // 5) set semaphore
        // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
        GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    }
}
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";
const char *Phrases[3][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French}
};

// use main1 to observe special characters
int main1(void){ // main1
    char l;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
  ST7735_FillScreen(0x0000);            // set screen to black
  for(phrase_t myPhrase=HELLO; myPhrase<= GOODBYE; myPhrase++){
    for(Language_t myL=English; myL<= French; myL++){
         ST7735_OutString((char *)Phrases[LANGUAGE][myL]);
      ST7735_OutChar(' ');
         ST7735_OutString((char *)Phrases[myPhrase][myL]);
      ST7735_OutChar(13);
    }
  }
  Clock_Delay1ms(3000);
  ST7735_FillScreen(0x0000);       // set screen to black
  l = 128;
  while(1){
    Clock_Delay1ms(2000);
    for(int j=0; j < 3; j++){
      for(int i=0;i<16;i++){
        ST7735_SetCursor(7*j+0,i);
        ST7735_OutUDec(l);
        ST7735_OutChar(' ');
        ST7735_OutChar(' ');
        ST7735_SetCursor(7*j+4,i);
        ST7735_OutChar(l);
        l++;
      }
    }
  }
}

// use main2 to observe graphics
int main2(void){ // main2
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_DrawBitmap(22, 159, PlayerShip0, 18,8); // player ship bottom
  ST7735_DrawBitmap(53, 151, Bunker0, 18,5);
  ST7735_DrawBitmap(42, 159, PlayerShip1, 18,8); // player ship bottom
  ST7735_DrawBitmap(62, 159, PlayerShip2, 18,8); // player ship bottom
  ST7735_DrawBitmap(82, 159, PlayerShip3, 18,8); // player ship bottom
  ST7735_DrawBitmap(0, 9, SmallEnemy10pointA, 16,10);
  ST7735_DrawBitmap(20,9, SmallEnemy10pointB, 16,10);
  ST7735_DrawBitmap(40, 9, SmallEnemy20pointA, 16,10);
  ST7735_DrawBitmap(60, 9, SmallEnemy20pointB, 16,10);
  ST7735_DrawBitmap(80, 9, SmallEnemy30pointA, 16,10);

  for(uint32_t t=500;t>0;t=t-5){
    SmallFont_OutVertical(t,104,6); // top left
    Clock_Delay1ms(50);              // delay 50 msec
  }
  ST7735_FillScreen(0x0000);   // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_OutString("Nice try,");
  ST7735_SetCursor(1, 3);
  ST7735_OutString("Earthling!");
  ST7735_SetCursor(2, 4);
  ST7735_OutUDec(1234);
  while(1){
  }
}

// use main3 to test switches and LEDs
int main3(void){ // main3
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  ST7735_FillScreen(ST7735_BLACK);

  int refChar = 0;
  while(1){
    switchA = Switch_InA();
    switchB = Switch_InB();
    switchC = Switch_InC();
    switchD = Switch_InD();

  }
}
// use main4 to test sound outputs
int main4(void){ uint32_t last=0,now;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(ADC0,6,0); // ADC1 channel 6 is PB20, TExaS scope
  __enable_irq();
  while(1){
    now = Switch_InA(); // one of your buttons
    if((last == 0)&&(now == 1)){
      Sound_Shoot(); // call one of your sounds
    }
    if((last == 0)&&(now == 2)){
      Sound_Killed(); // call one of your sounds
    }
    if((last == 0)&&(now == 4)){
      Sound_Explosion(); // call one of your sounds
    }
    if((last == 0)&&(now == 8)){
      Sound_Fastinvader1(); // call one of your sounds
    }
    last = now;
    // modify this to test all your sounds
  }
}
// ALL ST7735 OUTPUT MUST OCCUR IN MAIN
int main(void){ // final main
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  ST7735_FillScreen(ST7735_BLACK);
  ADC1init();     //PB18 = ADC1 channel 5, slidepot
  ADC2init();
  Switch_Init(); // initialize switches
  LED_Init();    // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(0,0,&TExaS_LaunchPadLogicPB27PB26); // PB27 and PB26
    // initialize interrupts on TimerG12 at 30 Hz
  TimerG12_IntArm(80000000/30,2);
  // initialize all data structures
  __enable_irq();

  ST7735_DrawBitmap(0, 160, paperiomap, 128 , 160);
  ST7735_DrawBitmap(redSprite.characterX, redSprite.characterY, redSprite.img, 8, 8);

  while(1){
        if(updateRedFlag) {
            ST7735_DrawBitmap(redSprite.characterX, redSprite.characterY, redSprite.img, 8, 8);
            if(showRedTrail) {
                switch(redSprite.direction) {
                    case 0:
                        ST7735_DrawBitmap(redSprite.characterX, redSprite.characterY + 1, pinkTrail, 8, 1);
                        break;
                    case 1:
                        ST7735_DrawBitmap(redSprite.characterX - 1, redSprite.characterY, pinkTrail, 1, 8);
                        break;
                    case 2:
                        ST7735_DrawBitmap(redSprite.characterX, redSprite.characterY - 8, pinkTrail, 8, 1);
                        break;
                    case 3:
                        ST7735_DrawBitmap(redSprite.characterX + 8, redSprite.characterY, pinkTrail, 1, 8);
                        break;
                }
            }

            updateRedFlag = 0;
        }
        if(redCaptured) {
            for(int i = 0; i <= redCapIndex; i++) {
                ST7735_DrawBitmap(redPossibleCaptureX[i], redPossibleCaptureY[i], redFill, 1, 8);
                ST7735_DrawBitmap(redPossibleCaptureX[i], redPossibleCaptureY[i], redFill, 8, 1);
//                redCapturedTerritoryX[redTerritoryIndex] = redPossibleCaptureX[i];
//                redCapturedTerritoryY[redTerritoryIndex] = redPossibleCaptureY[i];
//                redTerritoryIndex++;
            }
            redCaptured = 0;
            redCapIndex = 0;
        }
    // wait for semaphore
       // clear semaphore
       // update ST7735R
    // check for end game or level switch
  }
}
