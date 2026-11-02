#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <math.h>
#include <string.h>
//------------WINDOW---------------//
#define HUD_H 4    // wysokosc paska statystyk
#define BORDER 1
//----------CONTROLS---------------//
#define QUIT 'q'        //exit the game
#define SLOWER 'o'      //change the
#define FASTER 'p'      //swallows speed
//----------PLAYER MOVEMENT---------//
#define GORA 'w'
#define DOL 's'
#define LEWO 'a'
#define PRAWO 'd'
//-----------SWALLOW SPEED-------------//
#define SPEED_SCALE 0.6f
//-----------STARS------------------//
#define MAX_STARS 50     //max 50 stars on screen and size of the array holding all the stars       
#define STARS_MIN_V 1       //stars 
#define STARS_MAX_V 4       //velocities
//------------HUNTERS--------------//
#define MAX_HUNTERS 5       //game starts from max 3 hunters on screen and then this number increases until it hits MAX_HUNTERS
#define HUNTER_STATE_NORMAL 0   //hunters normal movement
#define HUNTER_STATE_PAUSE 1    //hunters pause/wait before dashing
#define HUNTER_STATE_DASH 2     //hunters dash
//------------TAXI-----------------//
#define TAXI_KEY 't'        //t to call taxi
#define TAXI_SPEED 1.0f     //taxis speed
//------------FOG-----------------//
#define FOG_RISE 4      //how many columns (left and right) increase

//-----------STRUCTS-----------//

typedef struct {
    const char *name;   // (kind of) players name 
    int level;          // (kind of) games level
    int stars_collected;    //how many stars swallow got
    int stars_needed;       //how many stars swallow needs to beat the game
    int time_left;      //how long until the games over
    int lives;      //how many lives does swallow have
    int speed;      //swallows speed
} Stats;

typedef struct {
    WINDOW *win;
    float x, y;   //actual position
    float dx, dy;  //direction
    float speed;    //swallow speed
    char symbol[4];     //his skin
    int frame;      //frame alternation -> ascii animation
}Swallow;

typedef struct{
    float x, y;       //stars position in window
    float vy;         //star speed
    int active;     //1-exists, 0-free space in array
    int frame;      //same as in swallow, frame alternation
} Star;

typedef struct {
    float x, y;     //actual position
    float dx, dy;   //hunters direction
    float speed;    //hunters speed
    int active;     //1-hunter is on the screen 0-no
    int bounces;    //how many bounces left
    int type;   //hunters type
    int width;      //hunters width
    int height;     //hunters height
    int state;      //normal/pause/dash
    int t3;         //timer for pause and dash
    int dashed;     //did hunter already dashedd 0-no 1-yes
    float target_x;     //hunters x destination
    float target_y;     //hunters y destination
} Hunter;

typedef struct {
    float x, y;     //actual position
    float speed;    //taxis speed
    int active;             //0 - no taxi 1-taxi
    int carrying;           //0-swallow not in taxi 1-carrying swallow
    int width;      //taxis 
    int height;     //size
} Taxi;

typedef struct {
    int left;           //fog from left side
    int right;          //fog from right side
    int t10;            //timer for fog rising
}Fog;

typedef struct {
    int GAME_W;         //game width
    int GAME_H;         //game height
    int stars_needed;       //how many stars needed to beat the game
    int time_limit;     //how long until times up
    int MIN_SPEED;      //swallows min
    int MAX_SPEED;      //and max speed
    int hunter_spawn_ms;    //co jaki czas respi sie hunter
    float HUNTER_SPEED;     //hunters speed
    int hunter_damage;      //hunters damage
    int HUNTER_BOUNCE;      //hunters bounce count at the start
    int star_points;        //how many points gain per star
    int h_t[5];         //hunters templates
    int seed;           //srand(seed) 
    int star_spawn_ms;      //co ile respia sie gwazdki
} Config;

typedef struct {
    int dx;     //-1 - lewo, 1 = prawo, 0 - no wind
    int dy;     //-1 - gora, 1 = dol, 0 - no wind
    int acc_ms;     //timer 5s
} Wind;

//------------CONFIG-------------//
//defaultowe wartosci m.in. po to zeby miec podstawowe wartosci jak cos nie zadziala, albo nie wpisze sie do txt
void config_defaults(Config *cfg){
    cfg->GAME_W = 80;
    cfg->GAME_H = 20;                   
    cfg->stars_needed = 10;
    cfg->time_limit = 80;
    cfg->MIN_SPEED = 1;
    cfg->MAX_SPEED = 5;
    cfg->hunter_spawn_ms = 3000;
    cfg->HUNTER_SPEED = 0.4f;
    cfg->hunter_damage = 1;
    cfg->HUNTER_BOUNCE = 5;
    cfg->star_points = 1;
    cfg->h_t[0] = 0;
    cfg->h_t[1] = 1;
    cfg->h_t[2] = 2;
    cfg->h_t[3] = 3;
    cfg->h_t[4] = 4;
    cfg->seed = time(NULL);
    cfg->star_spawn_ms = 300;
}

void config_load_file(Config *cfg, const char *filename){
    FILE *f = fopen(filename, "r");     //opening the text file to read
    if (!f){
        return;     //if file doesnt exist/work then we use the default values
    }
    char key[64];       //array for the text in file
    float fvalue;       //value taken from the file

    while (fscanf(f, "%63s %f", key, &fvalue) == 2){        //read the entire file, firstly string then value
        //look for the exact word in "" in the file, if it finds the word it looks for -> set value do tej zmiennej
        if (strcmp(key, "GAME_W") == 0) cfg->GAME_W = fvalue;
        else if (strcmp(key, "GAME_H") == 0) cfg->GAME_H = fvalue;
        else if (strcmp(key, "S_NEEDED") == 0) cfg->stars_needed = fvalue;
        else if (strcmp(key, "TIME_LIMIT") == 0) cfg->time_limit = fvalue;
        else if (strcmp(key, "MIN_SPEED") == 0) cfg->MIN_SPEED = fvalue;
        else if (strcmp(key, "MAX_SPEED") == 0) cfg->MAX_SPEED = fvalue;
        else if (strcmp(key, "H_SPAWN_MS") == 0) cfg->hunter_spawn_ms = fvalue;
        else if (strcmp(key, "H_SPEED") == 0) cfg->HUNTER_SPEED = fvalue;
        else if (strcmp(key, "H_DAMAGE") == 0) cfg->hunter_damage = fvalue;
        else if (strcmp(key, "H_BOUNCE") == 0) cfg->HUNTER_BOUNCE = fvalue;
        else if (strcmp(key, "S_POINTS") == 0) cfg->star_points = fvalue;
        //looks for the word with the first letter being H, second _ and third T
        else if (key[0] == 'H' && key[1] == '_' && key[2] == 'T') {     //to save up some space
            int idx = key[3] - '1';     //key[3] represents the next number -1 cuz of types being 0,1,2,3,4
            if (idx >= 0 && idx < 5) {      //key[3] must be either >= 0 or < 5 cuz of the hunter templates being 0-4
            cfg->h_t[idx] = fvalue;         //setting the value to the index
            }
        }
        else if (strcmp(key, "SEED") == 0) cfg->seed = fvalue;
        else if (strcmp(key, "S_SPAWN_MS") == 0) cfg->star_spawn_ms = fvalue;
    }
    if (cfg->seed == 0) cfg->seed = time(NULL);     //if seed = 0 then we set time(NULL)
    fclose(f);
}

//-------------CZAS--------------//

int countdown(int reset_to_seconds) {
    static time_t t_start = 0;      //moment when the timer began
    static int start_value = 120;       //tymczasowa wartosc, poziej zostanie nadpisana

    if (reset_to_seconds > 0) {
        start_value = reset_to_seconds;     //tu zostaje nadpisana wartoscia wpisana w config
        t_start = time(NULL);       //start the timer
    }
    if (t_start == 0) {     //if timer never started
        t_start = time(NULL);       //start the timer
    }

    int elapsed = (int)(time(NULL) - t_start);      //how many seconds after the timer started
    int left = start_value - elapsed;       //how many seconds left

    if (left > 0){      //if seconds left > 0 return how many seconds left
        return left;
    }
    else {
        return 0;       //stop the timer
    }
}

int timer_ms(int *acc_ms, int interval_ms)
{
    *acc_ms += 40;              // dodaj jedna klatka = napms(40) do sumy
    if (*acc_ms < interval_ms) {        //jesli suma < wartosci wpisanej do funckji
        return 0;       //zwraca 0
    } else {        //jesli suma >= wartosci wpisanej do funckji
        *acc_ms %= interval_ms;     //zostawiamy reszte
        return 1;       //zwracamy 1
    }
}

//setting different timers for different amount of time
int timer_3s(int *acc_ms)
{
    return timer_ms(acc_ms, 3000);
}

int timer_5s(int *acc_ms)
{
    return timer_ms(acc_ms, 5000);
}

int timer_10s(int *acc_ms)
{
    return timer_ms(acc_ms, 10000);
}


//-------------STATYSTYKI-----------------//

//setting default values for stats
void stats_init(Stats *s, const Config *cfg) {
    s->name = "YOU";
    s->level = 1;
    s->stars_collected = 0;
    s->stars_needed = cfg->stars_needed;        //getting values 
    s->time_left = cfg->time_limit;             //from the config file
    s->lives = 10;      //place to change te amount of swallows lifes
    s->speed = cfg->MIN_SPEED;          //here too
}
//updating the time left place in stats
void stats_update(Stats *s) {
    s->time_left = countdown(0);
}

//--------------PREDKOSC----------------//
void speed_change(Stats *s, int delta, const Config *cfg){
    s->speed += delta;      //change speed by value set as delta
    if (s->speed < cfg->MIN_SPEED){     //if value < min speed set in config
        s->speed = cfg->MIN_SPEED;      //set speed to min speed
    }
    if (s->speed > cfg->MAX_SPEED){     //if value > max speed set in config
        s->speed = cfg->MAX_SPEED;      //set speed to max speed
    }
}

//-----------WIND FUNCTION DECLARATION----------//

float s_speed_w_wind(const Swallow *sw, const Wind *wind);

//-------------OKNO----------------//
//drawing the window for the game to be played in
void draw_win_game(WINDOW *win_game){
    werase(win_game);
    box(win_game, 0, 0);
}
//dawing the hud to show te statistics and overall information
void draw_win_hud(WINDOW *win_hud, const Stats *s, const Wind *wind) {
    werase(win_hud);
    box(win_hud, 0, 0);
    char wch = '-';     //symbol if there is no wind
    //symbols for the winds direction to be displayed in the hud
    if (wind->dx < 0){      //left
        wch = '<';      
    }
    else if (wind->dx > 0){     //right
        wch = '>';
    }
    else if(wind->dy < 0){      //up
        wch = '^';
    }
    else if (wind->dy > 0){     //down
        wch = 'v';
    }
    //basic information to be displayed
    mvwprintw(win_hud, 1, 2, "Player: %s | Level: %d | Time: %3ds | Stars: %d/%d | Life: %d | Speed: %d",
        s->name, s->level, s->time_left, s->stars_collected, s->stars_needed, s->lives, s->speed);
    mvwprintw(win_hud, 2, 2, "PRESS: T to call TAXI and Q to QUIT | Wind: %c", wch);
    wrefresh(win_hud);
}

//--------------FOG----------------//
//default setting for the fog
void fog_init(Fog *fog){
    fog->left = 0;      //no fog from the 
    fog->right = 0;     //left and right side
    fog->t10 = 0;       //timer for 10s
}

void fog_update(Fog *fog, const Config *cfg){
    if (timer_10s(&fog->t10)){      //update fog every 10s
        int max_shrink = cfg->GAME_W - 2*BORDER;        //setting max size for the fog
        int current = fog->left + fog->right;       //current combined state of fog
        if(current + 2*FOG_RISE < max_shrink){      //grow if current fog < max size
            fog->left += FOG_RISE;          //increase the fog
            fog->right += FOG_RISE;         //size on the screen
        }
    }
}

void fog_draw(const Fog *fog, WINDOW *win_game, const Config *cfg){
    //dont draw the fog in te first 10s of the game
    if (fog->left <= 0 && fog->right <= 0){ 
        return;
    }
    //ustawianie zmiennych zeby zaoszczedzic na rozmiarze funkcji
    int h = cfg->GAME_H;
    int w = cfg->GAME_W;
    //setting the fogs color
    wattron(win_game, COLOR_PAIR(7));
    //drawing the fog
    for (int y = 0; y < h; y++){
        for (int x = 0; x < fog->left; x++){    //left side
            mvwaddch(win_game, y, x, '#');
        }
        for (int x = w - fog->right; x < w ; x++){      //right side
            mvwaddch(win_game, y, x, '#');
        }
    }
    //turn off the color
    wattroff(win_game, COLOR_PAIR(7));
}

//-----------STARS---------------//

//every star is inactive
void stars_init(Star stars[], int max){
    for(int i = 0; i < max; i++){
        stars[i].active = 0;
        stars[i].frame = 0;
    }
}

//stars spawning
void stars_spawn_tick(Star stars[], int max, const Config *cfg){
    int w = cfg->GAME_W;
    static int star_timer_acc = 0;
    //if timer returns 1 (after star_spawn_ms set in config) star spawns
    if (!timer_ms(&star_timer_acc, cfg->star_spawn_ms)){
        return;
    }

    for(int i = 0; i < max; i++){       //looking for free slot
        if (!stars[i].active){ 
            stars[i].active = 1;        //star is active
            stars[i].y = BORDER - 1;        //border - spawn place
            stars[i].x = BORDER + rand() % (w - 2 * BORDER);        //finding a random column (x) to spawn a star
            int level = STARS_MIN_V + rand() % (STARS_MAX_V - STARS_MIN_V + 1);     //random (from 1 to 4) velocity for stars
            stars[i].vy = 0.3f * level;     //stars velocity * its level 
            stars[i].frame = 0;     //reseting the stars animation
            break;  //leaving the loop after spawning the star
        }
    }
}

//stars movement, collision, and drawing
void stars_movement_draw(Star stars[], int max, WINDOW *win, Swallow *sw, Stats *stats, const Config *cfg){
    int h = cfg->GAME_H;
    //setting float values to int for ncurses to work
    int swx = (int)sw->x;
    int swy = (int)sw->y;

    for(int i = 0; i < max; i++){
        if(!stars[i].active) {
            continue;      //skip free slots
        }

        stars[i].y += stars[i].vy;      //moving down
        //position in int
        int sy = (int)stars[i].y;
        int sx = (int)stars[i].x;

        //swallow body hitbox
        int sw_left   = swx;
        int sw_right  = swx + 2;
        int sw_top    = swy;
        int sw_bottom = swy;   

        // star hitbox
        int st_left   = sx;
        int st_right  = sx;
        int st_top    = sy;
        int st_bottom = sy;
        //if stars position = swallows position
        if (st_right >= sw_left && st_left <= sw_right && st_bottom >= sw_top && st_top <= sw_bottom){
            stars[i].active = 0;    //deactivating star so it disappears
            stats->stars_collected += cfg->star_points;     //counting it as star collected
            continue;
        }

        if (sy >= h){      //star touching border - disappear
            stars[i].active = 0;
            continue;
        }
        //stars ascii animation / frame alternation
        int phase = (stars[i].frame / 15) % 2;      //wiekszy dzielnik -> dluzej zostaje jeden kolor
        int color_pair;     //different colors
        if (phase == 0){
            color_pair = 1;
        }
        else{
            color_pair = 6;
        }
        //setting the colors
        wattron(win, COLOR_PAIR(color_pair));
        mvwaddch(win, sy, sx, '*');
        wattroff(win, COLOR_PAIR(color_pair)); 
        //increases
        stars[i].frame++;
    }
}

//----------SWALLOW-----------------//
//inicjalizacja swallow
void swallow_init(Swallow *sw, WINDOW *win, const Config *cfg){
    sw->win = win;      //swallow jest w oknie gry
    sw->x = cfg->GAME_W / 2;      //ustawienie pozycji
    sw->y = cfg->GAME_H / 2;      //na srodku ekranu
    sw->dx = 1;         //domyslnie leci w prawo
    sw->dy = 0;         //nie porusza sie w pionie na poczatku
    sw->speed = 1.0f;       //predkosc swallowa
    //wyglad swallowa
    sw->symbol[0] = '~';
    sw->symbol[1] = '*';
    sw->symbol[2] = '~';
    sw->symbol[3] = '\0';
    sw->frame = 0;      //start animation from frame = 0
}

void swallow_draw(Swallow *sw){                   //wstawianie swallowa
    wattron(sw->win, COLOR_PAIR(2));
    //wiekszy dzielnik -> wolniejsza animacja
    //group into phase so one form longer 
    int phase = (sw->frame / 5) % 2;
    if (phase == 0){
        mvwaddnstr(sw->win, sw->y, sw->x, "~*~", 3);
    }
    else {
        mvwaddnstr(sw->win, sw->y, sw->x, "^*^", 3);
    }

    wattroff(sw->win, COLOR_PAIR(2));
    //increasing the frame value every frame
    sw->frame++;
}

void swallow_clear(const Swallow *sw){
    //czyszczenie starego miejsca swallowa
    mvwaddnstr(sw->win, sw->y, sw->x, "   ", 3);
}

void swallow_move(Swallow *sw, const Fog *fog, const Config *cfg, Wind *wind){
    //saving up space
    int h = cfg->GAME_H;
    int w = cfg->GAME_W;
    //clearing swallows previous position
    swallow_clear(sw);
    
    int left_limit = BORDER;
    int right_limit = w - 3;    // w - 3 cuz swallow is 3 width
    //adding fog level to the swallows movement limit
    if (fog){
        left_limit  += fog->left;
        right_limit -= fog->right;
    }
    //dla bezpieczenstwa
    if (left_limit > right_limit){
        left_limit = right_limit;
    }
    //cumulating swallows default speed + winds affect
    float move_speed = s_speed_w_wind(sw, wind);

    int at_x_border = (sw->x <= left_limit || sw->x >= right_limit);     //sprawdzamy czy
    int at_y_border = (sw->y <= BORDER || sw->y >= h - BORDER - 1);     //jest na scianie

    if (at_x_border){           //ruch w poziomie
        if (sw->x <= left_limit) {      //jesli jest na lewej granicy
            sw->dx = 1;     //odbijamy w prawo
        }
        else if (sw->x >= right_limit) {     //jesli jest na prawej granicy
            sw->dx = -1;    //odbijamy w lewo
        }
        sw->x += sw->dx * move_speed;   //update position
    }
    else {
        float new_x = sw->x + sw->dx * move_speed;     //liczenie nowej pozycji

        if (new_x <= left_limit){       //sprawdzenie czy nowa pozycja walnie w sciane
            sw->dx = 1;     //odbijamy w prawo
            sw->x = left_limit + sw->dx * sw->speed;
        }
        else if (new_x >= right_limit){
            sw->dx = -1;    //odbijamy w lewo
            sw->x = right_limit + sw->dx * sw->speed;
        }
        else {
            sw->x = new_x;      //normalny movement
        }
    }

    if (at_y_border){           //ruch w pionie
        if (sw->y <= BORDER) {      //jesli jest na gornej granicy
            sw->dy = 1;     //odbijamy w dol
        }
        else if (sw->y >= h - 1) {     //jesli jest na dolnej granicy
            sw->dy = -1;    //odbijamy w gore
        }
        sw->y += sw->dy * move_speed;
    }
    else {
        float new_y = sw->y + sw->dy * move_speed;     //liczenie nowej pozycji
        if (new_y <= 0){       //sprawdzenie czy nowa pozycja walnie w sciane
            sw->dy = 1;     //odbijamy w dol
            sw->y = BORDER + sw->dy * sw->speed;
        }
        else if (new_y >= h){
            sw->dy = -1;    //odbijamy w gore
            sw->y = h - BORDER - 1 + sw->dy * sw->speed;
        }
        else {
            sw->y = new_y;      //normalny movement
        }
    }
    //draw updated swallows position
    swallow_draw(sw);
}
//aaply wsad movement
void swallow_movement(Swallow *sw, int ch){
    switch (ch) {
        case GORA:          // do gory
            sw->dx = 0;
            sw->dy = -1;
            break;
        case DOL:          // w dol
            sw->dx = 0;
            sw->dy = 1;
            break;
        case LEWO:          // w lewo
            sw->dx = -1;
            sw->dy = 0;
            break;
        case PRAWO:          // w prawo
            sw->dx = 1;
            sw->dy = 0;
            break;
        default:
            // inne klawisze ignorujemy
            break;
    }
}

//--------------HUNTERS------------------//
//setting default values
void hunters_init(Hunter hunters[], int max){
    for (int i = 0; i < max; i++){
        hunters[i].active = 0;  //hunter not on the screen
        //hunters position
        hunters[i].x = 0.0f;
        hunters[i].y = 0.0f;
        hunters[i].state = HUNTER_STATE_NORMAL; //so he doesnt dash or stop
        hunters[i].t3 = 0;  //resetting the timer for hunters spawn, wait and dash
        hunters[i].dashed = 0;      //0 - didnt dash 1 - dashed
    }
}

void hunter_types (Hunter *h, const Config *cfg){
    int idx = rand() % 5;           //random shape picked from 5 different
    //taking hunters templates from config
    int t = cfg->h_t[idx];
    //if hunters value taken from config file is:
    if (t < 0) {    //<0
        t = 0;      //set this template to 0
    }
    if (t > 4) {    //>4
        t = 4;      //set tis template to 4
    }
    
    h->type = t;

    switch(t){
        case 0:             //1x2
        h->width = 1;
        h->height = 2;
        break;

        case 1:             //2x1
        h->width = 2;
        h->height = 1;
        break;

        case 2:             //1x3
        h->width = 1;
        h->height = 3;
        break;

        case 3:             //3x1
        h->width = 3;
        h->height = 1;
        break;

        case 4:             //2x2
        h->width = 2;
        h->height = 2;
        break;
    }
}

void hunter_spawn_one(Hunter hunters[], int max, WINDOW *win_game, Swallow *sw, int bounces, const Config *cfg){
    //saving up space in the function
    int H = cfg->GAME_H;
    int W = cfg->GAME_W;
    //looking for free slot in the array
    for (int i = 0; i < max; i++){
        if (hunters[i].active == 0){    //if hunter is inactive
            Hunter *h = &hunters[i];
            h->active = 1;
            h->state = HUNTER_STATE_NORMAL;
            h->t3 = 0;
            h->speed = cfg->HUNTER_SPEED;
            h->dashed = 0;
            int safe_margin = 1;        //so hunter doesnt spawn on border loosing bounces

            hunter_types(h, cfg);            //losowanie wygladu huntera
            h->bounces = bounces;
            //setting up side to spawn the hunter
            int side = rand() % 2;  //left or right side
            if (side == 0){
                h->x = BORDER + 1;      //left side
            }
            else {
                h->x = W - BORDER - 1 - (h->width - 1);     //right side
            }
            //setting up places (y) to spawn the hunter
            int min_y = BORDER + 1 + safe_margin;
            int max_y = H - BORDER - 1 - safe_margin - (h->height - 1);
            //dla bezpieczenstwa
            if (max_y < min_y) {
                max_y = min_y;
            }
            //random y considering hunters height
            h->y = min_y + rand() % (max_y - min_y + 1);
            //hunters center
            float hx_center = h->x + h->width / 2.0f;
            float hy_center = h->y + h->height / 2.0f;
            //swallows x center being hunters target
            h->target_x = sw->x + 1.0f;
            h->target_y = sw->y;

            float vx = h->target_x - hx_center;
            float vy = h->target_y - hy_center;
            //vector to swallows place
            float len = sqrtf(vx*vx + vy*vy);
            if (len == 0.0f) {
                len = 1.0f;
            }
            //so the speed is const
            h->dx = vx / len;
            h->dy = vy / len;

            break;
        }
    }
}

void hunters_draw(Hunter hunters[], int max, WINDOW *win_game){
    for (int i = 0; i < max; i++){
        if (hunters[i].active != 1){        //skipping the inactive
            continue;
        }

        Hunter *h = &hunters[i];
        //hunters size in int
        int ix = (int)h->x;
        int iy = (int)h->y;
        //setting hunters color
        wattron(win_game, COLOR_PAIR(4));
        //space for bounce count above hunter in the middle 
        int label_x = ix + h->width / 2;

        mvwprintw(win_game, iy - 1, label_x, "%d", h->bounces);     //bounce count
        //hunters types/templates
        switch (h->type){
            case 0:                             //1x2
            mvwprintw(win_game, iy, ix, "^");
            mvwprintw(win_game, iy + 1, ix, "@");
            break;

            case 1:                             //2x1
            mvwprintw(win_game, iy, ix, "@@");
            break;

            case 2:                             //1x3
            mvwprintw(win_game, iy, ix, "^");
            mvwprintw(win_game, iy + 1, ix, "@");
            mvwprintw(win_game, iy + 2, ix, "v");
            break;

            case 3:                              //3x1
            mvwprintw(win_game, iy, ix, "<@>");
            break;

            case 4:                             //2x2
            mvwprintw(win_game, iy,     ix, "^^");
            mvwprintw(win_game, iy + 1, ix, "@@");
            break;
        }
        //turn off colors
        wattroff(win_game, COLOR_PAIR(4));
    }
}

void hunter_bounce(Hunter *h, int W, int H){
    int bounced = 0;
    //movement in the opposite direction
    // poziom
    if (h->x <= BORDER){                    //left border
        h->x = BORDER;
        h->dx = -h->dx;
        bounced = 1;
    }
    else if (h->x + (h->width - 1) >= W - BORDER - 1){              //right border
        h->x = W - BORDER - 1 - (h->width - 1);
        h->dx = -h->dx;
        bounced = 1;
    }

    // pion
    if (h->y <= BORDER){            //upper border
        h->y = BORDER;
        h->dy = -h->dy;
        bounced = 1;
    }
    else if (h->y + (h->height - 1) >= H - BORDER - 1){         //dolny border
        h->y = H - BORDER - 1 - (h->height - 1);
        h->dy = -h->dy;
        bounced = 1;
    }

    // odbicie = zmniejszamy bounces
    if (bounced){
        h->bounces--;
        if (h->bounces <= 0){
            h->active = 0;  // hunter znika, slot wolny
        }
    }
}


void hunters_move(Hunter hunters[], int max, WINDOW *win_game, Swallow *sw, const Config *cfg){
    //saving up space once again
    int H = cfg->GAME_H;
    int W = cfg->GAME_W;

    for (int i = 0; i < max; i++){
        Hunter *h = &hunters[i];
        if (!h->active){    //if hunter inactive ->skip
            continue;
        }
        if (h->state == HUNTER_STATE_NORMAL){   //hunter before stopping/dashing
            //hunters movement
            h->x += h->dx * h->speed;
            h->y += h->dy * h->speed;
            hunter_bounce(h, W, H);

            int cx = (int)(h->x + h->width  / 2.0f);    //hunters center
            int cy = (int)(h->y + h->height / 2.0f);
            int tx = (int)h->target_x;      //target obrany przez huntera
            int ty = (int)h->target_y;      //w momencie spawnu
            //if hunter reached his target without hitting swallow -> switch to pause phase
            if (!h->dashed && cx == tx && cy == ty){
                h->state = HUNTER_STATE_PAUSE;
                h->t3 = 0;
                h->dx = 0.0f;   //hunter stops
                h->dy = 0.0f;
            }
        }
        else if (h->state == HUNTER_STATE_PAUSE){
            if (timer_3s(&h->t3)){
                float cx = h->x + h->width / 2.0f;          //hunters
                float cy = h->y + h->height / 2.0f;         //center
                float tx = sw->x + 1.0f;                    //swallows
                float ty = sw->y;                           //center
                float vx = tx - cx;
                float vy = ty - cy;
                float len = sqrtf(vx*vx + vy*vy);       //calculating swallows position
                if (len == 0) len = 1;
                h->dx = vx / len;
                h->dy = vy / len;
                h->speed = cfg->HUNTER_SPEED * 2.0f;        //dashing = increasing hunters speed
                h->t3 = 0;  //resetting timer
                h->state = HUNTER_STATE_DASH;   
            }
        }
        else if (h->state == HUNTER_STATE_DASH){
            h->x += h->dx * h->speed;
            h->y += h->dy * h->speed;
            hunter_bounce(h, W, H);

            if (timer_3s(&h->t3)){
                h->state = HUNTER_STATE_NORMAL; //after 3s goes back to his normal phase
                h->speed = cfg->HUNTER_SPEED;   //setting speed to default
                h->t3 = 0;  //reseting timer
                h->dashed = 1;          //already dashed, wont no more
            }
        }

    }
}

void hunter_collision(Hunter hunters[], int max, Swallow *sw, Stats *stats, const Config *cfg){
    //swallows position in int
    int sw_x = (int)sw->x;
    int sw_y = (int)sw->y;
    //hurt cooldown - swallows immortality
    static int hurt_cooldown = 0;
    if (hurt_cooldown > 0){
        hurt_cooldown--;
        return;
    }
    //skipping inactive hunters
    for (int i = 0; i < max; i++){
        Hunter *h = &hunters[i];
        if (h->active != 1) continue;
        //hunters position in int
        int hx = (int)h->x;
        int hy = (int)h->y;
        //checking if any hunter touches swallow
        for (int yy = 0; yy < h->height; yy++){     //loop for y
            for (int xx = 0; xx < h->width; xx++){  //loop for x

                int px = hx + xx;       //globalne x tej komorki
                int py = hy + yy;       //globalny y tej komorki

                //checking if this pixel is on swallow
                if (py == sw_y && px >= sw_x && px <= sw_x + 2){    //sw h = 1 w = 3
                    stats->lives -= cfg->hunter_damage;                 //loosing health
                    hurt_cooldown = 25;             //immortality fo swallow after being hit
                    h->dashed = 1;                  //if hunter hits swallow -> wont dash no more
                    return;
                }
            }
        }
    }
}

//------------------TAXI--------------------//
//default values for taxi
void taxi_init(Taxi *taxi){
    taxi->active = 0;   //taxi inactive
    taxi->carrying = 0; //doesnt carry swallow
    //taxis starting position
    taxi->x = 0.0f; 
    taxi->y = 0.0f;
    taxi->speed = TAXI_SPEED;   //taxis speed
    //taxis size
    taxi->width = 3;
    taxi->height = 2;
}

void taxi_spawn(Taxi *taxi, WINDOW *win_game, const Config *cfg) {
    //saving up space
    int H = cfg->GAME_H;
    int W = cfg->GAME_W;
    //taxis size
    taxi->width = 3;
    taxi->height = 2;
    //taxis spawn place
    taxi->x = W - BORDER - taxi->width - 1;     //spawn at right border
    taxi->y = H/2;                              //mid height screen
    //setting taxi as active
    taxi->active = 1;
    taxi->carrying = 0;
}

void taxi_move(Taxi *taxi, WINDOW *win_game, Swallow *sw, Star stars[], int max_stars, Hunter hunters[], int max_hunters, int *hunter_timer_acc, const Config *cfg){
    //if taxi already active, taxi wont spawn again
    if (!taxi->active){
        return;
    }
    //taxis movement
    taxi->x -= taxi->speed;

    if (taxi->x <= BORDER){     //if taxi at left border
        taxi->x = BORDER;   

        if (taxi->carrying){    //restart game
            swallow_init(sw, win_game, cfg);                 //reset swallow
            stars_init(stars, max_stars);               //reset stars
            hunters_init(hunters, max_hunters);         //reset hunters
            *hunter_timer_acc = 0;                      //reset hunters spawn timer
        }

        taxi->active = 0;                   //either way taxi despawns
        taxi->carrying = 0;                 //taxi no loger carry
    }
}

void taxi_draw(Taxi *taxi, WINDOW *win_game){
    if (!taxi->active){                         //taxi not active -> exit this function        
        return;
    }
    //taxis position
    int ix = (int)taxi->x;              
    int iy = (int)taxi->y;      
    //set color
    wattron(win_game, COLOR_PAIR(5));
    //taxis skin
    mvwprintw(win_game, iy, ix, "<##");
    mvwprintw(win_game, iy + 1, ix, "<##");
    //turn off color
    wattroff(win_game, COLOR_PAIR(5));
}

void taxi_collision(Taxi *taxi, Swallow *sw){
    //if taxi inactive or carrying then skip
    if (!taxi->active || taxi->carrying){
        return;
    }
    //swallows position in int
    int sw_x = (int)sw->x;
    int sw_y = (int)sw->y;
    //taxis position in int
    int tx = (int)taxi->x;
    int ty = (int)taxi->y;
    //swallows:
    int sw_left   = sw_x;               //left side
    int sw_right  = sw_x + 2;           //right side
    int sw_top    = sw_y;               //height
    int sw_bottom = sw_y;               //height
    //taxis:
    int tx_left   = tx;                         //left side
    int tx_right  = tx + taxi->width  - 1;      //right side
    int tx_top    = ty;                         //height
    int tx_bottom = ty + taxi->height - 1;      //height
    //comparing taxis and swallows position
    if (sw_right >= tx_left && sw_left <= tx_right && sw_bottom >= tx_top && sw_top <= tx_bottom){
        //getting swallow out the window (literally)
        taxi->carrying = 1; //taxi carrying swallow
        swallow_clear(sw);  //deleting swallow
        //move swallow out the screen
        sw->x = -10;
        sw->y = -10;
    }
}

//----------------WIND----------------//
//winds default settigs
void wind_init(Wind *wind){
    wind->dx = 0;
    wind->dy = 0;
    wind->acc_ms = 0;
}

void wind_update(Wind *w){
    //the same as in stars -> timer returns 1 after 5s and the function works
    if (!timer_5s(&w->acc_ms)){
        return;
    }

    int r = rand() % 4;     //random wind direction -> right, left, up, down

    if (r == 0){        //left
        w->dx = -1;
        w->dy = 0;
    }

    else if (r == 1){       //right
        w->dx = 1;
        w->dy = 0;
    }

    else if (r == 2){       //up
        w->dx = 0;
        w->dy = -1;
    }

    else {              //down
        w->dx = 0;
        w->dy = 1;
    }
}

float s_speed_w_wind(const Swallow *sw, const Wind *wind){
    float speed_factor = 1.0f;  //setting winds default multiplier
    //is swallow moving with/against wind
    int with_wind = 0;
    int against_wind = 0;

    //poziom
    if (sw->dx != 0 && wind->dx != 0) {
        if (sw->dx == wind->dx){    //if the same direction 
            with_wind = 1;
        }
        else if (sw->dx == -wind->dx) { //if against winds direction
            against_wind = 1;
        }
    }

    // pion
    if (sw->dy != 0 && wind->dy != 0) { //if the same direction
        if (sw->dy == wind->dy){
            with_wind = 1;
        }
        else if (sw->dy == -wind->dy){  //if against winds direction
            against_wind = 1;
        }
    }

    if (with_wind){     //if swallows direction is the same as winds
        speed_factor *= 1.3f;   // increase speed +30%
    }
    else if (against_wind){     //if swallows direction is against winds
        speed_factor *= 0.7f;  // decrease speeb by -30%
    }
    return sw->speed * speed_factor;    //return final speed
}

//--------------RANKING----------------//

long long save_score(const Stats *stats, const Config *cfg){
    //setting difficulty multiplier
    int difficulty = cfg->hunter_damage + (5 - cfg->hunter_spawn_ms / 1000);
    if (difficulty < 1){    //so score isnt negative
        difficulty = 1;
    }
    //liczenie scoru 
    long long score = stats->stars_collected * stats->time_left * stats->lives * difficulty;
    //opening file
    FILE *f = fopen("ranking.txt", "a");
    if (f){
        fprintf(f, "%lld\n", score);    //saving score to file
        fclose(f);
    }
    return score;
}

int load_scores(const char *filename, long long scores[], int max_scores){
    //opening file to read
    FILE *f = fopen(filename, "r");
    //if file doesnt open
    if(!f) {
        return 0;
    }
    int count = 0;
    //reading scores from file
    while(count < max_scores && fscanf(f, "%lld", &scores[count]) == 1){
        count++;
    }
    fclose(f);
    return count;
}
//bubble sort
void sort_scores(long long scores[], int count){
    for(int i = 0; i < count - 1; i++){
        for(int j = i + 1; j < count; j++){
            if(scores[j] > scores[i]){
                long long tmp = scores[i];
                scores[i] = scores[j];
                scores[j] = tmp;
            }
        }
    }
}
//printing out top scores, max 5
void draw_top_scores(WINDOW *w, long long scores[], int count){
    int top;
    //limit number max 5
    if(count < 5){
        top = count;
    }
    else {
        top = 5;
    }
    //header
    mvwprintw(w, 5, 3, "TOP SCORES:");
    //print out scores in sorted order
    for(int i = 0; i < top; i++){
        mvwprintw(w, 6 + i, 3, "%d) %lld", i + 1, scores[i]);  
    }
}

//----------END SCREEN (odnosnik)---------//

void show_end_screen(WINDOW *win, int score, const Config *cfg);

//----------GLOWNE PETLE GRY----------------//

int input(int ch, Stats *stats, Swallow *sw, Taxi *taxi, WINDOW *win_game, const Config *cfg)
{
    if (ch == QUIT) //if ch = q -> leave game
        return 0;

    if (ch == SLOWER){      //if ch = o -> slower
        speed_change(stats, -1, cfg);
    }
    else if(ch == FASTER){      //if ch = p -> faster
        speed_change(stats, +1, cfg);
    }
    else if(ch == TAXI_KEY){        //if ch = t -> taxi spawns
        if(!taxi->active && !taxi->carrying){   //if taxi inactive and not carrying
            taxi_spawn(taxi, win_game, cfg);    //spawn taxi
        }
    }
    else { 
        if (!taxi->carrying){   //swallow moves only if not in taxi
            swallow_movement(sw, ch);   //normal swallow movement
        }
    }
    //update game
    stats_update(stats);

    return 1;
}

static void main_swallow_and_stars(WINDOW *win_game, Stats *stats, Swallow *sw, Star stars[], Taxi *taxi, Fog *fog, const Config *cfg, Wind *wind){
    draw_win_game(win_game);
    //apply swallows movement mechanics
    if (!taxi->carrying) {
        sw->speed = stats->speed * SPEED_SCALE;
        swallow_move(sw, fog, cfg, wind);
    }
    //stars spawning, movement and colissions
    stars_spawn_tick(stars, MAX_STARS, cfg);
    stars_movement_draw(stars, MAX_STARS, win_game, sw, stats, cfg);
}

static void main_hunters(WINDOW *win_game, Stats *stats, Swallow *sw, Hunter hunters[], int *hunter_timer_acc, int *hunter_spawn_count, int *hunter_max_on_screen, int taxi_carrying, const Config *cfg){
    if (timer_ms(hunter_timer_acc, cfg->hunter_spawn_ms)) {
        //count currently active hunters
        int active = 0;
        for (int i = 0; i < MAX_HUNTERS; i++)
            if (hunters[i].active) active++;

        if (active < *hunter_max_on_screen) {
            //increasing bounce count every 4 spawns
            int bounces = cfg->HUNTER_BOUNCE + (*hunter_spawn_count / 4);
            //spawning hunter
            hunter_spawn_one(hunters, MAX_HUNTERS, win_game, sw, bounces, cfg);
            (*hunter_spawn_count)++;
            //increasing max hunter on screen every 3 spawns
            if ((*hunter_spawn_count) % 3 == 0 && *hunter_max_on_screen < MAX_HUNTERS) {
                (*hunter_max_on_screen)++;
            }
        }
    }

    hunters_move(hunters, MAX_HUNTERS, win_game, sw, cfg);
    //if taxi not carrying -> normal hunter collision
    if (!taxi_carrying){
        hunter_collision(hunters, MAX_HUNTERS, sw, stats, cfg);
    }
    //draw hunters
    hunters_draw(hunters, MAX_HUNTERS, win_game);
}
//every taxi function
static void main_taxi(WINDOW *win_game, Swallow *sw, Star stars[], Taxi *taxi, Hunter hunters[], int *hunter_timer_acc, const Config *cfg){

    taxi_move(taxi, win_game, sw, stars, MAX_STARS, hunters, MAX_HUNTERS, hunter_timer_acc, cfg);

    taxi_collision(taxi, sw);

    taxi_draw(taxi, win_game);
}
//every previous main function
static void main_game_world(WINDOW *win_game, WINDOW *win_hud, Stats *stats, Swallow *sw, Star stars[], Hunter hunters[], Taxi *taxi, int *hunter_timer_acc, int *hunter_spawn_count, int *hunter_max_on_screen, Fog *fog, const Config *cfg, Wind *wind){

    main_swallow_and_stars(win_game, stats, sw, stars, taxi, fog, cfg, wind);

    main_hunters(win_game, stats, sw, hunters, hunter_timer_acc, hunter_spawn_count, hunter_max_on_screen, taxi->carrying, cfg);

    main_taxi(win_game, sw, stars, taxi, hunters, hunter_timer_acc, cfg);

    fog_update(fog, cfg);
    fog_draw(fog, win_game, cfg);

    wrefresh(win_game);
    draw_win_hud(win_hud, stats, wind);
}


void main_game(WINDOW *win_game, WINDOW *win_hud, Stats *stats, Swallow *sw, Star stars[], Hunter hunters[], Taxi *taxi, const Config *cfg, Wind *wind){
    int running = 1;
    int hunter_timer_acc = 0;
    int hunter_spawn_count = 0;
    int hunter_max_on_screen = 3;

    Fog fog;
    fog_init(&fog);

    while (running) {

        int ch = getch();

        running = input(ch, stats, sw, taxi, win_game, cfg);
        //update wind
        wind_update(wind);
        //end game conditions
        if (stats->stars_collected >= cfg->stars_needed || stats->lives <= 0 || stats->time_left <= 0){
            long long score = save_score(stats, cfg);
            werase(win_hud);
            wrefresh(win_hud);
            show_end_screen(win_game, score, cfg);
            return;
        }

        main_game_world(win_game, win_hud, stats, sw, stars, hunters, taxi, &hunter_timer_acc, &hunter_spawn_count, &hunter_max_on_screen, &fog, cfg, wind);
        //delay 40ms
        napms(40);
    }
}


//------------ZAMYKANIE OKNA---------------//

void close_window(WINDOW *win_game, WINDOW *win_hud){
    delwin(win_hud);
    delwin(win_game);
    endwin();
}

//--------------START NCURSES----------//

void init_ncurses(){
    initscr();
    refresh();

    cbreak();       //keyboard read
    noecho();       //no typing from keyboard
    curs_set(0);    //no cursor
    keypad(stdscr, TRUE);       //special keys
    nodelay(stdscr, TRUE);      //getch() doesnt block the program
}

//-------------COLORS--------------//

void init_colors(){
    use_default_colors();
    start_color();

    init_pair(1, COLOR_YELLOW, COLOR_BLACK);         //stars color
    init_pair(2, COLOR_MAGENTA, COLOR_BLACK);        //swallows color
    init_pair(3, COLOR_BLACK, COLOR_BLACK);          //background color
    init_pair(4, COLOR_RED, COLOR_BLACK);            //hunters color
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);         //taxi color
    init_pair(6, COLOR_WHITE, COLOR_BLACK);          //different color for stars for frame alternation
    init_pair(7, COLOR_WHITE, COLOR_WHITE);          //fog color
}

//-------------END SCREEN---------------//

void show_end_screen(WINDOW *win, int score, const Config *cfg){
    //big numbers (score)
    long long scores[100];
    int count = load_scores("ranking.txt", scores, 100);
    sort_scores(scores, count);

    werase(win);
    box(win, 0, 0);

    wbkgd(win, COLOR_PAIR(3));
    wattron(win, COLOR_PAIR(6));

    mvwprintw(win, 1, 3, "GAME OVER!");
    mvwprintw(win, 3, 3, "Your score: %lld", score);

    draw_top_scores(win, scores, count);

    mvwprintw(win, 12, 3, "Press Q to quit");
    wrefresh(win);
    //if ch = q -> quit, if not then do nothing
    int ch;
    do {
        ch = wgetch(win);
    }while (ch != QUIT);
}

//-----------MAIN FUNCTION-------------//
int main(){
    //load up every function
    Config cfg;
    config_defaults(&cfg);
    config_load_file(&cfg, "config.txt");
    srand(cfg.seed);

    init_ncurses();

    init_colors();

    WINDOW *win_game = newwin(cfg.GAME_H, cfg.GAME_W, 0, 0);
    WINDOW *win_hud = newwin(HUD_H, cfg.GAME_W, cfg.GAME_H, 0);

    wbkgd(win_game, COLOR_PAIR(3));

    Stats stats;
    stats_init(&stats, &cfg);

    countdown(stats.time_left);

    Swallow sw;
    swallow_init(&sw, win_game, &cfg);

    Star stars[MAX_STARS];
    stars_init(stars, MAX_STARS);

    Hunter hunters[MAX_HUNTERS];
    hunters_init(hunters, MAX_HUNTERS);

    Taxi taxi;
    taxi_init(&taxi);

    Wind wind;
    wind_init(&wind);

    main_game(win_game, win_hud, &stats, &sw, stars, hunters, &taxi, &cfg, &wind);

    close_window(win_game, win_hud);

    return 0;
}