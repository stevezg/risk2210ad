# Risk 2210 A.D. — C++ game engine

A rules engine for *Risk 2210 A.D.* (Avalon Hill, 2001) written in C++17 with no
external dependencies. It ships with a simple heuristic AI, an interactive
text-mode player, and a self-play test suite.

## Build & run

```sh
cmake -S . -B build
cmake --build build -j
./build/risk2210_tests                     # engine invariants + 160 self-play games
./build/risk2210_cli --players 4 --seed 42 # watch four AIs play
./build/risk2210_cli --players 3 --human 0 # you are Red, two AIs
```

## Layout

| File | Purpose |
|------|---------|
| `include/risk2210/Types.h` | enums, constants (`kCommanderCost`, `kNumYears`, ...) and `Result` |
| `include/risk2210/Map.h`, `src/Map.cpp` | the board: 42 land, 13 water, 14 lunar territories, 14 regions, adjacency |
| `include/risk2210/Cards.h`, `src/Cards.cpp` | command card catalogue and per-commander deck composition |
| `include/risk2210/Game.h`, `src/Game.cpp` | the rules engine: setup, bidding, turn phases, combat, cards, scoring |
| `include/risk2210/Agent.h` | decision-maker interface the engine calls into |
| `include/risk2210/Agents.h`, `src/Agents.cpp` | `RandomAgent` (AI) and `HumanCliAgent` (terminal) |
| `src/main.cpp` | CLI driver |
| `tests/tests.cpp` | map checks, rule checks, self-play invariant fuzzing |

## Rules implemented

* Setup: 4 random devastated land territories, 35/30/25 starting MODs for 3/4/5
  players, territory claiming, starting Space Station + Land Commander + Diplomat,
  3 starting energy. 2-player games use the official neutral-army variant.
* Each year: sealed-bid energy auction for turn order (ties broken by die roll).
* Turn: collect MODs and energy (territories ÷ 3, min 3, plus continent/colony
  bonuses; +1 MOD per Space Station) → deploy → hire commanders (3E) / build
  stations (5E, max 4, land only) / buy up to 4 cards (1E, matching commander
  required) → play cards → invade → fortify once along a friendly path.
* Combat: 1–3 attack dice vs 1–2 defence dice; ties to the defender. Attacking
  commanders swap in 8-sided dice per the rulebook table (Nuclear always, Land
  for land, Naval for water, Space for lunar, Diplomat never); every defending
  commander swaps in a d8; all units in a Space Station defend with d8s.
  Commanders that rolled d8s must move into a captured territory. Captured
  Space Stations change hands (or are destroyed if the attacker already has 4).
* Water needs a Naval Commander in play; the Moon needs a Space Commander,
  is entered only from a Space Station to a landing site (Sea of Crisis, Bay
  of Dew, Tycho), and Earth can only be invaded from the Moon via *Invade Earth*.
* 3-territory bonus (once per turn): +1 energy and a command card.
* Player elimination, empty-territory occupation, "must attack once" rule.
* Command cards with engine-implemented effects: Reinforcements, Assemble
  MODs, Stealth MODs (reactive), Cease Fire (reactive), Energy Crisis,
  Redeployment, Energy Extraction, Scatter Bombs, The Mother, Armageddon,
  Invade Earth, Scout Forces, and Colony Influence scoring cards.
* Final scoring after year 5: territories + bonuses + influence cards, with
  energy and then unit count as tie-breakers.

## Known approximations

* Lunar adjacency and the lunar colony bonus values (Cresinion 2,
  Delphot 2, Sajon 3), are reconstructed rather than
  transcribed; all of it lives in `src/Map.cpp` and is easy to correct.
* The command card catalogue is a representative subset with the same flavour
  as each official deck, not a card-for-card transcription. Add cards in
  `src/Cards.cpp` and implement new `CardKind`s in `Game::applyCard`.
