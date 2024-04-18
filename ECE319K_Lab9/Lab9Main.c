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

Point_t redCapturedTerritory[720];

Point_t rTL = {0, 0};
//rTL.x = 0;
//rTl.y = 0;
Point_t rBL = {0, 30};
//rBL.x = 0;
//rBL.y = 30;
Point_t rTR = {30, 0};
//rTR.x = 30;
//rTR.y = 0;
Point_t rBR = {30, 30};
//rBR.x = 30;
//rBR.y = 30;

//redCapturedTerritory[0].x = 0;
//redCapturedTerritory[0].y = 0;
//redCapturedTerritory[1] = rBR;
//redCapturedTerritory[2] = rTR;
//redCapturedTerritory[3] = rTL;

int16_t redCapIndex = 4;

int16_t redPossCapIndex = 0;
int redTerritoryIndex = 0;

int8_t redLeftTerritory = 0;

int8_t showRedTrail = 0;

int crossProduct(Point_t p1, Point_t p2, Point_t p3) {
    return (p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y);
}

// Function to compute the orientation of three points (p, q, r)
int orientation(Point_t p, Point_t q, Point_t r) {
    int val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (val == 0) return 0;  // colinear
    return (val > 0) ? 1 : 2; // clock or counterclock wise
}

// Function to compute the convex hull of a set of points using Jarvis's algorithm
void convexHull(Point_t points[], int n) {
    // If the number of points is less than 3, convex hull is not possible
    if (n < 3) return;

    // Initialize the result array to store convex hull points
    //int hullIndex = 0;

    // Find the leftmost point
    int leftmost = 0;
    for (int i = 1; i < n; i++) {
        if (points[i].x < points[leftmost].x) {
            leftmost = i;
        }
    }

    // Start from the leftmost Point_t and keep moving counterclockwise
    // until we reach the start Point_t again
    int p = leftmost, q;
    do {
        // Add current Point_t to result
        redCapturedTerritory[redCapIndex] = points[p];
        redCapIndex++;

        // Find the next Point_t q such that the triplet (p, q, r) is counterclockwise
        q = (p + 1) % n;
        for (int r = 0; r < n; r++) {
            if (orientation(points[p], points[r], points[q]) == 2) {
                q = r;
            }
        }

        // Set q as the next Point_t for the next iteration
        p = q;

    } while (p != leftmost); // Continue until we reach the start Point_t again

//    numOfVertices = hullIndex;
//    return numOfVertices;
}

int InsidePolygon(Point_t polygon[], int N, Point_t p)
{
  int counter = 0;
  int i;
  double xinters;
  Point_t p1,p2;

  p1 = polygon[0];
  for (i=1;i<=N;i++) {
    p2 = polygon[i % N];
    if (p.y > MIN(p1.y,p2.y)) {
      if (p.y <= MAX(p1.y,p2.y)) {
        if (p.x <= MAX(p1.x,p2.x)) {
          if (p1.y != p2.y) {
            xinters = (p.y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
            if (p1.x == p2.x || p.x <= xinters)
              counter++;
          }
        }
      }
    }
    p1 = p2;
  }

  if (counter % 2 == 0)
    return 0;
  else
    return 1;
}

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
            redPossibleCapture[redPossCapIndex].x = redSprite.characterX;
            redPossibleCapture[redPossCapIndex].y = redSprite.characterY;
            redPossCapIndex++;
            redLeftTerritory = 1;
            showRedTrail = 1;
        }

        int8_t inTerr = 0;

//        for(int i = 0; i < redCapIndex - 2; i++) {
//            if(redSprite.characterX > redCapturedTerritory[i].x && redSprite.characterY > redCapturedTerritory[i + 1].y && redSprite.characterY < redCapturedTerritory[i + 2].y) {
//                inTerr = 1;
//            }
//            if(redSprite.characterX < redCapturedTerritory[redCapIndex - i].x && redSprite.characterY > redCapturedTerritory[redCapIndex - i - 1].y && redSprite.characterY < redCapturedTerritory[redCapIndex - i - 2].y) {
//                inTerr = 1;
//            }

//            if(redSprite.characterX <= redSprite.terrBotRightX && redSprite.characterX <= redSprite.terrTopRightX && redSprite.characterX >= redSprite.terrBotLeftX){
//                if(redSprite.characterY <= redSprite.terrBotRightY && redSprite.characterY <= redSprite.terrBotLeftY && redSprite.characterY > redSprite.terrTopRightY){
//                    inTerr = 1;
//                }
//            }

            Point_t redCharLoc = {redSprite.characterX, redSprite.characterY};
            inTerr = InsidePolygon(redCapturedTerritory, redCapIndex, redCharLoc);

//        }

//        if(redSprite.characterX <= redSprite.terrBotRightX && redSprite.characterX <= redSprite.terrTopRightX && redSprite.characterX >= redSprite.terrBotLeftX){
//            if(redSprite.characterY <= redSprite.terrBotRightY && redSprite.characterY <= redSprite.terrBotLeftY && redSprite.characterY > redSprite.terrTopRightY){
          if(inTerr == 1){
                if(redLeftTerritory) {
                    redCaptured = 1;
                    redLeftTerritory = 0;
                }
                showRedTrail = 0;
//          }
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
  ST7735_DrawBitmap(blueSprite.characterX, blueSprite.characterY, blueSprite.img, 8, 8);

  redCapturedTerritory[0].x = 0;
  redCapturedTerritory[0].y = 30;
  redCapturedTerritory[1].x = 30;
  redCapturedTerritory[1].y = 30;
  redCapturedTerritory[2].x = 30;
  redCapturedTerritory[2].y = 0;
  redCapturedTerritory[3].x = 0;
  redCapturedTerritory[3].y = 0;

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
            for(int i = 0; i <= redPossCapIndex; i++) {
                ST7735_DrawBitmap(redPossibleCapture[i].x, redPossibleCapture[i].y, redFill, 1, 8);
                ST7735_DrawBitmap(redPossibleCapture[i].x, redPossibleCapture[i].y, redFill, 8, 1);

//                redCapturedTerritoryX[redTerritoryIndex] = redPossibleCaptureX[i];
//                redCapturedTerritoryY[redTerritoryIndex] = redPossibleCaptureY[i];
//                redTerritoryIndex++;
            }
            convexHull(redPossibleCapture, redPossCapIndex);
            redCaptured = 0;
            redPossCapIndex = 0;
        }
    // wait for semaphore
       // clear semaphore
       // update ST7735R
    // check for end game or level switch
  }
}
