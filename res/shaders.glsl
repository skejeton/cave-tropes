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

void main() {
    frag_color = vec4((fs_uv.y+fs_uv.x+fs_normal)/4.0+vec3(0.5, 0.5, 0.5), 1.0);
}
@end

@program cube vs fs