#include <Arduino.h>
#include <SPI.h>
//include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    16 // Reset pin # (or -1 if sharing Arduino reset pin), reset oled on gpio_16 with low
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define BTN_PIN   0 // jump dinosaur button <--> gpio0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include "images.h"
#include "myFuncs.h"

static bool ifJumpBtnPressed = false;
static bool gameover = false;



// ================ Global variables for this game logic ====================
const uint8_t jumpMostHeight = display.height() - DNSAUR_HEIGHT + 6;  //configurable var.

uint8_t dino_h = 0;  // the dino_h for dinosaur jumping coordinates. range:[0, display.height()-DNSAUR_HEIGHT+10]
int dx = 0;  // the delta_x coord for monster's moving , range[ -MNSTR_WIDTH--mostRight, display.width()--mostLeft ] 
             // dx increase from -MNSTR_WIDTH to display.width(), with respect to monster moving from RIGHT to LEFT
uint8_t mstrSpeed = 1;  // 2=slow  5=fast  10=crazy. keep this as ONE and regulate the beatTime2.
const uint8_t beatTime1 = 100;    // beat time for movment of bitMaps. const , init value is configurable 
const uint8_t beatT2init= 30;     // init beat time for mnstr moving left. 
uint8_t beatTime2 = beatT2init;     // beat time for mnstr moving left.
uint8_t beatTime3 = 60;     // beat time for dinosaur jumping.     not const , init value is configurable
uint16_t score = 0;
uint8_t bitMapFrame_idx = 0;
unsigned long lastMillis1 = millis();
unsigned long lastMillis2 = millis();
unsigned long lastMillis3 = millis();
// vars for control dinosaur jumping
static bool dinoGoingUp = true;
// <<<<<<<<<<<<< end of Global variables for this game logic <<<<<<<<<<<<<<<<



/////////////////////////////////////////////////////////////////////////
///////////////////////  user's function  ///////////////////////////////
/////////////////////////////////////////////////////////////////////////

// Handle the button push as an interrupt
void IRAM_ATTR buttonPush() {
  ifJumpBtnPressed = true;
  Serial.printf("\nbotton pressed. beatTime2=%d \n", beatTime2);
  // gameover = true;  // for testing the effect of game over. comment it out when running.
}


void dino_jump(){
  if(ifJumpBtnPressed){
    // jump the dinosaur by changing the dino_h
    if(dino_h < jumpMostHeight && dinoGoingUp == true){
      dino_h += 1; 
      if(dino_h == jumpMostHeight){
        dinoGoingUp = false;
      }
    }
    else{
      dino_h -= 1; 
      if(dino_h == 0){
        ifJumpBtnPressed = false;   // set ifJumpBtnPressed into false after finished.
        dinoGoingUp = true;
      }
    }
  }
}


/*
 * Here the ifCollision function has tech content. You can implement it in different ways.
 * ifCollision() monitors the global variables which are related to the location of dinosaur & monster
 * in this case, the two global variables dino_h and dx 
 * the possible ways to implement this function:
 *   1. Advantage way: do the matrix AND operation, if there is any ONE in the result, collision is true.
 *   2. set several rules to judge if they are collision.
 *   3. get pixle value around the monster. if any 1 in them, collision happens. 
 * Although the way3 is not the perfect, but it should be the most cheap way. So we choose the way3.
 * 
*/
bool ifCollision(){
  if(display.width()-MNSTR_WIDTH-dx <= 19+1){   // if monster enters the area of dinosaur, do judgement.
    // declare array for x,y of sense area that is surrounding the bitmap of monster.
    uint8_t x_sa[19] = {display.width()-MNSTR_WIDTH-dx-1,
                        display.width()-MNSTR_WIDTH-dx-1,
                        display.width()-MNSTR_WIDTH-dx-1,
                        display.width()-MNSTR_WIDTH-dx-1,
                        display.width()-MNSTR_WIDTH-dx-1,
                        display.width()-MNSTR_WIDTH-dx-1,
                        display.width()-MNSTR_WIDTH-dx-1,
                        display.width()-MNSTR_WIDTH-dx ,
                        display.width()-MNSTR_WIDTH-dx+1,
                        display.width()-MNSTR_WIDTH-dx+2,
                        display.width()-MNSTR_WIDTH-dx+3,      // x for the one on top_line left
                        display.width()-MNSTR_WIDTH-dx+4,
                        display.width()-MNSTR_WIDTH-dx+5,
                        display.width()-MNSTR_WIDTH-dx+6,
                        display.width()-MNSTR_WIDTH-dx+7,      // x for the one on top_line right
                        display.width()-MNSTR_WIDTH-dx+8,      
                        display.width()-MNSTR_WIDTH-dx+9,    
                        display.width()-MNSTR_WIDTH-dx+10,  
                        display.width()-MNSTR_WIDTH-dx+11};    // x for the one on right_line buttom
    
    uint8_t y_sa[19] = {display.height()-3,
                        display.height()-4,
                        display.height()-5,
                        display.height()-6,
                        display.height()-7,
                        display.height()-8,
                        display.height()-9,
                        display.height()-10,
                        display.height()-11,
                        display.height()-12,
                        display.height()-13,       // y for the one on top_line left
                        display.height()-13,
                        display.height()-13,
                        display.height()-13,
                        display.height()-13,       // y for the one on top_line right
                        display.height()-12,
                        display.height()-11,
                        display.height()-10,
                        display.height()-9};       // y for the one on right_line buttom
    // judging the collision here:
    for(int i=0; i<19; i++){
      if(display.getPixel(x_sa[i], y_sa[i]) == true ){
        return true;
      }
    }
    return false;
  }
  else{
    return false;
  }
}
/////////////////////////////////////////////////////////////////////////
///////////////////////  setup function  ////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);   // PRG button <--> gpio0
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), buttonPush, FALLING); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // display the banner when get started.
  display.clearDisplay();
  display.drawBitmap(0, 0, bannr_BMP_images, display.width(), display.height(), WHITE);
  display.display();
  delay(1500); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // testdrawbitmap();    // Draw a small bitmap image
  // testdrawMYbitmap();
  // // testanimate(logo_bmp, LOGO_WIDTH, LOGO_HEIGHT); // Animate bitmaps
  // testanimate(monsterBMP_12x12_frame_0, 12, 12); // Animate bitmaps

  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(display.width()/2, 0);

}


void loop() {
  //============= global highest control: if game is over ===========//
  if(gameover == true){
    // display.setTextSize(2); // Draw 2X-scale text
    // display.setCursor(14, 10);
    // display.setTextColor(SSD1306_WHITE);
    // display.print("Game Over!");
    display.drawBitmap(14, 9, goBMP_gameOver100x20, 100, 20, 1);
    display.display();

    // Scroll in various directions, pausing in-between:
    display.startscrollright(0x00, 0x0F);
    delay(500);
    display.stopscroll();
    delay(100);
    display.startscrollleft(0x00, 0x0F);
    delay(1000);
    display.stopscroll();
    delay(100);
    
    display.startscrollright(0x00, 0x0F);
    delay(500);
    display.stopscroll();
    // delay(100);
  }
  //============= global highest control: if game not over ===========//
  else{
    //clear disp and print score
    display.clearDisplay();
    display.setCursor(display.width()/2-14, 0);
    display.printf("score: %d", score);

    // if(digitalRead(0) == LOW){
    //   Serial.println("btn is pressed.");  //replace this with interrupt service function.
    // }

    if(millis() - lastMillis1 > beatTime1){   // 1st layer movement.
      lastMillis1 = millis();
      //toggle bitMapFrame index between 0/1:
      if(bitMapFrame_idx == 0){
        bitMapFrame_idx = 1;
      }else{
        bitMapFrame_idx = 0;
      }

    }

    if(millis() - lastMillis2 > beatTime2){   // 2nd layer movement. 
      lastMillis2 = millis();
      //change dx for monster's moving forward
      dx += mstrSpeed;  // dx increase. mnster move from right to left moving of monster, allow changing monster speed
                        // note: both the mstrSpeed and beatTime2 can regulate the speed of monster moving left.

      // regulate the beatTime2 to vary the speed when score goes higher
      if( beatTime2 > 13){
        beatTime2 = beatT2init - score/3;
      }

      // update position of monster.
      if (dx > display.width() && dinoGoingUp==true){    // do not release new monster before dinosaur's landing
      // if (dx > display.width() ){       
        dx = -MNSTR_WIDTH;
        score ++;
      }
    }

    if(millis() - lastMillis3 > beatTime3){   // 3rd beat for dinosaur jumping. split it out for independency beat for this jump
      lastMillis3 = millis();
      //put code here for dinosaur jumping
      dino_jump();
    }

    //this % way is not smooth. deprecated:
    // if ( millis() % (2*beatTime1) == 0) {    //set frame index = 0, for frame_0. You can use this to make cartoon animation
    //   bitMapFrame_idx = 0;
    // }
    // if ( millis() % (2*beatTime1) == beatTime1) {  //this % way is not smooth. deprecated.
    //   bitMapFrame_idx = 1;
    // }

    //draw dinosaur:  pos_x, pos_y, bitMap, bitmap_w, bitmap_h, colour
    display.drawBitmap(0, 
                      display.height()-DNSAUR_HEIGHT - dino_h, 
                      dinasaurBMP_allArray[bitMapFrame_idx], DNSAUR_WIDTH, DNSAUR_HEIGHT, 1);
    //draw monster:
    display.drawBitmap(display.width()-MNSTR_WIDTH-dx, 
                      display.height()-MNSTR_HEIGHT, 
                      monsterBMP_allArray[bitMapFrame_idx], MNSTR_WIDTH, MNSTR_HEIGHT, 1);
    display.display();





    //=========== judge if collision on dinosaur and monster =========
    gameover = ifCollision();

  }
}
