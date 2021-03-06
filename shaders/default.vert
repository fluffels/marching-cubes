#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "quaternions.glsl"
#include "uniforms.glsl"

layout(location=0) in vec4 inPosition;
layout(location=1) in vec4 inNormal;

layout(location=0) out vec4 outColor;
layout(location=1) out float outLight;

void main() {
    vec4 p = inPosition;
    p -= uniforms.eye;
    p = rotate_vertex_position(uniforms.rotation, p);
    p = uniforms.proj * p;
    gl_Position = p;
    vec3 lightV = uniforms.eye.xyz - inPosition.xyz;
    float dist = length(lightV);
    vec3 lightDir = lightV / dist;
    outColor = inNormal;
    outLight = dot(lightDir, inNormal.xyz) * (1 / dist);
}
