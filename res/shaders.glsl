@vs vs
in vec3 position;
in vec3 normal;
in vec2 uv;
out vec2 fs_uv;
out vec3 fs_normal;
uniform vs_params {
    mat4 mvp;
};

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    fs_uv = uv;
    fs_normal = normal;
}
@end

@fs fs
in vec2 fs_uv;
in vec3 fs_normal;
out vec4 frag_color;
const float fog_distance = 10.0;
const float ambient = 0.5;
const vec3 light = vec3(0.1, 1.0, 0.3);

void main() {
    vec3 color = (fs_uv.y+fs_uv.x+fs_normal)/4.0+vec3(0.5, 0.5, 0.5);
    float factor = max(dot(fs_normal, normalize(light)), 0.0);
    factor += 0.7;
    factor *= smoothstep(0.3, 1, gl_FragCoord.w*fog_distance);

    frag_color = vec4(color * clamp(factor, 0.0, 1.0), 1.0);
}
@end

@program cube vs fs