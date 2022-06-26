#ifndef CT_CAMERA_H
#define CT_CAMERA_H

#include "lib/HandmadeMath.h"

struct camera {
    float fov_deg;
    float aspect;
    hmm_vec3 position;
    hmm_vec2 yaw_pitch;
    hmm_mat4 vp;

    static camera init(float fov_deg);
    void set_aspect(float aspect);
    void rotate(hmm_vec2 by);
    void move(float forward, float sideways, float upward);

    hmm_mat4 get_vp();
};

#endif