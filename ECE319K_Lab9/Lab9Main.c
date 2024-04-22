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
#include <math.h>
#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)
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

int8_t redWin = 0;
int8_t blueWin = 0;
int8_t updateMapFlag = 0;
int8_t updateBlueFlag = 0;
int8_t updateRedFlag = 0;
int8_t redCaptured = 0;
int8_t blueCaptured = 0;
int8_t onIntroScreen = 1;
int8_t isEnglish = 1;

uint32_t redPixelCount = 4800;
uint32_t bluePixelCount = 4800;

int32_t sliderY = 0;

uint32_t switchA = 0;
uint32_t switchB = 0;
uint32_t switchC = 0;
uint32_t switchD = 0;

uint32_t switchE = 0;
uint32_t switchF = 0;
uint32_t switchG = 0;
uint32_t switchH = 0;

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

struct Point {
    uint16_t x;
    uint16_t y;
};
typedef struct Point Point_t;

sprite_t blueSprite = {111, 17, blueChar, 0x6f3f, 2, 98, 0, 98, 30, 128, 30, 128, 0};
sprite_t redSprite = {17, 17, redChar, 0xfcaf, 2, 0, 0, 0, 30, 30, 30, 30, 0};

//int16_t redPossibleCaptureX[640];
//int16_t redPossibleCaptureY[640];

Point_t redPossibleCapture[640];
Point_t bluePossibleCapture[640];

Point_t redCapturedTerritory[960];
Point_t blueCapturedTerritory[960];

int16_t redCapIndex = 4;
int16_t blueCapIndex = 4;
int16_t prevRedCapIndex = 4;
int16_t prevBlueCapIndex = 4;

int16_t redPossCapIndex = 0;
int16_t bluePossCapIndex = 0;
int redTerritoryIndex = 0;
int blueTerritoryIndex = 0;

int8_t redLeftTerritory = 0;
int8_t blueLeftTerritory = 0;

int8_t showRedTrail = 0;
int8_t showBlueTrail = 0;

int crossProduct(Point_t p1, Point_t p2, Point_t p3) {
    return (p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y);
}

// Function to compute the orientation of three points (p, q, r)
int orientation(Point_t p, Point_t q, Point_t r) {
    int val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (val == 0) return 0;  // colinear
    return (val > 0) ? 1 : 2; // clock or counterclock wise
}

// whichArray tells which array to add to, 0 - redCapturedTerritory, 1 - blueCaptured Territory
void convexHull(Point_t points[], int n, int8_t whichArray) {
    // If the number of points is less than 3, convex hull is not possible
    if (n < 3) return;

    int leftmost = 0;
    for (int i = 1; i < n; i++) {
        if (points[i].x < points[leftmost].x) {
            leftmost = i;
        }
    }

    int p = leftmost, q;
    do {
        switch(whichArray) {
            case 0:
                redCapturedTerritory[redCapIndex] = points[p];
                redCapIndex++;
                break;
            case 1:
                blueCapturedTerritory[blueCapIndex] = points[p];
                blueCapIndex++;
        }


        // Find the next Point_t q such that the triplet (p, q, r) is counterclockwise
        q = (p + 1) % n;
        for (int r = 0; r < n; r++) {
            if (orientation(points[p], points[r], points[q]) == 2) {
                q = r;
            }
        }

        // Set q as the next Point_t for the next iteration
        p = q;

    } while (p != leftmost);
}

bool isInsideConvexHull(Point_t convexHull[], int hullSize, Point_t p) {
    // Check if the point is inside the convex hull using winding number algorithm
    int windingNumber = 0;
    for (int i = 0; i < hullSize; i++) {
        int next = (i + 1) % hullSize;
        if (convexHull[i].y <= p.y) {
            if (convexHull[next].y > p.y && crossProduct(convexHull[i], convexHull[next], p) > 0) {
                windingNumber++;
            }
        } else {
            if (convexHull[next].y <= p.y && crossProduct(convexHull[i], convexHull[next], p) < 0) {
                windingNumber--;
            }
        }
    }
    return windingNumber != 0;
}

// Function to fill a polygon given its CapturedTerritory
void fillPolygon(int n, int8_t color) {
    switch(color) {
        case 0:
            for(int i = redCapturedTerritory[0].x; i < 128; i += 8) {
                for(int j = redCapturedTerritory[0].y; j < 160; j += 8) {
                    Point_t pixelLoc = {i, j};
                    if(isInsideConvexHull(redCapturedTerritory, redCapIndex, pixelLoc)) {
                        ST7735_DrawBitmap(redSprite.characterX, redSprite.characterY, redSprite.img, 8, 8);
                        ST7735_DrawBitmap(blueSprite.characterX, blueSprite.characterY, blueSprite.img, 8, 8);
                        ST7735_DrawBitmap(i, j, redFill, 1, 8);
                        ST7735_DrawBitmap(i, j, redFill, 8, 1);
                        redPixelCount += 64;
//                        ST7735_DrawPixel(i, j, 0x20FD);
                    }
                }
            }
            break;
        case 1:
            for(int i = 0; i < 128; i += 8) {
                for(int j = 0; j < 160; j += 8) {
                    Point_t pixelLoc = {i, j};
                    if(isInsideConvexHull(blueCapturedTerritory, blueCapIndex, pixelLoc)) {
                        ST7735_DrawBitmap(blueSprite.characterX, blueSprite.characterY, blueSprite.img, 8, 8);
                        ST7735_DrawBitmap(redSprite.characterX, redSprite.characterY, redSprite.img, 8, 8);
                        ST7735_DrawBitmap(i, j, blueFill, 1, 8);
                        ST7735_DrawBitmap(i, j, blueFill, 8, 1);
                        bluePixelCount += 64;
//                        ST7735_DrawPixel(i, j, 0x20FD);
                    }
                }
            }
            break;
//            if(redPixelCount > 35000) {
//                redWin = 1;
//            }
    }
}

// games  engine runs at 30Hz
void TIMG12_IRQHandler(void){
    uint32_t pos,msg;
    if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
        GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
        GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    // game engine goes here
        // 1) sample slide pot

        if(!onIntroScreen && !redWin && !blueWin){
            if(!redCaptured){
                switchA = Switch_InA();
                switchB = Switch_InB();
                switchC = Switch_InC();
                switchD = Switch_InD();

                if(switchA) {
                    if(redSprite.direction != 2) {
                        redSprite.direction = 0;
                    }
                }
                if(switchD) {
                    if(redSprite.direction != 3) {
                        redSprite.direction = 1;
                    }
                }
                if(switchC) {
                    if(redSprite.direction != 0) {
                        redSprite.direction = 2;
                    }
                }
                if(switchB) {
                    if(redSprite.direction != 1) {
                        redSprite.direction = 3;
                    }
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

                int8_t inRedTerr = 0;
                Point_t redCharLoc = {redSprite.characterX, redSprite.characterY};
                inRedTerr = isInsideConvexHull(redCapturedTerritory, redCapIndex, redCharLoc);

                if(!inRedTerr){
                    redPossibleCapture[redPossCapIndex].x = redSprite.characterX;
                    redPossibleCapture[redPossCapIndex].y = redSprite.characterY;
                    Point_t redCharLoc = {redSprite.characterX, redSprite.characterY};
                    redCapturedTerritory[redCapIndex] = redCharLoc;
                    redPossCapIndex++;
                    redLeftTerritory = 1;
                    showRedTrail = 1;

                }
                if(inRedTerr == 1){
                    if(redLeftTerritory) {
                        redCaptured = 1;
                        redLeftTerritory = 0;
                    }
                    showRedTrail = 0;
                }
            }

            if(!blueCaptured){
                switchE = Switch_InE();
                switchF = Switch_InF();
                switchG = Switch_InG();
                switchH = Switch_InH();

                if(switchE) {
                    if(blueSprite.direction != 2) {
                        blueSprite.direction = 0;
                    }
                }
                if(switchF) {
                    if(blueSprite.direction != 3) {
                        blueSprite.direction = 1;
                    }
                }
                if(switchG) {
                    if(blueSprite.direction != 0) {
                        blueSprite.direction = 2;
                    }
                }
                if(switchH) {
                    if(blueSprite.direction != 1) {
                        blueSprite.direction = 3;
                    }
                }

                switch(blueSprite.direction) {
                    case 0:
                        if(blueSprite.characterY - 1 > 7) {
                            blueSprite.characterY -= 1;
                            updateBlueFlag = 1;
                        }
                        break;
                    case 1:
                        if(blueSprite.characterX + 1 < 121) {
                            blueSprite.characterX += 1;
                            updateBlueFlag = 1;
                        }
                        break;
                    case 2:
                        if(blueSprite.characterY + 1 < 160) {
                            blueSprite.characterY += 1;
                            updateBlueFlag = 1;
                        }
                        break;
                    case 3:
                        if(blueSprite.characterX - 1 > 0) {
                            blueSprite.characterX -= 1;
                            updateBlueFlag = 1;
                        }
                        break;
                }

                int8_t inBlueTerr = 0;
                Point_t blueCharLoc = {blueSprite.characterX, blueSprite.characterY};
                inBlueTerr = isInsideConvexHull(blueCapturedTerritory, blueCapIndex, blueCharLoc);

                if(!inBlueTerr){
                    bluePossibleCapture[bluePossCapIndex].x = blueSprite.characterX;
                    bluePossibleCapture[bluePossCapIndex].y = blueSprite.characterY;
                    Point_t blueCharLoc = {blueSprite.characterX, blueSprite.characterY};
                    blueCapturedTerritory[blueCapIndex] = blueCharLoc;
                    bluePossCapIndex++;
                    blueLeftTerritory = 1;
                    showBlueTrail = 1;

                }
                if(inBlueTerr == 1){
                    if(blueLeftTerritory) {
                        blueCaptured = 1;
                        blueLeftTerritory = 0;
                    }
                    showBlueTrail = 0;
                }
            }

        }
        // 4) start sounds

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
  ST7735_DrawBitmap(blueSprite.characterX, blueSprite.characterY, blueSprite.img, 8, 8);

  redCapturedTerritory[0].x = 0;
  redCapturedTerritory[0].y = 0;
  redCapturedTerritory[1].x = 30;
  redCapturedTerritory[1].y = 0;
  redCapturedTerritory[2].x = 30;
  redCapturedTerritory[2].y = 160;
  redCapturedTerritory[3].x = 0;
  redCapturedTerritory[3].y = 160;

  blueCapturedTerritory[0].x = 90;
  blueCapturedTerritory[0].y = 0;
  blueCapturedTerritory[1].x = 120;
  blueCapturedTerritory[1].y = 0;
  blueCapturedTerritory[2].x = 90;
  blueCapturedTerritory[2].y = 160;
  blueCapturedTerritory[3].x = 120;
  blueCapturedTerritory[3].y = 160;

  uint32_t ADC2input = ADC2in();
  sliderY = Convert2(ADC2input);
  uint32_t switchA = Switch_InA();

  while(1){
          if(onIntroScreen) {
                ST7735_DrawBitmap(0, 160, Startbackground, 128 , 160);
                ST7735_DrawBitmap(7, 120, languageSelect, 40 , 40);
                while(onIntroScreen){
                    ST7735_DrawBitmap(83, 22, crown, 18 , 8);
                    for(uint32_t delay = 0; delay < 500000; delay++){
                    }
                    ADC2input = ADC2in();
                    sliderY = Convert2(ADC2input);
                    if(sliderY == 1){
                        ST7735_DrawBitmap(0, 160, startButton, 128 , 35);
                        ST7735_DrawBitmap(0, 114, yellowselector, 5 , 20);
                        isEnglish = 1;
                    }

                    if(sliderY == 0){
                        ST7735_DrawBitmap(0, 114, lightBlue, 5 , 20);
                        ST7735_DrawBitmap(0, 160, pigLatinStart, 128 , 35);
                        ST7735_DrawBitmap(0, 124, yellowselector, 5 , 20);
                        isEnglish = 0;
                    }

                    switchA = Switch_InA();
                    if(switchA == 1) {
                        onIntroScreen = 0;
                        ST7735_DrawBitmap(0, 160, paperiomap, 128 , 160);
                        redSprite.direction = 2;
                        blueSprite.direction = 2;
                    }
                }
              }
      if(!onIntroScreen) {
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
            }
            if(redCaptured) {
                for(int i = 0; i <= redPossCapIndex; i++) {
                    ST7735_DrawBitmap(redPossibleCapture[i].x, redPossibleCapture[i].y, redFill, 1, 8);
                    ST7735_DrawBitmap(redPossibleCapture[i].x, redPossibleCapture[i].y, redFill, 8, 1);
                }
                prevRedCapIndex = redCapIndex;
                convexHull(redPossibleCapture, redPossCapIndex, 0);
                fillPolygon(redCapIndex, 0);
                redCaptured = 0;
                redPossCapIndex = 0;
            }

            if(updateBlueFlag) {
                ST7735_DrawBitmap(blueSprite.characterX, blueSprite.characterY, blueSprite.img, 8, 8);
                if(showBlueTrail) {
                    switch(blueSprite.direction) {
                        case 0:
                            ST7735_DrawBitmap(blueSprite.characterX, blueSprite.characterY + 1, blueTrail, 8, 1);
                            break;
                        case 1:
                            ST7735_DrawBitmap(blueSprite.characterX - 1, blueSprite.characterY, blueTrail, 1, 8);
                            break;
                        case 2:
                            ST7735_DrawBitmap(blueSprite.characterX, blueSprite.characterY - 8, blueTrail, 8, 1);
                            break;
                        case 3:
                            ST7735_DrawBitmap(blueSprite.characterX + 8, blueSprite.characterY, blueTrail, 1, 8);
                            break;
                    }
                }
            }
            if(blueCaptured) {
                for(int i = 0; i <= bluePossCapIndex; i++) {
                    ST7735_DrawBitmap(bluePossibleCapture[i].x, bluePossibleCapture[i].y, blueFill, 1, 8);
                    ST7735_DrawBitmap(bluePossibleCapture[i].x, bluePossibleCapture[i].y, blueFill, 8, 1);
                }
                prevBlueCapIndex = blueCapIndex;
                convexHull(bluePossibleCapture, bluePossCapIndex, 1);
                fillPolygon(blueCapIndex, 1);
                blueCaptured = 0;
                bluePossCapIndex = 0;
            }
      }
    // wait for semaphore
       // clear semaphore
       // update ST7735R
    // check for end game or level switch
  }
}
