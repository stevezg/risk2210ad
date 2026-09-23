// AUTO-GENERATED from index.html's MAP/LINKS/REGIONS (the corrected source of truth).
// Regenerate from index.html if the board data changes; do not hand-edit adjacency here.
#include "MapGraph.h"

#include <stdexcept>

namespace risk2210 {

MapGraph MapGraph::CreateStandard() {
  MapGraph m;
  int NA = m.AddRegion("North America", TerritoryType::Land, 5);
  int SA = m.AddRegion("South America", TerritoryType::Land, 2);
  int EU = m.AddRegion("Europe", TerritoryType::Land, 5);
  int AF = m.AddRegion("Africa", TerritoryType::Land, 3);
  int AS = m.AddRegion("Asia", TerritoryType::Land, 7);
  int AU = m.AddRegion("Australia", TerritoryType::Land, 2);
  int USP = m.AddRegion("US Pacific", TerritoryType::Water, 2);
  int ASP = m.AddRegion("Asia Pacific", TerritoryType::Water, 1);
  int NAT = m.AddRegion("N. Atlantic", TerritoryType::Water, 2);
  int SAT = m.AddRegion("S. Atlantic", TerritoryType::Water, 1);
  int IND = m.AddRegion("Indian", TerritoryType::Water, 2);
  int CRE = m.AddRegion("Cresinion", TerritoryType::Moon, 2);
  int DEL = m.AddRegion("Delphot", TerritoryType::Moon, 2);
  int SAJ = m.AddRegion("Sajon", TerritoryType::Moon, 3);

  m.AddTerritory("Northwestern Oil Emirate", TerritoryType::Land, NA, 7.83f, 9.54f);
  m.AddTerritory("Nunavut", TerritoryType::Land, NA, 16.97f, 9.16f);
  m.AddTerritory("Exiled States of America", TerritoryType::Land, NA, 35.51f, 8.78f);
  m.AddTerritory("Alberta", TerritoryType::Land, NA, 13.84f, 16.03f);
  m.AddTerritory("Canada", TerritoryType::Land, NA, 19.84f, 18.7f);
  m.AddTerritory("Republique du Quebec", TerritoryType::Land, NA, 26.63f, 20.61f);
  m.AddTerritory("Continental Biospheres", TerritoryType::Land, NA, 13.05f, 25.57f);
  m.AddTerritory("American Republic", TerritoryType::Land, NA, 19.06f, 29.77f);
  m.AddTerritory("Mexico", TerritoryType::Land, NA, 12.79f, 37.4f);
  m.AddTerritory("Nuevo Timoto", TerritoryType::Land, SA, 18.8f, 51.76f);
  m.AddTerritory("Andean Nations", TerritoryType::Land, SA, 15.14f, 58.4f);
  m.AddTerritory("Amazon Desert", TerritoryType::Land, SA, 21.93f, 60.69f);
  m.AddTerritory("Argentina", TerritoryType::Land, SA, 14.88f, 73.66f);
  m.AddTerritory("Iceland GRC", TerritoryType::Land, EU, 39.69f, 15.27f);
  m.AddTerritory("New Avalon", TerritoryType::Land, EU, 39.95f, 23.28f);
  m.AddTerritory("Jotenheim", TerritoryType::Land, EU, 49.35f, 9.92f);
  m.AddTerritory("Warsaw Republic", TerritoryType::Land, EU, 47.52f, 24.05f);
  m.AddTerritory("Ukrayina", TerritoryType::Land, EU, 59.53f, 19.85f);
  m.AddTerritory("Andorra", TerritoryType::Land, EU, 39.95f, 32.82f);
  m.AddTerritory("Imperial Balkania", TerritoryType::Land, EU, 50.13f, 28.85f);
  m.AddTerritory("Saharan Empire", TerritoryType::Land, AF, 41.78f, 47.33f);
  m.AddTerritory("Egypt", TerritoryType::Land, AF, 50.65f, 43.51f);
  m.AddTerritory("Ministry of Djibouti", TerritoryType::Land, AF, 55.35f, 52.29f);
  m.AddTerritory("Zaire Military Zone", TerritoryType::Land, AF, 50.91f, 63.36f);
  m.AddTerritory("Lesotho", TerritoryType::Land, AF, 51.96f, 76.34f);
  m.AddTerritory("Madagascar", TerritoryType::Land, AF, 61.36f, 75.95f);
  m.AddTerritory("Middle East", TerritoryType::Land, AS, 61.88f, 41.37f);
  m.AddTerritory("Afghanistan", TerritoryType::Land, AS, 67.36f, 30.92f);
  m.AddTerritory("Enclave of the Bear", TerritoryType::Land, AS, 69.19f, 17.94f);
  m.AddTerritory("Siberia", TerritoryType::Land, AS, 74.93f, 9.16f);
  m.AddTerritory("Sakha", TerritoryType::Land, AS, 84.07f, 8.78f);
  m.AddTerritory("Pevek", TerritoryType::Land, AS, 91.38f, 9.92f);
  m.AddTerritory("Alden", TerritoryType::Land, AS, 82.25f, 19.47f);
  m.AddTerritory("Khan Industrial State", TerritoryType::Land, AS, 83.03f, 26.34f);
  m.AddTerritory("Japan", TerritoryType::Land, AS, 92.69f, 31.68f);
  m.AddTerritory("Hong Kong", TerritoryType::Land, AS, 80.42f, 35.11f);
  m.AddTerritory("United Indiastan", TerritoryType::Land, AS, 73.89f, 44.27f);
  m.AddTerritory("Angkhor Wat", TerritoryType::Land, AS, 83.03f, 47.71f);
  m.AddTerritory("Java Cartel", TerritoryType::Land, AU, 87.73f, 63.74f);
  m.AddTerritory("New Guinea", TerritoryType::Land, AU, 96.34f, 65.27f);
  m.AddTerritory("Aboriginal League", TerritoryType::Land, AU, 86.42f, 82.06f);
  m.AddTerritory("Australian Testing Ground", TerritoryType::Land, AU, 93.99f, 80.15f);
  m.AddTerritory("Poseidon", TerritoryType::Water, USP, 6.27f, 21.76f);
  m.AddTerritory("Hawaiian Preserve", TerritoryType::Water, USP, 6.27f, 33.97f);
  m.AddTerritory("New Atlantis", TerritoryType::Water, USP, 8.62f, 45.8f);
  m.AddTerritory("Neo Tokyo", TerritoryType::Water, ASP, 93.73f, 40.84f);
  m.AddTerritory("Sung Tzu", TerritoryType::Water, ASP, 94.26f, 52.67f);
  m.AddTerritory("Western Ireland", TerritoryType::Water, NAT, 32.64f, 32.06f);
  m.AddTerritory("New York", TerritoryType::Water, NAT, 25.33f, 37.4f);
  m.AddTerritory("Nova Brasilia", TerritoryType::Water, NAT, 28.72f, 50.0f);
  m.AddTerritory("Neo Paulo", TerritoryType::Water, SAT, 31.33f, 69.47f);
  m.AddTerritory("The Ivory Reef", TerritoryType::Water, SAT, 39.95f, 66.41f);
  m.AddTerritory("South Ceylon", TerritoryType::Water, IND, 69.97f, 61.07f);
  m.AddTerritory("Microcorp", TerritoryType::Water, IND, 68.67f, 73.66f);
  m.AddTerritory("Akara", TerritoryType::Water, IND, 76.5f, 80.92f);
  m.AddTerritory("Harpalus", TerritoryType::Moon, CRE, 43.0f, 11.0f);
  m.AddTerritory("Sea of Rains", TerritoryType::Moon, CRE, 44.0f, 30.0f);
  m.AddTerritory("Ocean of Storms", TerritoryType::Moon, CRE, 31.0f, 49.0f);
  m.AddTerritory("Bay of Dew", TerritoryType::Moon, CRE, 21.0f, 34.0f);
  m.AddTerritory("Aristotle", TerritoryType::Moon, DEL, 62.0f, 16.0f);
  m.AddTerritory("Sea of Serenity", TerritoryType::Moon, DEL, 64.0f, 37.0f);
  m.AddTerritory("Sea of Crisis", TerritoryType::Moon, DEL, 74.0f, 30.0f);
  m.AddTerritory("Sea of Nectar", TerritoryType::Moon, DEL, 77.0f, 52.0f);
  m.AddTerritory("Rhaeticus", TerritoryType::Moon, SAJ, 54.0f, 56.0f);
  m.AddTerritory("Byrgius", TerritoryType::Moon, SAJ, 21.0f, 63.0f);
  m.AddTerritory("Sea of Clouds", TerritoryType::Moon, SAJ, 42.0f, 64.0f);
  m.AddTerritory("Straight Wall", TerritoryType::Moon, SAJ, 64.0f, 70.0f);
  m.AddTerritory("Marsh of Diseases", TerritoryType::Moon, SAJ, 32.0f, 77.0f);
  m.AddTerritory("Tycho", TerritoryType::Moon, SAJ, 52.0f, 79.0f);

  m.MarkLandingSite("Sea of Crisis");
  m.MarkLandingSite("Bay of Dew");
  m.MarkLandingSite("Tycho");

  m.Link("Northwestern Oil Emirate", "Nunavut");
  m.Link("Northwestern Oil Emirate", "Alberta");
  m.Link("Northwestern Oil Emirate", "Pevek");
  m.Link("Nunavut", "Alberta");
  m.Link("Nunavut", "Canada");
  m.Link("Nunavut", "Exiled States of America");
  m.Link("Exiled States of America", "Republique du Quebec");
  m.Link("Exiled States of America", "Iceland GRC");
  m.Link("Alberta", "Canada");
  m.Link("Alberta", "Continental Biospheres");
  m.Link("Canada", "Republique du Quebec");
  m.Link("Canada", "Continental Biospheres");
  m.Link("Canada", "American Republic");
  m.Link("Republique du Quebec", "American Republic");
  m.Link("Continental Biospheres", "American Republic");
  m.Link("Continental Biospheres", "Mexico");
  m.Link("American Republic", "Mexico");
  m.Link("Mexico", "Nuevo Timoto");
  m.Link("Nuevo Timoto", "Andean Nations");
  m.Link("Nuevo Timoto", "Amazon Desert");
  m.Link("Andean Nations", "Amazon Desert");
  m.Link("Andean Nations", "Argentina");
  m.Link("Amazon Desert", "Argentina");
  m.Link("Amazon Desert", "Saharan Empire");
  m.Link("Iceland GRC", "New Avalon");
  m.Link("Iceland GRC", "Jotenheim");
  m.Link("New Avalon", "Jotenheim");
  m.Link("New Avalon", "Warsaw Republic");
  m.Link("New Avalon", "Andorra");
  m.Link("Jotenheim", "Warsaw Republic");
  m.Link("Jotenheim", "Ukrayina");
  m.Link("Warsaw Republic", "Andorra");
  m.Link("Warsaw Republic", "Imperial Balkania");
  m.Link("Warsaw Republic", "Ukrayina");
  m.Link("Ukrayina", "Imperial Balkania");
  m.Link("Ukrayina", "Enclave of the Bear");
  m.Link("Ukrayina", "Afghanistan");
  m.Link("Ukrayina", "Middle East");
  m.Link("Andorra", "Imperial Balkania");
  m.Link("Andorra", "Saharan Empire");
  m.Link("Imperial Balkania", "Middle East");
  m.Link("Imperial Balkania", "Egypt");
  m.Link("Imperial Balkania", "Saharan Empire");
  m.Link("Saharan Empire", "Egypt");
  m.Link("Saharan Empire", "Ministry of Djibouti");
  m.Link("Saharan Empire", "Zaire Military Zone");
  m.Link("Egypt", "Middle East");
  m.Link("Egypt", "Ministry of Djibouti");
  m.Link("Ministry of Djibouti", "Zaire Military Zone");
  m.Link("Ministry of Djibouti", "Lesotho");
  m.Link("Ministry of Djibouti", "Madagascar");
  m.Link("Zaire Military Zone", "Lesotho");
  m.Link("Lesotho", "Madagascar");
  m.Link("Middle East", "Afghanistan");
  m.Link("Middle East", "United Indiastan");
  m.Link("Afghanistan", "Enclave of the Bear");
  m.Link("Afghanistan", "Hong Kong");
  m.Link("Afghanistan", "United Indiastan");
  m.Link("Enclave of the Bear", "Siberia");
  m.Link("Enclave of the Bear", "Hong Kong");
  m.Link("Siberia", "Sakha");
  m.Link("Siberia", "Alden");
  m.Link("Siberia", "Khan Industrial State");
  m.Link("Siberia", "Hong Kong");
  m.Link("Sakha", "Pevek");
  m.Link("Sakha", "Alden");
  m.Link("Pevek", "Alden");
  m.Link("Pevek", "Khan Industrial State");
  m.Link("Pevek", "Japan");
  m.Link("Alden", "Khan Industrial State");
  m.Link("Khan Industrial State", "Japan");
  m.Link("Khan Industrial State", "Hong Kong");
  m.Link("Hong Kong", "Angkhor Wat");
  m.Link("Hong Kong", "United Indiastan");
  m.Link("United Indiastan", "Angkhor Wat");
  m.Link("Angkhor Wat", "Java Cartel");
  m.Link("Java Cartel", "New Guinea");
  m.Link("Java Cartel", "Aboriginal League");
  m.Link("New Guinea", "Aboriginal League");
  m.Link("New Guinea", "Australian Testing Ground");
  m.Link("Aboriginal League", "Australian Testing Ground");
  m.Link("Poseidon", "Continental Biospheres");
  m.Link("Poseidon", "Hawaiian Preserve");
  m.Link("Hawaiian Preserve", "New Atlantis");
  m.Link("New Atlantis", "Nuevo Timoto");
  m.Link("New Atlantis", "Neo Tokyo");
  m.Link("Neo Tokyo", "Japan");
  m.Link("Neo Tokyo", "Hong Kong");
  m.Link("Neo Tokyo", "Sung Tzu");
  m.Link("Sung Tzu", "Java Cartel");
  m.Link("Western Ireland", "New Avalon");
  m.Link("Western Ireland", "New York");
  m.Link("New York", "American Republic");
  m.Link("New York", "Nova Brasilia");
  m.Link("Nova Brasilia", "Amazon Desert");
  m.Link("Neo Paulo", "Amazon Desert");
  m.Link("Neo Paulo", "The Ivory Reef");
  m.Link("The Ivory Reef", "Saharan Empire");
  m.Link("South Ceylon", "United Indiastan");
  m.Link("South Ceylon", "Microcorp");
  m.Link("Microcorp", "Madagascar");
  m.Link("Microcorp", "Akara");
  m.Link("Akara", "Aboriginal League");
  m.Link("Aristotle", "Harpalus");
  m.Link("Aristotle", "Sea of Crisis");
  m.Link("Aristotle", "Sea of Rains");
  m.Link("Aristotle", "Sea of Serenity");
  m.Link("Bay of Dew", "Byrgius");
  m.Link("Bay of Dew", "Harpalus");
  m.Link("Bay of Dew", "Ocean of Storms");
  m.Link("Bay of Dew", "Sea of Rains");
  m.Link("Byrgius", "Marsh of Diseases");
  m.Link("Byrgius", "Ocean of Storms");
  m.Link("Byrgius", "Sea of Clouds");
  m.Link("Harpalus", "Sea of Rains");
  m.Link("Marsh of Diseases", "Sea of Clouds");
  m.Link("Marsh of Diseases", "Tycho");
  m.Link("Ocean of Storms", "Sea of Clouds");
  m.Link("Ocean of Storms", "Sea of Rains");
  m.Link("Rhaeticus", "Sea of Clouds");
  m.Link("Rhaeticus", "Sea of Nectar");
  m.Link("Rhaeticus", "Sea of Rains");
  m.Link("Rhaeticus", "Sea of Serenity");
  m.Link("Rhaeticus", "Straight Wall");
  m.Link("Sea of Clouds", "Sea of Rains");
  m.Link("Sea of Clouds", "Straight Wall");
  m.Link("Sea of Clouds", "Tycho");
  m.Link("Sea of Crisis", "Sea of Nectar");
  m.Link("Sea of Crisis", "Sea of Serenity");
  m.Link("Sea of Nectar", "Sea of Serenity");
  m.Link("Sea of Nectar", "Straight Wall");
  m.Link("Sea of Rains", "Sea of Serenity");
  m.Link("Straight Wall", "Tycho");

  return m;
}

int MapGraph::AddRegion(const std::string& name, TerritoryType type, int bonus) {
  regions_.push_back(Region{name, type, bonus, {}});
  return static_cast<int>(regions_.size()) - 1;
}

void MapGraph::AddTerritory(const std::string& name, TerritoryType type, int region, float x, float y) {
  TerritoryNode t;
  t.id = static_cast<int>(terrs_.size());
  t.name = name; t.type = type; t.region = region; t.x = x; t.y = y;
  terrs_.push_back(t);
  regions_[region].territories.push_back(t.id);
}

void MapGraph::MarkLandingSite(const std::string& name) { terrs_[Find(name)].landingSite = true; }

void MapGraph::Link(const std::string& a, const std::string& b) {
  int ia = Find(a), ib = Find(b);
  if (ia < 0 || ib < 0) throw std::runtime_error("MapGraph::Link: unknown territory " + (ia < 0 ? a : b));
  if (AreAdjacent(ia, ib)) return;
  terrs_[ia].adj.push_back(ib);
  terrs_[ib].adj.push_back(ia);
}

int MapGraph::Find(const std::string& name) const {
  for (const auto& t : terrs_) if (t.name == name) return t.id;
  return -1;
}

bool MapGraph::AreAdjacent(int a, int b) const {
  for (int n : terrs_.at(a).adj) if (n == b) return true;
  return false;
}

std::vector<int> MapGraph::TerritoriesOfType(TerritoryType type) const {
  std::vector<int> out;
  for (const auto& t : terrs_) if (t.type == type) out.push_back(t.id);
  return out;
}

}  // namespace risk2210
