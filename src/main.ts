import "./style.css";
import { $, buildBoard, ensureYearTrack, render } from "./render";
import { newGame, ui } from "./state";
import { runGame } from "./turn-flow";

buildBoard();

function buildYearPicker() {
  const box = $("year-picker"); box.innerHTML = "";
  for (const n of [3, 5, 7, 10]) {
    const b = document.createElement("button"); b.className = "small" + (n === ui.yearLimit ? " toggled" : "");
    b.textContent = String(n); b.addEventListener("click", () => { ui.yearLimit = n; buildYearPicker(); });
    box.appendChild(b);
  }
}
buildYearPicker();

function start(spectate: boolean) {
  newGame(spectate); $("menu").style.display = "none";
  ensureYearTrack();
  const sp = $("actions2"); sp.innerHTML = "";
  const b = document.createElement("button"); b.textContent = "Bot speed: normal"; b.className = "small";
  b.addEventListener("click", () => { ui.botDelay = ui.botDelay > 100 ? 40 : 350; b.textContent = ui.botDelay > 100 ? "Bot speed: normal" : "Bot speed: fast"; }); sp.appendChild(b);
  render(); runGame();
}
$("btn-start").addEventListener("click", () => start(false));
$("btn-spectate").addEventListener("click", () => start(true));

// Dev/debug console access: plain function declarations no longer auto-attach to `window`
// once they're ES module members, so this hook deliberately replaces that -- it's what the
// bot-vs-bot console-harness testing technique used throughout this project's history relies on.
// Namespace objects (not spread copies) are exposed so property access stays live -- e.g.
// `debug.state.S` still reflects the current game after `debug.state.newGame(true)` reassigns it:
// `debug.state.newGame(true); debug.turnFlow.runGame();`
import * as stateModule from "./state";
import * as renderModule from "./render";
import * as turnFlowModule from "./turn-flow";
(window as any).debug = { state: stateModule, render: renderModule, turnFlow: turnFlowModule, start };
