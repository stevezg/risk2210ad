#pragma once
// GLSL 330 shaders (macOS OpenGL 3.3 core), embedded so the binary has no asset paths.

namespace app::shaders {

// Full-screen tactical grid drawn in world space so it pans/zooms with the camera.
inline const char* kGridFS = R"(
#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform vec2 resolution;   // physical pixels
uniform vec2 camTarget;    // world coords at screen centre
uniform float camZoom;     // physical pixels per world unit
uniform float time;

float gridLine(vec2 p, float spacing, float width) {
    vec2 g = abs(fract(p / spacing - 0.5) - 0.5) * spacing;
    float d = min(g.x, g.y);
    return 1.0 - smoothstep(0.0, width, d);
}

void main() {
    vec2 px = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y) * resolution;
    vec2 world = (px - resolution * 0.5) / camZoom + camTarget;

    vec3 base = vec3(0.030, 0.040, 0.075);
    float wpx = 1.0 / camZoom;                      // one pixel in world units
    float minor = gridLine(world, 25.0, 1.4 * wpx) * 0.26;
    float major = gridLine(world, 100.0, 2.0 * wpx) * 0.55;
    vec3 gridCol = vec3(0.10, 0.55, 0.80);
    vec3 col = base + gridCol * (minor + major);

    // slow energy sweep across the grid
    float sweep = exp(-pow((world.x + world.y * 0.4 - mod(time * 90.0, 2400.0) + 600.0) / 60.0, 2.0));
    col += gridCol * sweep * 0.25 * (minor + major) * 4.0;

    // vignette
    vec2 uv = fragTexCoord - 0.5;
    col *= 1.0 - dot(uv, uv) * 0.7;
    finalColor = vec4(col, 1.0);
}
)";

// Bright-pass + blur, added on top of the scene: cheap single-pass bloom.
inline const char* kBloomFS = R"(
#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec2 resolution;
uniform float intensity;
uniform float threshold;

void main() {
    vec2 texel = 1.0 / resolution;
    vec4 src = texture(texture0, fragTexCoord);
    vec3 glow = vec3(0.0);
    float wsum = 0.0;
    for (int x = -4; x <= 4; ++x) {
        for (int y = -4; y <= 4; ++y) {
            vec2 off = vec2(float(x), float(y)) * texel * 2.5;
            vec3 c = texture(texture0, fragTexCoord + off).rgb;
            float lum = dot(c, vec3(0.299, 0.587, 0.114));
            float w = exp(-(float(x * x + y * y)) / 12.0);
            glow += max(c - threshold, 0.0) * w;
            wsum += w;
        }
    }
    glow /= wsum;
    finalColor = vec4(src.rgb + glow * intensity, 1.0);
}
)";

// Optional commander-terminal look: scanlines, slight curvature, chroma split, flicker.
inline const char* kCrtFS = R"(
#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;
uniform float strength;

void main() {
    vec2 uv = fragTexCoord;
    vec2 c = uv - 0.5;
    uv = uv + c * dot(c, c) * 0.12 * strength;          // barrel distortion
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) { finalColor = vec4(0.0, 0.0, 0.0, 1.0); return; }
    float ca = 0.0015 * strength;
    float r = texture(texture0, uv + vec2(ca, 0.0)).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uv - vec2(ca, 0.0)).b;
    vec3 col = vec3(r, g, b);
    float scan = 0.5 + 0.5 * sin(uv.y * resolution.y * 1.5);
    col *= 1.0 - 0.18 * strength * scan;
    float sweep = smoothstep(0.0, 0.02, abs(fract(time * 0.12) - uv.y)) ;
    col *= 0.96 + 0.04 * sweep;
    col *= 1.0 - 0.35 * strength * dot(c, c);
    finalColor = vec4(col, 1.0);
}
)";

}  // namespace app::shaders
