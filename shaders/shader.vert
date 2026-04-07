#version 450

layout(push_constant) uniform PushConstants {
    mat4 MVP;
} pc;

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 0) out vec4 vColor;

void main() {
    gl_Position = pc.MVP * vec4(position, 1.0);
    vColor = color;
}