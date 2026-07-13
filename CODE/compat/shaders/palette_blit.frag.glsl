#version 450

// Fragment samplers (SDL_GPU SPIRV: fragment stage set=2, binding=0 and 1)
// Bound with SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2)
layout(set = 2, binding = 0) uniform sampler2D u_indexed;  // R8_UNORM: one palette index per texel
layout(set = 2, binding = 1) uniform sampler2D u_palette;  // R8G8B8A8_UNORM: 256x1 RGBA palette

layout(location = 0) in  vec2 v_uv;
layout(location = 0) out vec4 f_color;

void main() {
    // R8_UNORM single channel gives value in [0,1]; round to recover the exact index byte.
    int idx = int(round(texture(u_indexed, v_uv).r * 255.0));

    // 0xFE (254) is the engine's transparency sentinel — show whatever is beneath.
    if (idx == 254) {
        f_color = vec4(0.0);
        return;
    }

    // Look up colour in the 256x1 palette texture.
    f_color = texture(u_palette, vec2((float(idx) + 0.5) / 256.0, 0.5));
}
