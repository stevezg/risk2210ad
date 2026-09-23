#pragma once
#include <string>
#include <vector>

#include "Types.h"

namespace risk2210 {

struct CardDef {
  int id = -1;
  std::string name;
  Commander deck = Commander::Land;
  int cost = 0;
  CardTiming timing = CardTiming::Before;
  CardKind kind = CardKind::Reinforce;
  TerritoryType target = TerritoryType::Land;
  std::string text;
  int amount = 0;  // kind-specific (Scatter Bomb's territory-card count)
};

/// The five base-game command decks. Data is generated from index.html's CARD_DEFS --
/// that file is the corrected source of truth (every card ruling from this project lives
/// there); regenerate this file from it rather than hand-editing if the cards ever change.
class CardCatalogue {
 public:
  static const std::vector<CardDef>& All();
  static const CardDef& Get(int id) { return All().at(id); }
  static std::vector<int> BuildDeck(Commander deck);
};

}  // namespace risk2210
