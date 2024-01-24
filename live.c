#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ARRLEN(arr) (sizeof (arr) / sizeof (arr)[0])
#define dbg(fmt, val) fprintf(stderr, "%s: " fmt "\n", #val, (val))

typedef struct {
        Vector2 pos;
        Vector2 vel;
        Vector2 acc;
} Particle;


typedef Vector2 (*force_fn)(void* udata, Vector2 pos);
typedef void (*single_step_fn)(Particle* p, float dt, force_fn calc_force, void* force_udata);



void euler(Particle* p, float dt, force_fn calc_force, void* force_udata){

    Vector2 force = calc_force(force_udata, p->pos);
    p->acc = force;

    Vector2 vel = p->vel;
    p->vel =
        Vector2Add(
            p->vel,
            Vector2Scale(p->acc, dt)
        );

    p->pos =
        Vector2Add(
            p->pos,
            Vector2Scale(vel, dt)
        );
}

void euler2(Particle* p, float dt, force_fn calc_force, void* force_udata){

    Vector2 force = calc_force(force_udata, p->pos);
    p->acc = force;

    p->vel =
        Vector2Add(
            p->vel,
            Vector2Scale(p->acc, dt)
        );

    p->pos =
        Vector2Add(
            p->pos,
            Vector2Scale(p->vel, dt)
        );
}

void verlet(Particle* p, float dt, force_fn calc_force, void* force_udata){
    Vector2 new_acc = calc_force(force_udata, p->pos);

    Vector2 new_pos;
    Vector2 new_vel;

    new_pos.x = p->pos.x + p->vel.x*dt + p->acc.x*(dt*dt*0.5);
    new_vel.x = p->vel.x + (p->acc.x+new_acc.x)*(dt*0.5);

    new_pos.y = p->pos.y + p->vel.y*dt + p->acc.y*(dt*dt*0.5);
    new_vel.y = p->vel.y + (p->acc.y+new_acc.y)*(dt*0.5);

    p->pos = new_pos;
    p->vel = new_vel;
    p->acc = new_acc;
}

void heun(Particle* p, float dt, force_fn calc_force, void* force_udata){
    Vector2 new_acc = calc_force(force_udata, p->pos);
    Vector2 velBar = Vector2Add(p->vel, Vector2Scale(new_acc, dt));
    Vector2 posBar =
        Vector2Add(
            p->pos,
            Vector2Scale(p->vel, dt)
        );


    Vector2 newer_acc = calc_force(force_udata, posBar);
    Vector2 new_vel;
    new_vel.x = p->vel.x + dt*0.5*(new_acc.x + newer_acc.x);
    new_vel.y = p->vel.y + dt*0.5*(new_acc.y + newer_acc.y);

    Vector2 new_pos;
    new_pos.x = p->pos.x + dt*0.5*(p->vel.x + new_vel.x);
    new_pos.y = p->pos.y + dt*0.5*(p->vel.y + new_vel.y);

    p->pos = new_pos;
    p->vel = new_vel;
    p->acc = new_acc;
}

void derivative(float x[4], float dx[4], force_fn calc_force, void* force_udata){
    dx[0] = x[2];
    dx[1] = x[3];
    Vector2 f = calc_force(force_udata, *(Vector2*)x);
    dx[2] = f.x;
    dx[3] = f.y;
}

void runge_kutta(Particle* p, float dt, force_fn calc_force, void* force_udata){

    float* x = (float*)p;

    #define forall for (int i = 0; i < 4; i++)
    float k1[4];
    derivative(x, k1, calc_force, force_udata);
    forall k1[i] *= dt;

    float tmp[4];
    forall tmp[i] = x[i] + 0.5 * k1[i];
    float k2[4];
    derivative(tmp, k2, calc_force, force_udata);
    forall k2[i] *= dt;

    forall tmp[i] = x[i] + 0.5 * k2[i];
    float k3[4];
    derivative(tmp, k3, calc_force, force_udata);
    forall k3[i] *= dt;

    forall tmp[i] = x[i] + k3[i];
    float k4[4];
    derivative(tmp, k4, calc_force, force_udata);
    forall k4[i] *= dt;

    forall x[i] += (1.0f/6.0f)*(k1[i] + 2*k2[i] + 2*k3[i] + k4[i]);

    #undef forall
}


static struct{
    int steps;
    Color color;
    single_step_fn fn;
} single_step_config[] = {
    //{1,     RED,    euler },
    //{10,    ORANGE, euler },
    //{100,   YELLOW, euler },
    //{1000,  GREEN,  euler },
    //{10000, BLUE,   euler },
    //{1,     PURPLE, euler2},
    //{10,    MAGENTA,euler2},
    //{100,   VIOLET, euler2},
    //{100,   BROWN, verlet},


    {100,   YELLOW, euler },
    {1000,  GREEN,  euler },
    {1,     RED, heun},
    {10,    PINK,heun},
    {100,   VIOLET, heun},
    {1000, BLUE,   euler },
    {10, ORANGE, runge_kutta},
    {1000, YELLOW, runge_kutta},
};

typedef struct{
    Particle* single_step;
    int warp;
    RenderTexture trail_tex;
} State;



void reset_sim(State* state){

    for (int i = 0; i < ARRLEN(single_step_config); i++) {
        state->single_step[i].pos = (Vector2){200, 0};
        state->single_step[i].vel = (Vector2){0, 100};
    }

}

void before_reload(void* state_void){
}
void after_reload(void* state_void){
    State* state = (State*)state_void;


    state->single_step =
        realloc(
            state->single_step, 
            ARRLEN(single_step_config) * sizeof *state->single_step
        );
}

Vector2 gravity(void* data, Vector2 p){
    float M = *(float*)data;
    float r = Vector2Length(p);
    Vector2 f = Vector2Normalize(p);
    return Vector2Scale(f, -M/(r*r));
}

void update(void* state_void){
    State* state = (State*)state_void;

    uint32_t screen_width = GetScreenWidth();
    uint32_t screen_height = GetScreenHeight();

    if(IsKeyPressed(KEY_ENTER)){
        reset_sim(state);

        BeginTextureMode(state->trail_tex);
        ClearBackground(BLACK);
        EndTextureMode();
    }

    if(IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT))
        state->warp++;
    if(IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT))
        state->warp--;
    if(IsKeyPressed(KEY_DOWN) || state->warp < 0)
        state->warp=0;

    

    if(screen_width != state->trail_tex.texture.width 
        || screen_height != state->trail_tex.texture.height){
        UnloadRenderTexture(state->trail_tex);
        state->trail_tex = LoadRenderTexture(screen_width, screen_height);
    }

    BeginDrawing();
    ClearBackground(BLACK);

    Camera2D cam = {0};
    cam.offset = (Vector2){ screen_width/2.0f, screen_height/2.0f };
    cam.zoom = 1;


    BeginTextureMode(state->trail_tex);
    DrawRectangle(0, 0, screen_width, screen_height, ColorAlpha(BLACK, 0.02));
    BeginMode2D(cam);


    for (int w = 0; w < state->warp; w++) {
    for (int p = 0; p < ARRLEN(single_step_config); p++){
        Particle* particle = &state->single_step[p];
        int steps = single_step_config[p].steps;

        float dt = 1.0/(60.0*steps);
        //float dt = 1;

        //dbg("%f", dt);

        single_step_fn fn = single_step_config[p].fn;

        for (int i = 0; i < steps; i++) {
            float G = 10000000;
            fn(particle, dt, gravity, &G);
        }
    }


        for (int p = 0; p < ARRLEN(single_step_config); p++){

            Color c = single_step_config[p].color;
            DrawCircleV(state->single_step[p].pos, 3, c);
        }
        DrawCircleV((Vector2){0}, 10, WHITE);


    }

    EndMode2D();
    EndTextureMode();
    DrawTexturePro(state->trail_tex.texture, 
        (Rectangle){ 0, 0, 
            (float)state->trail_tex.texture.width, 
            -(float)state->trail_tex.texture.height 
        }, 
        (Rectangle){ 
            0, 0, 
            (float)state->trail_tex.texture.width, 
            (float)state->trail_tex.texture.height
        },
        (Vector2){ 0, 0 }, 0.0f, WHITE
    );
    BeginMode2D(cam);


    BeginBlendMode(BLEND_ALPHA);

        for (int p = 0; p < ARRLEN(single_step_config); p++){

            Color c = single_step_config[p].color;
            DrawCircleV(state->single_step[p].pos, 10, ColorAlpha(c, 1));
            DrawTextEx(
                GetFontDefault(),
                TextFormat("%d", p),
                Vector2Add(state->single_step[p].pos, (Vector2){12,12}),
                10, 0, c
            );
        }
        DrawCircleV((Vector2){0}, 10, WHITE);


    EndBlendMode();
    EndMode2D();

    DrawFPS(10, 10);
    DrawText(
        TextFormat("timewarp: %d", state->warp),
        10,
        50,
        20,
        WHITE
    );

    EndDrawing();
}

void* init() {
    State* state = calloc(8, 512); // should be enough
    if(!state){
        puts("buy more ram");
        exit(1);
    }
    state->trail_tex = LoadRenderTexture(100, 100);
    state->single_step = malloc(ARRLEN(single_step_config) * sizeof *state->single_step);

    reset_sim(state);

    return state;
}
