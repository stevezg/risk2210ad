# Risk 2210 A.D. — browser edition

A complete, single-file digital adaptation of the 2001 Avalon Hill board game, played over a
photo of the physical board. No frameworks, no build step, no dependencies: open `index.html`
in Safari or Chrome and play.

![the board](board_photo.jpg)

You play RED against the BLUE machine intelligence; a neutral GRAY army holds the ground
between you (the official 2-player rules). A local server is only needed if your browser blocks
font loading from `file://`:

```sh
python3 -m http.server 8765   # then open http://localhost:8765/
```

## Playing

Bid energy secretly for turn order, then recruit MODs, hire commanders, build Space Stations
and buy command cards, invade, and fortify — highest score after the last year wins. Click a
territory to select it; legal targets light up. Cards in your hand glow when they can be played,
and hovering one explains what it does and why. The comms log under the board narrates every roll.

Pick how many years to play on the title screen (3/5/7/10, default 5 — the official length).
When the game reaches that limit you get a checkpoint: current standings, and a choice to stop
for final scoring or keep playing 1/3/5 more years. You can keep extending indefinitely.

* `board_photo.jpg` / `moon_photo.jpg` — the Earth board and the lunar board. Territory nodes are
  placed with CSS percentages so they stay locked to the artwork at any window size (`TDEF`).
* The **EARTH / MOON** buttons (top-left of the map) switch boards; the button for the board you
  are not looking at shows a badge counting the highlighted territories waiting there. Landing
  sites (Sea of Crisis, Bay of Dew, Tycho) carry a gold ring.
* `cover.jpg` — box art on the title screen. `gunship.ttf` — Gunship by Iconian Fonts
  (free for non-commercial use).

## Map data

Adjacency is transcribed from the official board and lunar schematics (Wikimedia Commons),
extracted geometrically rather than by eye: each connector path was walked with the SVG
`getPointAtLength` API and split at every territory circle it passes through. Notable
corrections over a by-eye reading of the board photo: **Nova Brasilia connects to Amazon
Desert** (not Nuevo Timoto, and not to Saharan Empire), **Poseidon connects to Continental
Biospheres** (not the Northwestern Oil Emirate), **Sung Tzu connects to New Guinea** (not Java
Cartel), Hawaiian Preserve has no link to Mexico, and Saharan Empire borders Imperial Balkania
as well as Andorra. The two Pacific links that run off the edges of the board wrap around:
Northwestern Oil Emirate–Pevek and **New Atlantis–Neo Tokyo** (confirmed against the connector
lines running off both edges of the physical board photo — not Hawaiian Preserve, which has no
wrap link at all). The Moon uses all 30 edges of the lunar schematic, and Earth↔Moon travel is
only possible through the three lunar landing sites (Sea of Crisis, Bay of Dew, Tycho), each
reachable solely from a Space Station on Earth.

Implements the 2001 gameplay manual and FAQ: sealed-bid turn order, income in MODs and energy,
commanders with the d8 table, Space Stations (d8 defence, launches to the Moon), water/Moon
gating, all five base-game command decks with their effects, 3-territory bonus, elimination,
fortify chains through stations and landing sites, a configurable/extendable game length (see
above) and final scoring with Colony Influence (+3) and energy/unit tie-breaks. Armageddon is
simplified to "your nuclear cards are free this turn".

## Architecture

Short version: it's all in `index.html` — vanilla JS, no framework, no build step. For the long
version — what data structures and algorithms run the game, whether we need a state-management
library, how this could become multiplayer, whether the old C++ engine could come back as a
WASM core under a Svelte/React/Vue frontend, and what "optimize this" actually means for a page
this size — see **[ARCHITECTURE.md](ARCHITECTURE.md)**.

## Files

| File | |
|---|---|
| `index.html` | the entire game — layout, styles, rules engine, bot and UI |
| `board_photo.jpg` | the Earth board |
| `moon_photo.jpg` | the lunar board |
| `cover.jpg` | box art for the title screen |
| `gunship.ttf` | the display font (see `LICENSE-gunship.txt`) |

## Credits

Risk 2210 A.D. is © 2001 Avalon Hill / Hasbro. This is a personal, non-commercial adaptation
built from the published gameplay manual, FAQ and command card summary. Gunship font by
Daniel Zadorozny (Iconian Fonts), free for non-commercial use.
