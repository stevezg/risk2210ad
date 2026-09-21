#include "risk2210/Cards.h"

namespace risk2210 {

namespace {

struct Entry {
    CardDef def;
    int copies;
};

std::vector<Entry> buildCatalogue() {
    using K = CardKind;
    using T = CardTiming;
    using C = Commander;
    std::vector<Entry> e;
    auto add = [&](const char* name, C deck, int cost, T timing, K kind, TerrType target, const char* text, int copies) {
        CardDef d;
        d.id = static_cast<int>(e.size());
        d.name = name;
        d.deck = deck;
        d.cost = cost;
        d.timing = timing;
        d.kind = kind;
        d.target = target;
        d.text = text;
        e.push_back({d, copies});
    };

    // ---- Land deck --------------------------------------------------------
    add("Reinforcements (Land)", C::Land, 0, T::BeforeFirstInvasion, K::Reinforcements, TerrType::Land,
        "Place 3 MODs, one each on 3 different land territories you occupy.", 4);
    add("Assemble MODs", C::Land, 1, T::BeforeFirstInvasion, K::AssembleMods, TerrType::Land,
        "Place 3 MODs on one land territory you occupy.", 4);
    add("Stealth MODs (Land)", C::Land, 0, T::OnInvasionDeclared, K::StealthMods, TerrType::Land,
        "Play after an invasion into a land territory is declared. Place 3 additional defending MODs there.", 4);
    add("Scout Forces", C::Land, 0, T::BeforeFirstInvasion, K::ScoutForces, TerrType::Land,
        "Draw a land territory card. When you occupy that territory, immediately place 5 MODs on it.", 3);
    add("Land Colony Influence", C::Land, 0, T::Scoring, K::ColonyInfluence, TerrType::Land,
        "Final scoring: +1 to your score (Land Commander must be in play).", 3);

    // ---- Diplomat deck ----------------------------------------------------
    add("Energy Crisis", C::Diplomat, 0, T::BeforeFirstInvasion, K::EnergyCrisis, TerrType::Land,
        "Collect 1 energy from each opponent.", 4);
    add("Cease Fire", C::Diplomat, 2, T::OnInvasionDeclared, K::CeaseFire, TerrType::Land,
        "Play after an opponent declares an invasion into a territory you occupy. The invasion is cancelled.", 4);
    add("Redeployment", C::Diplomat, 1, T::BeforeFirstInvasion, K::Redeployment, TerrType::Land,
        "Move up to 3 MODs from one territory you occupy to any other territory you occupy.", 4);
    add("Diplomatic Influence", C::Diplomat, 0, T::Scoring, K::ColonyInfluence, TerrType::Land,
        "Final scoring: +1 to your score (Diplomat Commander must be in play).", 3);

    // ---- Naval deck -------------------------------------------------------
    add("Reinforcements (Water)", C::Naval, 0, T::BeforeFirstInvasion, K::Reinforcements, TerrType::Water,
        "Place 3 MODs, one each on 3 different water territories you occupy.", 4);
    add("Energy Extraction", C::Naval, 0, T::BeforeFirstInvasion, K::EnergyExtraction, TerrType::Water,
        "Collect 1 energy for each water territory you occupy (maximum 4).", 4);
    add("Stealth MODs (Water)", C::Naval, 0, T::OnInvasionDeclared, K::StealthMods, TerrType::Water,
        "Play after an invasion into a water territory is declared. Place 3 additional defending MODs there.", 3);
    add("Assemble MODs (Water)", C::Naval, 1, T::BeforeFirstInvasion, K::AssembleMods, TerrType::Water,
        "Place 3 MODs on one water territory you occupy.", 2);
    add("Water Colony Influence", C::Naval, 0, T::Scoring, K::ColonyInfluence, TerrType::Water,
        "Final scoring: +1 to your score (Naval Commander must be in play).", 3);

    // ---- Nuclear deck -----------------------------------------------------
    add("Scatter Bomb (Land)", C::Nuclear, 1, T::BeforeFirstInvasion, K::ScatterBomb, TerrType::Land,
        "Turn over 3 land territory cards. Destroy half the opponents' units on those territories (round up).", 3);
    add("Scatter Bomb (Water)", C::Nuclear, 1, T::BeforeFirstInvasion, K::ScatterBomb, TerrType::Water,
        "Turn over 3 water territory cards. Destroy half the opponents' units on those territories (round up).", 2);
    add("Scatter Bomb (Moon)", C::Nuclear, 1, T::BeforeFirstInvasion, K::ScatterBomb, TerrType::Moon,
        "Turn over 3 lunar territory cards. Destroy half the opponents' units on those territories (round up).", 2);
    add("The Mother", C::Nuclear, 3, T::BeforeFirstInvasion, K::TheMother, TerrType::Land,
        "Draw a land territory card. Destroy all units there and in every adjacent land territory, then devastate it.", 3);
    add("Armageddon", C::Nuclear, 3, T::BeforeFirstInvasion, K::Armageddon, TerrType::Land,
        "Every territory on the board loses half of its units (round down).", 2);
    add("Nuclear Influence", C::Nuclear, 0, T::Scoring, K::ColonyInfluence, TerrType::Land,
        "Final scoring: +1 to your score (Nuclear Commander must be in play).", 2);

    // ---- Space deck -------------------------------------------------------
    add("Reinforcements (Moon)", C::Space, 0, T::BeforeFirstInvasion, K::Reinforcements, TerrType::Moon,
        "Place 3 MODs, one each on 3 different lunar territories you occupy.", 4);
    add("Invade Earth", C::Space, 2, T::BeforeFirstInvasion, K::InvadeEarth, TerrType::Moon,
        "Draw a land territory card. This turn you may invade that territory from any lunar territory.", 3);
    add("Stealth MODs (Moon)", C::Space, 0, T::OnInvasionDeclared, K::StealthMods, TerrType::Moon,
        "Play after an invasion into a lunar territory is declared. Place 3 additional defending MODs there.", 3);
    add("Assemble MODs (Moon)", C::Space, 1, T::BeforeFirstInvasion, K::AssembleMods, TerrType::Moon,
        "Place 3 MODs on one lunar territory you occupy.", 2);
    add("Lunar Colony Influence", C::Space, 0, T::Scoring, K::ColonyInfluence, TerrType::Moon,
        "Final scoring: +1 to your score (Space Commander must be in play).", 3);

    return e;
}

const std::vector<Entry>& catalogue() {
    static const std::vector<Entry> c = buildCatalogue();
    return c;
}

}  // namespace

const std::vector<CardDef>& Cards::all() {
    static const std::vector<CardDef> defs = [] {
        std::vector<CardDef> v;
        for (const auto& e : catalogue()) v.push_back(e.def);
        return v;
    }();
    return defs;
}

std::vector<int> Cards::deckFor(Commander c) {
    std::vector<int> deck;
    for (const auto& e : catalogue())
        if (e.def.deck == c)
            for (int i = 0; i < e.copies; ++i) deck.push_back(e.def.id);
    return deck;
}

}  // namespace risk2210
