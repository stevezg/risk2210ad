import { CARD_DEFS, CMD_INITIAL, CMDS, MAP, NAMES, REGIONS, WHEN_TEXT } from "./data";
import {
  S, T, P, units, owned, hasCmd, controls, regionBonus, score, stations, cardCost, reactivePlayable, ui,
} from "./state";
import type { Button, Highlights } from "./types";
import { emit } from "./ui-events";

export const $ = (id: string) => document.getElementById(id)!;

export function log(msg: string, cls?: string) {
  const el = document.getElementById("log")!, d = document.createElement("div");
  if (cls) d.className = cls;
  d.textContent = msg; el.appendChild(d);
  while (el.childElementCount > 120) el.removeChild(el.firstChild!);
  el.scrollTop = el.scrollHeight;
}

export function buildBoard() {
  const earth = $("earth-wrap"), moon = $("moon-wrap");
  for (const n of NAMES) {
    const m = MAP[n];
    const el = document.createElement("div");
    el.className = "node"; el.id = "n-" + n; el.style.left = m.x + "%"; el.style.top = m.y + "%";
    el.innerHTML = `<span class="num"></span><span class="badges"></span>`;
    if (m.type === "moon") { if (m.site) el.classList.add("site"); moon.appendChild(el); } else earth.appendChild(el);
    el.addEventListener("click", () => emit({ type: "terr", name: n }));
    el.addEventListener("mouseenter", (e) => showTip(n, e));
    el.addEventListener("mousemove", (e) => moveTip(e));
    el.addEventListener("mouseleave", hideTip);
  }
  ensureYearTrack();
  $("view-earth").addEventListener("click", () => setView("earth"));
  $("view-moon").addEventListener("click", () => setView("moon"));
}

/** Switches the map between the Earth board and the lunar board. */
export function setView(v: "earth" | "moon") {
  ui.view = v;
  $("earth-layer").classList.toggle("on", v === "earth");
  $("moon-layer").classList.toggle("on", v === "moon");
  $("view-earth").classList.toggle("on", v === "earth");
  $("view-moon").classList.toggle("on", v === "moon");
  updateBadge();
}

/** Badges the map you are NOT looking at with the number of highlights waiting there. */
export function updateBadge() {
  let moon = 0, earth = 0;
  for (const set of [HL.option, HL.target, HL.fortify])
    for (const t of set) (MAP[t].type === "moon" ? moon++ : earth++);
  for (const [id, n, show] of [["moon-badge", moon, ui.view === "earth"], ["earth-badge", earth, ui.view === "moon"]] as const) {
    const b = $(id);
    b.textContent = String(n);
    b.style.display = (show && n) ? "block" : "none";
    b.title = n + " highlighted territories on that map";
  }
}

export function ensureYearTrack() {
  const yrs = $("years"), want = S ? Math.max(S.year, S.yearLimit, 5) : (ui.yearLimit || 5);
  while (yrs.children.length < want) { const d = document.createElement("div"); d.textContent = "'" + String(2205 + yrs.children.length + 1).slice(-2); yrs.appendChild(d); }
}

export function render() {
  if (!S) return;
  for (const n of NAMES) {
    const t = T(n), el = $("n-" + n);
    el.className = "node" + (MAP[n].site ? " site" : "");
    if (t.dev) el.classList.add("dev");
    else if (t.owner >= 0) el.classList.add("p" + t.owner);
    el.querySelector(".num")!.textContent = t.dev || t.owner < 0 ? "" : String(units(n));
    el.querySelector(".badges")!.textContent = Object.keys(t.cmd).map((c) => CMD_INITIAL[c as keyof typeof CMD_INITIAL]).join("");
    let st = el.querySelector(".station");
    if (t.station && !st) { st = document.createElement("span"); st.className = "station"; el.appendChild(st); }
    if (!t.station && st) st.remove();
  }
  applyHighlights();
  updateBadge();
  // meta
  [...$("years").children].forEach((d, i) => { d.className = i + 1 === S!.year ? "active" : i + 1 < S!.year ? "done" : ""; });
  $("year").textContent = S.year ? `${S.year} / 5  (${2205 + S.year} A.D.)` : "-";
  $("turn").textContent = S.cur >= 0 ? P(S.cur).name : "-";
  $("phase").textContent = S.phase;
  // players (rebuilt only when something they show has changed)
  const psig = S.players.map((p) => [p.energy, p.pool, p.hand.length, owned(p.id).length, score(p.id), stations(p.id), p.elim, CMDS.filter((c) => hasCmd(p.id, c)).join("")].join(",")).join("|") + "#" + S.cur;
  if (psig !== ui.psig) {
    ui.psig = psig;
    $("players").innerHTML = S.players.map((p) => `<div class="player ${S!.cur === p.id ? "active" : ""}" style="border-color:${p.color}">
    <div class="name" style="color:${p.color}">${p.name}${p.bot ? (p.neutral ? " · NEUTRAL" : " · BOT") : " · YOU"}${p.elim ? " · ELIMINATED" : ""}</div>
    <div class="stat">${p.neutral ? `TERRITORIES ${owned(p.id).length}` : `ENERGY ${p.bot && !ui.spectate && !p.neutral ? "?" : p.energy} · MODS ${p.pool}<br>TERR ${owned(p.id).length} · SCORE ${score(p.id)}<br>STATIONS ${stations(p.id)} · CARDS ${p.hand.length}<br>CMD ${CMDS.filter((c) => hasCmd(p.id, c)).map((c) => CMD_INITIAL[c]).join(" ") || "-"}`}</div></div>`).join("");
  }
  const rsig = Object.keys(REGIONS).map((k) => [0, 1, 2].find((p) => controls(p, k)) ?? -1).join(",");
  if (rsig !== ui.rsig) {
    ui.rsig = rsig;
    $("regions").innerHTML = Object.entries(REGIONS).map(([k, r]) => { const o = [0, 1, 2].find((p) => controls(p, k)); return `<div><span>${r.name.replace("North America", "N. AMERICA").replace("South America", "S. AMERICA").toUpperCase()} +${r.bonus}</span><span style="color:${o === undefined ? "#556" : P(o).color}">${o === undefined ? "-" : P(o).name}</span></div>`; }).join("");
  }
  renderHand();
}

export function renderHand() {
  const me = ui.spectate ? -1 : 0, el = $("hand");
  const hsig = me < 0 ? "spectate" : [P(me).hand.join(","), ui.mode, S!.cur, S!.phase, P(me).energy, P(me).invaded, P(me).pool].join("|");
  if (hsig === ui.hsig) return;
  ui.hsig = hsig;
  if (me < 0) { el.innerHTML = `<div style="font-size:10px;color:var(--dim)">Spectating - hands are hidden.</div>`; return; }
  const p = P(me);
  const reactive = ui.mode === "react";
  const canBefore = S!.cur === me && !p.invaded && (ui.mode === "recruit" && p.pool === 0 || ui.mode === "attack");
  const canEnd = S!.cur === me && ui.mode === "fortify";
  el.innerHTML = p.hand.map((id, i) => {
    const c = CARD_DEFS[id];
    const usable = reactive ? reactivePlayable(me, c) : ((canBefore && c.when === "before") || (canEnd && c.when === "end")) && hasCmd(me, c.deck) && p.energy >= cardCost(me, c);
    const why = usable ? "Playable now." : !hasCmd(me, c.deck) ? `Needs your ${c.deck} Commander in play.` : p.energy < cardCost(me, c) ? "Not enough energy." : p.jammed ? "Jammed this turn." : "Cannot be played at this moment.";
    return `<div class="ccard ${usable ? "playable" : ""}" data-i="${i}" data-why="${why.replace(/"/g, "&quot;")}"><span class="icon deck-${c.deck}">${c.deck[0]}</span><span class="cn">${c.name}</span>
      <div class="cm">${c.deck.toUpperCase()} · ${cardCost(me, c)} ENERGY · ${WHEN_TEXT[c.when]}</div></div>`;
  }).join("") || `<div style="font-size:10px;color:var(--dim)">No command cards. Buy them during recruitment (1 energy each, matching commander required).</div>`;
  el.querySelectorAll(".ccard.playable").forEach((d) => d.addEventListener("click", () => emit({ type: "card", idx: +(d as HTMLElement).dataset.i! })));
  el.querySelectorAll(".ccard").forEach((d) => {
    const c = CARD_DEFS[p.hand[+(d as HTMLElement).dataset.i!]];
    let hoverTimer: ReturnType<typeof setTimeout> | null = null, lastEvt: MouseEvent | null = null;
    d.addEventListener("mouseenter", (e) => {
      lastEvt = e as MouseEvent;
      hoverTimer = setTimeout(() => showHtmlTip(`<b>${c.name}</b><br>${c.text}<br><br>${(d as HTMLElement).dataset.why}`, lastEvt!), 3000);
    });
    d.addEventListener("mousemove", (e) => { lastEvt = e as MouseEvent; moveTip(lastEvt); });
    d.addEventListener("mouseleave", () => { if (hoverTimer) clearTimeout(hoverTimer); hideTip(); });
  });
}

export function setPrompt(title: string, text: string, buttons: Button[] = [], status = "") {
  $("prompt-title").textContent = title; $("prompt-text").textContent = text; $("status").textContent = status;
  const box = $("actions"); box.innerHTML = "";
  for (const b of buttons) {
    const el = document.createElement("button"); el.textContent = b.label; if (b.cls) el.className = b.cls;
    // Real HTML `disabled` buttons don't fire mouse events in most browsers, which silently ate
    // hover tooltips on every disabled recruit button. Fake the disabled look instead so hover
    // still works everywhere; the click handler enforces the actual disabled-ness.
    if (b.disabled) el.classList.add("disabled");
    el.addEventListener("click", () => { if (!b.disabled) emit({ type: "btn", id: b.id }); });
    if (b.tip) { el.addEventListener("mouseenter", (e) => showHtmlTip(b.tip!, e)); el.addEventListener("mousemove", moveTip); el.addEventListener("mouseleave", hideTip); }
    box.appendChild(el);
  }
}
export function setStatus(s?: string) { $("status").textContent = s || ""; }

export const HL: Highlights = { option: new Set(), target: new Set(), fortify: new Set(), selected: null, attack: null, defend: null };
export function clearHL() { HL.option.clear(); HL.target.clear(); HL.fortify.clear(); HL.selected = null; }
export function applyHighlights() {
  for (const n of NAMES) {
    const el = $("n-" + n); el.classList.remove("hl-option", "hl-target", "hl-fortify", "hl-selected", "hl-attack", "hl-defend");
    if (HL.option.has(n)) el.classList.add("hl-option");
    if (HL.target.has(n)) el.classList.add("hl-target");
    if (HL.fortify.has(n)) el.classList.add("hl-fortify");
    if (HL.selected === n) el.classList.add("hl-selected");
    if (HL.attack === n) el.classList.add("hl-attack");
    if (HL.defend === n) el.classList.add("hl-defend");
  }
}

export function showTip(n: string, e: MouseEvent) {
  const t = T(n), m = MAP[n], r = REGIONS[m.region];
  let body = `<b>${n}</b><br>${m.type.toUpperCase()} · ${r.name} (+${r.bonus})<br>`;
  if (t.dev) body += "DEVASTATED - impassable"; else if (t.owner < 0) body += "Unoccupied";
  else body += `${P(t.owner).name}: ${t.mods} MODs${Object.keys(t.cmd).map((c) => " + " + c).join("")}${t.station ? " · Space Station" : ""}`;
  if (m.site) body += "<br>Lunar landing site";
  showHtmlTip(body, e);
}
export function showHtmlTip(html: string, e: MouseEvent) { const tip = $("tooltip"); tip.innerHTML = html; tip.style.display = "block"; moveTip(e); }
export function moveTip(e: MouseEvent) { const tip = $("tooltip"); tip.style.left = Math.min(e.clientX + 16, innerWidth - 260) + "px"; tip.style.top = Math.min(e.clientY + 16, innerHeight - 100) + "px"; }
export function hideTip() { $("tooltip").style.display = "none"; }

export const fx = {
  layer(n: string) { return MAP[n].type === "moon" ? $("moon-wrap") : $("earth-wrap"); },
  pos(n: string) { return { x: MAP[n].x, y: MAP[n].y }; },
  visible(n: string) { return (MAP[n].type === "moon") === (ui.view === "moon"); },
  shock(n: string, color?: string) {
    if (!fx.visible(n)) return;
    const p = fx.pos(n), el = document.createElement("div");
    el.className = "shock"; el.style.left = p.x + "%"; el.style.top = p.y + "%"; el.style.borderColor = color || "#5ce1ff";
    fx.layer(n).appendChild(el); setTimeout(() => el.remove(), 750);
  },
  flash(n: string) { const el = $("n-" + n); el.classList.remove("flash"); void el.offsetWidth; el.classList.add("flash"); },
  async orb(from: string, to: string, color?: string) {
    if ((MAP[from].type === "moon") !== (MAP[to].type === "moon")) { fx.shock(to, color); return; } // Earth <-> Moon: no path to draw
    if (!fx.visible(from)) return;
    const a = fx.pos(from), b = fx.pos(to), el = document.createElement("div"); el.className = "orb";
    el.style.left = a.x + "%"; el.style.top = a.y + "%"; if (color) el.style.boxShadow = `0 0 12px 4px ${color}`;
    fx.layer(from).appendChild(el); await new Promise((r) => setTimeout(r, 30));
    el.style.left = b.x + "%"; el.style.top = b.y + "%"; await new Promise((r) => setTimeout(r, 620)); el.remove();
  },
  shake() { const b = $("board"); b.classList.remove("shake"); void b.offsetWidth; b.classList.add("shake"); },
};

/** A dedicated full-screen banner (not routed through setPrompt/userInput) so a bot/human phase
 *  loop that's still winding down can never steal the "click to continue" away from this screen --
 *  that hijack (via the shared ui.resolve slot) was the actual cause of "the game keeps going". */
export function showVictory(body: string) {
  const won = S!.winner === 0; // RED is always the human seat in campaign mode
  const el = $("victory"), titleEl = $("victory-title");
  if (ui.spectate) { titleEl.textContent = `${P(S!.winner).name} WINS`; titleEl.className = "v-title " + (S!.winner === 0 ? "win" : "lose"); }
  else { titleEl.textContent = won ? "YOU ARE VICTORIOUS" : "YOU HAVE BEEN DEFEATED"; titleEl.className = "v-title " + (won ? "win" : "lose"); }
  $("victory-sub").textContent = S!.conquest ? "TOTAL CONQUEST" : "FINAL SCORING";
  $("victory-body").textContent = body;
  el.classList.add("show");
  ($("victory-btn") as HTMLButtonElement).onclick = () => location.reload();
}
