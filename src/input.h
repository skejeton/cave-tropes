#ifndef CT_INPUT_H
#define CT_INPUT_H

#include "lib/sokol/sokol_app.h"
#include "lib/HandmadeMath.h"
#include "camera.h"

struct input_key {
    bool pressed, released, held;
};

struct input {
    input_key key_states[512]; 
    input_key mouse_states[3];
    hmm_vec2 mouse_pos;
    hmm_vec2 mouse_delta;

    input init();
    void update(); // called every frame
    void pass_event(const sapp_event *event);
};

#endif