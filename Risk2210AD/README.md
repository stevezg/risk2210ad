# Risk 2210 A.D. — Unity (macOS) port

Single-player digital adaptation of Risk 2210 A.D. against bot commanders, in the industrial
cyber-military style of the physical board. Unity 6 (6000.0 LTS), Built-in Render Pipeline,
UI Toolkit, C# 9. Native Apple Silicon standalone via the macOS player.

## Open & run

1. Unity Hub → **Add** → select this `Risk2210AD` folder (Unity 6000.0.x, any *Built-in Render
   Pipeline* template settings; URP is not used).
2. Wait for the import, then **Risk 2210 → Build Scenes** (menu bar). This writes
   `Assets/Scenes/MainMenu.unity` and `MainGame.unity` and registers them in Build Settings.
3. **Risk 2210 → Play from Main Menu**, or open `MainMenu.unity` and press Play.
   Pressing Play in an empty scene also works: `GameBootstrap` assembles the game at runtime.
4. Build: File → Build Settings → macOS → Apple Silicon.

If the project was created with the *Input System* package active, set
Edit → Project Settings → Player → Active Input Handling to **Both** (the board uses `Input`).

## Architecture

```
[ VIEW LAYER ]                                             [ UI LAYER ]
MapInputController ── hover/click ──► HudController (UI Toolkit: dashboards, hand, tooltips)
TacticalCamera / BoardView / TerritoryView                  │ GameCommand
AnimationDirector ◄── GameEvent stream ──┐                  ▼
CombatOverlay (3D dice)                  │        GameDirector.Submit()
                                         │                  │
[ CORE RULES ENGINE ]                    │                  ▼
GameState (graph + territories + players)└── PhaseState machine: Setup → YearInit →
MapGraph (TerritoryNode / BorderEdge / Region)   EnergyBidding → Deployment → Invasion →
CardCatalogue, Rules                             Fortification → (next player | YearInit) → GameOver
                                                            ▲
[ AI ]  AsyncBotRunner ── Task.Run(BotBrain.Decide(snapshot)) ┘
```

* **Core** (`Assets/Scripts/Core`) has no scene dependencies. `GameDirector` owns a `GameState`
  and the current `PhaseState`; the only way in is `Submit(GameCommand)`, the only way out is the
  ordered `GameEvent` stream (plus `CommandResult` errors). Mid-action decisions (defence dice,
  reactive cards, card targets, bonus deck) become `Prompt`s with continuations.
* **AI** (`Assets/Scripts/AI`) clones the state and decides one command per call on the thread
  pool, so bot turns never block rendering; commands are applied on the main thread once the
  animation queue is idle.
* **View** (`Assets/Scripts/View`) renders the graph as vector landforms (Voronoi cells clipped to
  organic radii, water platforms as discs) with `PolygonCollider2D` hit-boxes, neon border shader
  (`Assets/Materials/Resources`), grid background, bloom, transport orbs along borders, conquest
  shockwaves and colour fades, and the dice combat overlay.
* **UI** (`Assets/Scripts/UI`, `Assets/UI/Resources`) is UI Toolkit: year track, phase, player
  dashboards, continent/colony tallies, prompt + action bar, command hand with tooltips, log.

## Rules implemented

Setup (4 devastated lands, 35/30/25 MODs, claiming, starting station + Land Commander + Diplomat,
2-player neutral army), yearly sealed-bid turn-order auction, income (territories ÷ 3 min 3 +
continent/colony bonuses in MODs and energy, +1 MOD per Space Station), commanders (3E), Space
Stations (5E, max 4, land only), up to 4 cards/turn (1E, matching commander), command cards with
implemented effects, rulebook d8 table for commanders, Space Station d8 defence, Naval/Space
gating for water/Moon, Earth→Moon only via station→landing site, Invade Earth card, 3-territory
bonus, forced commander move-in, station capture, elimination, single fortify along friendly
chains, 5-year limit and final scoring with energy/unit tie-breaks.

## Roadmap status

| Phase | Item | Status |
|---|---|---|
| 1 | Asset pipeline (sliced textures) | Optional drop-in via `Resources/Textures/Territories`, see `Assets/Textures/README.md` |
| 1 | Vector landform hit-boxes | Done — `VectorLandforms` + `PolygonCollider2D` |
| 1 | Tactical camera | Done — clamped zoom, bounded pan, smooth focus, shake |
| 2 | Graph network | Done — `MapGraph` (nodes, typed edges, wrap links, landing sites) |
| 2 | Modular state machine | Done — `Core/States/*` |
| 3 | Highlight shaders | Done — `NeonTerritory.shader` hover/pulse, `Grid.shader`, `Bloom.shader` |
| 3 | UI layout | Done — UI Toolkit HUD mirroring the board's side panels |
| 4 | Interpolated force movement | Done — `AnimationDirector.Orb` |
| 4 | Combat overlay | Done — `CombatOverlay` 3D dice, snap-to-roll, loss flashes, screenshake |
| 4 | Conquest feedback | Done — shockwave rings + owner colour fade |

The C++ engine in the parent folder is the reference implementation this port was derived from
and remains useful for fast rule fuzzing (`risk2210_tests`).
