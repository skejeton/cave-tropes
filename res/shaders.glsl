@vs vs
in vec4 position;
in vec4 color0;
out vec4 color;
uniform vs_params {
    vec2 offset;
};

void main() {
    gl_Position = position + vec4(offset, 0, 0);
    color = color0;
}
@end

@fs fs
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@program triangle vs fs