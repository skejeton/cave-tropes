#include "input.h"
#include <cstdio>

input input::init()
{
    return input{};
}

void input::update()
{
    // Reset deltas
    this->mouse_delta = {0, 0};

    // Reset one-frame input states
    for (int i = 0; i < 512; ++i) {
        this->key_states[i].pressed = false;
        this->key_states[i].released = false;
    }

    for (int i = 0; i < 3; ++i) {
        this->mouse_states[i].pressed = false;
        this->mouse_states[i].released = false;
    }
}

void input::pass_event(const sapp_event *event)
{
    switch (event->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            this->key_states[event->key_code].held = true;
            this->key_states[event->key_code].pressed = true;
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            this->key_states[event->key_code].held = false;
            this->key_states[event->key_code].pressed = false;
            this->key_states[event->key_code].released = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            this->mouse_pos = {event->mouse_x, event->mouse_y};
            this->mouse_delta += {event->mouse_dx, event->mouse_dy};
            printf("Mouse delta %g %g\n", this->mouse_delta.X, this->mouse_delta.Y);
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            if (event->mouse_button >= 0 && event->mouse_button < 3) {
                this->mouse_states[event->mouse_button].held = true;
                this->mouse_states[event->mouse_button].pressed = true;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            if (event->mouse_button >= 0 && event->mouse_button < 3) {
                this->mouse_states[event->mouse_button].held = false;
                this->mouse_states[event->mouse_button].pressed = false;
                this->mouse_states[event->mouse_button].released = true;
            }
            break;
        default:
            break;
    }
}

void input::handle_camera(camera *camera)
{
    camera->rotate(hmm_vec2{this->mouse_delta.Y, -this->mouse_delta.X});

    if (this->key_states[SAPP_KEYCODE_LEFT_SHIFT].held) {
        camera->move(0, 0, -0.1);
    }

    if (this->key_states[SAPP_KEYCODE_SPACE].held) {
        camera->move(0, 0, 0.1);
    }

    if (this->key_states[SAPP_KEYCODE_W].held) {
        camera->move(0.1, 0, 0);
    }

    if (this->key_states[SAPP_KEYCODE_S].held) {
        camera->move(-0.1, 0, 0);
    }

    if (this->key_states[SAPP_KEYCODE_A].held) {
        camera->move(0, 0.1, 0);
    }

    if (this->key_states[SAPP_KEYCODE_D].held) {
        camera->move(0, -0.1, 0);
    }
}