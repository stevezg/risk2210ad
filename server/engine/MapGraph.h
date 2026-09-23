#pragma once
#include <string>
#include <vector>

#include "Types.h"

namespace risk2210 {

struct TerritoryNode {
  int id = -1;
  std::string name;
  TerritoryType type = TerritoryType::Land;
  int region = -1;
  float x = 0, y = 0;  // board-layout position (percent), for reference/UI only
  bool landingSite = false;
  std::vector<int> adj;
};

struct Region {
  std::string name;
  TerritoryType type = TerritoryType::Land;
  int bonus = 0;
  std::vector<int> territories;
};

/// The Risk 2210 A.D. board as a graph: 42 land + 13 water + 14 lunar territories.
/// Data is generated from index.html's MAP/LINKS/REGIONS -- that file is the corrected
/// source of truth (every adjacency fix from this project lives there); regenerate this
/// file from it rather than hand-editing if the board data ever needs to change again.
class MapGraph {
 public:
  static MapGraph CreateStandard();

  const std::vector<TerritoryNode>& Territories() const { return terrs_; }
  const std::vector<Region>& Regions() const { return regions_; }
  const TerritoryNode& operator[](int id) const { return terrs_.at(id); }
  int Count() const { return static_cast<int>(terrs_.size()); }

  int Find(const std::string& name) const;
  bool AreAdjacent(int a, int b) const;
  std::vector<int> TerritoriesOfType(TerritoryType t) const;

  int AddRegion(const std::string& name, TerritoryType type, int bonus);
  void AddTerritory(const std::string& name, TerritoryType type, int region, float x, float y);
  void Link(const std::string& a, const std::string& b);
  void MarkLandingSite(const std::string& name);

 private:
  std::vector<TerritoryNode> terrs_;
  std::vector<Region> regions_;
};

}  // namespace risk2210
