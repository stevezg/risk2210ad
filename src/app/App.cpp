#include "App.h"

#include <algorithm>
#include <cmath>
#include <random>

#include "Theme.h"
#include "imgui.h"
#include "rlImGui.h"

namespace app {

namespace {
constexpr int kInitW = 1500, kInitH = 940;
constexpr float kMinZoom = 0.35f, kMaxZoom = 4.0f;
}  // namespace

App::App() : map_(risk2210::Map::standard()), board_(map_) {}

Camera2D App::physicalCamera() const {
    float d = renderer_.dpi();
    Camera2D c = cam_;
    c.offset = {cam_.offset.x * d, cam_.offset.y * d};
    c.zoom = cam_.zoom * d;
    return c;
}

void App::resetCamera() {
    Rectangle b = board_.bounds();
    float sw = static_cast<float>(GetScreenWidth()), sh = static_cast<float>(GetScreenHeight());
    float zoom = std::min(sw / b.width, sh / b.height) * 0.95f;
    Vector2 target{b.x + b.width / 2, b.y + b.height / 2};
    animator_.cancel("cam");
    Vector2 t0 = cam_.target;
    float z0 = cam_.zoom;
    animator_.add(0.6f, Ease::InOutCubic, [=](float u) {
        cam_.target = lerp(t0, target, u);
        cam_.zoom = lerp(z0, zoom, u);
    }, {}, 0, "cam");
}

void App::focusOn(int territory) {
    if (territory < 0) return;
    Vector2 target = board_.position(territory);
    float zoom = std::max(cam_.zoom, 1.8f);
    animator_.cancel("cam");
    Vector2 t0 = cam_.target;
    float z0 = cam_.zoom;
    animator_.add(0.55f, Ease::InOutCubic, [=](float u) {
        cam_.target = lerp(t0, target, u);
        cam_.zoom = lerp(z0, zoom, u);
    }, {}, 0, "cam");
}

void App::pulse(Vector2 pos, Color c) {
    pulses_.push_back({pos, c, 0});
    size_t idx = pulses_.size() - 1;
    (void)idx;
    animator_.add(0.7f, Ease::OutQuad, [this, pos](float u) {
        for (auto& p : pulses_)
            if (p.pos.x == pos.x && p.pos.y == pos.y) p.t = u;
    }, [this, pos] {
        pulses_.erase(std::remove_if(pulses_.begin(), pulses_.end(), [&](const PulseFx& p) { return p.pos.x == pos.x && p.pos.y == pos.y; }),
                      pulses_.end());
    });
}

void App::launchOrb(int from, int to, Color color) {
    auto ids = board_.path(from, to);
    if (ids.size() < 2) return;
    auto orb = std::make_shared<TravelOrb>();
    orb->path = board_.pathPoints(ids);
    orb->color = color;
    orbs_.push_back(orb);
    float duration = 0.45f * (ids.size() - 1);
    Vector2 dest = orb->path.back();
    animator_.add(duration, Ease::InOutSine, [orb](float u) {
        orb->progress = u;
        orb->trail.push_back(orb->position());
        if (orb->trail.size() > 28) orb->trail.erase(orb->trail.begin());
    }, [this, orb, dest, color] {
        orb->finished = true;
        pulse(dest, color);
        orbs_.erase(std::remove(orbs_.begin(), orbs_.end(), orb), orbs_.end());
    });
}

void App::launchDemoOrb() {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> pick(0, map_.size() - 1);
    int from = pick(rng), to = pick(rng);
    for (int i = 0; i < 20 && board_.path(from, to).size() < 3; ++i) to = pick(rng);
    static const Color cols[] = {{80, 220, 255, 255}, {255, 80, 110, 255}, {120, 255, 140, 255}, {255, 200, 70, 255}};
    launchOrb(from, to, cols[pick(rng) % 4]);
}

void App::run() {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(kInitW, kInitH, "Risk 2210 A.D.");
    SetTargetFPS(60);
    renderer_.init();

    rlImGuiSetup(true);
    theme::apply();

    cam_.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    cam_.zoom = 1.0f;
    cam_.target = {500, 450};
    resetCamera();
    animator_.update(10);  // snap the intro tween
    if (shotFrames_ > 0) {  // make the screenshot show something moving
        for (int i = 0; i < 4; ++i) launchDemoOrb();
        selected_ = map_.find("Saharan Empire");
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        time_ += dt;
        renderer_.resize();
        cam_.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

        update(dt);

        BeginDrawing();
        ClearBackground(BLACK);
        renderer_.beginScene();
        drawScene(time_);
        renderer_.endScene();
        renderer_.present(post_, time_);
        rlImGuiBegin();
        drawUi();
        rlImGuiEnd();
        EndDrawing();
        if (shotFrames_ >= 0 && --shotFrames_ == 0) {
            Image img = LoadImageFromTexture(renderer_.sceneTexture());
            ImageFlipVertical(&img);
            ExportImage(img, shotFile_.c_str());
            UnloadImage(img);
            break;
        }
    }
    rlImGuiShutdown();
    renderer_.shutdown();
    CloseWindow();
}

void App::update(float dt) {
    handleInput(dt);
    animator_.update(dt);
}

void App::handleInput(float dt) {
    ImGuiIO& io = ImGui::GetIO();
    Vector2 mouse = GetMousePosition();
    Vector2 world = GetScreenToWorld2D(mouse, cam_);
    hovered_ = io.WantCaptureMouse ? -1 : board_.hitTest(world);

    if (!io.WantCaptureMouse) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            animator_.cancel("cam");
            Vector2 before = GetScreenToWorld2D(mouse, cam_);
            cam_.zoom = std::clamp(cam_.zoom * (1.0f + wheel * 0.12f), kMinZoom, kMaxZoom);
            Vector2 after = GetScreenToWorld2D(mouse, cam_);
            cam_.target.x += before.x - after.x;
            cam_.target.y += before.y - after.y;
        }
        bool panButton = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ||
                         (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && hovered_ < 0 && !IsKeyDown(KEY_LEFT_SHIFT));
        if (panButton && !dragging_) {
            dragging_ = true;
            dragStart_ = mouse;
            dragTargetStart_ = cam_.target;
            animator_.cancel("cam");
        }
        if (dragging_) {
            if (!panButton) dragging_ = false;
            else
                cam_.target = {dragTargetStart_.x - (mouse.x - dragStart_.x) / cam_.zoom,
                               dragTargetStart_.y - (mouse.y - dragStart_.y) / cam_.zoom};
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && hovered_ >= 0) {
            if (IsKeyDown(KEY_LEFT_SHIFT) && selected_ >= 0 && selected_ != hovered_) {
                launchOrb(selected_, hovered_, BoardView::playerColor(0));
            } else {
                selected_ = hovered_;
                focusOn(hovered_);
                pulse(board_.position(hovered_), {120, 235, 255, 255});
            }
        }
    }
    if (!io.WantCaptureKeyboard) {
        float speed = 600.0f * dt / cam_.zoom;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) cam_.target.y -= speed;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) cam_.target.y += speed;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) cam_.target.x -= speed;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) cam_.target.x += speed;
        if (IsKeyPressed(KEY_R)) resetCamera();
        if (IsKeyPressed(KEY_SPACE)) launchDemoOrb();
        if (IsKeyPressed(KEY_C)) post_.crt = !post_.crt;
        if (IsKeyPressed(KEY_B)) post_.bloom = !post_.bloom;
        if (IsKeyPressed(KEY_ESCAPE)) selected_ = -1;
    }
}

void App::drawScene(float time) {
    Camera2D pc = physicalCamera();
    if (showGrid_) {
        Shader grid = renderer_.gridShader();
        float res[2] = {static_cast<float>(renderer_.width()), static_cast<float>(renderer_.height())};
        float target[2] = {cam_.target.x, cam_.target.y};
        float zoom = pc.zoom;
        SetShaderValue(grid, GetShaderLocation(grid, "resolution"), res, SHADER_UNIFORM_VEC2);
        SetShaderValue(grid, GetShaderLocation(grid, "camTarget"), target, SHADER_UNIFORM_VEC2);
        SetShaderValue(grid, GetShaderLocation(grid, "camZoom"), &zoom, SHADER_UNIFORM_FLOAT);
        SetShaderValue(grid, GetShaderLocation(grid, "time"), &time, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(grid);
        renderer_.drawFullscreenQuad();
        EndShaderMode();
    } else {
        DrawRectangle(0, 0, renderer_.width(), renderer_.height(), {5, 7, 14, 255});
    }

    BeginMode2D(pc);
    board_.draw(nullptr, hovered_, selected_, time, style_);
    for (const auto& p : pulses_) {
        float r = 12 + 70 * p.t;
        unsigned char a = static_cast<unsigned char>(220 * (1 - p.t));
        DrawRing(p.pos, r, r + 3 * (1 - p.t) + 1, 0, 360, 48, {p.color.r, p.color.g, p.color.b, a});
        DrawRing(p.pos, r * 0.6f, r * 0.6f + 1.5f, 0, 360, 48, {255, 255, 255, static_cast<unsigned char>(a / 2)});
    }
    for (const auto& o : orbs_) board_.drawOrb(*o, time);
    EndMode2D();
}

void App::drawUi() {
    drawCommandDeck();
    drawHelp();
    drawYearTrack();
    drawTooltip();
    if (showDemoWindow_) ImGui::ShowDemoWindow(&showDemoWindow_);
}

void App::drawCommandDeck() {
    ImGui::SetNextWindowPos({16, 16}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({330, 0}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("COMMAND DECK", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        theme::header("RISK 2210 A.D.  //  TACTICAL TERMINAL");
        theme::stat("Frame", TextFormat("%d fps  %.1f ms", GetFPS(), GetFrameTime() * 1000));
        theme::stat("Camera", TextFormat("zoom %.2fx  (%.0f, %.0f)", cam_.zoom, cam_.target.x, cam_.target.y));
        theme::stat("Framebuffer", TextFormat("%dx%d @%.0fx", renderer_.width(), renderer_.height(), renderer_.dpi()));
        theme::stat("Hover", hovered_ >= 0 ? map_.territory(hovered_).name.c_str() : "-", theme::kAmber);
        theme::stat("Selected", selected_ >= 0 ? map_.territory(selected_).name.c_str() : "-", theme::kCyan);
        ImGui::Dummy({0, 4});

        if (ImGui::CollapsingHeader("Visual layer", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Tactical grid", &showGrid_);
            ImGui::Checkbox("Bloom", &post_.bloom);
            ImGui::SliderFloat("Intensity", &post_.bloomIntensity, 0.0f, 3.0f, "%.2f");
            ImGui::SliderFloat("Threshold", &post_.bloomThreshold, 0.0f, 1.0f, "%.2f");
            ImGui::Checkbox("CRT terminal filter", &post_.crt);
            ImGui::SliderFloat("CRT strength", &post_.crtStrength, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Border glow", &style_.glowStrength, 0.0f, 3.0f, "%.2f");
        }
        if (ImGui::CollapsingHeader("Animation pipeline", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Time scale", &animator_.speed, 0.1f, 3.0f, "%.2fx");
            theme::stat("Active tweens", TextFormat("%zu", animator_.count()));
            theme::stat("Transport orbs", TextFormat("%zu", orbs_.size()));
            if (ImGui::Button("Launch demo orb  [Space]")) launchDemoOrb();
            ImGui::SameLine();
            if (ImGui::Button("Reset camera  [R]")) resetCamera();
        }
        if (ImGui::CollapsingHeader("Debug")) ImGui::Checkbox("ImGui demo window", &showDemoWindow_);
    }
    ImGui::End();
}

void App::drawHelp() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({16, io.DisplaySize.y - 16}, ImGuiCond_Always, {0, 1});
    ImGui::SetNextWindowBgAlpha(0.55f);
    if (ImGui::Begin("controls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextColored(theme::kTextDim, "drag / WASD  pan     wheel  zoom     click  focus territory");
        ImGui::TextColored(theme::kTextDim, "shift+click  send transport from selected     Space  demo orb     B / C  bloom / CRT");
    }
    ImGui::End();
}

void App::drawYearTrack() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, 16}, ImGuiCond_Always, {0.5f, 0});
    ImGui::SetNextWindowBgAlpha(0.7f);
    if (ImGui::Begin("year", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextColored(theme::kTextDim, "CAMPAIGN");
        ImGui::SameLine();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        const int years = 5, current = 0;
        for (int y = 0; y < years; ++y) {
            ImVec2 a{p.x + y * 58.0f, p.y + 2}, b{a.x + 50, a.y + 16};
            bool active = y == current;
            dl->AddRectFilled(a, b, ImGui::ColorConvertFloat4ToU32(active ? theme::kCyan : ImVec4{0.1f, 0.2f, 0.3f, 0.8f}), 4);
            char label[16];
            snprintf(label, sizeof label, "%d", 2206 + y);
            ImVec2 ts = ImGui::CalcTextSize(label);
            dl->AddText({a.x + (50 - ts.x) / 2, a.y + (16 - ts.y) / 2}, active ? IM_COL32(5, 15, 25, 255) : IM_COL32(140, 170, 200, 255), label);
        }
        ImGui::Dummy({years * 58.0f, 20});
    }
    ImGui::End();
}

void App::drawTooltip() {
    if (hovered_ < 0) return;
    const auto& t = map_.territory(hovered_);
    const auto& r = map_.region(t.region);
    ImGui::SetNextWindowBgAlpha(0.9f);
    ImGui::BeginTooltip();
    ImGui::TextColored(theme::kCyan, "%s", t.name.c_str());
    ImGui::Separator();
    theme::stat("Type", risk2210::toString(t.type));
    theme::stat("Colony", TextFormat("%s  (+%d)", r.name.c_str(), r.bonus), theme::kAmber);
    theme::stat("Borders", TextFormat("%zu", t.adjacent.size()));
    if (t.lunarLandingSite) ImGui::TextColored(theme::kAmber, "Lunar landing site");
    ImGui::EndTooltip();
}

}  // namespace app
