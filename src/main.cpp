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
extern "C" {
#include "bin/shaders.h"
}

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    float bg_color[3];
    char window_title_base[256];
    char *window_title;
} state;

static void init(void)
{
    sg_desc desc = {};
    desc.context = sapp_sgcontext();
    sg_setup(&desc);

    simgui_desc_t simgui_desc = { };
    simgui_setup(&simgui_desc);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    /* a vertex buffer with 4 vertices */
    float vertices[] = {
        // positions            // colors
        -0.5f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
    };


    sg_buffer_desc buffer_desc = {
        .data = SG_RANGE(vertices),
        .label = "triangle-vertices"
    };

    state.bind.vertex_buffers[0] = sg_make_buffer(&buffer_desc);

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(triangle_shader_desc(sg_query_backend()));

    /* create a pipeline object (default render states are fine for triangle) */
    sg_pipeline_desc pipeline_desc = {
        .shader = shd,
        .label = "triangle-pipeline"
    };
    /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
    pipeline_desc.layout.attrs[ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3;
    pipeline_desc.layout.attrs[ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4;
    
    state.pip = sg_make_pipeline(&pipeline_desc);

    /* a pass action to framebuffer to black */
    state.pass_action = {};
    state.pass_action.colors[0] = { .action=SG_ACTION_CLEAR, .value={0.0f, 0.0f, 0.0f, 1.0f } };
}

void set_bgcolor(float color[3])
{
    state.pass_action.colors[0].value = {color[0], color[1], color[2], 1.0};
}

void frame(void)
{
    const int width = sapp_width();
    const int height = sapp_height();
    set_bgcolor(state.bg_color);
    sapp_set_window_title(state.window_title_base);


    simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });
    // DRAW IMGUI STUFF
    ImGui::Begin("Change background color");
        ImGui::ColorEdit3("Background color", state.bg_color);
        ImGui::InputTextMultiline("Version", state.window_title, 256-(state.window_title-state.window_title_base));
    ImGui::End();
    ImGui::ShowDemoWindow();


    // DRAW USER STUFF
    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 6, 1);

    simgui_render();
    sg_end_pass();
    sg_commit();
}

static void event(const sapp_event *event)
{
    if (simgui_handle_event(event)) {
        return;
    }
}

void cleanup(void)
{
    simgui_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    sapp_desc app_desc = {
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .width = 640,
        .height = 480,
        .gl_force_gles2 = true,
    };

    app_desc.event_cb = event;
    app_desc.window_title = "Cave Tropes 0.0.0";

    state.window_title = state.window_title_base;
    memcpy(state.window_title, "Cave Tropes 0.0.0", strlen("Cave Tropes 0.0.0"));
    state.window_title += strlen("Cave Tropes ");

    app_desc.icon.sokol_default = true;
    app_desc.enable_clipboard = true;
    return app_desc;
}