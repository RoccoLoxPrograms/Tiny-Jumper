#include <tice.h>
#include <keypadc.h>
#include <graphx.h>
#include <fileioc.h>
#include <stdint.h>

#include <math.h>

// these are the player's variables and many states of existence
float playerX;
float playerY;
float playerXVelocity;
float playerYVelocity;
bool playerHasJumped;
uint8_t playerGrounded;
bool playerAntiGravity;
bool anyAntiGravity;
bool invincibleMode = false;
int16_t weDontTalkAboutWhatThisVariableDoes;

// the game runs at 30.00732601 frames a second, or .0333251953 seconds per frame
uint16_t timer;
char strTemp[10];

uint8_t prevRandVar = 11;

bool dead = false;
bool quit = false;
bool goal = false;
uint8_t level = 0;

uint8_t levelX = 1;
uint8_t levelY = 0;

uint16_t tinyDATA[17];
const uint16_t goldTimes[16] = {288, 405, 233, 156, 245, 288, 534, 369, 363, 289, 264, 260, 350, 372, 489, 354};
uint8_t golds = 0;

uint8_t goalColor = 24;

// this function gets called at the end of every frame
void endOfFrame () {
  if (playerX < 0) playerX = 0;     // if the player hits the edge of the screen, then they stop
  if (playerX > 312) playerX = 312; // if the player hits the other edge of the screen, then they stop
  if (playerY < 0) playerY = 0;     // if the player hits the top, they stop
  if (playerY > 232) {              // if the player is on the bottom of the screen, then they stop
    playerY = 232;
    playerYVelocity = 0;
    playerGrounded = 3;
  }

  gfx_SetColor(tinyDATA[16]);
  // we don't talk about what weDontTalkAboutWhatThisVariableDoes is doing
  if (weDontTalkAboutWhatThisVariableDoes == 131) {
    for (int8_t i = 0; i < 4; i++) {
      gfx_HorizLine_NoClip(playerX + i, playerY + i, 2*(4 - i));
      gfx_HorizLine_NoClip(playerX + i, playerY + 7 - i, 2*(4 - i));
    }
  } else {
    // draws the player
    gfx_FillRectangle_NoClip(playerX, playerY, 8, 8);
  }

  gfx_BlitBuffer();

  while (timer_Get(1) < 1092);
}

// because I have to draw this invincible switch across multiple instances, I put it in this function to save space
void draw_invincible_switch () {
  gfx_SetColor(6*(invincibleMode));
  gfx_FillRectangle_NoClip(124, 118, 72, 18);
  gfx_SetColor(247);
  gfx_FillRectangle_NoClip(166 - 40*(invincibleMode), 120, 28, 14);
  gfx_SetTextBGColor(247);
  gfx_SetTextFGColor(0);
  gfx_SetTextScale(1, 1);
  if (invincibleMode) {
    gfx_PrintStringXY("on", 132, 123);
  }
  else {
    gfx_PrintStringXY("off", 169, 123);
  }

}

// resets the data, so all times are 0 and all levels are locked but level 1. This gets called if something happened to the appVar, or if you reset data from the options menu
void reset_data () {
  tinyDATA[0] = 0;
  tinyDATA[15] = 0;
  for (uint8_t levelNumber = 1; levelNumber < 15; levelNumber++) {
    tinyDATA[levelNumber] = 65535;
  }
  tinyDATA[16] = 6;
}

// converts a float to a real and then into the variable strTemp
void float_to_strTemp (float i, uint8_t precision) {
  real_t realTemp;
  realTemp = os_FloatToReal(i);
  os_RealToStr(strTemp, &realTemp, 8, 1, precision);
}

// the collision for regular platforms
void rectPlatform (float x, float y, float width, float height) {
  // the collisions for the top and bottom
  if (playerX + 8 > x && playerX < x + width) {
    if (playerYVelocity >= 0 && playerY + 8 >= y && playerY + 8 - playerYVelocity - 1 < y) {
      playerY = y - 8;
      playerYVelocity = 0;
      playerGrounded = 3*!playerAntiGravity;
    }
    else if (playerYVelocity < 0 && playerY <= y + height && playerY - playerYVelocity + 1 > y + height) {
      playerYVelocity = 0;
      playerY = y + height;
      playerGrounded = 3*playerAntiGravity;
    }
  }

  // the collisions for the sides
  if (playerY + 8 > y && playerY < y + height) {
    if (playerXVelocity >= 0 && playerX + 8 >= x && playerX + 7 - playerXVelocity < x) {
      playerX = x - 8;
      playerXVelocity = 0;
    }
    else if (playerXVelocity < 0 && playerX <= x + width && playerX - playerXVelocity + 1 > x + width) {
      playerX = x + width;
      playerXVelocity = 0;
    }
  }

}

// the collision for the spikes (and by extension, the lava) 
void spike (float x, float y, float width, float height) {
  if (playerY + 8 > y && playerY < y + height && playerX + 8 > x && playerX < x + width) dead = !invincibleMode; // the player only dies if they have invincible mode off
}

// the looks and collision for the end goal
void endGoal (float x, float y) {
  if (playerX + 8 > x && playerX < x + 16 && playerY + 8 > y && playerY < y + 16) goal = true;

  // this controls how the goal's colors change
  goalColor -= timer%2;
  for (uint8_t rectOffset = 0; rectOffset < 4; rectOffset++) {
    if (goalColor > 27) goalColor = 24;
    gfx_SetColor(goalColor);
    gfx_FillRectangle_NoClip(x + 2*rectOffset, y + 2*rectOffset, 16 - 4*rectOffset, 16 - 4*rectOffset);
    goalColor++;
  }

}

// the looks and collision for the bounce pads
void bouncePad (float x, float y, float width, float bounceSpeed) {
  // if the player jumps while on the bouncepad, then their speed is changed to the bounceSpeed
  if (playerX + 9 - playerXVelocity >= x && playerX - playerXVelocity - 1 <= x + width && playerY + 12.5 >= y && playerY + 10.5 <= y && playerYVelocity == -5) playerYVelocity = -bounceSpeed;
  // re-draws the bouncepad
  gfx_SetColor(249);
  gfx_FillRectangle_NoClip(x, y, width, 3);
  
}

// the anti-gravity!
void antiGravity (float x, float y, float width, float height) {
  // draws the anti-gravity
  gfx_SetColor(147);
  gfx_FillRectangle_NoClip(x, y, width, height);
  // collision and stuff for the anti-gravity
  if (playerY + 8 > y && playerY < y + height && playerX + 8 > x && playerX < x + width) {
    if (!playerAntiGravity) playerHasJumped = false;
    anyAntiGravity = true;
  }

}

// the player function
void player () {
  timer_Set(1, 0); // because the player() function is called every frame at the beginning of that frame, I can put this timer function in here.
  // this is the "speedrun" timer. It keeps track of your time and stores it into strTemp
  if (timer < 65534) timer++;
  float_to_strTemp(timer  * .0333251953, 2);

  kb_Scan(); // scan for new key presses
  if (kb_Data[7] & kb_Right) {                        // if the right key is pressed, then
    playerXVelocity += .75 * (playerXVelocity < 3);   // increase the XVelocity unless the player is going the max speed of 3.
  }
  else if (kb_Data[7] & kb_Left) {                    // (same for if the left key is pressed, but in the opposite direction)
    playerXVelocity -= .75*(playerXVelocity > -3);
  }
  else {
    playerXVelocity -= .75*fabsf(playerXVelocity)/playerXVelocity;  // if no keys are being pressed, then the player slides to a stop
  }
  if (kb_Data[1] & kb_2nd && playerGrounded) { // if the [2nd] key is pressed, then that means jump. Also, the player has to be grounded in order to jump
    playerYVelocity = 11*playerAntiGravity - 5.5;
    playerGrounded = 0;
    playerHasJumped = true;
  }
  if (playerGrounded < 3 && (2*playerAntiGravity - 1) * -playerYVelocity < 8) {
    playerYVelocity -= (2*playerAntiGravity - 1) * .5;
    if ((2*playerAntiGravity - 1) * -playerYVelocity >= 0) playerHasJumped = false;
  }
  if (playerHasJumped && !(kb_Data[1] & kb_2nd)) playerYVelocity = .5 * floor(playerYVelocity); // if the [2nd] key is released while jumping, then the jump is reduced. This gives the player more control.

  gfx_SetColor(247);
  gfx_FillRectangle_NoClip(playerX, playerY, 8, 8); // erase the player before moving it
  // the player moves according to its velocities
  playerX += playerXVelocity;
  playerY += playerYVelocity;
  if (kb_Data[6] & kb_Clear) quit = true; // quit the level if [clear] is pressed
  if (playerGrounded) playerGrounded--; // the playerGrounded variable is set to 3 if you're on the ground, and then decriment each frame that you're not on the ground. This essentially gives the player 2 extra frames to jump when they fall off the ledge, called "coyote time"
  anyAntiGravity = false; // to test if the player is in anti-gravity, the game first has to assume the player is not, before testing to see if they actually are

}

// this screen pops up when you die, and then restarts the level
void deadScreen () {
  gfx_SetDrawScreen();
  gfx_FillScreen(255);
  gfx_SetTextScale(2, 2);
  gfx_SetTextBGColor(255);
  uint8_t randVar;
  // picks a random variable that is different from the previous random variable
  do {
    randVar = randInt(0, 20);
  } while (randVar == prevRandVar);
  prevRandVar = randVar; // sets the previous random variable equal to the one that just got picked
  
  // display one of these death messages, based on the random variable
  if (!randVar) gfx_PrintStringXY("Ouch. You died.",63,70);
  if (randVar == 1) gfx_PrintStringXY("That looked like it hurt.",1,70);
  if (randVar == 2) {
    gfx_PrintStringXY("If you were playing a",18,50);
    gfx_PrintStringXY("different game, that",20,70);
    gfx_PrintStringXY("would've cost a life.",25,90);
    delay(500);
  }
  if (randVar == 3) gfx_PrintStringXY("You died and stuff.",32,70);
  if (randVar == 4) {
    gfx_PrintStringXY("You appear to have",32,60);
    gfx_PrintStringXY("died or something.",38,80);
  }
  if (randVar == 5) {
    gfx_PrintStringXY("Seriously? THAT'S how",12,60);
    gfx_PrintStringXY("you died?",97,80);
  }
  if (randVar == 6) gfx_PrintStringXY("Eres muerto.",75,70);
  if (randVar == 7) {
    gfx_PrintStringXY("You died.",101,60);
    gfx_PrintStringXY("How surprising.",56,80);
  }
  if (randVar == 8) {
    gfx_PrintStringXY("Congratulations! You",17,60);
    gfx_PrintStringXY("have died!!!",84,80);
  }
  if (randVar == 9) gfx_PrintStringXY("Should I call a doctor?",7,70);
  if (randVar == 10) {
    gfx_PrintStringXY("Maybe next time you",28,60);
    gfx_PrintStringXY("should try NOT dying.",19,80);
  }
  if (randVar == 11) {
    gfx_PrintStringXY("I'm getting a bit tired",12,60);
    gfx_PrintStringXY("of respawning you.",34,80);
  }
  if (randVar == 12) gfx_PrintStringXY("Tu es mort.",88,70);
  if (randVar == 13) gfx_PrintStringXY("Omae wa mou shindeiru.",10,70);
  if (randVar == 14) {
    gfx_PrintStringXY("Forsooth! Thou hast",24,60);
    gfx_PrintStringXY("died and whatnot.",44,80);
  }
  if (randVar == 15) gfx_PrintStringXY("Yet again, you died.", 31, 70);
  if (randVar == 16) {
    gfx_PrintStringXY("We regret to announce:", 6, 60);
    gfx_PrintStringXY("you are died", 74, 80);
  }
  if (randVar == 17) gfx_PrintStringXY("Were you even trying?", 12, 70);
  if (randVar == 18) {
    gfx_PrintStringXY("That death...", 78, 60);
    gfx_PrintStringXY("was glorious.", 72, 80);
  }
  if (randVar == 19) {
    gfx_PrintStringXY("Did you remember to", 27, 60);
    gfx_PrintStringXY("write your will?", 56, 80);
  }
  if (randVar == 20) {
    gfx_ZeroScreen();
    gfx_SetTextBGColor(0);
    gfx_SetTextFGColor(255);
    gfx_SetTextTransparentColor(0);
    gfx_PrintStringXY("you is ded lol", 72, 70);
    gfx_SetTextTransparentColor(255);
    gfx_SetTextFGColor(0);
  }
  delay(1200);
  gfx_SetTextScale(1, 1);
  gfx_SetTextBGColor(247);
  dead = false;
  goal = false;
  timer = 0;

}

// this screen pops up when you get to the goal, and then puts you in the next level
void goalScreen () {
  gfx_SetDrawScreen();
  gfx_FillScreen(255);
  gfx_SetTextScale(2, 2);
  gfx_SetTextBGColor(255);
  gfx_PrintStringXY("Wow, you did it. Your", 25, 50);
  gfx_PrintStringXY("time was:", 102, 70);
  gfx_PrintStringXY(strTemp, 160 - gfx_GetStringWidth(strTemp)/2, 90);
  // This checks to see if you got a better time, or if you beat the gold time. If invincible mode is active, it won't
  if ((timer < tinyDATA[level - 1] || tinyDATA[level - 1] == 0) && !invincibleMode) {
    // if you've beaten the gold time for the first time, display this
    if ((tinyDATA[level - 1] > goldTimes[level - 1] || tinyDATA[level - 1] == 0) && timer <= goldTimes[level - 1]) {
      gfx_PrintStringXY("You beat the gold time!", 9, 130);
      gfx_PrintStringXY("Wooh!", 124, 150);
    }
    // otherwise, you still beat your time, so display this
    else {
      gfx_PrintStringXY("Hey, you managed to", 28, 130);
      gfx_PrintStringXY("get a personal best!", 24, 150);
    }
    // unlocks the next level - but only if it is locked to begin with. Locked levels have a time of 65,535. Unlocked but uncompleted levels have a time of 0.
    if (level != 15 && tinyDATA[level] == 65535) tinyDATA[level] = 0;
    tinyDATA[level-1] = timer; // save that best time permanently (or at least until you beat it again)
  }
  // displays what level you're playing
  gfx_SetTextScale(1, 1);
  gfx_PrintStringXY("Level ", 1, 232);
  gfx_PrintInt(level, 1);
  // if invincible mode is active, display this
  if (invincibleMode) gfx_PrintStringXY("Invincible Mode is on.", 176, 232);

  // reset these settings for the menu
  goal = false;
  // waits for you to stop pressing a button, before waiting for you to press it again. This allows you to either pause the screen, so you can show someone your time; or, it allows you to skip this screen instead.
  while (kb_AnyKey()) kb_Scan();
  while (!(kb_AnyKey())) kb_Scan();
  quit = true; // quits to the menu

}

// this is the menu screen
void menu () {
  bool keyFirstPressed;
  playerAntiGravity = false;
  quit = false;
  gfx_SetDrawScreen();
  gfx_FillScreen(247);
  gfx_SetTextFGColor(0);
  for (uint8_t levelDrawerY = 0; levelDrawerY < 3; levelDrawerY++) {
    for (uint8_t levelDrawerX = 0;levelDrawerX < 5;levelDrawerX++) {
      if (tinyDATA[levelDrawerX + 5*levelDrawerY] == 65535) { // if the level is locked,
        gfx_SetColor(181);                                    // set the color to gray
        gfx_SetTextBGColor(181);
      }
      else if (!tinyDATA[levelDrawerX + 5*levelDrawerY]) { // if the level is incomplete,
        gfx_SetColor(183);                                 // set the color to very light green
        gfx_SetTextBGColor(183);
      }
      else if (tinyDATA[levelDrawerX + 5 * levelDrawerY] <= goldTimes[levelDrawerX + 5 * levelDrawerY]) { // if the level is under the gold time,
        gfx_SetColor(230);                                                                                // set the color to yellow (aka "gold" colored)
        gfx_SetTextBGColor(230);
      }
      else {
        gfx_SetColor(6); // otherwise, the level is green to show that it's completed
        gfx_SetTextBGColor(6);
      }
      gfx_FillRectangle_NoClip(levelDrawerX * 60 + 20, levelDrawerY * 60 + 60, 40, 40);
      
      // displays the number of the level in the center of the square
      float_to_strTemp(1 + levelDrawerX + 5 * levelDrawerY, 0);
      gfx_PrintStringXY(strTemp, levelDrawerX * 60 + 40 - gfx_GetStringWidth(strTemp)/2, levelDrawerY * 60 + 76);
    }
  }
  gfx_SetTextBGColor(247);
  gfx_PrintStringXY("Press [mode] for options", 77, 47);
  // gets how many gold levels you have done and stores it in golds
  golds = 0;
  for (uint8_t goldGetter = 0; goldGetter < 16; goldGetter++) {
    if (tinyDATA[goldGetter] && tinyDATA[goldGetter] <= goldTimes[goldGetter]) golds++;
  }
  if (golds > 14) gfx_PrintStringXY("BONUS LEVEL!", 118, 227);
  gfx_SetColor(6);
  gfx_FillRectangle_NoClip(0, 0, 320, 40);
  gfx_SetTextBGColor(6);
  gfx_SetTextFGColor(183);
  gfx_SetTextScale(2, 2);
  gfx_PrintStringXY("Tiny Jumper",80,6);
  gfx_SetTextScale(1, 1);
  gfx_PrintStringXY("Best time:", 6, 27);
  gfx_PrintStringXY("Gold time:", 215, 27);
  gfx_BlitScreen(); // copies this base frame to the buffer as well
  gfx_SetDrawBuffer(); // everything will now get drawn to the buffer
  while (kb_AnyKey()) kb_Scan(); //waits until the user stops pressing buttons before going to the menu

  while (!quit && !level) {
    // displays the cursor
    gfx_SetColor(0);
    if (levelY == 3) {
      levelX = 1;
      gfx_Rectangle_NoClip(116, 225, 89, 11);
    } else {
      gfx_Rectangle_NoClip(levelX * 60 - 40, levelY * 60 + 60, 40, 40);
    }
    // displays your current best time
    float_to_strTemp(tinyDATA[levelX + 5*levelY - 1] * .0333251953, 2);
    gfx_PrintStringXY(strTemp, 74, 27);
    // displays the time needed to get a golden level
    float_to_strTemp(goldTimes[levelX + 5*levelY - 1] * .0333251953, 2);
    gfx_PrintStringXY(strTemp, 281, 27);
    gfx_BlitBuffer();

    // This stuff defines the behavior of the cursor - that when you press and hold a key, the cursor will go that way once, pause, then keep going that direction
    while (kb_AnyKey() && timer_Get(1) < 3000 + 6000*keyFirstPressed) kb_Scan();
    keyFirstPressed = false;
    while (!kb_AnyKey()) {
      kb_Scan();
      keyFirstPressed = true;
    }
    timer_Set(1, 0);
    // erases the cursor-box
    gfx_SetColor(247);
    if (levelY == 3) {
      gfx_Rectangle_NoClip(116, 225, 89, 11);
    } else {
      gfx_Rectangle_NoClip(levelX * 60 - 40, levelY * 60 + 60, 40, 40);
    }
    // erases the best time/golden time
    gfx_SetColor(6);
    gfx_FillRectangle_NoClip(75, 27, 70, 8);
    gfx_FillRectangle_NoClip(277, 27, 40, 8);
    // decides which direction the cursor should go based on what buttons are being pressed
    do {
      if (kb_Data[7] & kb_Right) {
        levelX++;
        if (levelX == 6 - 4*(levelY == 3)) {
          levelX = 1;
          levelY++;
        }
      }
      else if (kb_Data[7] & kb_Down) levelY++;
      else if (kb_Data[7] & kb_Left) {
        levelX--;
        if (levelX == 0) {
          levelX = 5;
          levelY--;
        }
      }
      else if (kb_Data[7] & kb_Up) levelY--;
      if (levelY == 255) levelY = 2 + (golds > 14);
      if (levelY == 3 + (golds > 14)) levelY = 0;
    } while (tinyDATA[levelX + 5*levelY - 1] == 65535);
    // if [mode] is pressed, then do the settings screen
    if (kb_Data[1] & kb_Mode) level = 17;
    // if clear is pressed, quit
    if (kb_Data[6] & kb_Clear) quit = true;
    // if [2nd] is pressed, play the selected level
    if ((kb_Data[1] & kb_2nd) || (kb_Data[6] & kb_Enter)) {
      level = levelX + 5*levelY; //starts the level that the cursor has selected
      gfx_SetDrawScreen();
      // display this text in order to give the person time to get ready
      gfx_SetTextScale(2, 2);
      gfx_SetTextBGColor(255);
      gfx_SetTextFGColor(0);
      gfx_FillScreen(255);
      gfx_PrintStringXY("Are you ready?",59,70);
      delay(600);
      gfx_PrintStringXY("Get set...",102,90);
      delay(600);
      gfx_SetTextScale(1, 1);
      gfx_SetTextBGColor(247);
      timer = 0;
    }

  }
  
}

void options () {
  const uint8_t colors[12] = {6, 27, 192, 226, 230, 3, 17, 61, 152, 128, 252, 0};
  uint8_t colorPicker;
  uint8_t optionType = 0;
  bool reset = false;
  bool keyFirstPressed;
  // gets whatever color you actually have on your player and stores it in colorPicker
  for (uint8_t colorGetter = 0; colorGetter < 12; colorGetter++) {
    if (tinyDATA[16] == colors[colorGetter]) colorPicker = colorGetter;
  }
  gfx_SetDrawScreen();
  gfx_FillScreen(247);
  draw_invincible_switch();
  gfx_PrintStringXY("You won't die and your times won't be saved.", 15, 106);
  gfx_PrintStringXY("Tiny Jumper 1.1     (c) 2021 RoccoLox Programs", 10, 231);
  float_to_strTemp(golds, 0);
  gfx_PrintStringXY("Gold times so far:", 7, 24);
  gfx_PrintStringXY(strTemp, 127, 24);
  gfx_PrintStringXY("5 gold                         10 gold                   15 gold", 51, 36);
  gfx_SetTextScale(2, 2);
  gfx_PrintStringXY("Color Picker", 77, 6);
  gfx_PrintStringXY("Invincible Mode", 54, 88);
  gfx_PrintStringXY("Reset Times", 81, 158);
  //draws the different color options
  for (uint8_t colorDrawer = 0; colorDrawer < 12; colorDrawer++) {
    gfx_SetColor(colors[colorDrawer]);
    gfx_FillRectangle_NoClip(16 + colorDrawer*20 + 16*(colorDrawer > 0) + 16*(colorDrawer > 2) + 16*(colorDrawer == 11), 47, 18, 18);
  }
  // draws the lines to separate the colors
  gfx_SetColor(0);
  gfx_HorizLine_NoClip(5, 33, 311);
  gfx_HorizLine_NoClip(5, 67, 311);
  gfx_HorizLine_NoClip(5, 229, 311);
  if (golds < 5) gfx_VertLine_NoClip(42, 34, 33);
  if (golds < 10) gfx_VertLine_NoClip(100, 34, 33);
  if (golds < 15) gfx_VertLine_NoClip(268, 34, 33);
  // waits until you stop pressing a key
  while (kb_AnyKey()) kb_Scan();

  // the actual loop to pick the color or reset the times
  while (!quit) {
    if (optionType == 2) {
      gfx_SetColor(0);
      gfx_Rectangle_NoClip(79, 156, 160, 18);
      kb_Scan();
      if (kb_Data[1] & kb_2nd || kb_Data[6] & kb_Enter) {
        gfx_SetTextScale(2, 2);
        gfx_PrintStringXY("Are you sure?", 67, 177);
        gfx_PrintStringXY("No", 67, 197);
        gfx_PrintStringXY("Yes", 204, 197);
        do {
          gfx_SetColor(0);
          gfx_Rectangle_NoClip(65 + reset*137, 195, 34 + 16*reset, 18);
          while (kb_AnyKey()) kb_Scan();
          while (!kb_AnyKey()) kb_Scan();
          gfx_SetColor(247);
          gfx_Rectangle_NoClip(65 + reset*137, 195, 34 + 16*reset, 18);
          if (kb_Data[7] & kb_Left || kb_Data[7] & kb_Right) reset = !reset;
          if (kb_Data[6] & kb_Clear) {
            quit = true;
            reset = false;
          }
        } while (!(quit || kb_Data[1] & kb_2nd || kb_Data[6] & kb_Enter));
        quit = false;
        if (reset) {
          quit = true;
          reset_data();
          levelX = 1;
          levelY = 0;
        }
        reset = false;
        gfx_SetColor(247);
        gfx_FillRectangle_NoClip(67, 177, 186, 50);
        while (kb_AnyKey()) kb_Scan();
      }
      if (kb_Data[7] & kb_Up || kb_Data[7] & kb_Down) {
        optionType = 1 - (kb_Data[7] & kb_Down);
        gfx_SetColor(247);
        gfx_Rectangle_NoClip(79, 156, 160, 18);
        while (kb_AnyKey()) kb_Scan();
      }
      if (kb_Data[6] & kb_Clear || kb_Data[1] & kb_Mode) quit = true;
    }
    else if (!optionType) {
      // draws the cursor, on whatever color you have selected
      gfx_SetColor(0);
      gfx_Rectangle_NoClip(15 + colorPicker*20 + 16*(colorPicker > 0) + 16*(colorPicker > 2) + 16*(colorPicker == 11), 46, 20, 20);
      while (kb_AnyKey() && timer_Get(1) < 1600 + 7400*keyFirstPressed) kb_Scan();
      keyFirstPressed = false;
      while (!kb_AnyKey()) {
        kb_Scan();
        keyFirstPressed = true;
      }
      timer_Set(1, 0);
      // erases the cursor
      gfx_SetColor(247);
      gfx_Rectangle_NoClip(15 + colorPicker*20 + 16*(colorPicker > 0) + 16*(colorPicker > 2) + 16*(colorPicker == 11), 46, 20, 20);
      // if you press enter or 2nd, it puts the color you have selected on the player, and then quits the settings menu
      if (kb_Data[6] & kb_Enter || kb_Data[1] & kb_2nd) {
        tinyDATA[16] = colors[colorPicker];
        quit = true;
      }
      // detects left/right key presses and moves the cursor acccordingly. If you press right and the cursor is already to the very right, then the cursor moves to the other side
      do {
        if (kb_Data[7] & kb_Left) colorPicker--;
        if (kb_Data[7] & kb_Right) colorPicker++;
        if (colorPicker == 12) colorPicker = 0;
        if (colorPicker == 255) colorPicker = 11;
      } while (colorPicker > 2*(golds > 4) + 8*(golds > 9) + (golds > 14));
      // if you press the up/down keys, it moves you to the "reset times" menu
      if (kb_Data[7] & kb_Up || kb_Data[7] & kb_Down) {
        optionType = 2 - (kb_Data[7] & kb_Down);
        while (kb_AnyKey()) kb_Scan();
      }
      // If you press clear or mode again, it quits the settings menu
      if (kb_Data[6] & kb_Clear || kb_Data[1] & kb_Mode) quit = true;
    }
    else {
      gfx_SetColor(0);
      gfx_Rectangle_NoClip(122, 116, 76, 22);
      while (!kb_AnyKey()) kb_Scan();
      if (kb_Data[1] & kb_2nd || kb_Data[6] & kb_Enter || kb_Data[7] & kb_Left || kb_Data[7] & kb_Right) {
        invincibleMode = !invincibleMode;
        draw_invincible_switch();
        while (kb_AnyKey()) kb_Scan();
      }
      if (kb_Data[6] & kb_Clear || kb_Data[1] & kb_Mode) quit = true;
      if (kb_Data[7] & kb_Up || kb_Data[7] & kb_Down) {
        optionType = 2*(kb_Data[7] & kb_Down);
        gfx_SetColor(247);
        gfx_Rectangle_NoClip(122, 116, 76, 22);
        while (kb_AnyKey()) kb_Scan();
      }

    }

  }

  // resets these values for the home screen
  quit = false;
  gfx_SetDrawBuffer();
  gfx_SetTextBGColor(6);
  gfx_SetTextFGColor(183);
  gfx_SetTextScale(1, 1);
  level = 0;

}

// level 1 (who would have guessed)
void level1 () {
  playerX = 5;
  playerY = 227;
  // draws the platforms and spikes before the while loop to make it faster
  // these first ones are the regular platforms
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(200, 218, 100, 5);
  gfx_FillRectangle_NoClip(150, 201, 50, 5);
  gfx_FillRectangle_NoClip(100, 184, 50, 5);
  gfx_FillRectangle_NoClip(45, 167, 55, 5);
  gfx_FillRectangle_NoClip(0, 167, 25, 5);
  gfx_FillRectangle_NoClip(0, 147, 10, 5);
  gfx_FillRectangle_NoClip(15, 127, 10, 5);
  gfx_FillRectangle_NoClip(0, 107, 10, 5);
  gfx_FillRectangle_NoClip(15, 87, 50, 5);
  gfx_FillRectangle_NoClip(95, 87, 95, 5);
  gfx_FillRectangle_NoClip(210, 72, 110, 5);
  // these next ones are the spikes
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(54, 227, 50, 235, 58, 235);
  gfx_FillTriangle_NoClip(114, 227, 110, 235, 118, 235);
  // these last ones are the bits of lava
  gfx_SetColor(193);
  gfx_FillRectangle_NoClip(25, 168, 20, 4);
  gfx_FillRectangle_NoClip(65, 88, 30, 4);
  gfx_BlitScreen(); // copies this base frame to the buffer
  gfx_SetDrawBuffer(); // everything will now get drawn to the buffer

  while (!dead && !quit && !goal) {
    player(); // move the player
    // the collisions for all the platforms
    rectPlatform(0, 235, 320, 5);
    rectPlatform(200, 218, 100, 5);
    rectPlatform(150, 201, 50, 5);
    rectPlatform(100, 184, 50, 5);
    rectPlatform(45, 167, 55, 5);
    rectPlatform(0, 167, 25, 5);
    rectPlatform(0, 147, 10, 5);
    rectPlatform(15, 127, 10, 5);
    rectPlatform(0, 107, 10, 5);
    rectPlatform(15, 87, 50, 5);
    rectPlatform(95, 87, 95, 5);
    rectPlatform(210, 72, 110, 5);
    spike(50, 230, 9, 8);
    spike(110, 230, 9, 8);
    spike(25, 168, 20, 4);
    spike(65, 88, 30, 4);
    endGoal(300, 56);
    // prints the current time on a level
    gfx_PrintStringXY(strTemp, 1, 1);
    endOfFrame();
  }

}

// level 2 (if you couldn't already tell)
void level2 () {
  playerX = 5;
  playerY = 227;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(0, 205, 275, 5);
  gfx_FillRectangle_NoClip(195, 165, 5, 45);
  gfx_FillRectangle_NoClip(165, 130, 30, 5);
  gfx_FillRectangle_NoClip(230, 130, 90, 5);
  gfx_FillRectangle_NoClip(315, 45, 5, 195);
  gfx_FillRectangle_NoClip(110, 85, 165, 5);
  gfx_FillRectangle_NoClip(110, 90, 5, 75);
  gfx_FillRectangle_NoClip(0, 165, 115, 5);
  gfx_FillRectangle_NoClip(145, 45, 170, 5);
  gfx_FillRectangle_NoClip(0, 0, 5, 205);
  gfx_FillRectangle_NoClip(5, 0, 315, 5);
  gfx_SetColor(193);
  gfx_FillRectangle_NoClip(165, 135, 5, 45);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();
  
  while (!dead && !quit && !goal) {
    player();
    rectPlatform(0, 235, 320, 5);
    rectPlatform(0, 205, 275, 5);
    rectPlatform(195, 165, 5, 45);
    rectPlatform(165, 130, 30, 5);
    rectPlatform(230, 130, 90, 5);
    rectPlatform(315, 45, 5, 195);
    rectPlatform(110, 85, 165, 5);
    rectPlatform(110, 90, 5, 75);
    rectPlatform(0, 165, 115, 5);
    rectPlatform(145, 45, 170, 5);
    rectPlatform(0, 0, 5, 205);
    rectPlatform(5, 0, 315, 5);
    spike(165, 135, 5, 45);
    bouncePad(290, 233, 15, 7.5);
    bouncePad(205, 203, 15, 7);
    bouncePad(145, 203, 15, 9.5);
    bouncePad(295, 128, 15, 7);
    bouncePad(52, 163, 15, 11.5);
    endGoal(294, 29);
    gfx_PrintStringXY(strTemp, 7, 7);
    endOfFrame();
  }

}

void level3 () {
  playerX = 255;
  playerY = 227;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(265, 55, 5, 180);
  gfx_FillRectangle_NoClip(0, 214, 24, 5);
  gfx_FillRectangle_NoClip(48, 195, 24, 5);
  gfx_FillRectangle_NoClip(0, 5, 5, 230);
  gfx_FillRectangle_NoClip(0, 0, 320, 5);
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(180, 235, 184, 227, 188, 235);
  gfx_FillTriangle_NoClip(99, 235, 103, 227, 107, 235);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    antiGravity(5, 5, 260, 50);
    antiGravity(5, 55, 50, 110);
    playerAntiGravity = anyAntiGravity;
    rectPlatform(0, 235, 320, 5);
    rectPlatform(265, 55, 5, 180);
    spike(180, 227, 9, 8);
    spike(99, 227, 9, 8);
    rectPlatform(0, 214, 24, 5);
    rectPlatform(48, 195, 24, 5);
    rectPlatform(0, 5, 5, 230);
    rectPlatform(0, 0, 320, 5);
    gfx_SetColor(0);
    gfx_FillTriangle_NoClip(211, 4, 215, 12, 219, 4);
    gfx_FillTriangle_NoClip(51, 4, 55, 12, 59, 4);
    spike(211, 4, 9, 8);
    spike(51, 4, 9, 8);
    endGoal(287, 219);
    gfx_PrintStringXY(strTemp, 56, 56);
    endOfFrame();
  }

}

void level4 () {
  playerX = 2;
  playerY = 202;
  uint16_t spikeDrawer;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 222, 274, 18);
  gfx_FillRectangle_NoClip(0, 0, 320, 18);
  gfx_FillRectangle_NoClip(2, 210, 8, 30);
  gfx_FillRectangle_NoClip(272, 238, 48, 2);
  gfx_FillRectangle_NoClip(30, 185, 8, 52);
  gfx_FillRectangle_NoClip(80, 100, 8, 140);
  gfx_FillRectangle_NoClip(140, 210, 8, 40);
  gfx_FillRectangle_NoClip(160, 130, 9, 120);
  gfx_FillRectangle_NoClip(181, 200, 8, 40);
  gfx_FillRectangle_NoClip(240, 130, 8, 100);
  gfx_FillRectangle_NoClip(310, 223, 10, 17);
  gfx_SetColor(0);
  for (spikeDrawer = 0; spikeDrawer < 290; spikeDrawer += 10) {
    gfx_FillTriangle_NoClip(spikeDrawer, 222, spikeDrawer + 4, 214, spikeDrawer + 8, 222);
  }
  for (spikeDrawer = 0; spikeDrawer < 320;  spikeDrawer += 10){
    gfx_FillTriangle_NoClip(spikeDrawer, 17, spikeDrawer + 4, 25, spikeDrawer + 8, 17);
  }
  gfx_FillTriangle_NoClip(310, 222, 314, 214, 318, 222);
  gfx_FillTriangle_NoClip(160, 130, 164, 122, 168, 130);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    rectPlatform(2, 210, 8, 4);
    rectPlatform(272, 238, 48, 2);
    rectPlatform(30, 187, 8, 27);
    bouncePad(30, 185, 8, 10);
    rectPlatform(80, 100, 8, 114);
    bouncePad(80, 98, 8, 6);
    rectPlatform(140, 210, 8, 4);
    bouncePad(140, 208, 8, 10);
    rectPlatform(160, 130, 9, 84);
    spike(160, 122, 9, 8);
    rectPlatform(181, 200, 8, 14);
    bouncePad(181, 198, 8, 10);
    rectPlatform(240, 130, 8, 84);
    bouncePad(240, 128, 8, 7);
    rectPlatform(310, 224, 10, 16);
    spike(0, 216, 290, 8);
    spike(310, 216, 9, 8);
    endGoal(274, 223);
    gfx_PrintStringXY(strTemp, 1, 28);
    endOfFrame();
  }
  
}

void level5 () {
  playerX = 297;
  playerY = 227;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(286, 235, 29, 5);
  gfx_FillRectangle_NoClip(315, 0, 5, 240);
  gfx_FillRectangle_NoClip(281, 67, 5, 173);
  gfx_FillRectangle_NoClip(0, 0, 315, 5);
  gfx_FillRectangle_NoClip(35, 67, 246, 5);
  gfx_SetColor(193);
  gfx_FillRectangle_NoClip(30, 28, 5, 44);
  gfx_FillRectangle_NoClip(30, 72, 251, 5);
  gfx_FillRectangle_NoClip(59, 0, 58, 4);
  gfx_SetColor(147);
  gfx_HorizLine_NoClip(59, 4, 58);
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(226, 67, 230, 59, 234, 67);
  gfx_FillTriangle_NoClip(108, 67, 112, 59, 116, 67);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    antiGravity(286, 5, 29, 200);
    antiGravity(0, 5, 286, 27);
    antiGravity(0, 133, 281, 118);
    playerAntiGravity = anyAntiGravity;
    rectPlatform(286, 235, 29, 5);
    rectPlatform(315, 0, 5, 240);
    rectPlatform(281, 67, 5, 173);
    rectPlatform(0, 0, 60, 5);
    rectPlatform(117, 0, 198, 5);
    rectPlatform(35, 67, 246, 5);
    gfx_SetColor(228);
    gfx_FillRectangle_NoClip(63, 170, 220, 5);
    rectPlatform(63, 170, 220, 5);
    gfx_SetColor(0);
    gfx_FillTriangle_NoClip(242, 4, 246, 12, 250, 4);
    spike(242, 4, 9, 8);
    spike(226, 59, 9, 8);
    spike(108, 59, 9, 8);
    spike(59, 0, 58, 4);
    spike(30, 28, 5, 44);
    spike(30, 72, 251, 5);
    gfx_SetColor(193);
    gfx_FillRectangle_NoClip(30, 150, 6, 45);
    spike(30, 150, 6, 45);

    /* because the terminal velocity is 8, this temporarily disables that. But only if the player is within this hitbox
       I do this because the player needs to launch downwards through the anti-gravity at a speed higher than just 8. */
    if (playerY > 18 && playerY < 133 && playerX < 35 && playerYVelocity >= 8) playerYVelocity += .5;
    if (playerY > 133 && playerY < 231 && playerX < 35 && playerYVelocity <= -8) playerYVelocity -= .5;

    endGoal(254, 175);
    gfx_PrintStringXY(strTemp, 31, 78);
    endOfFrame();
  }
  
}

void level6 () {
  playerX = 154;
  playerY = 227;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(220, 220, 100, 5);
  gfx_FillRectangle_NoClip(233, 194, 73, 5);
  gfx_FillRectangle_NoClip(266, 169, 54, 5);
  gfx_FillRectangle_NoClip(304, 134, 16, 5);
  gfx_FillRectangle_NoClip(202, 134, 70, 5);
  gfx_FillRectangle_NoClip(256, 99, 64, 5);
  gfx_FillRectangle_NoClip(25, 195, 92, 5);
  gfx_FillRectangle_NoClip(0, 0, 5, 235);
  gfx_FillRectangle_NoClip(25, 142, 92, 5);
  gfx_FillRectangle_NoClip(121, 0, 5, 50);
  gfx_FillRectangle_NoClip(126, 45, 12, 5);
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(221, 220, 223, 216, 225, 220);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    antiGravity(256, 104, 64, 30);
    if (playerAntiGravity && !anyAntiGravity) playerGrounded = 0;
    playerAntiGravity = anyAntiGravity;
    rectPlatform(0, 235, 320, 5);
    rectPlatform(220, 220, 100, 5);
    rectPlatform(233, 194, 73, 5);
    rectPlatform(266, 169, 54, 5);
    rectPlatform(304, 134, 16, 5);
    rectPlatform(202, 134, 70, 5);
    rectPlatform(256, 99, 64, 5);
    rectPlatform(25, 195, 92, 5);
    rectPlatform(0, 0, 5, 235);
    rectPlatform(25, 142, 92, 5);
    rectPlatform(121, 0, 5, 50);
    rectPlatform(126, 45, 12, 5);
    bouncePad(25, 193, 12, 7.5);
    bouncePad(105, 140, 12, 10);
    spike(221, 216, 5, 4);
    endGoal(152, 0);
    gfx_PrintStringXY(strTemp, 6, 1);
    endOfFrame();
  }
  
}

void level7 () {
  playerX = 5;
  playerY = 200;
  float lavaY2 = 92;
  float lavaY1 = 211;
  float lavaChanger1 = .04;
  float lavaChanger2 = .2;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 208, 40, 32);
  gfx_FillRectangle_NoClip(280, 208, 40, 32);
  gfx_FillRectangle_NoClip(0, 134, 308, 5);
  gfx_FillRectangle_NoClip(12, 120, 308, 5);
  gfx_FillRectangle_NoClip(12, 88, 28, 32);
  gfx_FillRectangle_NoClip(280, 88, 40, 32);
  gfx_FillRectangle_NoClip(21, 14, 287, 5);
  gfx_FillRectangle_NoClip(0, 0, 320, 5);
  gfx_FillRectangle_NoClip(0, 5, 5, 21);
  gfx_FillRectangle_NoClip(0, 21, 26, 5);
  gfx_FillRectangle_NoClip(21, 14, 5, 12);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    rectPlatform(0, 208, 40, 32);
    rectPlatform(280, 208, 40, 32);
    rectPlatform(0, 134, 308, 5);
    rectPlatform(17, 120, 303, 5);
    rectPlatform(12, 88, 28, 37);
    rectPlatform(280, 88, 40, 32);
    rectPlatform(21, 14, 287, 5);
    rectPlatform(0, 0, 320, 5);
    // this stuff is the rising and falling lava
    gfx_SetColor(247);
    gfx_FillRectangle_NoClip(40, lavaY1, 240, 240 - lavaY1); // erase the previous lava
    gfx_FillRectangle_NoClip(40, lavaY2, 240, 120 - lavaY2);
    // draws all the platforms that are in the lava
    gfx_SetColor(228);
    gfx_FillRectangle_NoClip(65, 213, 20, 5);
    gfx_FillRectangle_NoClip(120, 211, 20, 5);
    gfx_FillRectangle_NoClip(182, 207, 20, 10);
    gfx_FillRectangle_NoClip(230, 213, 12, 3);
    gfx_FillRectangle_NoClip(56, 93, 16, 6);
    gfx_FillRectangle_NoClip(98, 89, 16, 9);
    gfx_FillRectangle_NoClip(149, 93, 16, 6);
    gfx_FillRectangle_NoClip(200, 89, 16, 9);
    gfx_FillRectangle_NoClip(242, 93, 16, 6);
    rectPlatform(65, 213, 20, 5);
    rectPlatform(120, 211, 20, 5);
    rectPlatform(182, 207, 20, 10);
    rectPlatform(230, 213, 12, 3);
    rectPlatform(56, 93, 16, 6);
    rectPlatform(98, 89, 16, 9);
    rectPlatform(149, 93, 16, 6);
    rectPlatform(200, 89, 16, 9);
    rectPlatform(242, 93, 16, 6);
    lavaY1 += lavaChanger1; // change the height of the lava
    lavaY2 += lavaChanger2;
    // if the lava is too low or too high, then it reverses
    if (lavaY1 > 215.8 || lavaY1 < 209.2) {lavaChanger1 *= -1;}
    if (lavaY2 > 98.8 || lavaY2 < 88.7) {lavaChanger2 *= -1;}
    // redraw the lava
    gfx_SetColor(193);
    gfx_FillRectangle_NoClip(40, lavaY1, 240, 240 - floor(lavaY1));
    gfx_FillRectangle_NoClip(40, lavaY2, 240, 120 - floor(lavaY2));
    spike(40, lavaY1, 240, 25);
    spike(40, lavaY2 - .5, 240, 25);
    bouncePad(308, 206, 12, 9);
    bouncePad(0, 132, 12, 7.5);
    bouncePad(308, 86, 12, 9);
    endGoal(5, 5);
    gfx_PrintStringXY(strTemp, 1, 27);
    endOfFrame();
  }

}

void level8 () {
  playerX = 281;
  playerY = 227;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(315, 5, 5, 230);
  gfx_FillRectangle_NoClip(251, 46, 5, 189);
  gfx_FillRectangle_NoClip(302, 189, 13, 12);
  gfx_FillRectangle_NoClip(256, 110, 13, 12);
  gfx_FillRectangle_NoClip(67, 46, 184, 5);
  gfx_FillRectangle_NoClip(0, 0, 320, 5);
  gfx_FillRectangle_NoClip(155, 34, 9, 12);
  gfx_FillRectangle_NoClip(62, 46, 5, 148);
  gfx_FillRectangle_NoClip(46, 46, 16, 9);
  gfx_FillRectangle_NoClip(0, 0, 5, 240);
  gfx_FillRectangle_NoClip(5, 185, 16, 9);
  gfx_FillRectangle_NoClip(67, 189, 123, 5);
  gfx_FillRectangle_NoClip(190, 95, 5, 99);
  gfx_FillRectangle_NoClip(128, 95, 62, 5);
  gfx_FillRectangle_NoClip(123, 95, 5, 50);
  gfx_FillRectangle_NoClip(128, 140, 24, 5);
  gfx_FillRectangle_NoClip(152, 120, 5, 25);
  gfx_SetColor(193);
  gfx_FillRectangle_NoClip(285, 96, 5, 26);
  gfx_FillRectangle_NoClip(62, 194, 66, 6);
  gfx_FillRectangle_NoClip(128, 194, 67, 11);
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(256, 47, 264, 51, 256, 55);
  gfx_FillTriangle_NoClip(200, 46, 204, 38, 208, 46);
  gfx_FillTriangle_NoClip(155, 34, 159, 26, 163, 34);
  gfx_FillTriangle_NoClip(110, 46, 114, 38, 118, 46);
  gfx_FillTriangle_NoClip(46, 46, 38, 50, 46, 54);
  gfx_FillTriangle_NoClip(5, 116, 13, 120, 5, 124);
  gfx_FillTriangle_NoClip(62, 116, 54, 120, 62, 124);
  gfx_FillTriangle_NoClip(21, 185, 29, 189, 21, 193);
  gfx_FillTriangle_NoClip(91, 235, 95, 227, 99, 235);
  gfx_FillTriangle_NoClip(158, 235, 162, 227, 166, 235);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    rectPlatform(0, 235, 320, 5);
    rectPlatform(315, 5, 5, 230);
    rectPlatform(251, 46, 5, 189);
    bouncePad(257, 233, 10, 8);
    rectPlatform(302, 189, 13, 12);
    bouncePad(304, 187, 10, 10.5);
    spike(285, 96, 5, 26);
    rectPlatform(256, 110, 13, 12);
    bouncePad(257, 108, 10, 9.5);
    spike(256, 46, 8, 8);
    rectPlatform(67, 46, 184, 5);
    rectPlatform(0, 0, 320, 5);
    spike(200, 38, 9, 8);
    rectPlatform(155, 34, 9, 12);
    spike(155, 26, 9, 8);
    spike(110, 38, 9, 8);
    rectPlatform(62, 46, 5, 148);
    rectPlatform(46, 46, 16, 9);
    spike(38, 46, 8, 9);
    rectPlatform(0, 0, 5, 240);
    spike(5, 116, 8, 9);
    spike(54, 116, 8, 9);
    rectPlatform(5, 185, 16, 9);
    spike(21, 185, 8, 9);
    rectPlatform(67, 189, 123, 5);
    spike(62, 194, 66, 6);
    spike(128, 194, 67, 11);
    spike(91, 227, 9, 8);
    spike(158, 227, 9, 8);
    bouncePad(240, 233, 10, 12.5);
    rectPlatform(190, 95, 5, 99);
    rectPlatform(128, 95, 62, 5);
    rectPlatform(123, 95, 5, 50);
    rectPlatform(128, 140, 24, 5);
    rectPlatform(152, 120, 5, 25);
    bouncePad(146, 187, 10, 8.5);
    endGoal(132, 124);
    gfx_PrintStringXY(strTemp, 6, 6);
    endOfFrame();
  }
  
}

void level9 () {
  playerX = 5;
  playerY = 175;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 183, 281, 5);
  gfx_FillRectangle_NoClip(107, 53, 64, 5);
  gfx_FillRectangle_NoClip(0, 0, 320, 5);
  gfx_FillRectangle_NoClip(251, 143, 12, 40);
  gfx_FillRectangle_NoClip(263, 53, 5, 130);
  gfx_FillRectangle_NoClip(268, 53, 13, 5);
  gfx_FillRectangle_NoClip(315, 0, 5, 235);
  gfx_FillRectangle_NoClip(302, 115, 13, 5);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(36, 212, 5, 23);
  gfx_SetColor(193);
  gfx_FillRectangle_NoClip(289, 5, 5, 15);
  gfx_FillRectangle_NoClip(289, 41, 5, 35);
  gfx_FillRectangle_NoClip(289, 96, 5, 58);
  gfx_FillRectangle_NoClip(289, 174, 5, 30);
  gfx_FillRectangle_NoClip(289, 232, 5, 3);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    antiGravity(107, 58, 64, 125);
    antiGravity(75, 188, 128, 47);
    if (playerAntiGravity && !anyAntiGravity) playerGrounded = 0;
    playerAntiGravity = anyAntiGravity;
    rectPlatform(0, 183, 282, 5);
    rectPlatform(107, 53, 64, 5);
    rectPlatform(0, 0, 320, 5);
    rectPlatform(251, 143, 12, 40);
    rectPlatform(263, 53, 5, 130);
    rectPlatform(268, 53, 14, 5);
    rectPlatform(315, 0, 5, 235);
    rectPlatform(301, 115, 14, 5);
    rectPlatform(0, 235, 320, 5);
    gfx_SetColor(228);
    gfx_FillRectangle_NoClip(171, 188, 5, 23);
    rectPlatform(171, 188, 5, 23);
    rectPlatform(36, 212, 5, 23);
    spike(289, 5, 5, 15);
    spike(289, 41, 5, 35);
    spike(289, 96, 5, 58);
    spike(289, 174, 5, 30);
    spike(289, 232, 5, 3);
    bouncePad(252, 141, 10, 10);
    endGoal(10, 219);
    gfx_PrintStringXY(strTemp, 1, 6);
    endOfFrame();
  }
  
}

void level10 () {
  playerX = 307;
  playerY = 173;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(205, 181, 115, 5);
  gfx_FillRectangle_NoClip(60, 181, 115, 5);
  gfx_FillRectangle_NoClip(0, 235, 37, 5);
  gfx_FillRectangle_NoClip(78, 235, 242, 5);
  gfx_FillRectangle_NoClip(170, 181, 5, 19);
  gfx_FillRectangle_NoClip(235, 47, 5, 19);
  // although both of these rects are actually 2 platforms each, they'll be covered over with anti-grav so it takes up less space to make them draw as one
  gfx_FillRectangle_NoClip(0, 0, 320, 5);
  gfx_FillRectangle_NoClip(0, 47, 320, 5);
  gfx_SetColor(193);
  gfx_FillRectangle_NoClip(55, 125, 5, 75);
  gfx_FillRectangle_NoClip(37, 236, 41, 4);
  gfx_FillRectangle_NoClip(205, 0, 30, 4);
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(22, 5, 26, 13, 30, 5);
  gfx_FillTriangle_NoClip(22, 47, 26, 39, 30, 47);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    antiGravity(105, 141, 170, 40);
    antiGravity(175, 181, 30, 19.5);
    antiGravity(205, 4, 30, 62);
    antiGravity(105, 5, 170, 42);
    playerAntiGravity = anyAntiGravity;
    rectPlatform(205, 181, 115, 5);
    rectPlatform(60, 181, 110, 5);
    spike(55, 125, 5, 75);
    rectPlatform(0, 235, 37, 5);
    rectPlatform(78, 235, 242, 5);
    spike(37, 236, 41, 4);
    rectPlatform(170, 181, 5, 20);
    rectPlatform(235, 47, 5, 20);
    rectPlatform(0, 0, 205, 5);
    rectPlatform(235, 0, 85, 5);
    spike(205, 0, 30, 4);
    rectPlatform(0, 47, 205, 5);
    rectPlatform(240, 47, 80, 5);
    gfx_SetColor(0);
    gfx_FillTriangle_NoClip(100, 4, 104, 12, 108, 4);
    gfx_FillTriangle_NoClip(101, 47, 105, 39, 109, 47);
    spike(100, 4, 9, 8);
    spike(101, 39, 9, 8);
    spike(22, 5, 9, 8);
    spike(22, 39, 9, 8);
    endGoal(10, 18);
    gfx_PrintStringXY(strTemp, 1, 53);
    endOfFrame();
  }
  
}

void level11 () {
  playerX = 45;
  playerY = 155;
  bool buttonPressed = false;
  gfx_FillScreen(193);
  gfx_SetColor(247);
  gfx_FillRectangle_NoClip(20, 20, 280, 200);
  gfx_FillRectangle_NoClip(300, 78, 20, 50);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(301, 73, 19, 5);
  gfx_FillRectangle_NoClip(301, 128, 19, 5);
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(174, 137, 178, 129, 182, 137);
  gfx_SetColor(6);
  gfx_FillRectangle_NoClip(303, 126, 15, 3);
  gfx_HorizLine_NoClip(304, 125, 13);
  gfx_HorizLine_NoClip(306, 124, 9);
  gfx_SetTextBGColor(193);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    rectPlatform(301, 73, 19, 5);
    rectPlatform(301, 128, 19, 5);
    spike(0, 0, 320, 20);
    spike(0, 220, 320, 20);
    spike(0, 20, 20, 200);
    spike(300.5, 20, 20, 58);
    spike(300.5, 128, 20, 92);
    // so there's this button in the stage. If the player gets within its hitbox, then the whole stage turns into anti-gravity, and the goal finally gets revealed!
    if (playerY > 116 && playerY < 126 && playerX > 295) buttonPressed = true;
    if (buttonPressed) {
      antiGravity(20, 20, 280, 200);
      antiGravity(300, 78, 20, 50);
      gfx_SetColor(0);
      gfx_FillTriangle_NoClip(174, 143, 178, 151, 182, 143);
      endGoal(16, 16);
    }
    playerAntiGravity = anyAntiGravity;
    gfx_SetColor(228);
    gfx_FillRectangle_NoClip(40, 163, 50, 5);
    rectPlatform(40, 163, 50, 5);
    gfx_FillRectangle_NoClip(110, 138, 12, 5);
    rectPlatform(110, 138, 12, 5);
    gfx_FillRectangle_NoClip(148, 138, 60, 5);
    rectPlatform(148, 138, 60, 5);
    gfx_FillRectangle_NoClip(243, 124, 12, 5);
    rectPlatform(243, 124, 12, 5);
    gfx_FillRectangle_NoClip(271, 99, 15, 5);
    rectPlatform(271, 99, 15, 5);
    spike(174, 130, 9, 21);
    gfx_PrintStringXY(strTemp, 6, 226);
    endOfFrame();
  }
  
}

void level12 () {
  playerX = 305;
  playerY = 227;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 235, 180, 5);
  gfx_FillRectangle_NoClip(260, 235, 60, 5);
  gfx_FillRectangle_NoClip(100, 135, 5, 100);
  gfx_FillRectangle_NoClip(105, 215, 16, 5);
  gfx_FillRectangle_NoClip(133, 192, 16, 5);
  gfx_FillRectangle_NoClip(105, 169, 16, 5);
  gfx_FillRectangle_NoClip(138, 147, 16, 5);
  gfx_FillRectangle_NoClip(154, 85, 166, 67);
  gfx_FillRectangle_NoClip(315, 5, 5, 85);
  gfx_FillRectangle_NoClip(220, 0, 100, 5);
  gfx_SetColor(193);
  gfx_FillRectangle_NoClip(180, 236, 80, 4);
  gfx_FillRectangle_NoClip(154, 152, 166, 4);
  gfx_FillRectangle_NoClip(0, 0, 220, 4);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    antiGravity(0, 28, 60, 178);
    playerAntiGravity = anyAntiGravity;
    rectPlatform(0, 235, 180, 5);
    bouncePad(262, 233, 12, 8.75);
    rectPlatform(260, 235, 60, 5);
    rectPlatform(100, 135, 5, 100);
    rectPlatform(105, 215, 16, 5);
    rectPlatform(133, 192, 16, 5);
    rectPlatform(105, 169, 16, 5);
    rectPlatform(138, 147, 16, 5);
    spike(154, 152, 166, 4);
    spike(180, 236, 80, 4);
    spike(0, 0, 220, 4);
    rectPlatform(154, 85, 166, 67);
    bouncePad(232, 83, 12, 8.5);
    rectPlatform(315, 5, 5, 85);
    rectPlatform(220, 0, 100, 5);
    endGoal(299, 26);
    gfx_PrintStringXY(strTemp, 1, 12);
    endOfFrame();
  }
  
}

void level13 () {
  playerX = 5;
  playerY = 31;
  uint16_t spikeDrawer;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 40, 280, 5);
  gfx_FillRectangle_NoClip(315, 0, 5, 240);
  gfx_FillRectangle_NoClip(275, 45, 5, 155);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(0, 45 ,5 , 235);
  gfx_FillRectangle_NoClip(52, 85, 5, 115);
  gfx_FillRectangle_NoClip(57, 195, 218, 5);
  gfx_FillRectangle_NoClip(107, 45, 5, 115);
  gfx_FillRectangle_NoClip(230, 45, 5, 76);
  gfx_SetColor(0);
  for (spikeDrawer = 50; spikeDrawer < 255; spikeDrawer += 50) {
    gfx_FillTriangle_NoClip(spikeDrawer, 40, spikeDrawer + 4, 32, spikeDrawer + 8, 40);
  }
  gfx_FillTriangle_NoClip(280, 41, 288, 45, 280, 49);
  gfx_FillTriangle_NoClip(315, 112, 307, 116, 315, 120);
  gfx_FillTriangle_NoClip(280, 191, 288, 195, 280, 199);
  for (spikeDrawer = 50; spikeDrawer < 255; spikeDrawer += 40) {
    gfx_FillTriangle_NoClip(spikeDrawer, 235, spikeDrawer + 4, 227, spikeDrawer + 8, 235);
  }
  gfx_FillTriangle_NoClip(5, 191, 13, 195, 5, 199);
  gfx_FillTriangle_NoClip(52, 86, 44, 90, 52, 94);
  gfx_FillTriangle_NoClip(275, 112, 267, 116, 275, 120);
  gfx_FillTriangle_NoClip(235, 112, 243, 116, 235, 120);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    rectPlatform(0, 40, 280, 5);
    rectPlatform(315, 0, 5, 240);
    rectPlatform(275, 45, 5, 155);
    rectPlatform(0, 235, 320, 5);
    rectPlatform(0, 45, 5, 235);
    rectPlatform(52, 85, 5, 115);
    rectPlatform(57, 195, 218, 5);
    rectPlatform(107, 45, 5, 115);
    rectPlatform(230, 45, 5, 76);
    for (spikeDrawer = 50; spikeDrawer < 255; spikeDrawer += 50) {
      spike(spikeDrawer, 32, 9, 8);
    }
    spike(280, 41, 9, 8);
    spike(307, 112, 9, 8);
    spike(280, 191, 9, 8);
    for (spikeDrawer = 50; spikeDrawer < 255; spikeDrawer += 40) {
      spike(spikeDrawer, 227, 9, 8);
    }
    spike(5, 191, 9, 8);
    spike(44, 86, 9, 8);
    spike(267, 112, 9, 8);
    spike(235, 112, 9, 8);
    bouncePad(10, 233, 15, 13);
    bouncePad(227, 193, 16, 12);
    endGoal(259, 45);
    gfx_PrintStringXY(strTemp, 1, 1);
    endOfFrame();
  }

}

void level14 () {
  playerX = 5;
  playerY = 227;
  uint16_t spikeDrawer;
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(48, 195, 224, 8);
  gfx_FillRectangle_NoClip(264, 53, 8, 145);
  gfx_FillRectangle_NoClip(0, 0, 320, 5);
  gfx_FillRectangle_NoClip(98, 53, 169, 8);
  gfx_FillRectangle_NoClip(48, 5, 8, 145);
  gfx_SetColor(193);
  gfx_FillRectangle_NoClip(56, 104, 160, 8);
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(158, 235, 162, 227, 166, 235);
  gfx_FillTriangle_NoClip(244, 235, 248, 227, 252, 235);
  for (spikeDrawer = 48; spikeDrawer < 220; spikeDrawer += 52) {
    gfx_FillTriangle_NoClip(spikeDrawer, 195, spikeDrawer + 4, 187, spikeDrawer + 8, 195);
  }
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    antiGravity(48, 203, 61, 32);
    antiGravity(176, 203, 64, 32);
    antiGravity(272, 5, 48, 198);
    antiGravity(56, 55, 43, 6);
    antiGravity(56, 61, 208, 14);
    antiGravity(56, 112, 160, 52);
    antiGravity(0, 5, 48, 159);
    antiGravity(48, 150, 8, 14);
    playerAntiGravity = anyAntiGravity;
    rectPlatform(0, 235, 320, 5);
    rectPlatform(48, 195, 224, 8);
    rectPlatform(264, 53, 8, 145);
    rectPlatform(0, 0, 320, 5);
    rectPlatform(98, 53, 169, 8);
    rectPlatform(48, 5, 8, 145);
    gfx_SetColor(0);
    gfx_FillTriangle_NoClip(52, 210, 56, 202, 48, 202);
    spike(49, 202, 9, 8);
    gfx_FillTriangle_NoClip(104, 210, 108, 202, 100, 202);
    spike(100, 202, 9, 8);
    spike(158, 227, 9, 8);
    gfx_FillTriangle_NoClip(208, 210, 212, 202, 204, 202);
    spike(204, 202, 9, 8);
    spike(244, 227, 9, 8);
    spike(56, 104, 160, 8);
    for (spikeDrawer = 48; spikeDrawer < 220; spikeDrawer += 52) {
      spike(spikeDrawer, 187, 8, 8);
    }
    endGoal(16, 5);
    gfx_PrintStringXY(strTemp, 57, 6);
    endOfFrame();
  }
  
}

void level15 () {
  playerX = 52;
  playerY = 227;
  bool buttonPressed = false;
  uint8_t platformDrawer;
  float platformY[5] = {140, 165, 186, 203, 216};
  float platformChanger[5] = {0, 0, 0, 0, 0};
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(0, 140, 60, 5);
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(123, 235, 127, 227, 131, 235);
  gfx_FillTriangle_NoClip(173, 235, 177, 227, 181, 235);
  gfx_FillTriangle_NoClip(223, 235, 227, 227, 231, 235);
  gfx_FillTriangle_NoClip(242, 203, 244, 199, 246, 203);
  gfx_FillTriangle_NoClip(177, 186, 179, 182, 181, 186);
  gfx_SetColor(6);
  gfx_FillRectangle_NoClip(2, 138, 15, 3);
  gfx_HorizLine_NoClip(3, 137, 13);
  gfx_HorizLine_NoClip(5, 136, 9);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    rectPlatform(0, 235, 320, 5);
    rectPlatform(0, 140, 60, 5);
    // all those moving platforms (although they won't move at the start - only when the button is pressed)
    for (platformDrawer = 0; platformDrawer < 5; platformDrawer++) {
      gfx_SetColor(247);
      gfx_FillRectangle_NoClip(50 * platformDrawer + 77, platformY[platformDrawer], 20, 5);
      platformY[platformDrawer] += platformChanger[platformDrawer];
      if (platformY[platformDrawer] > 216 - 36 * platformDrawer) platformChanger[platformDrawer] = -fabsf(platformChanger[platformDrawer]);
      if (platformY[platformDrawer] < 176 - 36 * platformDrawer) platformChanger[platformDrawer] = fabsf(platformChanger[platformDrawer]);
      gfx_SetColor(228);
      gfx_FillRectangle_NoClip(50 * platformDrawer + 77, platformY[platformDrawer], 20, 5);
      rectPlatform(50 * platformDrawer + 77, platformY[platformDrawer], 20, 5);
      if (playerX > 50 * platformDrawer + 68 && playerX < 50 * platformDrawer + 120 && playerGrounded == 2) playerY += platformChanger[platformDrawer];
    }
    // the button hitbox and other button-y stuff:
    if (playerY > 130 && playerY < 140 && playerX < 17 && !buttonPressed) {
      buttonPressed = true;
      gfx_SetColor(247);
      gfx_FillRectangle_NoClip(2, 136, 15, 4);
      gfx_FillRectangle_NoClip(177, 180, 24, 11);
      platformChanger[0] = .8;
      platformChanger[1] = .5;
      platformChanger[2] = .8;
      platformChanger[3] = .5;
      platformChanger[4] = -.875;
      platformY[2] = 193;
      gfx_SetColor(193);
      gfx_FillRectangle_NoClip(0, 227, 320, 13);
    }
    if (!buttonPressed) {
      spike(123, 227, 9, 8);
      spike(223, 227, 9, 8);
      spike(244, 199, 3, 3);
      spike(178, 182, 3, 3);
    }
    spike(173 - 173 * buttonPressed, 227, 9 + 311 * buttonPressed, 8);
    endGoal(300, 4);
    gfx_PrintStringXY(strTemp, 1, 1);
    endOfFrame();
  }
  
}

void bonusLevel () {
  playerX = 5;
  playerY = 227;
  uint8_t spikeDrawer;
  uint8_t platformDrawer;
  float lavaY = 174;
  float lavaChanger = -.2;
  float platformY[3] = {95, 64, 64};
  float platformChanger[3] = {-.2, .5, -.25};
  gfx_FillScreen(247);
  gfx_SetColor(228);
  gfx_FillRectangle_NoClip(0, 235, 320, 5);
  gfx_FillRectangle_NoClip(37, 129, 283, 5);
  gfx_FillRectangle_NoClip(25, 119, 12, 15);
  gfx_FillRectangle_NoClip(64, 100, 25, 5);
  gfx_SetColor(0);
  gfx_FillTriangle_NoClip(80, 235, 83, 229, 86, 235);
  gfx_FillTriangle_NoClip(170, 235, 173, 229, 176, 235);
  gfx_FillTriangle_NoClip(230, 235, 233, 229, 236, 235);
  gfx_SetColor(193);
  gfx_FillRectangle_NoClip(37, 120, 283, 9);
  gfx_BlitScreen();
  gfx_SetDrawBuffer();

  while (!dead && !quit && !goal) {
    player();
    antiGravity(0, 134, 320, 51);
    playerAntiGravity = anyAntiGravity;
    rectPlatform(0, 235, 320, 5);
    spike(80, 227, 7, 6);
    spike(170, 227, 7, 6);
    spike(230, 227, 7, 6);
    // this is the rising/falling lava code
    gfx_SetColor(247);
    gfx_FillRectangle_NoClip(0, 184, 298, 43); // erases the lava
    lavaY += lavaChanger; // changes the position of the lava
    if (lavaY < 150 || lavaY > 178) lavaChanger *= -1; // reverses the direction of the lava if it grows too much
    gfx_SetColor(193);
    gfx_FillRectangle_NoClip(0, lavaY, 298, 370 - 2*lavaY); // redraws the new lava
    spike(0, lavaY + 1, 298, 368 - 2 * lavaY);
    rectPlatform(64, 100, 25, 5);
    rectPlatform(37, 129, 283, 5);
    for (spikeDrawer = 80; spikeDrawer > 70; spikeDrawer += 50) {
      gfx_SetColor(0);
      gfx_FillTriangle_NoClip(spikeDrawer, 133, spikeDrawer+3, 139, spikeDrawer+6, 133);
      spike(spikeDrawer, 133, 7, 6);
    }
    rectPlatform(25, 119, 12, 15);
    bouncePad(306, 233, 12, 6.5);
    spike(37, 120, 283, 4);
    // the three moving platforms
    for (platformDrawer = 0; platformDrawer < 3; platformDrawer++) { // cycles through the three moving platforms
      gfx_SetColor(247);
      gfx_FillRectangle_NoClip(64 * platformDrawer + 123, platformY[platformDrawer], 25, 5); // erases it
      platformY[platformDrawer] += platformChanger[platformDrawer]; // moves it
      if (platformY[platformDrawer] > 108 - 40*(platformDrawer == 2) || platformY[platformDrawer] < 32 + 25*(!platformDrawer)) platformChanger[platformDrawer] *= -1; // reverses it if it goes past its parameters
      gfx_SetColor(228);
      gfx_FillRectangle_NoClip(64 * platformDrawer + 123, platformY[platformDrawer], 25, 5); // redraws it
      rectPlatform(64 * platformDrawer + 123, platformY[platformDrawer], 25, 5);
      // this bit ensures that the player moves properly downward when on a moving platform that is going down
      if (playerY < 110 && playerX > 64*platformDrawer + 114 && playerX < 64*platformDrawer + 162 && playerGrounded == 2) playerY += platformChanger[platformDrawer];
    }
    endGoal(299, 12);
    gfx_PrintStringXY(strTemp, 1, 1);
    endOfFrame();
  }
  
}

int main () {
  ti_var_t slot;
  // if the appvar "TnyTimes" does not exist, then reset the all the times.
  if (!(slot = ti_Open("TnyTimes", "r"))) {
    reset_data();
  }
  // otherwise, read the data from the appvar to get the current level times.
  else {
    ti_Read(&tinyDATA, 34, 1, slot);
  }
  gfx_Begin();
  timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);
  rtc_Enable(RTC_SEC_INT);
  srand(rtc_Time());

  while (!quit) {
    playerXVelocity = 0;
    playerYVelocity = 1;
    weDontTalkAboutWhatThisVariableDoes = randInt(0, 500);
    if (level == 1) level1();
    if (level == 2) level2();
    if (level == 3) level3();
    if (level == 4) level4();
    if (level == 5) level5();
    if (level == 6) level6();
    if (level == 7) level7();
    if (level == 8) level8();
    if (level == 9) level9();
    if (level == 10) level10();
    if (level == 11) level11();
    if (level == 12) level12();
    if (level == 13) level13();
    if (level == 14) level14();
    if (level == 15) level15();
    if (level == 16) bonusLevel();
    if (dead) deadScreen();
    if (goal) goalScreen();
    level *= !quit; // if quit is true, then the level will be 0 (aka it will return to the menu). Otherwise, the level is multiplied by 1.
    if (level == 0) menu();
    if (level == 17) options();
  }

  gfx_End();
  slot = ti_Open("TnyTimes", "w+");
  ti_Write(&tinyDATA, 34, 1, slot);
  ti_SetArchiveStatus(true, slot);

}
