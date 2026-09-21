#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Animation.h"
#include "BoardView.h"
#include "Renderer.h"
#include "raylib.h"
#include "risk2210/Map.h"

namespace app {

/// Expanding neon ring used for impacts / conquest feedback.
struct PulseFx {
    Vector2 pos;
    Color color;
    float t = 0;  // 0..1
};

class App {
public:
    App();
    void run();
    /// Dev aid: save a screenshot after `frames` frames and exit.
    void screenshotAfter(int frames, std::string file) { shotFrames_ = frames; shotFile_ = std::move(file); }

private:
    void update(float dt);
    void handleInput(float dt);
    void drawScene(float time);
    void drawUi();
    void drawCommandDeck();
    void drawTooltip();
    void drawHelp();
    void drawYearTrack();

    void focusOn(int territory);
    void resetCamera();
    void launchOrb(int from, int to, Color color);
    void launchDemoOrb();
    void pulse(Vector2 pos, Color c);

    Camera2D physicalCamera() const;  // logical camera scaled to the framebuffer

    risk2210::Map map_;
    BoardView board_;
    Renderer renderer_;
    Animator animator_;
    PostSettings post_;
    BoardStyle style_;

    Camera2D cam_{};  // logical (points) camera
    bool showGrid_ = true;
    bool dragging_ = false;
    Vector2 dragStart_{};
    Vector2 dragTargetStart_{};

    int hovered_ = -1;
    int selected_ = -1;
    std::vector<std::shared_ptr<TravelOrb>> orbs_;
    std::vector<PulseFx> pulses_;
    float time_ = 0;
    bool showDemoWindow_ = false;
    int shotFrames_ = -1;
    std::string shotFile_;
};

}  // namespace app
