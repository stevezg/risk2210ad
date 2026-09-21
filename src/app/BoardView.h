#pragma once
// World-space rendering of the territory graph (Earth, water colonies, Moon).
#include <vector>

#include "Animation.h"
#include "raylib.h"
#include "risk2210/Game.h"
#include "risk2210/Map.h"

namespace app {

struct BoardStyle {
    float nodeRadius = 20.0f;
    float glowStrength = 1.0f;
};

class BoardView {
public:
    explicit BoardView(const risk2210::Map& map);

    const risk2210::Map& map() const { return map_; }
    Vector2 position(int territory) const { return pos_[territory]; }
    Rectangle bounds() const { return bounds_; }
    /// Territory under a world-space point, or -1.
    int hitTest(Vector2 world) const;
    /// Shortest path through the graph (list of territory ids) or empty.
    std::vector<int> path(int from, int to) const;
    std::vector<Vector2> pathPoints(const std::vector<int>& ids) const;

    /// Draws edges and nodes. `game` may be null (Phase 1 canvas: unowned board).
    void draw(const risk2210::Game* game, int hovered, int selected, float time, const BoardStyle& style) const;
    void drawOrb(const TravelOrb& orb, float time) const;

    static Color playerColor(int p);
    static Color regionColor(int region);

private:
    const risk2210::Map& map_;
    std::vector<Vector2> pos_;
    Rectangle bounds_{};
};

}  // namespace app
