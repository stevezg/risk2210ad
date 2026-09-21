#include "risk2210/Map.h"

#include <stdexcept>

namespace risk2210 {

int Map::addRegion(const std::string& name, TerrType type, int bonus) {
    Region r;
    r.name = name;
    r.type = type;
    r.bonus = bonus;
    regions_.push_back(r);
    return static_cast<int>(regions_.size()) - 1;
}

int Map::addTerritory(const std::string& name, TerrType type, int region) {
    Territory t;
    t.id = static_cast<int>(terrs_.size());
    t.name = name;
    t.type = type;
    t.region = region;
    terrs_.push_back(t);
    regions_[region].territories.push_back(t.id);
    return t.id;
}

void Map::link(const std::string& a, const std::string& b) {
    int ia = find(a), ib = find(b);
    if (ia < 0 || ib < 0) throw std::runtime_error("Map::link: unknown territory " + (ia < 0 ? a : b));
    if (!adjacent(ia, ib)) {
        terrs_[ia].adjacent.push_back(ib);
        terrs_[ib].adjacent.push_back(ia);
    }
}

int Map::find(const std::string& name) const {
    for (const auto& t : terrs_)
        if (t.name == name) return t.id;
    return -1;
}

bool Map::adjacent(int a, int b) const {
    for (int n : terrs_.at(a).adjacent)
        if (n == b) return true;
    return false;
}

std::vector<int> Map::territoriesOfType(TerrType type) const {
    std::vector<int> out;
    for (const auto& t : terrs_)
        if (t.type == type) out.push_back(t.id);
    return out;
}

Map Map::standard() {
    Map m;

    // ---- Continents (land) ---------------------------------------------
    int na = m.addRegion("North America", TerrType::Land, 5);
    for (const char* n : {"Aleutian Empire", "Nunavut", "Exiled States of America", "Alberta", "Canada",
                          "Republique du Quebec", "Continental Biospheres", "American Republic",
                          "Mexitlopoctli"})
        m.addTerritory(n, TerrType::Land, na);

    int sa = m.addRegion("South America", TerrType::Land, 2);
    for (const char* n : {"Nuevo Timoto", "Andean Nations", "Amazon Desert", "Argentina"})
        m.addTerritory(n, TerrType::Land, sa);

    int eu = m.addRegion("Europe", TerrType::Land, 5);
    for (const char* n : {"Iceland GRC", "New Avalon", "Jotenheim", "Warsaw Republic", "Ukrayina", "Andorra",
                          "Imperial Balkania"})
        m.addTerritory(n, TerrType::Land, eu);

    int af = m.addRegion("Africa", TerrType::Land, 3);
    for (const char* n : {"Saharan Empire", "Egypt", "Ministry of Djibouti", "Zaire Military Zone", "Lesotho",
                          "Madagascar"})
        m.addTerritory(n, TerrType::Land, af);

    int as = m.addRegion("Asia", TerrType::Land, 7);
    for (const char* n : {"Middle East", "Afghanistan", "Enclave of the Bear", "Siberia", "Sakha", "Pevek",
                          "Alden", "Khan Industrial State", "Japan", "Hong Kong", "United Indiastan",
                          "Angkhor Wat"})
        m.addTerritory(n, TerrType::Land, as);

    int au = m.addRegion("Australia", TerrType::Land, 2);
    for (const char* n : {"Java Cartel", "New Guinea", "Aboriginal League", "Australian Testing Ground"})
        m.addTerritory(n, TerrType::Land, au);

    // ---- Water colonies ----------------------------------------------------
    int usp = m.addRegion("US Pacific", TerrType::Water, 2);
    for (const char* n : {"Poseidon", "Hawaiian Preserve", "New Atlantis"}) m.addTerritory(n, TerrType::Water, usp);

    int asp = m.addRegion("Asia Pacific", TerrType::Water, 1);
    for (const char* n : {"Sung Tzu", "Neo Tokyo"}) m.addTerritory(n, TerrType::Water, asp);

    int nat = m.addRegion("Northern Atlantic", TerrType::Water, 2);
    for (const char* n : {"Western Ireland", "New York City", "Nova Brasilia"}) m.addTerritory(n, TerrType::Water, nat);

    int sat = m.addRegion("Southern Atlantic", TerrType::Water, 1);
    for (const char* n : {"Neo Paulo", "The Ivory Reef"}) m.addTerritory(n, TerrType::Water, sat);

    int ind = m.addRegion("Indian", TerrType::Water, 2);
    for (const char* n : {"Akara", "South Ceylon", "Microcorp"}) m.addTerritory(n, TerrType::Water, ind);

    // ---- Lunar colonies ----------------------------------------------------
    int cre = m.addRegion("Cresinion", TerrType::Moon, 2);
    for (const char* n : {"Harpalus", "Sea of Rains", "Ocean of Storms", "Bay of Dew"}) m.addTerritory(n, TerrType::Moon, cre);

    int del = m.addRegion("Delphot", TerrType::Moon, 2);
    for (const char* n : {"Aristotle", "Sea of Serenity", "Sea of Crisis", "Sea of Nectar"}) m.addTerritory(n, TerrType::Moon, del);

    int saj = m.addRegion("Sajon", TerrType::Moon, 3);
    for (const char* n : {"Rhaeticus", "Byrgius", "Sea of Clouds", "Straight Wall", "Marsh of Diseases", "Tycho"})
        m.addTerritory(n, TerrType::Moon, saj);

    for (const char* n : {"Sea of Crisis", "Bay of Dew", "Tycho"}) m.terrs_[m.find(n)].lunarLandingSite = true;

    // ---- Land connections (classic Risk topology with 2210 renames; the
    //      East Africa–Middle East and Greenland–Ontario bridges are absent
    //      on the 2210 board) -------------------------------------------------
    const std::pair<const char*, const char*> landLinks[] = {
        {"Aleutian Empire", "Nunavut"}, {"Aleutian Empire", "Alberta"}, {"Aleutian Empire", "Pevek"},
        {"Nunavut", "Alberta"}, {"Nunavut", "Canada"}, {"Nunavut", "Exiled States of America"},
        {"Exiled States of America", "Republique du Quebec"}, {"Exiled States of America", "Iceland GRC"},
        {"Alberta", "Canada"}, {"Alberta", "Continental Biospheres"},
        {"Canada", "Republique du Quebec"}, {"Canada", "Continental Biospheres"}, {"Canada", "American Republic"},
        {"Republique du Quebec", "American Republic"},
        {"Continental Biospheres", "American Republic"}, {"Continental Biospheres", "Mexitlopoctli"},
        {"American Republic", "Mexitlopoctli"},
        {"Mexitlopoctli", "Nuevo Timoto"},
        {"Nuevo Timoto", "Andean Nations"}, {"Nuevo Timoto", "Amazon Desert"},
        {"Andean Nations", "Amazon Desert"}, {"Andean Nations", "Argentina"},
        {"Amazon Desert", "Argentina"}, {"Amazon Desert", "Saharan Empire"},
        {"Iceland GRC", "New Avalon"}, {"Iceland GRC", "Jotenheim"},
        {"New Avalon", "Jotenheim"}, {"New Avalon", "Warsaw Republic"}, {"New Avalon", "Andorra"},
        {"Jotenheim", "Warsaw Republic"}, {"Jotenheim", "Ukrayina"},
        {"Warsaw Republic", "Andorra"}, {"Warsaw Republic", "Imperial Balkania"}, {"Warsaw Republic", "Ukrayina"},
        {"Ukrayina", "Imperial Balkania"}, {"Ukrayina", "Enclave of the Bear"}, {"Ukrayina", "Afghanistan"},
        {"Ukrayina", "Middle East"},
        {"Andorra", "Imperial Balkania"}, {"Andorra", "Saharan Empire"},
        {"Imperial Balkania", "Middle East"}, {"Imperial Balkania", "Egypt"},
        {"Saharan Empire", "Egypt"}, {"Saharan Empire", "Ministry of Djibouti"}, {"Saharan Empire", "Zaire Military Zone"},
        {"Egypt", "Middle East"}, {"Egypt", "Ministry of Djibouti"},
        {"Ministry of Djibouti", "Zaire Military Zone"}, {"Ministry of Djibouti", "Lesotho"}, {"Ministry of Djibouti", "Madagascar"},
        {"Zaire Military Zone", "Lesotho"},
        {"Lesotho", "Madagascar"},
        {"Middle East", "Afghanistan"}, {"Middle East", "United Indiastan"},
        {"Afghanistan", "Enclave of the Bear"}, {"Afghanistan", "Hong Kong"}, {"Afghanistan", "United Indiastan"},
        {"Enclave of the Bear", "Siberia"}, {"Enclave of the Bear", "Hong Kong"},
        {"Siberia", "Sakha"}, {"Siberia", "Alden"}, {"Siberia", "Khan Industrial State"}, {"Siberia", "Hong Kong"},
        {"Sakha", "Pevek"}, {"Sakha", "Alden"},
        {"Pevek", "Alden"}, {"Pevek", "Khan Industrial State"}, {"Pevek", "Japan"},
        {"Alden", "Khan Industrial State"},
        {"Khan Industrial State", "Japan"}, {"Khan Industrial State", "Hong Kong"},
        {"Hong Kong", "Angkhor Wat"}, {"Hong Kong", "United Indiastan"},
        {"United Indiastan", "Angkhor Wat"},
        {"Angkhor Wat", "Java Cartel"},
        {"Java Cartel", "New Guinea"}, {"Java Cartel", "Aboriginal League"},
        {"New Guinea", "Aboriginal League"}, {"New Guinea", "Australian Testing Ground"},
        {"Aboriginal League", "Australian Testing Ground"},
    };
    for (const auto& [a, b] : landLinks) m.link(a, b);

    // ---- Water connections (from the printed board; Pacific links wrap
    //      around the board edge) ---------------------------------------------
    const std::pair<const char*, const char*> waterLinks[] = {
        {"Poseidon", "Aleutian Empire"}, {"Poseidon", "Hawaiian Preserve"},
        {"Hawaiian Preserve", "Mexitlopoctli"}, {"Hawaiian Preserve", "New Atlantis"}, {"Hawaiian Preserve", "Neo Tokyo"},
        {"New Atlantis", "Nuevo Timoto"}, {"New Atlantis", "Sung Tzu"},
        {"Neo Tokyo", "Japan"}, {"Neo Tokyo", "Hong Kong"}, {"Neo Tokyo", "Sung Tzu"},
        {"Sung Tzu", "Java Cartel"},
        {"Western Ireland", "New Avalon"}, {"Western Ireland", "New York City"},
        {"New York City", "American Republic"}, {"New York City", "Nova Brasilia"},
        {"Nova Brasilia", "Nuevo Timoto"}, {"Nova Brasilia", "Saharan Empire"},
        {"Neo Paulo", "Amazon Desert"}, {"Neo Paulo", "The Ivory Reef"},
        {"The Ivory Reef", "Saharan Empire"},
        {"South Ceylon", "United Indiastan"}, {"South Ceylon", "Microcorp"},
        {"Microcorp", "Madagascar"}, {"Microcorp", "Akara"},
        {"Akara", "Aboriginal League"},
    };
    for (const auto& [a, b] : waterLinks) m.link(a, b);

    // ---- Lunar connections --------------------------------------------------
    const std::pair<const char*, const char*> moonLinks[] = {
        {"Harpalus", "Bay of Dew"}, {"Harpalus", "Sea of Rains"},
        {"Sea of Rains", "Bay of Dew"}, {"Sea of Rains", "Ocean of Storms"}, {"Sea of Rains", "Sea of Serenity"},
        {"Sea of Rains", "Aristotle"},
        {"Ocean of Storms", "Bay of Dew"}, {"Ocean of Storms", "Rhaeticus"}, {"Ocean of Storms", "Byrgius"},
        {"Aristotle", "Sea of Serenity"},
        {"Sea of Serenity", "Sea of Crisis"}, {"Sea of Serenity", "Sea of Nectar"}, {"Sea of Serenity", "Rhaeticus"},
        {"Sea of Crisis", "Sea of Nectar"},
        {"Sea of Nectar", "Rhaeticus"}, {"Sea of Nectar", "Straight Wall"},
        {"Rhaeticus", "Sea of Clouds"}, {"Rhaeticus", "Byrgius"},
        {"Byrgius", "Sea of Clouds"}, {"Byrgius", "Marsh of Diseases"},
        {"Sea of Clouds", "Straight Wall"}, {"Sea of Clouds", "Marsh of Diseases"}, {"Sea of Clouds", "Tycho"},
        {"Straight Wall", "Tycho"},
        {"Marsh of Diseases", "Tycho"},
    };
    for (const auto& [a, b] : moonLinks) m.link(a, b);

    return m;
}

}  // namespace risk2210
