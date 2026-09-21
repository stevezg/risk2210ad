#pragma once
#include <string>
#include <vector>

#include "risk2210/Types.h"

namespace risk2210 {

struct Territory {
    int id = -1;
    std::string name;
    TerrType type = TerrType::Land;
    int region = -1;
    std::vector<int> adjacent;
    bool lunarLandingSite = false;  // Sea of Crisis, Bay of Dew, Tycho
};

/// A continent (land), water colony, or lunar colony.
struct Region {
    std::string name;
    TerrType type = TerrType::Land;
    int bonus = 0;
    std::vector<int> territories;
};

class Map {
public:
    /// The standard Risk 2210 A.D. board: 42 land, 13 water, 14 lunar territories.
    static Map standard();

    const std::vector<Territory>& territories() const { return terrs_; }
    const std::vector<Region>& regions() const { return regions_; }
    const Territory& territory(int id) const { return terrs_.at(id); }
    const Region& region(int id) const { return regions_.at(id); }
    int size() const { return static_cast<int>(terrs_.size()); }

    /// Returns -1 if no territory has this name (case-sensitive).
    int find(const std::string& name) const;
    bool adjacent(int a, int b) const;
    std::vector<int> territoriesOfType(TerrType t) const;

private:
    int addRegion(const std::string& name, TerrType type, int bonus);
    int addTerritory(const std::string& name, TerrType type, int region);
    void link(const std::string& a, const std::string& b);

    std::vector<Territory> terrs_;
    std::vector<Region> regions_;
};

}  // namespace risk2210
