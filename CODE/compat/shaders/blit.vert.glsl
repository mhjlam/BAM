#version 450

// Vertex uniform buffer (SDL_GPU SPIRV: vertex stage set=1, binding=0)
// Push with SDL_PushGPUVertexUniformData(cmd, 0, &uniforms, sizeof(uniforms))
layout(set = 1, binding = 0) uniform BlitUniforms {
    vec4 dst;  // xy = clip-space top-left, zw = clip-space size
    vec4 src;  // xy = UV top-left, zw = UV bottom-right
} u;

layout(location = 0) out vec2 v_uv;

// Draw a quad as 6 vertices (2 triangles, no vertex buffer).
// Vertex index encodes corner: 0=TL, 1=TR, 2=BL, 3=TR, 4=BR, 5=BL
void main() {
    const vec2 offsets[6] = vec2[6](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );
    vec2 off = offsets[gl_VertexIndex];
    v_uv        = mix(u.src.xy, u.src.zw, off);
    gl_Position = vec4(u.dst.xy + off * u.dst.zw, 0.0, 1.0);
}
