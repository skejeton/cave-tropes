#include "camera.h"
#include "lib/imgui/imgui.h"

static hmm_mat4 rotation_matrix(camera *camera)
{
    return HMM_Rotate(camera->yaw_pitch.Y, {0.0, 1.0, 0.0}) * HMM_Rotate(camera->yaw_pitch.X, {1.0, 0.0, 0.0});
}

static hmm_mat4 make_vp(camera *camera)
{
    hmm_vec4 eye = rotation_matrix(camera) * hmm_vec4{0.0f, 0.0f, 1.0f, 1.0f};
    hmm_mat4 view = HMM_LookAt(camera->position, camera->position + eye.XYZ, {0.0f, 1.0f, 0.0f});
    hmm_mat4 proj = HMM_Perspective(camera->fov_deg, camera->aspect, 0.1f, 100.0f);

    return proj * view;
}

camera camera::init(float fov_deg)
{
    camera result = {};
    result.fov_deg = fov_deg;
    result.vp = make_vp(&result);
    return result;
}

void camera::set_aspect(float aspect)
{
    this->aspect = aspect;
    this->vp = make_vp(this);
}

void camera::rotate(hmm_vec2 by)
{
    this->yaw_pitch += by;

    // Normalize
    this->yaw_pitch.X = fmod(this->yaw_pitch.X, 360);
    this->yaw_pitch.Y = fmod(this->yaw_pitch.Y, 360);

    // Restrict yaw
    if (this->yaw_pitch.X > 89.9) {
        this->yaw_pitch.X = 89.9;
    }
    if (this->yaw_pitch.X < -89.9) {
        this->yaw_pitch.X = -89.9;
    }

    this->vp = make_vp(this);
}

void camera::move(float forward, float sideways, float upward)
{
    this->position += (HMM_Rotate(this->yaw_pitch.Y, {0.0, 1.0, 0.0}) * hmm_vec4{sideways, upward, forward}).XYZ;
    this->vp = make_vp(this);
}

hmm_mat4 camera::get_vp() const
{
    return this->vp;
}