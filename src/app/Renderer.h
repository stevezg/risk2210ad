#pragma once
// Post-processing pipeline: scene -> bloom -> (optional CRT) -> screen.
#include "raylib.h"

namespace app {

struct PostSettings {
    bool bloom = true;
    float bloomIntensity = 1.4f;
    float bloomThreshold = 0.35f;
    bool crt = false;
    float crtStrength = 0.7f;
};

class Renderer {
public:
    void init();
    void shutdown();
    /// Recreates render targets if the physical framebuffer size changed.
    void resize();
    void beginScene();
    void endScene();
    /// Applies post-processing and draws the result to the back buffer.
    void present(const PostSettings& s, float time);

    int width() const { return w_; }
    int height() const { return h_; }
    float dpi() const { return dpi_; }
    Shader gridShader() const { return grid_; }
    /// Full-framebuffer quad with 0..1 texcoords (DrawRectangle's texcoords come from the font atlas).
    void drawFullscreenQuad() const;
    Texture2D sceneTexture() const { return scene_.texture; }

private:
    void loadShaders();
    RenderTexture2D scene_{}, post_{};
    Shader grid_{}, bloom_{}, crt_{};
    Texture2D white_{};
    int w_ = 0, h_ = 0;
    float dpi_ = 1;
};

}  // namespace app
