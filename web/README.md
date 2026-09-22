# Risk 2210 A.D. — browser edition

Single-file, dependency-free adaptation (`index.html`, vanilla ES6) played over a photo of the
physical board. You are RED against the BLUE bot; a neutral GRAY army holds the remaining
territories (official 2-player rules). Open `index.html` in Safari or Chrome — a local server
is only needed if your browser blocks font loading from `file://`:

```sh
cd web && python3 -m http.server 8765   # then open http://localhost:8765/
```

* `board_photo.jpg` — the board, cropped to its edges; territory nodes are placed with CSS
  percentages so they stay locked to the artwork at any window size (`TDEF` in the script).
* `gunship.ttf` — Gunship by Iconian Fonts (free for non-commercial use).
* Lunar colonies live in an inset over the ocean (dashed nodes are the landing sites).

Implements the 2001 gameplay manual and FAQ: sealed-bid turn order, income in MODs and energy,
commanders with the d8 table, Space Stations (d8 defence, launches to the Moon), water/Moon
gating, all five base-game command decks with their effects, 3-territory bonus, elimination,
fortify chains through stations and landing sites, five-year limit and final scoring with
Colony Influence (+3) and energy/unit tie-breaks. Armageddon is simplified to "your nuclear
cards are free this turn".
