#include "Renderer.h"

#include "Shaders.h"

namespace app {

void Renderer::loadShaders() {
    grid_ = LoadShaderFromMemory(nullptr, shaders::kGridFS);
    bloom_ = LoadShaderFromMemory(nullptr, shaders::kBloomFS);
    crt_ = LoadShaderFromMemory(nullptr, shaders::kCrtFS);
}

void Renderer::init() {
    loadShaders();
    Image px = GenImageColor(1, 1, WHITE);
    white_ = LoadTextureFromImage(px);
    UnloadImage(px);
    resize();
}

void Renderer::drawFullscreenQuad() const {
    DrawTexturePro(white_, {0, 0, 1, 1}, {0, 0, static_cast<float>(w_), static_cast<float>(h_)}, {0, 0}, 0, WHITE);
}

void Renderer::shutdown() {
    UnloadTexture(white_);
    UnloadRenderTexture(scene_);
    UnloadRenderTexture(post_);
    UnloadShader(grid_);
    UnloadShader(bloom_);
    UnloadShader(crt_);
}

void Renderer::resize() {
    int w = GetRenderWidth(), h = GetRenderHeight();
    if (w == w_ && h == h_) return;
    if (scene_.id) UnloadRenderTexture(scene_);
    if (post_.id) UnloadRenderTexture(post_);
    w_ = w;
    h_ = h;
    dpi_ = GetScreenWidth() > 0 ? static_cast<float>(w) / GetScreenWidth() : 1.0f;
    scene_ = LoadRenderTexture(w, h);
    post_ = LoadRenderTexture(w, h);
    SetTextureFilter(scene_.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(post_.texture, TEXTURE_FILTER_BILINEAR);
}

void Renderer::beginScene() {
    BeginTextureMode(scene_);
    ClearBackground(BLACK);
}

void Renderer::endScene() { EndTextureMode(); }

void Renderer::present(const PostSettings& s, float time) {
    const Rectangle src{0, 0, static_cast<float>(w_), -static_cast<float>(h_)};  // render textures are y-flipped
    const Rectangle dstPhys{0, 0, static_cast<float>(w_), static_cast<float>(h_)};
    const Rectangle dstScreen{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
    float res[2] = {static_cast<float>(w_), static_cast<float>(h_)};

    RenderTexture2D* final = &scene_;
    if (s.bloom) {
        BeginTextureMode(post_);
        BeginShaderMode(bloom_);
        SetShaderValue(bloom_, GetShaderLocation(bloom_, "resolution"), res, SHADER_UNIFORM_VEC2);
        SetShaderValue(bloom_, GetShaderLocation(bloom_, "intensity"), &s.bloomIntensity, SHADER_UNIFORM_FLOAT);
        SetShaderValue(bloom_, GetShaderLocation(bloom_, "threshold"), &s.bloomThreshold, SHADER_UNIFORM_FLOAT);
        DrawTexturePro(scene_.texture, src, dstPhys, {0, 0}, 0, WHITE);
        EndShaderMode();
        EndTextureMode();
        final = &post_;
    }
    if (s.crt) {
        BeginShaderMode(crt_);
        SetShaderValue(crt_, GetShaderLocation(crt_, "resolution"), res, SHADER_UNIFORM_VEC2);
        SetShaderValue(crt_, GetShaderLocation(crt_, "time"), &time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(crt_, GetShaderLocation(crt_, "strength"), &s.crtStrength, SHADER_UNIFORM_FLOAT);
    }
    DrawTexturePro(final->texture, src, dstScreen, {0, 0}, 0, WHITE);
    if (s.crt) EndShaderMode();
}

}  // namespace app
