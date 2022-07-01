// SPDX: MIT
#define SOKOL_IMPL
#if defined(_MSC_VER)
#define SOKOL_D3D11
#define SOKOL_LOG(str) OutputDebugStringA(str)
#elif defined(__EMSCRIPTEN__)
#define SOKOL_GLES2
#elif defined(__APPLE__)
// NOTE: on macOS, sokol.c is compiled explicitly as ObjC
#define SOKOL_METAL
#else
#define SOKOL_GLCORE33
#endif

#include "lib/imgui/imgui.h"
#include "lib/sokol/sokol_app.h"
#include "lib/sokol/sokol_gfx.h"
#include "lib/sokol/sokol_glue.h"
#include "lib/sokol/util/sokol_imgui.h"
#include "lib/HandmadeMath.h"
extern "C" {
#include "bin/shaders.h"
}

#undef SOKOL_APP_IMPL
#undef SOKOL_IMPL

//--
#include "camera.h"
#include "input.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    float bg_color[3];
    char window_title_base[256];
    char *window_title;

    ::camera camera;
    ::input input;
} state;

static void init(void)
{
    sg_desc desc = {};
    desc.context = sapp_sgcontext();
    sg_setup(&desc);

    simgui_desc_t simgui_desc = { };
    simgui_setup(&simgui_desc);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    /* a vertex buffer with 24 vertices */
    float vertices[] = {
        // pos                normal    uv
        -0.5, 0.5, -0.5,      0, 1, 0,  0, 1,
        -0.5, 0.5, 0.5,       0, 1, 0,  1, 1,
        0.5, 0.5, 0.5,        0, 1, 0,  1, 0,
        0.5, 0.5, -0.5,       0, 1, 0,  0, 0,

        
        -0.5, 0.5, 0.5,       -1, 0, 0, 0, 0,
        -0.5, -0.5, 0.5,      -1, 0, 0, 0, 1,
        -0.5, -0.5, -0.5,     -1, 0, 0, 1, 1,
        -0.5, 0.5, -0.5,      -1, 0, 0, 1, 0,

        
        0.5, 0.5, 0.5,        1, 0, 0, 0, 0,
        0.5, -0.5, 0.5,       1, 0, 0, 0, 1,
        0.5, -0.5, -0.5,      1, 0, 0, 1, 1,
        0.5, 0.5, -0.5,       1, 0, 0, 1, 0,

        
        0.5, 0.5, 0.5,        0, 0, -1, 0, 0,
        0.5, -0.5, 0.5,       0, 0, -1, 0, 1,
        -0.5, -0.5, 0.5,      0, 0, -1, 1, 1,
        -0.5, 0.5, 0.5,       0, 0, -1, 1, 0,

        
        0.5, 0.5, -0.5,       0, 0, 1, 0, 0,
        0.5, -0.5, -0.5,      0, 0, 1, 0, 1,
        -0.5, -0.5, -0.5,     0, 0, 1, 1, 1,
        -0.5, 0.5, -0.5,      0, 0, 1, 1, 0,

        
        -0.5, -0.5, -0.5,     0, -1, 0, 0, 1,
        -0.5, -0.5, 0.5,      0, -1, 0, 1, 1,
        0.5, -0.5, 0.5,       0, -1, 0, 1, 0,
        0.5, -0.5, -0.5,      0, -1, 0, 0, 0
    };

    uint16_t indices[] = {
        0, 1, 2,  0, 2, 3,
        6, 5, 4,  7, 6, 4,
        8, 9, 10,  8, 10, 11,
        14, 13, 12,  15, 14, 12,
        16, 17, 18,  16, 18, 19,
        22, 21, 20,  23, 22, 20
    };

    sg_buffer_desc index_buffer = {};
    index_buffer.data = SG_RANGE(indices);
    index_buffer.type = SG_BUFFERTYPE_INDEXBUFFER;
    index_buffer.label = "cube-indices";

    sg_buffer_desc buffer_desc = {};
    buffer_desc.data = SG_RANGE(vertices);
    buffer_desc.label = "cube-vertices";

    state.bind.index_buffer = sg_make_buffer(&index_buffer);
    state.bind.vertex_buffers[0] = sg_make_buffer(&buffer_desc);

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(cube_shader_desc(sg_query_backend()));

    /* create a pipeline object (default render states are fine for triangle) */
    sg_pipeline_desc pipeline_desc = {};
    /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
    pipeline_desc.shader = shd;
    pipeline_desc.label = "cube-pipeline";
    pipeline_desc.layout.buffers[0].stride = 4*8;
    pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
    pipeline_desc.layout.attrs[ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3;
    pipeline_desc.layout.attrs[ATTR_vs_normal].format = SG_VERTEXFORMAT_FLOAT3;
    pipeline_desc.layout.attrs[ATTR_vs_uv].format = SG_VERTEXFORMAT_FLOAT2;
    pipeline_desc.depth.write_enabled = true;
    pipeline_desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    
    state.pip = sg_make_pipeline(&pipeline_desc);

    /* a pass action to framebuffer to black */
    state.pass_action = {};
    state.pass_action.colors[0] = { .action=SG_ACTION_CLEAR, .value={0.0f, 0.0f, 0.0f, 1.0f } };

    state.camera = camera::init(45);
}

void set_bgcolor(float color[3])
{
    state.pass_action.colors[0].value = {color[0], color[1], color[2], 1.0};
}

void draw_cube(hmm_vec3 pos)
{
    hmm_mat4 m_m = HMM_Translate(pos);
    hmm_mat4 mvp = state.camera.get_vp() * m_m;

    vs_params_t params = {};
    memcpy(params.mvp, mvp.Elements, sizeof mvp.Elements);
    auto params_range = SG_RANGE(params);

    // DRAW USER STUFF
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &params_range);

    sg_draw(0, 36, 1);
}

void frame(void)
{
    const int width = sapp_width();
    const int height = sapp_height();

    if (state.input.key_states[SAPP_KEYCODE_ESCAPE].pressed) {
        sapp_lock_mouse(false);
    }
    if (state.input.mouse_states[SAPP_MOUSEBUTTON_LEFT].pressed) {
        printf("LOCK!\n");
        sapp_lock_mouse(true);
    }
    if (sapp_mouse_locked()) {
        state.input.handle_camera(&state.camera);
    }
    state.camera.set_aspect(float(width)/float(height));
    // Reset imgui rotations (HACK?)
    state.camera.rotate({0, 0});
    set_bgcolor(state.bg_color);
    sapp_set_window_title(state.window_title_base);

    simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });
    // DRAW IMGUI STUFF

    static bool show_ui = false;
    ImGui::BeginMainMenuBar();
        ImGui::Checkbox("Show UI", &show_ui);
    ImGui::EndMainMenuBar();

    if (show_ui) {
        ImGui::SetNextWindowSize({float(width), float(height)});
        ImGui::SetNextWindowPos({0, 16});
        ImGui::Begin("Stats", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
            ImGui::PushStyleColor(ImGuiCol_Button, 0xAA202222);
            ImGui::GetItemRectMin();
            char buf[256];
            snprintf(buf, 256, "FPS: %.1f\n", ImGui::GetIO().Framerate);
            ImGui::Button(buf);
            ImGui::PopStyleColor();
        ImGui::End();

        ImGui::Begin("Debug Stuff");
            ImGui::ColorEdit3("Background color", state.bg_color);
            ImGui::InputText("Version", state.window_title, 256-(state.window_title-state.window_title_base));
            ImGui::DragFloat2("Rotation", state.camera.yaw_pitch.Elements);

            static bool show_demo_window = false;
            ImGui::Checkbox("Show demo window", &show_demo_window);
            if (show_demo_window) {
                ImGui::ShowDemoWindow();
            }
            static bool show_debug_window = false;
            ImGui::Checkbox("Show debug window", &show_debug_window);
            if (show_debug_window) {
                ImGui::ShowMetricsWindow();
            }  
        ImGui::End();
    }

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    draw_cube({-2, 0, 5});
    draw_cube({-1, 0, 5});
    draw_cube({0, 0, 5});
    draw_cube({1, 0, 5});
    draw_cube({2, 0, 5});
    draw_cube({-3, 1, 5});
    draw_cube({3, 1, 5});
    draw_cube({-1, 2, 5});
    draw_cube({1, 2, 5});

    simgui_render();
    sg_end_pass();
    sg_commit();
    state.input.update();
}

static void event(const sapp_event *event)
{
    if (simgui_handle_event(event)) {
        return;
    }
    state.input.pass_event(event);

}

void cleanup(void)
{
    simgui_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    sapp_desc app_desc = {};

    app_desc.init_cb = init;
    app_desc.frame_cb = frame;
    app_desc.cleanup_cb = cleanup;
    app_desc.width = 640;
    app_desc.height = 480;
    app_desc.gl_force_gles2 = true;
    app_desc.event_cb = event;
    app_desc.window_title = "Cave Tropes 0.0.0";

    state.window_title = state.window_title_base;
    memcpy(state.window_title, "Cave Tropes 0.0.0", strlen("Cave Tropes 0.0.0"));
    state.window_title += strlen("Cave Tropes ");

    app_desc.icon.sokol_default = true;
    app_desc.enable_clipboard = true;
    return app_desc;
}