// AUTO-GENERATED from index.html's CARD_DEFS (the corrected source of truth).
// Regenerate from index.html if the card rulings change; do not hand-edit this table.
#include "Cards.h"

namespace risk2210 {
namespace {
struct Entry { CardDef def; int copies; };
std::vector<Entry> BuildCatalogue() {
  std::vector<Entry> e;
  auto add = [&](const char* name, Commander deck, int copies, int cost, CardTiming timing, CardKind kind, TerritoryType target, const char* text, int amount) {
    CardDef d; d.id = static_cast<int>(e.size()); d.name = name; d.deck = deck; d.cost = cost;
    d.timing = timing; d.kind = kind; d.target = target; d.text = text; d.amount = amount;
    e.push_back({d, copies});
  };
  add("Cease Fire", Commander::Diplomat, 2, 2, CardTiming::React, CardKind::CeaseFire, TerritoryType::Land, "Prevent the invasion. The attacker cannot attack any of your territories for the rest of their turn.", 0);
  add("Colony Influence", Commander::Diplomat, 4, 0, CardTiming::Score, CardKind::Influence, TerritoryType::Land, "If your Diplomat Commander is still alive, move your score marker ahead 3 spaces.", 0);
  add("Decoys Revealed", Commander::Diplomat, 2, 0, CardTiming::Before, CardKind::Decoys, TerritoryType::Land, "Move any number of your commanders to any number of territories you control.", 0);
  add("Energy Crisis", Commander::Diplomat, 2, 0, CardTiming::Before, CardKind::EnergyCrisis, TerritoryType::Land, "Collect one energy from each opponent.", 0);
  add("Evacuation", Commander::Diplomat, 2, 0, CardTiming::React, CardKind::Evacuation, TerritoryType::Land, "Move all units from the attacked territory to any territory you occupy.", 0);
  add("MOD Reduction", Commander::Diplomat, 2, 2, CardTiming::Before, CardKind::ModReduction, TerritoryType::Land, "All of your opponents must remove 4 MODs. Then you remove 2 MODs.", 0);
  add("Redeployment", Commander::Diplomat, 3, 0, CardTiming::End, CardKind::Redeployment, TerritoryType::Land, "Take an extra free move this turn, after you have finished attacking.", 0);
  add("Territorial Station", Commander::Diplomat, 3, 1, CardTiming::Before, CardKind::TerrStation, TerritoryType::Land, "Place a Space Station on any land territory you occupy.", 0);
  add("Assemble MODs", Commander::Land, 3, 1, CardTiming::Before, CardKind::Assemble, TerritoryType::Land, "Place 3 MODs on any one land territory you occupy.", 0);
  add("Colony Influence", Commander::Land, 2, 0, CardTiming::Score, CardKind::Influence, TerritoryType::Land, "If your Land Commander is still alive, move your score marker ahead 3 spaces.", 0);
  add("Frequency Jam", Commander::Land, 2, 0, CardTiming::Before, CardKind::Jam, TerritoryType::Land, "Choose a player. The chosen player cannot play command cards during your turn.", 0);
  add("Land Death Trap", Commander::Land, 1, 3, CardTiming::React, CardKind::DeathTrap, TerritoryType::Land, "Your opponent must destroy half the units in the invading territory. Round up.", 0);
  add("Reinforcements", Commander::Land, 3, 0, CardTiming::Before, CardKind::Reinforce, TerritoryType::Land, "Place 3 MODs, one each on 3 different land territories you occupy.", 0);
  add("Scout Forces", Commander::Land, 3, 0, CardTiming::Before, CardKind::Scout, TerritoryType::Land, "Draw a land territory card and keep it secret. When you occupy this territory, immediately place 5 MODs there.", 0);
  add("Stealth MODs", Commander::Land, 5, 0, CardTiming::React, CardKind::Stealth, TerritoryType::Land, "Place 3 additional defending MODs in the defending land territory.", 0);
  add("Stealth Station", Commander::Land, 1, 0, CardTiming::React, CardKind::StealthStation, TerritoryType::Land, "Place a Space Station in the defending land territory.", 0);
  add("Assemble MODs", Commander::Naval, 3, 1, CardTiming::Before, CardKind::Assemble, TerritoryType::Water, "Place 3 MODs on any one water territory you occupy.", 0);
  add("Colony Influence", Commander::Naval, 2, 0, CardTiming::Score, CardKind::Influence, TerritoryType::Water, "If your Naval Commander is still alive, move your score marker ahead 3 spaces.", 0);
  add("Frequency Jam", Commander::Naval, 2, 0, CardTiming::Before, CardKind::Jam, TerritoryType::Water, "Choose a player. The chosen player cannot play command cards during your turn.", 0);
  add("Hidden Energy", Commander::Naval, 5, 0, CardTiming::Before, CardKind::HiddenEnergy, TerritoryType::Water, "Draw a water territory card. If you occupy it at the end of your turn, collect 4 energy.", 0);
  add("Reinforcements", Commander::Naval, 2, 0, CardTiming::Before, CardKind::Reinforce, TerritoryType::Water, "Place 3 MODs, one each on 3 different water territories you occupy.", 0);
  add("Stealth MODs", Commander::Naval, 5, 0, CardTiming::React, CardKind::Stealth, TerritoryType::Water, "Place 3 additional defending MODs in the defending water territory.", 0);
  add("Water Death Trap", Commander::Naval, 1, 3, CardTiming::React, CardKind::DeathTrap, TerritoryType::Water, "Your opponent must destroy half the units in the invading territory. Round up.", 0);
  add("Aqua Brother", Commander::Nuclear, 1, 3, CardTiming::Before, CardKind::Zone, TerritoryType::Water, "Roll a 6-sided die. Destroy one unit in each territory of the water colony rolled (1-5; 6 = roll again).", 0);
  add("Assassin Bomb", Commander::Nuclear, 3, 1, CardTiming::Before, CardKind::Assassin, TerritoryType::Land, "Choose an opponent's commander. Roll an 8-sided die: on a 3 or higher, destroy it.", 0);
  add("Armageddon", Commander::Nuclear, 1, 4, CardTiming::Before, CardKind::Armageddon, TerritoryType::Land, "Nuclear exchange: this turn your nuclear command cards cost no energy.", 0);
  add("The Mother", Commander::Nuclear, 1, 3, CardTiming::Before, CardKind::Zone, TerritoryType::Land, "Roll a 6-sided die. Destroy one unit in each territory of the continent rolled: 1 N.America, 2 S.America, 3 Europe, 4 Africa, 5 Asia, 6 Australia.", 0);
  add("Nicky Boy", Commander::Nuclear, 1, 3, CardTiming::Before, CardKind::Zone, TerritoryType::Moon, "Roll a 6-sided die. Destroy one unit in each territory of the lunar colony rolled: 1-2 Cresinion, 3-4 Delphot, 5-6 Sajon.", 0);
  add("Rocket Strike Land", Commander::Nuclear, 2, 2, CardTiming::Before, CardKind::Rocket, TerritoryType::Land, "Choose an opponent's land territory. Roll a 6-sided die; that many units are destroyed.", 0);
  add("Rocket Strike Moon", Commander::Nuclear, 2, 2, CardTiming::Before, CardKind::Rocket, TerritoryType::Moon, "Choose an opponent's lunar territory. Roll a 6-sided die; that many units are destroyed.", 0);
  add("Rocket Strike Water", Commander::Nuclear, 2, 2, CardTiming::Before, CardKind::Rocket, TerritoryType::Water, "Choose an opponent's water territory. Roll a 6-sided die; that many units are destroyed.", 0);
  add("Scatter Bomb Land", Commander::Nuclear, 3, 1, CardTiming::Before, CardKind::Scatter, TerritoryType::Land, "Turn over 3 land territory cards. Destroy half the opponents' units on those territories (round up).", 3);
  add("Scatter Bomb Moon", Commander::Nuclear, 2, 1, CardTiming::Before, CardKind::Scatter, TerritoryType::Moon, "Turn over 2 lunar territory cards. Destroy half the opponents' units on those territories (round up).", 2);
  add("Scatter Bomb Water", Commander::Nuclear, 2, 1, CardTiming::Before, CardKind::Scatter, TerritoryType::Water, "Turn over 2 water territory cards. Destroy half the opponents' units on those territories (round up).", 2);
  add("Assemble MODs", Commander::Space, 3, 1, CardTiming::Before, CardKind::Assemble, TerritoryType::Moon, "Place 3 MODs on any one lunar territory you control.", 0);
  add("Colony Influence", Commander::Space, 2, 0, CardTiming::Score, CardKind::Influence, TerritoryType::Moon, "If your Space Commander is still alive, move your score marker ahead 3 spaces.", 0);
  add("Energy Extraction", Commander::Space, 1, 1, CardTiming::Before, CardKind::Extraction, TerritoryType::Moon, "If you occupy every territory of a lunar colony at the end of this turn, collect 7 energy.", 0);
  add("Frequency Jam", Commander::Space, 2, 0, CardTiming::Before, CardKind::Jam, TerritoryType::Moon, "Choose a player. The chosen player cannot play command cards during your turn.", 0);
  add("Invade Earth", Commander::Space, 3, 0, CardTiming::Before, CardKind::InvadeEarth, TerritoryType::Moon, "Turn over land territory cards until one you do not occupy. This turn you may attack it from any lunar territory.", 0);
  add("Orbital Mines", Commander::Space, 2, 2, CardTiming::React, CardKind::DeathTrap, TerritoryType::Moon, "Your opponent must destroy half the units in the invading territory. Round up.", 0);
  add("Reinforcements", Commander::Space, 3, 0, CardTiming::Before, CardKind::Reinforce, TerritoryType::Moon, "Place 3 MODs, one each on 3 different lunar territories you occupy.", 0);
  add("Stealth MODs", Commander::Space, 4, 0, CardTiming::React, CardKind::Stealth, TerritoryType::Moon, "Place 3 additional defending MODs in the defending lunar territory.", 0);
  return e;
}
const std::vector<Entry>& Catalogue() { static const std::vector<Entry> c = BuildCatalogue(); return c; }
}  // namespace

const std::vector<CardDef>& CardCatalogue::All() {
  static const std::vector<CardDef> defs = [] {
    std::vector<CardDef> v;
    for (const auto& e : Catalogue()) v.push_back(e.def);
    return v;
  }();
  return defs;
}

std::vector<int> CardCatalogue::BuildDeck(Commander deck) {
  std::vector<int> d;
  for (const auto& e : Catalogue())
    if (e.def.deck == deck) for (int i = 0; i < e.copies; i++) d.push_back(e.def.id);
  return d;
}

}  // namespace risk2210
