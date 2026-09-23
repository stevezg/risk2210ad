# Architecture, data structures & algorithms

**Status: Phase 1 of the multiplayer rewrite is done.** `server/engine/` is a real, tested C++17
rules engine for 2-5 players (the browser game below is still 2-player-only) -- see
["Multiplayer rewrite: status"](#multiplayer-rewrite-status) near the end of this doc for what's
built, what's next, and how to run it. Everything below this point describes the browser game as
it stands today; the rewrite reuses its rules and data wholesale (see that section for exactly
how).

This doc answers the questions that came up building this: what the stack actually is, whether
the old C++ engine still exists anywhere, what data structures and algorithms the game runs on,
whether it needs a state-management library, how it could become multiplayer, and what a
C++-core/JS-view hybrid would look like if we ever want one.

## 1. Current stack: what's actually here

**Everything is in `index.html`.** ~1,100 lines of HTML + CSS + vanilla ES6 JavaScript, no build
step, no bundler, no npm, no framework. Open the file (or serve it with any static file server)
and it runs. That's the entire tech stack.

There is **no C++ anywhere in this repo anymore.** Earlier in this project's life there were two
other implementations, both deleted in commit `b7104ba` at the user's request, once the browser
version was clearly the one we wanted to keep:

- A C++17 rules engine (`src/`, `include/`) with a `raylib` + Dear ImGui GUI — commit `edcb5db`
  through `c77952d`.
- A Unity/C# port of that same engine (`Risk2210AD/`) — commit `3d1941b` through `99dc9be`.

Neither was "rewritten into" the HTML version — the HTML/JS engine is an **independent, from-scratch
implementation** of the same ruleset, not a transpilation or port of the C++ one. It happens to be
architecturally similar (same graph model, same turn-phase structure) because it's implementing the
same board game, not because code was carried over.

Nothing is lost — git history is forever. The last commit with everything intact and building is
`a57d6bc`. To resurrect the C++ engine into a subfolder without disturbing the web game:

```sh
git checkout a57d6bc -- src include tests CMakeLists.txt
mkdir engine-cpp && git mv src include tests CMakeLists.txt engine-cpp/
```

(and similarly `git checkout a57d6bc -- Risk2210AD` for the Unity port).

## 2. "Can we use the C++ engine as the core with a Vue/React/Svelte frontend?"

Yes — this is a well-trodden pattern: **compile the C++ rules engine to WebAssembly with
Emscripten, and drive it from a reactive JS frontend.** Concretely:

```
┌─────────────────────────┐       ┌──────────────────────────────┐
│  Svelte / React / Vue    │  JS  │  risk2210.wasm                 │
│  components (board,      │◄────►│  (the C++ GameDirector,        │
│  HUD, hand, modals)      │ calls │   compiled via Emscripten +    │
│  subscribe to a store     │       │   embind bindings)             │
└─────────────────────────┘       └──────────────────────────────┘
```

Rough shape of the work:
1. Wrap the C++ `GameDirector` (or the equivalent state+rules module) with
   [`embind`](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html) so its
   public methods (`submit(command)`, `state()`, event callbacks) are callable from JS.
2. `emcc` compiles it to a `.wasm` + a small `.js` glue loader.
3. The frontend framework owns **only presentation and input capture** — it calls into the WASM
   module for every rule decision and renders whatever state/event the module reports back. No
   game logic lives in JS at that point.
4. Pick Svelte over React/Vue here if you go this route: it compiles away at build time (no
   virtual-DOM diffing framework shipped to the client), which pairs naturally with "the real
   logic already lives in a compiled WASM binary, don't add a second heavy runtime on top."

**Should we, though?** Honest tradeoffs:

- **Performance is not the reason to do this.** A Risk turn — even the heaviest bot turn, dozens of
  dice rolls and a graph search — is microseconds of work. WASM buys nothing here that the current
  JS doesn't already do fast enough (see [§6](#6-actual-performance-notes)).
- **The real reasons to do it would be:** (a) you want one engine shared across multiple frontends
  (web + a future native/Unity client) instead of maintaining parallel rule implementations, or
  (b) you want C++'s stronger type system and a real unit-test harness (GoogleTest/Catch2) around
  the rules, which is harder to enforce in loose JS.
- **The cost:** a real build pipeline (Emscripten SDK, a bundler for the frontend framework, embind
  boundary code to maintain), versus today's "open one HTML file in a browser." That's a
  meaningful jump in operational complexity for a project whose whole current appeal is
  zero-install, zero-build simplicity.

**Recommendation:** don't do this yet. If/when this game grows a second client (native app,
Unity, a dedicated mobile shell) *or* the ruleset grows complex enough that JS's lack of types
becomes a real correctness risk, that's the trigger to revisit. Until then, the WASM boundary is
solving a problem we don't have. If you want to go ahead anyway, say so and I'll scope it as its
own workstream (recovering the C++ engine per §1, standing up Emscripten, picking Svelte, wiring
embind) rather than folding it into the current file.

## 3. State management: do we need one?

**Not a library, no** — but the state model is worth being explicit about, because it's the thing
that makes multiplayer (§4) and any future undo/replay feature easy or hard.

Today: one mutable global object, `S` (see `newGame()`), holding the whole game — territories,
players, decks, the in-progress invasion, everything. Every rule function (`attack()`,
`declareInvasion()`, card effects, …) mutates `S` directly and then calls `render()`, which reads
`S` fresh and updates the DOM. There's no reducer, no action log, no framework — it's the simplest
thing that works, and at "one page, one game, one browser tab" scale that's a deliberate choice,
not a shortcut: React/Redux/Zustand/Pinia would add a build step and a dependency for a
problem — "keep 69 territories and 2 players' worth of state in sync with the DOM" — that a plain
object and a handful of `render()` calls already solve.

**Where it would start to matter:**
- **Undo / replay / spectator scrubbing** — currently the comms log is descriptive text, not
  data; you can't reconstruct board state at turn N from it.
- **Multiplayer** — a server needs to validate and broadcast *actions*, not push whole-object
  snapshots around forever (see §4).
- **Testing** — there's no way right now to assert "after these 5 moves, X territory has 3 MODs"
  without driving the actual UI.

All three of those point at the same fix, and it's the one concrete structural change I'd
recommend: **turn the implicit mutations into an explicit, append-only action log.**

```js
// instead of scattering `T(n).mods++` calls through 20 functions, every mutation becomes:
function apply(action) {                 // pure: (state, action) -> mutates state in place
  switch (action.type) {
    case "deploy": T(action.to).mods += action.n; break;
    case "attack": /* dice roll IS non-deterministic input, see §5 */ break;
    // ...
  }
  actionLog.push(action);                // <- this is the part we don't have today
}
```

That single list (`actionLog`) is simultaneously: your undo stack (replay from action 0 to N-1),
your multiplayer sync payload (server broadcasts `action`, not `state`), and your test fixture
(assert on state after replaying a fixed action list). It's a small refactor — the rule logic
barely changes, it's the same `T(n).mods += k` code, just funneled through one dispatcher instead
of called ad hoc — and it's the thing that unlocks everything else. I'd do this *before* touching
multiplayer or a frontend framework; it's valuable independent of either.

## 4. Making this multiplayer

The architecture problem today: game logic and UI are the same code. `attack()` both *decides the
outcome* and *waits on `userInput()` for a human click*. That's fine single-machine, but a
multiplayer server can't "wait on a click" — it has to accept a message, validate it against rules
the *server* trusts, and broadcast the result.

**Shape of the fix**, building on §3's action log:

1. **Split "rules" from "input."** The action-log `apply(action)` function becomes the one and
   only place rules are enforced. It needs to become side-effect-free of the DOM (it mostly
   already is — `T()`, `units()`, combat resolution don't touch the page).
2. **Server is authoritative.** A small Node.js process (or a Cloudflare Worker / Deno Deploy —
   doesn't need to be heavy) holds the canonical `S` and `actionLog`. Clients send *proposed*
   actions (`{type: "attack", dice: 2}`); the server calls the same `apply()`, and if it doesn't
   throw / isn't rejected by validation, broadcasts the resulting action + diff to all connected
   clients over WebSockets.
3. **Hide hidden information per-connection.** Right now `S.players[p].hand` and secret bids are
   just sitting in the object a spectator's console can read (`ui.spectate` merely *hides them in
   the UI*, see `renderHand()` — it's a presentation-layer fig leaf, not real hiding). A real
   multiplayer server needs to send each client a **filtered view**: your own hand in full,
   opponents' hand *sizes* only, bids withheld until reveal. That's a per-connection projection
   function over `S`, not a change to `S` itself.
4. **Randomness moves server-side.** Dice rolls, territory-card draws, shuffles — all currently
   `Math.random()` calls scattered through client code (§5). In multiplayer these must happen on
   the server (or with a commit-reveal scheme if you want provable fairness) so a modified client
   can't lie about its roll.
5. **Reconnection is "replay the action log."** A client that drops and reconnects just requests
   the full action log (or a periodic snapshot + the tail) and reconstructs `S` by replaying —
   free, once §3 exists.

None of this requires a frontend framework or WASM — it's a Node WebSocket server plus a thin
network client module that replaces `userInput()`/`choose()`'s "wait for a DOM click" with "wait
for a server message," while the rule functions themselves barely change. This is meaningfully
less work than the WASM/Svelte rewrite in §2, and is the more natural "next big feature" for this
project if that's a direction you want — happy to scope it properly (message protocol, server
framework choice, matchmaking/lobby) as its own task when you're ready.

## 5. Data structures & algorithms

A tour of what's actually doing work, since this was asked for directly:

| Concern | Structure / algorithm | Where |
|---|---|---|
| The board | **Graph, adjacency-list representation.** `MAP` is a dict of 69 nodes (42 land + 13 water + 14 lunar); each node carries an `adj: string[]` neighbor list. `LINKS` is the edge list the graph is built from. | `MAP`, `LINKS` |
| Continents/colonies | Each node tags a `region`; `REGIONS` is a small lookup table of bonus values. Control-checking (`controls(p, region)`) is a linear scan of that region's members — cheap, ≤12 nodes. | `REGIONS`, `controls()` |
| Fortify-path legality | **BFS (breadth-first search)** over the territory graph, restricted to nodes the player owns, with two synthetic edges folded in per BFS step (any friendly Space Station ↔ any friendly lunar landing site) so "Earth → Moon → Earth" routes are reachable without a literal edge for every station/site pair. Unweighted graph, so BFS gives shortest-path reachability in O(V+E). | `fortifyPath()` |
| Turn/phase sequencing | **A coroutine-style state machine built on `async`/`await`**, not an explicit finite-state-machine table. Each phase is an `async function` (`bidding()`, `humanRecruit()`, `humanAttack()`, `botTurn()`, …) that either runs bot logic straight through or `await`s a `Promise` that a DOM click / modal button resolves (`userInput()`, `choose()`, `modal()`). This is continuation-passing in disguise — the "state machine" is really just "where the `await` currently is." It reads linearly (a big win for a solo dev maintaining rules text-to-code), at the cost of the whole game literally not existing as inspectable data (§3 is the fix if that becomes a problem). | `runGame()`, `takeTurn()`, `userInput()` |
| Combat resolution | Classic Risk dice algorithm: roll N attacker / M defender dice, **sort each descending, compare pairwise** (highest vs highest, etc.), defender wins ties. Commander d8 upgrades are just "which of these N rolls uses a d8 instead of a d6," decided before rolling (`attackD8()`). | `attack()` |
| Command/territory decks | Array-based draw pile + **Fisher–Yates shuffle** (`shuffle()`), reshuffled from the discard when exhausted (`drawTerr()`, `drawCard()`) — mirrors the physical game's card-deck mechanic exactly (draw from top, reshuffle when empty). | `shuffle()`, `drawTerr()`, `drawCard()` |
| Bot decisions | **Greedy heuristic scoring**, not search/minimax. E.g. invasion targets are scored (`attacker units − defender units`, bonus for hitting the human) and the bot picks randomly among its **top 3** rather than always the single best — cheap, and the randomness keeps it from being perfectly predictable. No lookahead, no game-tree search; Risk's branching factor (dozens of legal moves × dozens of territories × combat's own randomness) makes even shallow minimax expensive for very little payoff against a casual opponent. | `botTurn()` |
| Highlight state | **Sets** (`HL.option`, `HL.target`, `HL.fortify`) for O(1) "is this territory highlighted?" checks when re-painting all 69 DOM nodes every render. | `HL`, `applyHighlights()` |
| Scoring / tie-breaks | Multi-key comparator sort: score, then energy, then unit count — standard lexicographic tie-break, exactly per the rulebook's stated tie order. | `finalScoring()` |
| Randomness source | Plain `Math.random()` (`rnd()`, `die()`) — **not seeded.** Every game is different and replay/regression-testing a specific game isn't currently possible. A seeded PRNG (mulberry32 or similar, ~4 lines) is a cheap upgrade if reproducible games ever matter (bug reports, "replay this exact game"). | `rnd()`, `die()` |

## 6. Actual performance notes

"Optimize this" is a reasonable ask, so here's where time actually goes, rather than guessing:

- **Already done:** the player-roster, continent-control, and hand panels rebuild their DOM only
  when a cheap signature string (`ui.psig`/`ui.rsig`/`ui.hsig`, built from the handful of fields
  that actually affect what's rendered) changes, instead of re-stringifying + re-parsing HTML on
  every `render()` call. This mattered because `render()` fires dozens of times per bot turn.
- **Territory nodes are updated in place, not rebuilt** — `render()` writes into existing
  `<div>`s' `.textContent`/`className` rather than recreating 69 DOM elements every frame.
- **What's *not* a bottleneck, deliberately:** `owned(p)`, `countUnits(p)`, `regionBonus(p)` all
  do an O(69) scan over `NAMES` every call, and they're called a lot. At 69 territories that's tens
  of microseconds — not worth the bug-surface of hand-maintained incremental counters. If the
  board ever grew 100x this would be worth revisiting; it isn't today.
- **The pacing you *feel* (bot turns "taking a while") is intentional**, not a performance problem
  — `ui.botDelay` / `sleep()` calls exist so a spectator can follow the action, with a fast-forward
  toggle already in the HUD.
- **A genuinely free win if `render()` calls ever multiply further:** batch them through
  `requestAnimationFrame` (coalesce N mutations between frames into 1 paint) instead of calling
  `render()` synchronously after every single state change. Not needed today, easy to add later.
- **The only slowness anyone will actually hit:** Chrome throttles `setTimeout` in background
  (unfocused/hidden) tabs by 10-100x. That's a browser policy, not this app — it only shows up
  when spectating in a backgrounded tab, which is exactly the scenario browser automation testing
  hits and a real player at their desk won't.


## Multiplayer rewrite: status

### What's built (Phase 1)

`server/engine/` is a standalone, networking-free C++17 static library implementing the full
ruleset for **2-5 players** -- the official player count, not just the browser game's fixed
Red-vs-Blue-plus-neutral. It was built by mechanically porting `index.html`'s data and rules
(not the older, now-superseded C++/Unity engines from this project's history) and generalizing
every place the browser version hardcoded `[0, 1]` into a loop over `numRealPlayers`.

| File | Contents |
|---|---|
| `Types.h` | Enums (`TerritoryType`, `Commander`, `Phase`, `CardKind`, `CardTiming`) and `Result` |
| `MapGraph.h` / `.cpp` | The board graph. `.cpp` is **generated** from `index.html`'s `MAP`/`LINKS`/`REGIONS` -- regenerate it from there if the board ever changes; don't hand-edit adjacency |
| `Cards.h` / `.cpp` | The 5 command decks. `.cpp` is likewise **generated** from `index.html`'s `CARD_DEFS` |
| `GameState.h` | Per-territory/per-player state, `Prompt` (a pending decision with a `std::function` continuation), all generalized to N players |
| `Commands.h` / `GameEvents.h` | The action/event vocabulary a client or bot speaks |
| `GameDirector.h` / `.cpp` | The engine: setup, bidding, deployment, invasion, fortification, all 23 card-effect implementations, elimination/conquest, final scoring |
| `tests/self_play_test.cpp` | A minimal (not yet "smart" -- that's Phase 2) bot plus a headless fuzz harness |

Verified: **2,000 self-play games (500 seeds x each of 2/3/4/5 players), 1.3M+ commands, 0
invariant violations**, running in under 1.5 seconds total. Also spot-checked directly: the
Sung Tzu-Java Cartel link, the New Atlantis-Neo Tokyo Pacific wrap, Poseidon-Continental
Biospheres, and Nova Brasilia-Amazon Desert -- the corrected adjacency from this project's map
fixes -- are all present and exercised in the ported graph.

One deliberate deviation from the original phase-by-phase file layout floated for this rewrite:
turn-phase logic (setup/bidding/deployment/invasion/fortification) lives as clearly-separated
methods directly on `GameDirector` rather than as a polymorphic `PhaseState` class hierarchy in
separate files. For a single-maintainer engine this was simpler to get right in one pass and
easier for the self-play harness to drive directly; splitting it out later, if it ever earns its
keep, doesn't change any of the rules logic itself.

A real bug the fuzz test caught immediately: the "Reinforcements" card's prompt continuation
captured its enclosing function's stack locals **by reference** (`[&]`) so it could recurse,
but `Prompt` continuations are called *after* the function that created them has already
returned -- so those references were dangling by the time a player answered the prompt,
crashing with `std::bad_function_call`. Fixed by heap-allocating the continuation's state
(`shared_ptr`), the same pattern the "Decoys Revealed" card already used correctly.

### What's next (Phases 2-5, not started)

See the "Roadmap after this session" section of the original planning doc for the agreed shape:
AI personalities + difficulty (Phase 2), the WebSocket server + protocol (Phase 3), turning
`index.html` into a thin network client while keeping its look pixel-for-pixel identical
(Phase 4), and an actual two-machine playtest (Phase 5).
