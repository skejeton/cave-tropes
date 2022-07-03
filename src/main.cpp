// SPDX: MIT
#include <cstdint>
#include <cstdlib>
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

// Internal includes

#include "camera.h"
#include "input.h"

float cube_vertices[] = {
    // pos                normal    uv
    // +y
    -0.5, 0.5, -0.5,      0, 1, 0,  0, 1,
    -0.5, 0.5, 0.5,       0, 1, 0,  1, 1,
    0.5, 0.5, 0.5,        0, 1, 0,  1, 0,
    0.5, 0.5, -0.5,       0, 1, 0,  0, 0,

    // -x    
    -0.5, 0.5, 0.5,       -1, 0, 0, 0, 0,
    -0.5, -0.5, 0.5,      -1, 0, 0, 0, 1,
    -0.5, -0.5, -0.5,     -1, 0, 0, 1, 1,
    -0.5, 0.5, -0.5,      -1, 0, 0, 1, 0,

    // +x
    0.5, 0.5, 0.5,        1, 0, 0, 0, 0,
    0.5, -0.5, 0.5,       1, 0, 0, 0, 1,
    0.5, -0.5, -0.5,      1, 0, 0, 1, 1,
    0.5, 0.5, -0.5,       1, 0, 0, 1, 0,

    // -z
    0.5, 0.5, 0.5,        0, 0, -1, 0, 0,
    0.5, -0.5, 0.5,       0, 0, -1, 0, 1,
    -0.5, -0.5, 0.5,      0, 0, -1, 1, 1,
    -0.5, 0.5, 0.5,       0, 0, -1, 1, 0,

    // +z
    0.5, 0.5, -0.5,       0, 0, 1, 0, 0,
    0.5, -0.5, -0.5,      0, 0, 1, 0, 1,
    -0.5, -0.5, -0.5,     0, 0, 1, 1, 1,
    -0.5, 0.5, -0.5,      0, 0, 1, 1, 0,

    // -y    
    -0.5, -0.5, -0.5,     0, -1, 0, 0, 1,
    -0.5, -0.5, 0.5,      0, -1, 0, 1, 1,
    0.5, -0.5, 0.5,       0, -1, 0, 1, 0,
    0.5, -0.5, -0.5,      0, -1, 0, 0, 0
};

uint16_t cube_indices[] = {
    0, 1, 2,  0, 2, 3,       // +y
    6, 5, 4,  7, 6, 4,       // -x
    8, 9, 10,  8, 10, 11,    // +x
    14, 13, 12,  15, 14, 12, // -z
    16, 17, 18,  16, 18, 19, // +z
    22, 21, 20,  23, 22, 20  // -y
};

struct render_properties {
    bool wireframe_mode;
    bool disable_vsync;
};

struct render {
    sg_pipeline pip;
    sg_shader shader;
    sg_bindings bind;
    sg_pass_action pass_action;

    ::render_properties previous_properties, properties;
    ::camera camera;
};

static void set_rounding(float rounding)
{
    ImGui::GetStyle().TabRounding = rounding;
    ImGui::GetStyle().ChildRounding = rounding;
    ImGui::GetStyle().FrameRounding = rounding;
    ImGui::GetStyle().GrabRounding = HMM_MAX(rounding-1, 0);
    ImGui::GetStyle().WindowRounding = rounding;
}

void init_render_pipeline(::render *render)
{
    /* create a pipeline object (default render states are fine for triangle) */
    sg_pipeline_desc pipeline_desc = {};

    /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
    pipeline_desc.shader = render->shader;
    pipeline_desc.label = "cube-pipeline";
    pipeline_desc.layout.buffers[0].stride = 4*8;
    pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
    pipeline_desc.layout.attrs[ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3;
    pipeline_desc.layout.attrs[ATTR_vs_normal].format = SG_VERTEXFORMAT_FLOAT3;
    pipeline_desc.layout.attrs[ATTR_vs_uv].format = SG_VERTEXFORMAT_FLOAT2;
    pipeline_desc.depth.write_enabled = true;
    pipeline_desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pipeline_desc.cull_mode = SG_CULLMODE_FRONT;
    if (render->properties.wireframe_mode) {
        pipeline_desc.primitive_type = SG_PRIMITIVETYPE_LINE_STRIP;
    } else {
        pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    }
    if (render->properties.disable_vsync) {
        // disable fps cap
#ifdef SOKOL_GLCORE33 
        _sapp_glx_swapinterval(0);
#else
        fprintf(stderr, "Renderer: Can not disable vsync for this backend\n");
#endif
    } else {
        _sapp_glx_swapinterval(1);
    }

    if (render->pip.id == 0) { // init new
        render->pip = sg_make_pipeline(&pipeline_desc);
    } else { // recreate
        sg_uninit_pipeline(render->pip);
        sg_init_pipeline(render->pip, &pipeline_desc);
    }

    printf("Created new pipeline %d\n", render->pip.id);
}

void flush_render_pipeline(::render *render)
{
    // check if there's a reason to re initialize
    if (memcmp(&render->previous_properties, &render->properties, sizeof render->properties) == 0) {
        return;
    }

    render->previous_properties = render->properties;

    init_render_pipeline(render);
}

////////////
// Render
::render init_render()
{
    ::render state = {};
    sg_desc desc = {};
    desc.context = sapp_sgcontext();
    sg_setup(&desc);

    sg_buffer_desc index_buffer = {};
    index_buffer.data = SG_RANGE(cube_indices);
    index_buffer.type = SG_BUFFERTYPE_INDEXBUFFER;
    index_buffer.label = "cube-indices";

    sg_buffer_desc buffer_desc = {};
    buffer_desc.data = SG_RANGE(cube_vertices);
    buffer_desc.label = "cube-vertices";

    state.bind.index_buffer = sg_make_buffer(&index_buffer);
    state.bind.vertex_buffers[0] = sg_make_buffer(&buffer_desc);

    /* create shader from code-generated sg_shader_desc */
    state.shader = sg_make_shader(cube_shader_desc(sg_query_backend()));

    init_render_pipeline(&state);

    /* a pass action to framebuffer to black */
    state.pass_action = {};
    state.pass_action.colors[0] = { .action=SG_ACTION_CLEAR, .value={0.0f, 0.0f, 0.0f, 1.0f } };

    state.camera = camera::init(45);
    return state;
}

void begin_render(::render *render) {
    flush_render_pipeline(render);
    sg_begin_default_pass(render->pass_action, sapp_width(), sapp_height());
}

void end_render(::render *render) {
    sg_end_pass();
    sg_commit();
}

void set_bgcolor(::render *render, float color[3])
{
    render->pass_action.colors[0].value = sg_color{color[0], color[1], color[2], 1.0};
}

// Flags for choosing sides of cube to display
typedef uint8_t cube_side_flags;
#define CUBE_SIDE_FLAG_PX 0b1
#define CUBE_SIDE_FLAG_NX 0b10
#define CUBE_SIDE_FLAG_PY 0b100
#define CUBE_SIDE_FLAG_NY 0b1000
#define CUBE_SIDE_FLAG_PZ 0b10000
#define CUBE_SIDE_FLAG_NZ 0b100000

void draw_cube_flags(::render *render, cube_side_flags flags)
{
    // FIXME(skejeton): this is a shit way to draw it
    if (flags & CUBE_SIDE_FLAG_PY) {
        sg_draw(0, 6, 1);
    }
    if (flags & CUBE_SIDE_FLAG_NX) {
        sg_draw(6, 6, 1);
    }
    if (flags & CUBE_SIDE_FLAG_PX) {
        sg_draw(12, 6, 1);
    }
    if (flags & CUBE_SIDE_FLAG_PZ) {
        sg_draw(18, 6, 1);
    }
    if (flags & CUBE_SIDE_FLAG_NZ) {
        sg_draw(24, 6, 1);
    }
    if (flags & CUBE_SIDE_FLAG_NY) {
        sg_draw(30, 6, 1);
    }

}

void draw_cube(::render *render, hmm_vec3 pos, cube_side_flags flags)
{
    if (flags == 0) {
        return;
    }
    hmm_mat4 m_m = HMM_Translate(pos);
    hmm_mat4 mvp = render->camera.get_vp() * m_m;

    vs_params_t params = {};
    memcpy(params.mvp, mvp.Elements, sizeof mvp.Elements);
    auto params_range = SG_RANGE(params);

    // DRAW USER STUFF
    sg_apply_pipeline(render->pip);
    sg_apply_bindings(&render->bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &params_range);

    draw_cube_flags(render, flags);
}

void handle_camera_input(::render *render, ::input const &input)
{
    ::camera *camera = &render->camera;

    // FIXME(skejeton): LEAKING INPUT.H API!
    camera->rotate(hmm_vec2{input.mouse_delta.Y/4, -input.mouse_delta.X/4});

    if (input.key_states[SAPP_KEYCODE_LEFT_SHIFT].held) {
        camera->move(0, 0, -0.1);
    }

    if (input.key_states[SAPP_KEYCODE_SPACE].held) {
        camera->move(0, 0, 0.1);
    }

    if (input.key_states[SAPP_KEYCODE_W].held) {
        camera->move(0.1, 0, 0);
    }

    if (input.key_states[SAPP_KEYCODE_S].held) {
        camera->move(-0.1, 0, 0);
    }

    if (input.key_states[SAPP_KEYCODE_A].held) {
        camera->move(0, 0.1, 0);
    }

    if (input.key_states[SAPP_KEYCODE_D].held) {
        camera->move(0, -0.1, 0);
    }
}

#define WORLD_SIZE 32

struct chunk {
    bool data[WORLD_SIZE][WORLD_SIZE][WORLD_SIZE];
    cube_side_flags mesh_map[WORLD_SIZE][WORLD_SIZE][WORLD_SIZE];
};

///////////
// World 
struct world {
    //        x   y   z
    bool data[WORLD_SIZE][WORLD_SIZE][WORLD_SIZE]; // world chunk 0, true = block set, false = no block
    cube_side_flags mesh_map[WORLD_SIZE][WORLD_SIZE][WORLD_SIZE];
};

#define WORLD_ITER(x, y, z) for (int x = 0; x < WORLD_SIZE; ++x) for (int y = 0; y < WORLD_SIZE; ++y) for (int z = 0; z < WORLD_SIZE; ++z)

///////////
// State
struct state {
    float bg_color[3];
    bool wireframe_mode;

    ::render render;
    ::input input;
    ::world world;
};

static ::state GLOBAL_state;

void handle_camera(::state *state)
{
    if (state->input.key_states[SAPP_KEYCODE_ESCAPE].pressed) {
        sapp_lock_mouse(false);
    }
    if (state->input.mouse_states[SAPP_MOUSEBUTTON_LEFT].pressed) {
        sapp_lock_mouse(true);
    }
    if (sapp_mouse_locked()) {
        handle_camera_input(&state->render, state->input);
    }

    state->render.camera.set_aspect(sapp_widthf()/sapp_heightf());
    // Reset imgui rotations (HACK?)
    state->render.camera.rotate({0, 0});
}

static bool check_block(::world const *world, int x, int y, int z)
{
    if (x < 0 || y < 0 || z < 0 || x >= WORLD_SIZE || y >= WORLD_SIZE || z >= WORLD_SIZE) {
        return false;
    }

    return world->data[x][y][z];
}


static cube_side_flags get_side_flags(::world const *world, int x, int y, int z)
{
    cube_side_flags output = 0;
    struct {
        int x, y, z;
    } const static neighbours[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

    int i = 0;
    for (auto neighbor : neighbours) {
        output = output | ((cube_side_flags)(!check_block(world, x+neighbor.x, y+neighbor.y, z+neighbor.z)) << i);
        i++;
    }

    return output;
}

void generage_world_mesh_map(::world *world) 
{
    WORLD_ITER(x, y, z) {
        world->mesh_map[x][y][z] = get_side_flags(world, x, y, z);
    }
}

::world generate_world()
{
    ::world output = {};

    WORLD_ITER(x, y, z) {
        const int y_treshold = sin(x/10.0)*3 + cos(z/10.0)*3 + 20;
        if (y <= y_treshold)  {
            output.data[x][y][z] = 1;
        }
    }

    generage_world_mesh_map(&output);
    return output;
}


void draw_world(::render *render, ::world const &world)
{
    // DRAW USER STUFF
    vs_params_t params = {};
    auto params_range = SG_RANGE(params);
    sg_apply_pipeline(render->pip);
    sg_apply_bindings(&render->bind);

    WORLD_ITER(x, y, z) {
        if (world.data[x][y][z]) {
            cube_side_flags flags = world.mesh_map[x][y][z];
            if (flags == 0) {
                continue;
            }
            hmm_mat4 m_m = HMM_Translate({float(x), float(y), float(z)});
            hmm_mat4 mvp = render->camera.get_vp() * m_m;

            memcpy(params.mvp, mvp.Elements, sizeof mvp.Elements);
            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &params_range);

            draw_cube_flags(render, flags);
        }
    }
}

static void init(void)
{
    GLOBAL_state.render = init_render();
    GLOBAL_state.world = generate_world();

    simgui_desc_t simgui_desc = { };
    simgui_setup(&simgui_desc);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    set_rounding(3);
}

static void ui(void)
{

    simgui_new_frame({sapp_width(), sapp_height(), sapp_frame_duration(), sapp_dpi_scale()});

    static bool show_ui = false;
    ImGui::BeginMainMenuBar();
        ImGui::Checkbox("Show UI", &show_ui);
    ImGui::EndMainMenuBar();

    if (show_ui) {
        ImGui::SetNextWindowSize({sapp_widthf(), sapp_heightf()});
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
            ImGui::ColorEdit3("Background color", GLOBAL_state.bg_color);
            ImGui::DragFloat2("Rotation", GLOBAL_state.render.camera.yaw_pitch.Elements);
            ImGui::Checkbox("Wireframe", &GLOBAL_state.render.properties.wireframe_mode);
            ImGui::Checkbox("Disable VSync", &GLOBAL_state.render.properties.disable_vsync);

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
    simgui_render();
}

void frame(void)
{
    handle_camera(&GLOBAL_state);
    set_bgcolor(&GLOBAL_state.render, GLOBAL_state.bg_color);
    sapp_set_window_title("Cave Tropes 0.0.1");

    begin_render(&GLOBAL_state.render);
    {
        draw_world(&GLOBAL_state.render, GLOBAL_state.world);
        ui();
    }
    end_render(&GLOBAL_state.render);

    GLOBAL_state.input.update();
}

static void event(const sapp_event *event)
{
    if (simgui_handle_event(event)) {
        return;
    }
    GLOBAL_state.input.pass_event(event);
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
    app_desc.width = 1048;
    app_desc.height = 768;
    app_desc.gl_force_gles2 = true;
    app_desc.event_cb = event;
    // TODO(skejeton): Re enable sample count to 4 when I optimize world rendering
    // app_desc.sample_count = 4;

    app_desc.icon.sokol_default = true;
    app_desc.enable_clipboard = true;
    return app_desc;
}