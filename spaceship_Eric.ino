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
rendering enemy bullet
hitbox check

reduce memory(just incase)
1, only keep 1 enemy_time_last_bullet
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
int heroship_health = 10;
static bool forcesheld_activate = false;
static long forcesheld_ticking;
static byte cooldown = 1;
static bool forcesheld_ready;
static long forcesheld_active_timer;
static long forcesheld_trigger_time;
static bool take_damage;
static long take_damage_time;

//enemy ship
static const int MAX_ENEMYSHIP = 5; //max ship amount for enemy ships
static byte ENEMY_VARIENT[MAX_ENEMYSHIP];          //1: enemy type 1, 2: enemy type 2...
static char EnemyX[MAX_ENEMYSHIP];
static char EnemyY[MAX_ENEMYSHIP];
static bool enemyship_active[MAX_ENEMYSHIP];
unsigned static long time_last_ship;
static byte enemy_speed;
static byte enemy_ticking_counter[MAX_ENEMYSHIP];
static byte enemy_ticking_counter2[MAX_ENEMYSHIP];
static int enemy_health[MAX_ENEMYSHIP];
static byte Enemy_remain = 100;
static bool idk1[MAX_ENEMYSHIP]; //diffferent ship has different use for this
static long enemy_time_counter[MAX_ENEMYSHIP];

//bullet
static const int MAX_BULLETS = 20;
static byte bulletX[MAX_BULLETS];
static byte bulletY[MAX_BULLETS];
static bool bulletActive[MAX_BULLETS];
unsigned static long time_last_bullet;

//enemy bullet
static const int MAX_ENEMYBULLETS = 20;
static byte enemy_bulletX[MAX_BULLETS];
static byte enemy_bulletY[MAX_BULLETS];
static bool enemy_bulletActive[MAX_BULLETS];
unsigned static long enemy_time_last_bullet[MAX_ENEMYSHIP];

//genration tool
static long regeneration_tool_generation_time;
static bool regeneration_tool_exist;
static char regeneration_toolX;
static char regeneration_toolY;
static char transport_shipX;
static const char transport_shipY = 26;
static byte regeneration_tool_ticking_counter;

//all ship models
static const unsigned char spaceship[] U8X8_PROGMEM  = {  //the spaceship
  0x06, 0x00, 0x1E, 0x00, 0x7E, 0x00, 0xFF, 0x03, 0xFF, 0x0C, 0xFF, 0x03, 0x7E, 0x00, 0x1E, 0x00, 0x06, 0x00,
};
static const unsigned char enemyship1[] U8X8_PROGMEM  = {
  0x60, 0x00, 0xE0, 0x00, 0xF0, 0x00, 0xFC, 0x03, 0xF7, 0x01, 0xFC, 0x03, 0xF0, 0x00, 0xE0, 0x00, 0x60, 0x00,
};
static const unsigned char enemyship1_inverted[] U8X8_PROGMEM  = {
  0x18, 0x00, 0x1C, 0x00, 0x3C, 0x00, 0xFF, 0x00, 0xBE, 0x03, 0xFF, 0x00, 0x3C, 0x00, 0x1C, 0x00, 0x18, 0x00,
};
static const unsigned char enemyship2[] U8X8_PROGMEM = {
    0x00, 0x0E, 0x00, 0x07, 0x80, 0x07, 0xE0, 0x0F, 0x7C, 0x1F, 0x63, 0x1F, 0x7C, 0x1F, 0xE0, 0x0F, 0x80, 0x07, 0x00, 0x07, 0x00, 0x0E,
};
static const unsigned char enemyship3[] U8X8_PROGMEM = {
    0x38, 0x00, 0xFE, 0x00, 0x93, 0x01, 0x39, 0x01, 0x7C, 0x00, 0x74, 0x00, 0x7C, 0x00, 0x39, 0x01, 0x93, 0x01,0xFE, 0x00, 0x38, 0x00,
};

static const unsigned char enemyship4[] U8X8_PROGMEM = {
    0x80, 0x0F, 0xE0, 0x0F, 0x80, 0x03, 0xF0, 0x07, 0xFE, 0x0F, 0xFF, 0x1F, 0xE0, 0x1F, 0xFF, 0x1F, 0xFE, 0x0F, 0xF0, 0x07, 0x80, 0x03, 0xE0, 0x0F, 0x80, 0x0F, 
};
static const unsigned char enemyship5[] U8X8_PROGMEM = {
    0xFC, 0x7F, 0x54, 0x55, 0xFC, 0x7F, 0x01, 0x21, 0xFF, 0x2F, 0xFE, 0x3F, 0xFF, 0x2F, 0x01, 0x21, 0xFC, 0x7F, 0x54, 0x55, 0xFC, 0x7F, 
};
static const unsigned char transport_ship[] U8X8_PROGMEM = {
    0x7E, 0x00, 0xFC, 0x00, 0x7E, 0x00, 0x18, 0x00, 0xFE, 0x00, 0xFF, 0x03, 0xFF, 0x07, 0xFF, 0x03, 0xFE, 0x00, 0x18, 0x00, 0x7E, 0x00, 0xFC, 0x00, 0x7E, 0x00,
};
static const unsigned char regeneration_tool[] U8X8_PROGMEM = {
    0x36, 0x7F, 0x7F, 0x3E, 0x1C, 0x08,
};
/**************************************************************/

void setup(){
  pinMode(2, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  u8g2.begin();
  u8g2.setBitmapMode(1);
  u8g2.enableUTF8Print();
  //u8g2.setFont(u8g2_font_profont11_tr);
  Serial.begin(9600);

  heroship_health = 10;
  //setup bullet logic
  for (int i = 0; i < MAX_BULLETS; i++) {
    bulletActive[i] = false;
  }
  //setup enemyship logic
  for (int i = 0; i < MAX_ENEMYSHIP; i++) {
    enemyship_active[i] = false;
  }

  for(int i = 0; i < MAX_ENEMYBULLETS; i++){
    enemy_bulletActive[i] = false;
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

  //ticking the enemy bullet
  ticking_enemybullet();

  //shoot bullet:
  bullet();

  //forcesheld
  forcesheld();

  invincible_time();

  //hero regeneration
  regeneration_tool_auto_generation();
  ticking_regeneration_tool();

  //spaceship code
  spaceship_Full();

}

/**************************************************************/

void spaceship_Full(){
  //check hitbox
  hitBox();

  //rendering text:
  y += speed_y;
  x += speed_x;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_profont11_tr);
  u8g2.drawStr(1, 8, "HP:");
  u8g2.setCursor(18, 8);
  u8g2.print(heroship_health);
  u8g2.setFont(u8g2_font_tiny5_tf);
  u8g2.drawStr(90, 6, "REMAIN:");
  u8g2.setCursor(120, 6);
  u8g2.print(Enemy_remain);
  u8g2.drawStr(45, 6, "CD:");
  u8g2.setCursor(60, 6);
  u8g2.print(cooldown); 

  //rendering heroship
  u8g2.drawXBMP(x, y, 12, 9, spaceship);
  

  //rendering force sheld:
  if(forcesheld_activate){
    byte hero_centerX = x + 6;
    byte hero_centerY = y + 4;

    u8g2.drawCircle(hero_centerX, hero_centerY, 8);

  }

  //rendering transportation ship
  if(transport_shipX >= 0){
    u8g2.drawXBMP(transport_shipX, transport_shipY, 11, 13, transport_ship);
  }
  //rendering regeneration tool
  if(regeneration_tool_exist){
    u8g2.drawXBMP(regeneration_toolX, regeneration_toolY, 7, 6, regeneration_tool);
  }

  //rendering bullet
  for(int i = 0; i < MAX_BULLETS; i++){
    if(bulletActive[i]){
      u8g2.drawPixel(bulletX[i], bulletY[i]);     // First pixel
      u8g2.drawPixel(bulletX[i] + 1, bulletY[i]); // Second pixel
    }
  }

  //rendering enemy bullet
  for(int i = 0; i < MAX_ENEMYBULLETS; i++){
    if(enemy_bulletActive[i]){
      u8g2.drawPixel(enemy_bulletX[i], enemy_bulletY[i]);     // First pixel
      u8g2.drawPixel(enemy_bulletX[i] + 1, enemy_bulletY[i]); // Second pixel
    }
  }

  //rendering enemyship
  byte sizeX;
  byte sizeY;
  byte *pEnemyship;
  for(int i = 0; i < MAX_ENEMYSHIP; i++){
    if(enemyship_active[i]){
      switch(ENEMY_VARIENT[i]){
        case 1: sizeX = 10; sizeY = 9;  if(idk1[i]){pEnemyship = enemyship1_inverted; break;}else{pEnemyship = enemyship1; break;}
        case 2: sizeX = 13; sizeY = 11; pEnemyship = enemyship2; break;
        case 3: sizeX = 9;  sizeY = 11; pEnemyship = enemyship3; break;
        case 4: sizeX = 13; sizeY = 13; pEnemyship = enemyship4; 
          if(!idk1[i]){
            byte arcX = EnemyX[i] - 1;
            byte arcY = EnemyY[i] + 6;
            long current_time = millis();
            long devide = 1000;
            long tier4_charging = (current_time - enemy_time_counter[i]) / devide;
            switch(tier4_charging){
              case 3: u8g2.drawHLine(0, arcY, 110); if(EnemyY[i] + 6 > y & EnemyY[i] + 6 <= y + 7 & !take_damage){heroship_health --; take_damage = true; take_damage_time = millis();} break;
              case 0: u8g2.drawArc(arcX, arcY, 7, 85, 170);
              case 1: u8g2.drawArc(arcX, arcY, 5, 85, 170);
              case 2: u8g2.drawArc(arcX, arcY, 3, 85, 170);
            }
          }
          break;
        case 5: sizeX = 15; sizeY = 11; pEnemyship = enemyship5; 
          if(!idk1[i]){
            long devide = 500;  long current_time = millis();
            byte line_startX = enemy_ticking_counter[i] - 5;
            byte line_startY = enemy_ticking_counter2[i] - 5;
            //long tier5_charging = (current_time - enemy_time_counter[i]) / devide;
            switch((current_time - enemy_time_counter[i]) / devide){
              case 0: case 2: case 4: u8g2.drawCircle(enemy_ticking_counter[i], enemy_ticking_counter2[i], 7); break;
              case 1: case 3: case 5: u8g2.drawLine(line_startX, line_startY,line_startX + 10, line_startY + 10); break;
              case 6: u8g2.drawDisc(enemy_ticking_counter[i], enemy_ticking_counter2[i], 7); if(x > enemy_ticking_counter[i] - 9 & x < enemy_ticking_counter[i] + 9 & y > enemy_ticking_counter2[i] - 9 & y < enemy_ticking_counter2[i] + 9 & !take_damage){heroship_health --; take_damage = true; take_damage_time = millis();} break;
            }
          }
          break;
      }
      u8g2.drawXBMP(EnemyX[i], EnemyY[i], sizeX, sizeY, pEnemyship);
    }
  }

  u8g2.sendBuffer();
}

/**************************************************************/

//joystick
void joystick_data_receive(){
  x_value = analogRead(A0);
  y_value = analogRead(A1);

  if (x_value > x_mid + dead_zone && right_allowed){ // right (plus)
    speed_x = map(x_value, x_mid + dead_zone, x_max, 1, 3);
  }else if (x_value < x_mid - dead_zone && left_allowed) { // left (minus)
    speed_x = map(x_value, x_mid - dead_zone, 0, 1, 3);
    speed_x = speed_x * -1;
  }else { // middle (plus)
    speed_x = 0;
  }

  if (y_value > y_mid + dead_zone && up_allowed){ // up (minus)
    speed_y = map(y_value, y_mid + dead_zone, y_max, 1, 3);
    speed_y = speed_y * -1;
  }else if (y_value < x_mid - dead_zone && down_allowed){ // down (plus)
    speed_y = map(y_value, y_mid - dead_zone, 0, 1, 3);
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
  //generate bullet
  unsigned long time_now = millis();
  if(!digitalRead(2) & time_now - time_last_bullet >= 500){
    for (int i = 0; i <= MAX_BULLETS; i++) {
      if(!bulletActive[i]){
        bulletX[i] = x + 12; // Start at the tip of the spaceship
        bulletY[i] = y + 4;  // Center vertically
        bulletActive[i] = true;
        time_last_bullet = time_now;
        /*
        Serial.print("bullet_activated: ");
        Serial.println(bullet_activated);
        Serial.print("i: ");
        Serial.println(i);
        Serial.print(bulletX[i]);
        Serial.println(bulletY[i]);
        */
        break;
        
      }
    }
  }

  //update and recycle bullet
  for(int i = 0; i < MAX_BULLETS; i++){
    if(!bulletActive[i]){continue;}
    bulletX[i] += 2;
    //if(!bulletActive[i]){continue;}
    if(bulletX[i] > 128){
      bulletActive[i] = false;
      continue;
    }
  }
}

/**************************************************************/

void enemyShip_Logics(){
  //generate enemies
  unsigned long time_now = millis();
  if(time_now - time_last_ship >= 5000){
    for (int i = 0; i < MAX_ENEMYSHIP; i++) {
      if(enemyship_active[i]){continue;}
      if(!enemyship_active[i]){
        ENEMY_VARIENT[i] = random(1, 6);
        //ENEMY_VARIENT[i] = 5;
        enemy_ticking_counter[i] = 0;
        EnemyX[i] = 127;
        //EnemyY[i] = random(1, 55);
        switch(ENEMY_VARIENT[i]){
          case 1: EnemyY[i] = y;              enemy_health[i] = 3; break;
          case 2: EnemyY[i] = y - 1;          enemy_health[i] = 5; break;
          case 3: EnemyY[i] = random(1, 55);  enemy_health[i] = 5; break;
          case 4: EnemyY[i] = random(1, 55);  enemy_health[i] = 5; idk1[i] = true; break;
          case 5: EnemyY[i] = random(1, 55);  enemy_health[i] = 7; idk1[i] = true; break;
        }

        enemyship_active[i] = true;
        time_last_ship = time_now;
        break;
      }
    }
  }
  
  //Enemy_actions
  for(int i = 0; i < MAX_ENEMYSHIP; i++){
    if(!enemyship_active[i]){continue;}

    switch(ENEMY_VARIENT[i]){
      case 1: tier1_Enemy(i); break;//tier 1 ship
      case 2: tier2_Enemy(i); break;//tier 2 ship
      case 3: tier3_Enemy(i); break;//tier 3 ship(cloaker)
      case 4: tier4_Enemy(i); break;//tier 4 ship(tank)
      case 5: tier5_Enemy(i); break;//tier 5 ship(platform)
    }
  
    //recycle enemies  
    if(EnemyX[i] <= 0 | enemy_health[i] <= 0){
    if(!regeneration_tool_exist & random(1, 21) == 1){generate_regeneration_tool(EnemyX[i], EnemyY[i]);}
    //if(!regeneration_tool_exist){generate_regeneration_tool(EnemyX[i], EnemyY[i]);}
    enemyship_active[i] = false;
    Enemy_remain --;
    continue;
    }
  }  
}

void tier1_Enemy(byte i){
  char distance = EnemyX[i] - x;
  if(distance <= 50 & distance >= -10 & !idk1[i]){//idk here use to makeship comming back
    if(enemy_ticking_counter[i] > 5){
      enemy_speed = 1;
      enemy_ticking_counter[i] = 0;
    }else{
      enemy_speed = 0;
      enemy_ticking_counter[i]++;
    }
    //enemyship shooting
    enemy_Bullet(i, EnemyX[i], EnemyY[i]);
  }else if(distance < -10 | idk1[i]){
    idk1[i] = true;
    enemy_speed = -1;
  }else{
    enemy_speed = 1;
  }

  if(idk1[i] & distance >= 50){
    idk1[i] = false;
  }
  EnemyX[i] -= enemy_speed;

  if(EnemyY[i] < y & distance >= 5 & enemy_ticking_counter[i] >= 1){//MOVE ON y
    EnemyY[i] ++;
  }else if(EnemyY[i] > y & distance >= 5 & enemy_ticking_counter[i] >= 1){
    EnemyY[i] --;
  }

}
void tier2_Enemy(byte i){
  byte distance_to_hero = EnemyX[i] - x;
  byte enemyship_center = EnemyY[i] + 5;
  //Enemy_actions

  if(distance_to_hero - x < 30 & 0 < distance_to_hero & enemyship_center > y & enemyship_center < y + 8){ //if Enemy is -5 to 20 pixel in front of hero
    if(enemy_ticking_counter[i] > 5){
      enemy_speed = 1;
      enemy_ticking_counter[i] = 0;
    }else{
      enemy_speed = 0;
      enemy_ticking_counter[i]++;
    }
    //enemyship shooting
    enemy_Bullet(i, EnemyX[i], EnemyY[i]);
  }else{
    enemy_speed = 1;
  }  
  EnemyX[i] -= enemy_speed;
}
void tier3_Enemy(byte i){
  byte enemyship_center = EnemyY[i] + 5;
  if(EnemyX[i] - x < 50){
    enemy_speed = 1;

    if(EnemyY[i] < y){
      EnemyY[i] ++;
    }
    if(EnemyY[i] > y){
      EnemyY[i] --;
    }
  }else{
    if(enemy_ticking_counter[i] >= 1){
      enemy_speed = 1;
      enemy_ticking_counter[i] = 0;
    }else{
      enemy_speed = 0;
      enemy_ticking_counter[i] = 1;
    }
  }
  EnemyX[i] -= enemy_speed;
}
void tier4_Enemy(byte i){
  //move on X:
  if(EnemyX[i] > 113){EnemyX[i] --;}
  //move on Y
  
  if(enemy_ticking_counter[i] >= 20 & idk1[i]){
    if(EnemyY[i] + 2 < y){
      EnemyY[i] ++;
    }
    if(EnemyY[i] + 2 > y){
      EnemyY[i] --;
    }
    enemy_ticking_counter[i] = 0;
  }
  enemy_ticking_counter[i] ++;

  //charge and shoot
  if(EnemyY[i] + 2 == y & idk1[i]){//if its infront of heroship, and is done cool down
    enemy_time_counter[i] = millis();
    Serial.println(enemy_time_counter[i]);
    idk1[i] = false; //in this case, "idk!" means cool down is done
  }

  //and end cool down
  if(millis() - enemy_time_counter[i] >= 13000){
    idk1[i] = true;
  }
  
}
void tier5_Enemy(byte i){
  //move on X:
  if(EnemyX[i] > 115){EnemyX[i] --;}

  //launch missile
  if(idk1[i]){//idk means done cool down, ready for next launch
    enemy_ticking_counter[i] = x + 5; //ticking counter use to keep x and y coord
    enemy_ticking_counter2[i] = y + 4;
    enemy_time_counter[i] = millis();
    idk1[i] = false;
  }

  //reset cool down
  long current_time = millis();
  if(current_time - enemy_time_counter[i] >= 13000){idk1[i] = true;}
}
void tier100_hp(){//the add hp tool for ship thing

}
/**************************************************************/

void enemy_Bullet(byte Enemy, byte x_this, byte y_this){
  unsigned long time_now = millis();
  byte bullet_generating_positionY;
  if(time_now - enemy_time_last_bullet[Enemy] >= 3000){
    for(int i = 0; i < MAX_ENEMYBULLETS; i++){
      if(!enemy_bulletActive[i]){
        switch(ENEMY_VARIENT[Enemy]){
          case 1: bullet_generating_positionY = 4; break;
          case 2: bullet_generating_positionY = 5; break;
        }

        enemy_bulletX[i] = x_this - 1;
        enemy_bulletY[i] = y_this + bullet_generating_positionY;
        enemy_bulletActive[i] = true;
        enemy_time_last_bullet[Enemy] = time_now;
        break;
      }
    }
  }
}

void ticking_enemybullet(){
  for(int i = 0; i < MAX_ENEMYBULLETS; i++){
    if(!enemy_bulletActive[i]){continue;}
    enemy_bulletX[i] -= 2;
    if(enemy_bulletX[i] <= 1){
      enemy_bulletActive[i] = false;
      continue;
    }
  }
}

/**************************************************************/

void hitBox(){
  //heroship hitbox:(smaller than the ship because the ship is in inregular shape)
  byte right_hitbox = x + 8;
  byte left_hitbox = x + 1;
  byte top_hitbox = y + 1;
  byte down_hitbox = y + 7;

  byte Enemy_left;
  byte Enemy_top;
  byte Enemy_right;
  byte Enemy_down;

  //if heroship collide with bullet
  for(int i = 0; i < MAX_ENEMYBULLETS; i++){
    if(enemy_bulletActive[i] & enemy_bulletX[i] <= right_hitbox & enemy_bulletX[i] >= left_hitbox & enemy_bulletY[i] <= down_hitbox & enemy_bulletY[i] >= top_hitbox){
      if(!forcesheld_activate & !take_damage){
        heroship_health --;
        take_damage = true;
        take_damage_time = millis();
      }
      enemy_bulletActive[i] = false;
    }
  }

  //if heroship collide with enemyship & if enemyship collide with bullet & if ray ship beam a ray
  for(int i = 0; i < MAX_ENEMYSHIP; i++){
    if(!enemyship_active[i]){continue;}

    switch(ENEMY_VARIENT[i]){
      case 1:
        Enemy_left = EnemyX[i] + 2;
        Enemy_top = EnemyY[i] + 1; 
        Enemy_right = EnemyX[i] + 8;
        Enemy_down = EnemyY[i] + 7;
        break;
      case 2:
        Enemy_left = EnemyX[i] + 2;
        Enemy_top = EnemyY[i] + 2; 
        Enemy_right = EnemyX[i] + 11;
        Enemy_down = EnemyY[i] + 8;
        break;
      case 3:
        Enemy_left = EnemyX[i];
        Enemy_top = EnemyY[i] + 1; 
        Enemy_right = EnemyX[i] + 8;
        Enemy_down = EnemyY[i] + 9;
        break;
      case 4:
        Enemy_left = EnemyX[i] + 1;
        Enemy_top = EnemyY[i]; 
        Enemy_right = EnemyX[i] + 11;
        Enemy_down = EnemyY[i] + 12;
      case 5:
        Enemy_left = EnemyX[i] + 2;
        Enemy_top = EnemyY[i]; 
        Enemy_right = EnemyX[i] + 14;
        Enemy_down = EnemyY[i] + 10;
    }

    //if enemy collide with bullet:
    for(int b = 0; b < MAX_BULLETS; b++){
      if(!bulletActive[b]){continue;}
      if(bulletX[b] >= Enemy_left & bulletX[b] <= Enemy_right & bulletY[b] >= Enemy_top & bulletY[b] <= Enemy_down){
        enemy_health[i] --;
        bulletActive[b] = false;
        continue;
      }
    }


    //if enemy in hero hitbox
    if(collide_hitbox_check(right_hitbox, left_hitbox, top_hitbox, down_hitbox, Enemy_right, Enemy_left, Enemy_top, Enemy_down)){
      enemy_collide(i);
    }

  }

  //regeneration tool collide with heroship
  if(regeneration_tool_exist & collide_hitbox_check(right_hitbox, left_hitbox, top_hitbox, down_hitbox, regeneration_toolX, regeneration_toolX + 6, regeneration_toolY, regeneration_toolY + 5)){heroship_health += 2; regeneration_tool_exist = false;}

  //if heroship collide with boss
}

bool collide_hitbox_check(byte right_hitbox, byte left_hitbox, byte top_hitbox, byte down_hitbox, byte Enemy_right, byte Enemy_left, byte Enemy_top, byte Enemy_down){
  if(Enemy_top >= top_hitbox & Enemy_top <= down_hitbox){
    if(Enemy_left >= left_hitbox & Enemy_left <= right_hitbox){//enemy top left corner
      return true;
    }
    if(Enemy_right >= left_hitbox & Enemy_right <= right_hitbox){//top right corner
        return true;
    }
  }
  if(Enemy_down >= top_hitbox & Enemy_down <= down_hitbox){
    if(Enemy_left >= left_hitbox & Enemy_left <= right_hitbox){//bottom left corner
      return true;
    }
    if(Enemy_right >= left_hitbox & Enemy_right <= right_hitbox){//top bottom corner
      return true;
    }
  }
  //if hero in enemy hitbox
  if(top_hitbox >= Enemy_top & top_hitbox <= Enemy_down){
    if(left_hitbox >= Enemy_left & left_hitbox <= Enemy_right){//heroship top left
      return true;
    }
    if(right_hitbox >= Enemy_left & right_hitbox <= Enemy_right){//heroship top right
      return true;
    }
  }
  if(down_hitbox >= Enemy_top & down_hitbox <= Enemy_down){
    if(left_hitbox >= Enemy_left & left_hitbox <= Enemy_right){//heroship bottom left
      return true;
    }
    if(right_hitbox >= Enemy_left & right_hitbox <= Enemy_right){//heroship bottom right
      return true;
    }
  }
  return false;
}

void enemy_collide(byte i){
  if(!forcesheld_activate & !take_damage){
    heroship_health -= 2;
    take_damage = true;
    take_damage_time = millis();
  }
  enemy_health[i] -= 10;
}

/**************************************************************/

void forcesheld(){
  long current_time = millis();
  if(current_time - forcesheld_ticking >= 1000 & cooldown > 0 & !forcesheld_ready){
    cooldown --;
    forcesheld_ticking = current_time;
  }

  if(cooldown <= 0){forcesheld_ready = true;}

  if(!digitalRead(7) & forcesheld_ready){
    forcesheld_activate = true;
    forcesheld_ready = false;
    cooldown = 38;
    forcesheld_trigger_time = current_time;
  }

  if(current_time - forcesheld_trigger_time >= 8000){
    forcesheld_activate = false;
  }
}
/**************************************************************/

void invincible_time(){
  if(!take_damage){return;}
  long current_time = millis();
  if(current_time - take_damage_time >= 1000){
    take_damage = false;
  }
}

/**************************************************************/

//generating regeneration tool and deliver it with ally transportation ship
void regeneration_tool_auto_generation(){
  long current_time = millis();
  if(current_time - regeneration_tool_generation_time >= 30000 & !regeneration_tool_exist){
    if(transport_shipX < 30){
      transport_shipX ++;
    }else{generate_regeneration_tool(transport_shipX, transport_shipY);}
  }

  if(regeneration_tool_exist & transport_shipX >= 0){
    transport_shipX --;
  }
}
void ticking_regeneration_tool(){
  if(!regeneration_tool_exist){return;}
  if(regeneration_toolX >= 0){
    if(regeneration_tool_ticking_counter >= 8){
      regeneration_toolX --;
      regeneration_tool_ticking_counter = 0;
    }
    regeneration_tool_ticking_counter ++;
  }else{regeneration_tool_exist = false;}

}
void generate_regeneration_tool(char initialX, char initialY){
  regeneration_toolX = initialX;
  regeneration_toolY = initialY;
  regeneration_tool_generation_time = millis();
  regeneration_tool_exist = true;
}
/**************************************************************/
/*
regeneration tool:

00110110
01111111
01111111
00111110
00011100
00001000

0x36, 0x7F, 0x7F, 0x3E, 0x1C, 0x08
*/