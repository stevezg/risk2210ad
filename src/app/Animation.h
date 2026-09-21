#pragma once
// Frame-independent tween/animation pipeline. Everything visual that changes
// over time goes through an Animator so it can be paused, sped up or chained.
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "raylib.h"

namespace app {

enum class Ease { Linear, InQuad, OutQuad, InOutQuad, InOutCubic, OutBack, OutElastic, InOutSine };

inline float ease(Ease e, float t) {
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    switch (e) {
        case Ease::Linear: return t;
        case Ease::InQuad: return t * t;
        case Ease::OutQuad: return 1 - (1 - t) * (1 - t);
        case Ease::InOutQuad: return t < 0.5f ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2.0f) / 2;
        case Ease::InOutCubic: return t < 0.5f ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3.0f) / 2;
        case Ease::OutBack: {
            const float c1 = 1.70158f, c3 = c1 + 1;
            return 1 + c3 * std::pow(t - 1, 3.0f) + c1 * std::pow(t - 1, 2.0f);
        }
        case Ease::OutElastic: {
            if (t == 0 || t == 1) return t;
            const float c4 = (2 * PI) / 3;
            return std::pow(2.0f, -10 * t) * std::sin((t * 10 - 0.75f) * c4) + 1;
        }
        case Ease::InOutSine: return -(std::cos(PI * t) - 1) / 2;
    }
    return t;
}

inline Vector2 lerp(Vector2 a, Vector2 b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}; }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

/// One time-based interpolation. `onUpdate` receives the eased 0..1 value.
struct Tween {
    std::string tag;
    float elapsed = 0;
    float duration = 1;
    float delay = 0;
    Ease easing = Ease::InOutCubic;
    std::function<void(float)> onUpdate;
    std::function<void()> onDone;
    bool done = false;
};

class Animator {
public:
    Tween& add(float duration, Ease easing, std::function<void(float)> onUpdate, std::function<void()> onDone = {},
               float delay = 0, std::string tag = {}) {
        auto t = std::make_shared<Tween>();
        t->duration = duration;
        t->easing = easing;
        t->onUpdate = std::move(onUpdate);
        t->onDone = std::move(onDone);
        t->delay = delay;
        t->tag = std::move(tag);
        tweens_.push_back(t);
        return *t;
    }

    void update(float dt) {
        dt *= speed;
        // iterate by index: callbacks may add new tweens
        for (size_t i = 0; i < tweens_.size(); ++i) {
            auto t = tweens_[i];
            if (t->done) continue;
            if (t->delay > 0) {
                t->delay -= dt;
                if (t->delay > 0) continue;
                dt = -t->delay;  // spill the remainder into the tween
                t->delay = 0;
            }
            t->elapsed += dt;
            float u = t->duration <= 0 ? 1 : t->elapsed / t->duration;
            if (t->onUpdate) t->onUpdate(ease(t->easing, u));
            if (u >= 1) {
                t->done = true;
                if (t->onDone) t->onDone();
            }
        }
        tweens_.erase(std::remove_if(tweens_.begin(), tweens_.end(), [](const auto& t) { return t->done; }), tweens_.end());
    }

    void cancel(const std::string& tag) {
        for (auto& t : tweens_)
            if (t->tag == tag) t->done = true;
    }
    bool busy() const { return !tweens_.empty(); }
    size_t count() const { return tweens_.size(); }
    float speed = 1.0f;

private:
    std::vector<std::shared_ptr<Tween>> tweens_;
};

/// A glowing energy node travelling along a polyline (e.g. a path through the
/// territory graph). Rendered by the board view; driven by an Animator.
struct TravelOrb {
    std::vector<Vector2> path;
    Color color = {80, 220, 255, 255};
    float progress = 0;  // 0..1 across the whole path
    float radius = 7;
    bool finished = false;
    std::vector<Vector2> trail;

    Vector2 position() const {
        if (path.size() < 2) return path.empty() ? Vector2{0, 0} : path.front();
        float total = 0;
        std::vector<float> seg;
        for (size_t i = 1; i < path.size(); ++i) {
            float l = std::hypot(path[i].x - path[i - 1].x, path[i].y - path[i - 1].y);
            seg.push_back(l);
            total += l;
        }
        float d = progress * total;
        for (size_t i = 0; i < seg.size(); ++i) {
            if (d <= seg[i] || i == seg.size() - 1) return lerp(path[i], path[i + 1], seg[i] > 0 ? d / seg[i] : 1);
            d -= seg[i];
        }
        return path.back();
    }
};

}  // namespace app
