/*
   Hello Arduboy World from Mr Speaker.
 
   TODO:
   - pool ball physics
   - splash screen
   - choose character
   - projectile/action
   - keep set score
   - slight delay between get_pellet and set_pellet
*/
#include <Arduboy2.h>
#include <Sprites.h>
Arduboy2 ab;
Sprites sprites;
BeepPin1 beep;

constexpr uint8_t WIN_NUM = 5;
constexpr byte W = 8;
constexpr byte H = 8;
constexpr uint8_t frameRate = 30;
constexpr uint16_t toneGet1 = beep.freq(200);
constexpr uint16_t toneGet2 = beep.freq(261);
constexpr uint8_t toneTimeBeep = 250 / (1000 / 30);
 
const uint8_t img[] PROGMEM = {
  8, 8,
 
  0b00000000,
  0b01110000,
  0b00111100,
  0b00101110,
  0b00111110,
  0b00101110,
  0b00111100,
  0b01110000,
 
  0b00000000,
  0b00100001,
  0b01111111,
  0b01101101,
  0b00111110,
  0b00111110,
  0b00011100,
  0b00000000,
 
  0b00001100,
  0b00111110,
  0b01110100,
  0b01111100,
  0b01110100,
  0b00111100,
  0b00001110,
  0b00000000,
 
  0b00000000,
  0b00111000,
  0b01111100,
  0b01111100,
  0b11011010,
  0b11111110,
  0b01000010,
  0b00000000,
 
  0b00010000,
  0b00000000,
  0b00111000,
  0b00101110,
  0b01101101,
  0b00111000,
  0b01000000,
  0b00001000,
 
  0b00000000,
  0b01000000,
  0b00011000,
  0b01110100,
  0b11011110,
  0b00110000,
  0b00000010,
  0b00000000,
};
 
constexpr byte P1_ID = 0;
constexpr byte P2_ID = 1;
constexpr char P1_LEFT = UP_BUTTON;
constexpr char P1_RIGHT = LEFT_BUTTON;
constexpr char P1_ACTION = DOWN_BUTTON;
constexpr char P2_LEFT = B_BUTTON;
constexpr char P2_RIGHT = A_BUTTON;
constexpr char P2_ACTION = RIGHT_BUTTON;
 
struct Pop {
  byte id;
  float vx;
  float vy;
  float x;
  float y;
  int hit;
  int dir;
  char score;
};
 
struct Star {
  float x;
  float y;
  float z;
};
float stars_dx = 0;
float stars_dy = 0;
 
struct Pellet {
  int x;
  int y;
  int hit;
};
 
int state = 0;
Pop p1 = {};
Pop p2 = {} ;
Pellet pel = {};

bool champion = 0;
char text[16];
char t;
const int MAX_STARS = 20;
Star stars[MAX_STARS];
 
void setup() {
  ab.boot();
  ab.flashlight();
  ab.audio.begin();
  ab.waitNoButtons();
  beep.begin();
 
  ab.setFrameRate(frameRate);
 
  p1.id = P1_ID;
  p2.id = P2_ID;
  init_game();
}
 
void init_game() {
  state = 0;
  t = 0;
};
 
float dist(int x1, int y1, int x2, int y2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  return sqrt(dx * dx + dy * dy);
}
 
void init_stars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(WIDTH);
    stars[i].y = random(HEIGHT);
    stars[i].z = random(3) + 1;
  }
}
 
void update_stars() {
  float dx = p1.vx + p2.vx;
  float dy = p1.vy + p2.vy;
  float m = dist(dx, dy, 0, 0);
  dx = m ? dx / m : 0;
  dy = m ? dy / m : 0;
  stars_dx += dx > stars_dx ? 0.01 : -0.01;
  stars_dx += dx > stars_dx ? 0.01 : -0.01;
 
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x += 0.4 * stars_dx * stars[i].z;
    stars[i].y += 0.4 * dy * stars[i].z;
    if (stars[i].x <= 0) {
      stars[i].x = WIDTH;
    }
    if (stars[i].x > WIDTH) {
      stars[i].x = 0;
    }
    if (stars[i].y < 0) {
      stars[i].y = HEIGHT;
    }
    if (stars[i].y > HEIGHT) {
      stars[i].y = 0;
    }
  }
}
 
void render_stars(int t) {
  const int hw = WIDTH / 2;
  const int hh = HEIGHT / 2;
  for (int i = 0; i < MAX_STARS; i++) {
    ab.drawPixel(stars[i].x, stars[i].y, WHITE);
  }
}
 
void set_dir(Pop *p, float sp) {
  const float turn_friction = 1;//0.99;
  if (p->dir == 0) {
    p->vx *= turn_friction;
    p->vy -= sp;
  }
  else if (p->dir == 1) {
    p->vx += sp;
    p->vy *= turn_friction;
  }
  else if (p->dir == 2) {
    p->vx *= turn_friction;
    p->vy += sp;
  }
  else {
    p->vx -= sp;
    p->vy *= turn_friction;
  }
}
 
void play_tone(uint16_t tone, uint8_t time) {
  // beep.tone(tone, time);
}
 
void get_pellet(Pellet *pel, Pop *p) {
  play_tone(p->id == P1_ID ? toneGet1 : toneGet2, toneTimeBeep);
  bool win = false;
  p->score++;
  for (int i = 0; i < 10; i++ ) {
    sprintf(text, "%2d", WIN_NUM - p->score);
    ab.setCursor((p->id == P2_ID ? WIDTH - 12 : -3), i * 10);
    ab.print(text);
  }

  pel->hit = 50;

  if (p->score == WIN_NUM) {
    win = true;
    champion = p->id;
    ab.display();
    ab.delayShort(500);
    init_game();
  }
}
 
void set_pellet() {
  bool far_enough = false;
  while (!far_enough) {
    pel.x = random(WIDTH - 20) + 10;
    pel.y = random(HEIGHT - 10) + 5;
    int d1 = dist(p1.x + WIDTH, p1.y + HEIGHT, pel.x + WIDTH, pel.y + HEIGHT);
    int d2 = dist(p2.x + WIDTH, p2.y + HEIGHT, pel.x + WIDTH, pel.y + HEIGHT);
    if (d1 > 20 && d2 > 20) {
      far_enough = true;
    }
  }
  pel.hit = 0;
}
 
void reset_arena() {
  p1.x = 0;
  p1.y = HEIGHT / 2 - 4;
  p1.dir = 1;
  p1.vx = 0;
  p1.vy = -0.2;
  p1.score = 0;
 
  p2.x = WIDTH - 8;
  p2.y = HEIGHT / 2 - 4;
  p2.vx = 0;
  p2.vy = 0.2;
  p2.dir = 3;
  p2.score = 0;
 
  set_pellet();
  pel.hit = 20;
  //pel.x = 500;
  //pel.y = 500;
 
  ab.clear();
  play_tone(toneGet2, toneTimeBeep);
  ab.delayShort(100);
  play_tone(toneGet1, toneTimeBeep);
  ab.delayShort(100);
  play_tone(toneGet2, toneTimeBeep);
  ab.delayShort(100);
  play_tone(toneGet1, toneTimeBeep);
  ab.delayShort(100);
 
  init_stars();
}
 
void render_arena(int t) {
  render_stars(t);
 
  sprintf(text, "%1d - %1d", WIN_NUM - p1.score, WIN_NUM - p2.score);
  ab.setCursor(WIDTH / 2 - 12, 1);
  ab.print(text);
  ab.fillCircle(pel.x + 4, pel.y + 4, 5, BLACK);
  if (pel.hit % 4 == 0) {
    sprites.drawOverwrite(pel.x, pel.y, img, t % 2 == 0 ? 4 : 5);
  }
  if (champion != 0 || t % 2 == 0) {
    sprites.drawOverwrite(p1.x, p1.y, img, p1.dir);
  }
  if (champion != 1 || t % 2 == 0) {
    sprites.drawOverwrite(p2.x, p2.y, img, p2.dir);
  }
}
 
void wrap_player(Pop *p) {
  if (p->x > WIDTH) p->x = -W;
  if (p->x < -W) p->x = WIDTH;
  if (p->y > HEIGHT) p->y = -H;
  if (p->y < -H) p->y = HEIGHT;
}

void add_particles(float x, float y) {
  // TODO
}
 
void update_arena() {
  beep.timer();
 
  update_stars();
 
  // User input
  ab.pollButtons();
 
  if (ab.justPressed(P1_LEFT)) p1.dir = ((p1.dir + 1) + 4) % 4;
  if (ab.justPressed(P1_RIGHT)) p1.dir = ((p1.dir - 1) + 4) % 4;
  if (ab.justPressed(P1_ACTION)) { }
 
  if (ab.justPressed(P2_LEFT)) p2.dir = ((p2.dir + 1) + 4) % 4;
  if (ab.justPressed(P2_RIGHT)) p2.dir = ((p2.dir - 1) + 4) % 4;
  if (ab.pressed(P2_ACTION)) { }
 
  if (ab.pressed(P1_ACTION) && ab.pressed(P2_ACTION)) {
    init_game();
  }
 
  // Physics
  float sp = 0.1;
  set_dir(&p1, sp);
  set_dir(&p2, sp);
 
  p1.x += p1.vx;
  p1.y += p1.vy;
  p2.x += p2.vx;
  p2.y += p2.vy;
 
  const float friction = 0.94;
  p1.vx *= friction;
  p1.vy *= friction;
  p2.vx *= friction;
  p2.vy *= friction;
 
  // Wrap
  wrap_player(&p1);
  wrap_player(&p2);
 
  // Pickups
  if (pel.hit > 0) {
    if(--pel.hit == 0) {
      set_pellet();
    }
  }
  else {
    int d1 = dist(p1.x, p1.y, pel.x, pel.y);
    int d2 = dist(p2.x, p2.y, pel.x, pel.y);
    if (d1 < d2 && d1 < 8) {
      get_pellet(&pel, &p1);
      
    }
    if (d2 < d1 && d2 < 8) {
      get_pellet(&pel, &p2);
    }
  }

  if (p1.hit > 0) p1.hit--;
  if (p2.hit > 0) p2.hit--;  
  if (dist(p1.x, p1.y, p2.x, p2.y) < 10) {

    if (p1.hit == 0 && p2.hit == 0) {
      p1.hit = 20;
      p1.hit = 20;
      
      //p1.vx = -p1.vx * 2;
      //p1.vy = -p1.vy * 2;
      float tx = p1.vx;
      float ty = p1.vy;
      p1.vx = p2.vx * 3;
      p1.vy = p2.vy * 3;
      p2.vx = tx * 3;
      p2.vy = ty * 3;

      float dx = (p1.x + p2.x) / 2;
      float dy = (p1.y + p2.y) / 2;
      add_particles(dx, dy);
      
      //newVelX1 = (firstBall.speed.x * (firstBall.mass – secondBall.mass) + (2 * secondBall.mass * secondBall.speed.x)) / (firstBall.mass + secondBall.mass);
      //newVelY1 = (firstBall.speed.y * (firstBall.mass – secondBall.mass) + (2 * secondBall.mass * secondBall.speed.y)) / (firstBall.mass + secondBall.mass);
      //newVelX2 = (secondBall.speed.x * (secondBall.mass – firstBall.mass) + (2 * firstBall.mass * firstBall.speed.x)) / (firstBall.mass + secondBall.mass);
      //newVelY2 = (secondBall.speed.y * (secondBall.mass – firstBall.mass) + (2 * firstBall.mass * firstBall.speed.y)) / (firstBall.mass + secondBall.mass);
    }
       
    p1.x += random(2) - 1;
    p1.y += random(2) - 1;
    p2.x += random(2) - 1;
    p2.y += random(2) - 1;
   
    
  }
}
 
void render_splash() {
  ab.clear();
  ab.drawRect(0, 0, 100, 100, WHITE);
  ab.setCursor(35, 35);
  ab.print(F("READY"));
}
 
void update_splash() {
  bool pressed = ab.pressed(P1_ACTION) || ab.pressed(P2_ACTION);
  if (pressed) {
    state = 1;
    t = 0;
    reset_arena();
  }
}
 
void update_game() {
  if (state == 0) {
    update_splash();
  } else {
    update_arena();
  }
}
 
void render_game(int t) {
  ab.clear();
  if (state == 0) {
    render_splash();
  } else {
    render_arena(t);
  }
  ab.display();
}
 
void loop() {
  if (!(ab.nextFrame())) return;
  update_game();
  render_game(t++);
}
