/**************************************************************
construction for this device:
  -(use arduino mega as controler)

  12864B lcd display:
  -GND    GND
  -VCC    5V
  -RS     53
  -R/W    49
  -E      52
  -PSB    GND
  -RST    8
  -BLA    5V
  -BLK    GND

  joystick:
  -GND    GND
  -+5V    5V
  -VRX    A1
  -VRY    A0
  -SW     5

  shoot button:
  -leftside   GND
  -rightside  2

**************************************************************/
/*
to do list:
generate enemy bullet
generate enemy ship
rendering enemy bullet
rendering enemy ship
hitbox check
*/
#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

using namespace std;

U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);    //full buffer


//joystick
static const int x_mid = 478;
static const int x_max = 1023;
static const int y_mid = 500;
static const int y_max = 1023;
static const byte dead_zone = 80;
static int x_value;
static int y_value;

//border
bool up_allowed = true;
bool down_allowed = true;
bool left_allowed = true;
bool right_allowed = true;


//spaceship
static int x = 5;         //the current x position of the ship
static int y = 29;        //the current y position of the ship
static int speed_x = 0;   //the speed of the ship on x axis
static int speed_y = 0;

//enemy ship
static const int MAX_ENEMYSHIP = 5; //max ship amount for enemy ships
static byte ENEMY_VARIENT[MAX_ENEMYSHIP];          //1: enemy type 1, 2: enemy type 2...
static byte EnemyX[MAX_ENEMYSHIP];
static byte EnemyY[MAX_ENEMYSHIP];
static bool enemyship_active[MAX_ENEMYSHIP];
unsigned static long time_last_ship;


//bullet
static const int MAX_BULLETS = 20;
static byte bulletX[MAX_BULLETS];
static byte bulletY[MAX_BULLETS];
static bool bulletActive[MAX_BULLETS];
unsigned static long time_last_bullet;
static byte bullet_activated;

static const unsigned char spaceship[] U8X8_PROGMEM  = {  //the spaceship
  0x06, 0x00, 0x1E, 0x00, 0x7E, 0x00, 0xFF, 0x03, 0xFF, 0x0C, 0xFF, 0x03, 0x7E, 0x00, 0x1E, 0x00, 0x06, 0x00,
};
/*
const unsigned char enemyship2 [] PROGMEM = {
    0xff, 0x88, 0xff, 0x18, 0xfe, 0x18, 0xf8, 0x08, 0xc0, 0x80, 0x39, 0x00, 0xc0, 0x80, 0xf8, 0x08, 0xfe, 0x18, 0xff, 0x18, 0xff, 0x88,
};
*/

const unsigned char enemyship2 [] PROGMEM = {
    0x00, 0x0E, 0x00, 0x07, 0x80, 0x07, 0xE0, 0x0F, 0x7C, 0x1F, 0x63, 0x1F, 0x7C, 0x1F, 0xE0, 0x0F, 0x80, 0x07, 0x00, 0x07, 0x00, 0x0E,
};

//debug print
//static String x_status;
//static String y_status;

/**************************************************************/

void setup(){
  pinMode(2, INPUT_PULLUP);
  u8g2.begin();
  u8g2.setBitmapMode(1);
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_profont22_tf);
  Serial.begin(9600);

  //setup bullet logic
  for (int i = 0; i < MAX_BULLETS; i++) {
    bulletActive[i] = false;
  }
  //setup enemyship logic
  for (int i = 0; i < MAX_ENEMYSHIP; i++) {
    enemyship_active[i] = false;
  }
}

/**************************************************************/

void loop(){
  //border
  detect_Border();  

  //joystick code
  joystick_data_receive();

  //generate enemy ship
  enemyShip_Logics();

  //shoot bullet:
  bullet();

  //spaceship code
  spaceship_Full();

  //serial print for debug
  //debug_Print();
}

/**************************************************************/

void spaceship_Full(){
  //check hitbox


  //rendering heroship
  y += speed_y;
  x += speed_x;
  u8g2.clearBuffer();
  u8g2.drawStr(5, 18, "shipbattle");
  u8g2.drawXBMP(x, y, 12, 9, spaceship);

  //rendering bullet
  for(int i = 0; i < bullet_activated; i++){
    bulletX[i] += 2;
    if(!bulletActive[i]){continue;}
    if(bulletX[i] > 128){
      bulletActive[i] = false;
      bullet_activated--;
      continue;
    }
    u8g2.drawPixel(bulletX[i], bulletY[i]);     // First pixel
    u8g2.drawPixel(bulletX[i] + 1, bulletY[i]); // Second pixel
  }

  //rendering enemy bullet

  //rendering enemyship
  for(int i = 0; i < MAX_ENEMYSHIP; i++){
    EnemyX[i] -= 1;
    if(!enemyship_active[i]){continue;}
    if(EnemyX[i] < 0){
      enemyship_active[i] = false;
      continue;
    }

    if(ENEMY_VARIENT[i] == 2){u8g2.drawXBMP(EnemyX[i], EnemyY[i], 13, 11, enemyship2);}
  }

  u8g2.sendBuffer();
}

/**************************************************************/

//joystick
void joystick_data_receive(){
  x_value = analogRead(A0);
  y_value = analogRead(A1);

  if (x_value > x_mid + dead_zone && right_allowed){ // right (plus)
    speed_x = map(x_value, x_mid + dead_zone, x_max, 1, 6);
  }else if (x_value < x_mid - dead_zone && left_allowed) { // left (minus)
    speed_x = map(x_value, x_mid - dead_zone, 0, 1, 6);
    speed_x = speed_x * -1;
  }else { // middle (plus)
    speed_x = 0;
  }

  if (y_value > y_mid + dead_zone && up_allowed){ // up (minus)
    speed_y = map(y_value, y_mid + dead_zone, y_max, 1, 6);
    speed_y = speed_y * -1;
  }else if (y_value < x_mid - dead_zone && down_allowed){ // down (plus)
    speed_y = map(y_value, y_mid - dead_zone, 0, 1, 6);
  }else { // middle
    speed_y = 0;
  } 
}

/**************************************************************/

void detect_Border() {
  down_allowed = !(y >= 55);
  up_allowed = !(y <= 0);
  right_allowed = !(x >= 116);
  left_allowed = !(x <= 0);
}

/**************************************************************/

void bullet(){
  unsigned long time_now = millis();
  if(!digitalRead(2) & time_now - time_last_bullet >= 500 & bullet_activated < MAX_BULLETS){
    for (int i = 0; i <= MAX_BULLETS; i++) {
      if(!bulletActive[i]){
        bulletX[i] = x + 12; // Start at the tip of the spaceship
        bulletY[i] = y + 4;  // Center vertically
        bulletActive[i] = true;
        bullet_activated ++;
        time_last_bullet = time_now;
        Serial.print("bullet_activated: ");
        Serial.println(bullet_activated);
        Serial.print("i: ");
        Serial.println(i);
        Serial.print(bulletX[i]);
        Serial.println(bulletY[i]);
        break;
      }
    }
  }
}

/**************************************************************/

void enemyShip_Logics(){
  unsigned long time_now = millis();
  if(time_now - time_last_ship >= 10000){
    for (int i = 0; i <= MAX_ENEMYSHIP; i++) {
      if(!enemyship_active[i]){
        //ENEMY_VARIENT[i] = random(1, 4);
        ENEMY_VARIENT[i] = 2;
        EnemyX[i] = 128;
        EnemyY[i] = random(1, 64);
        enemyship_active[i] = true;
        time_last_ship = time_now;
        break;
      }
    }
  }
}

/**************************************************************/

void hitBox(){
  //if heroship collide with bullet

  //if heroship collide with enemyship

  //if heroship collide with boss
}

/**************************************************************/
/*
Enemyship2
00000000 00000

00000000 01110
00000000 11100
00000001 11100
00000111 11110
00111110 11111
11000110 11111
00111110 11111
00000111 11110
00000001 11100
00000000 11100
00000000 01110

00000000 00001110
00000000 00000111
10000000 00000111
11100000 00001111
01111100 00011111
01100011 00011111
01111100 00011111
11100000 00001111
10000000 00000111
00000000 00000111
00000000 00001110

const unsigned char enemyship2 [] PROGMEM = {
    0x00, 0x0E, 0x00, 0x07, 0x80, 0x07, 0xE0, 0x0F, 0x7C, 0x1F, 0x63, 0x1F, 0x7C, 0x1F, 0xE0, 0x0F, 0x80, 0x07, 0x00, 0x07, 0x00, 0x0E,
};

*/
