/*
   Hello Arduboy World from Mr Speaker.
 
   TODO:
   - choose character
   - keep set score
   - juice on pickup
   - particles on collision
   - better sounds
*/
#include <Arduboy2.h>
#include <Sprites.h>

Arduboy2 ab;
Sprites sprites;
BeepPin1 beep;

constexpr uint8_t frameRate = 30;

constexpr byte W = 8;
constexpr byte H = 8;

constexpr uint8_t WIN_NUM = 5;
constexpr int PEL_FLASH_TIME = 2;
constexpr int PEL_SPAWN_TIME = 30;
constexpr int COLL_MIN_TIME = 20;
constexpr float BOUNCE_MULTIPLIER = 3;

constexpr uint16_t toneGet1 = beep.freq(200);
constexpr uint16_t toneGet2 = beep.freq(261);
constexpr uint8_t toneTimeBeep = 250 / (1000 / 30);

// Can this be PROGMEM?
constexpr float SIN_T[] = {
  0,
  0.3826,
  0.7071,
  0.9238,
  1,
  0.9238,
  0.7071,
  0.3826,
  0,
  -0.3826,
  -0.7071,
  -0.9238,
  -1,
  -0.9238,
  -0.7071,
  -0.3826
};

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
  char boost;
};
 
struct Star {
  float x;
  float y;
  float z;
};
float stars_dx = 0;
float stars_dy = 0;
 
int state = 0;
Pop p1 = {};
Pop p2 = {} ;
Pop pel = {};

bool champion = 0;
int next_pellet = 0;

char text[16];
unsigned int t;

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
  init_stars_3d();
};
 
float dist(int x1, int y1, int x2, int y2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  return sqrt(dx * dx + dy * dy);
}
 
void init_stars_3d() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(WIDTH) - (WIDTH / 2);
    stars[i].y = random(HEIGHT) - (HEIGHT / 2);
    stars[i].z = random(WIDTH);
  }
}
void update_stars_3d() {
  for (int i = 0; i < MAX_STARS; i++) {
    if (stars[i].z--<=0) {
      stars[i].z = WIDTH;
    }
  }
}

void render_stars_3d() {
  const int hw = WIDTH / 2;
  const int hh = HEIGHT / 2;
  for (int i = 0; i < MAX_STARS; i++) {
    float z = stars[i].z / 10;
    if (z != 0) {
      ab.drawPixel(hw + (stars[i].x / z), hh + (stars[i].y / z), WHITE);
    }
  }
}

void init_stars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(WIDTH);
    stars[i].y = random(HEIGHT);
    stars[i].z = random(3) + 2;
  }
}
 
void update_stars() {
  float dx = p1.vx + p2.vx;
  float dy = p1.vy + p2.vy;
  float m = dist(dx, dy, 0, 0);
  dx = -(m ? dx / m : 0);
  dy = -(m ? dy / m : 0);
  stars_dx += dx > stars_dx ? 0.02 : -0.02;
  stars_dy += dy > stars_dy ? 0.02 : -0.02;
 
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x += 0.4 * stars_dx * stars[i].z;
    stars[i].y += 0.4 * stars_dy * stars[i].z;
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
 
void get_pellet(Pop *pel, Pop *p) {
  play_tone(p->id == P1_ID ? toneGet1 : toneGet2, toneTimeBeep);
  bool win = false;
  p->score++;
  for (int i = 0; i < 10; i++ ) {
    sprintf(text, "%2d", WIN_NUM - p->score);
    ab.setCursor((p->id == P2_ID ? WIDTH - 12 : -3), i * 10);
    ab.print(text);
  }

  next_pellet = PEL_SPAWN_TIME;

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
      float dir = random(360) * DEG_TO_RAD;
      pel.vx = cos(dir); 
      pel.vy = sin(dir);
      pel.score++;
    }
  }
}
 
void reset_arena() {
  p1.x = 0;
  p1.y = HEIGHT / 2 - 4;
  p1.dir = 1;
  p1.vx = 0;
  p1.vy = -0.2;
  p1.score = 0;
  p1.boost = 0;
 
  p2.x = WIDTH - 8;
  p2.y = HEIGHT / 2 - 4;
  p2.vx = 0;
  p2.vy = 0.2;
  p2.dir = 3;
  p2.score = 0;
  p2.boost = 0;
 
  next_pellet = PEL_SPAWN_TIME;

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
 
void wrap_player(Pop *p) {
  if (p->x > WIDTH) p->x = -W;
  if (p->x < -W) p->x = WIDTH;
  if (p->y > HEIGHT) p->y = -H;
  if (p->y < -H) p->y = HEIGHT;
}

void add_particles(float x, float y) {
  // TODO
}
 
void update_arena(int t) {
  beep.timer();
 
  update_stars();
 
  // User input
  ab.pollButtons();

  if (p1.boost < 50) p1.boost++;
  if (p2.boost < 50) p2.boost++;
  bool p1boost = false;
  bool p2boost = false;
  if (ab.justPressed(P1_LEFT)) p1.dir = ((p1.dir + 1) + 4) % 4;
  if (ab.justPressed(P1_RIGHT)) p1.dir = ((p1.dir - 1) + 4) % 4;
  if (ab.justPressed(P1_ACTION)) {
    if (p1.boost > 10) {
      p1.boost -= 2;
      p1boost = true;
    }
  }
 
  if (ab.justPressed(P2_LEFT)) p2.dir = ((p2.dir + 1) + 4) % 4;
  if (ab.justPressed(P2_RIGHT)) p2.dir = ((p2.dir - 1) + 4) % 4;
  if (ab.pressed(P2_ACTION)) { 
    if (p2.boost > 10) {
      p2.boost -= 2;
      p2boost = true;
    }  
  }
 
  if (ab.pressed(P1_ACTION) && ab.pressed(P2_ACTION)) {
    init_game();
  }

  // Physics
  float sp = 0.1;
  float sp_b = 0.3;
  set_dir(&p1, p1boost ? sp_b : sp);
  set_dir(&p2, p2boost  ? sp_b : sp);
 
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
  if (pel.score > 1) {
    pel.x += pel.vx;
    pel.y += pel.vy;
  }
  else {
    pel.y += SIN_T[(int)(t / 2.0) % 16] / 2;
  }
  wrap_player(&pel);
  
  if (next_pellet > 0) {
    if (--next_pellet == 0) {
      set_pellet();
    }
  }
  
  if (next_pellet == 0) {
    int d1 = dist(p1.x, p1.y, pel.x, pel.y);
    int d2 = dist(p2.x, p2.y, pel.x, pel.y);
    if (d1 < 8 || d2 < 8) {
      get_pellet(&pel, d1 < d2 ? &p1 : &p2);
    }
  }

  if (p1.hit > 0) p1.hit--;
  if (p2.hit > 0) p2.hit--;  
  if (dist(p1.x, p1.y, p2.x, p2.y) < 10) {

    if (p1.hit == 0 && p2.hit == 0) {
      p1.hit = COLL_MIN_TIME;
      p2.hit = COLL_MIN_TIME;

      float tx = p1.vx;
      float ty = p1.vy;
      p1.vx = p2.vx * BOUNCE_MULTIPLIER;
      p1.vy = p2.vy * BOUNCE_MULTIPLIER;
      p2.vx = tx * BOUNCE_MULTIPLIER;
      p2.vy = ty * BOUNCE_MULTIPLIER;

      // Hit location
      float dx = (p1.x + p2.x) / 2;
      float dy = (p1.y + p2.y) / 2;
      add_particles(dx, dy);
      
    }
    // MOve around a lil bit.
    // not needed now we bounce so much
    // but probably good to ensure never stuck
    p1.x += random(2) - 1;
    p1.y += random(2) - 1;
    p2.x += random(2) - 1;
    p2.y += random(2) - 1;
   
    
  }
}

void render_arena(int t) {
  render_stars(t);

  // Score
  sprintf(text, "%1d:%1d", WIN_NUM - p1.score, WIN_NUM - p2.score);
  ab.setCursor(WIDTH / 2 - 12, 1);
  ab.print(text);

  // Pellet
  if (next_pellet == 0) {
    ab.fillCircle(pel.x + 4, pel.y + 4, 5, BLACK);
    sprites.drawOverwrite(pel.x, pel.y, img, t % 2 == 0 ? 4 : 5);
  }

  // Players
  if (champion != 0 || t % 2 == 0) {
    sprites.drawOverwrite(p1.x, p1.y, img, p1.dir);
  }
  if (champion != 1 || t % 2 == 0) {
    sprites.drawOverwrite(p2.x, p2.y, img, p2.dir);
  }
}
 
void render_splash() {
  ab.clear();
  render_stars_3d();

  byte CENTRE = HEIGHT / 2;
  
  ab.setCursor(WIDTH / 2 - 15, CENTRE - 3);
  ab.print(F("READY"));

  sprites.drawOverwrite(15, (SIN_T[(t / 2) % 16] * 2) + CENTRE - (H / 2), img, 1);  
  sprites.drawOverwrite(WIDTH - 15 - W, (SIN_T[((t / 2) + 3) % 16] * 2) + CENTRE - (H / 2), img, 3);
}
 
void update_splash() {
  bool pressed = ab.pressed(P1_ACTION) || ab.pressed(P2_ACTION);
  if (pressed) {
    state = 1;
    t = 0;
    reset_arena();
  }
  update_stars_3d();
}
 
void update_game(int t) {
  if (state == 0) {
    update_splash();
  } else {
    update_arena(t);
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
  update_game(t);
  render_game(t);
  t++;
}
