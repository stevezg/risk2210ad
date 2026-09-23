import { CARD_DEFS, MAP, NAMES, REGIONS, RULES } from "./data";
import {
  S, T, P, active, owned, score, regionBonus, income, countUnits, borderTerrs, drawTerr, devastate, rnd, die,
  resetTurnFlags, hasCmd, controls, ui, gameEnded,
} from "./state";
import { HL, clearHL, ensureYearTrack, log, render, setPrompt, showVictory } from "./render";
import { choose, modal, modalNumber, userInput } from "./ui-events";
import { humanAttack, humanFortify, humanRecruit } from "./human-phases";
import { botTurn } from "./bot";

export async function runGame() {
  await setup();
  let y = 0;
  while (true) {
    y++;
    S!.year = y; ensureYearTrack(); log(`=== Year ${y} (${2205 + y} A.D.) ===`, "y");
    await bidding();
    for (const p of S!.order) {
      if (!active(p)) continue;
      await takeTurn(p);
      if (conquestWinner() >= 0) { S!.conquest = true; finalScoring(); return; }
    }
    if (y >= S!.yearLimit) {
      const more = await checkpoint();
      if (!more) break;
      S!.yearLimit = y + more;
    }
  }
  finalScoring();
}

/** Returns the surviving player id once the other real commander has no territories left, or -1 if the war is still on. */
export function conquestWinner(): number {
  const standing = [0, 1].filter(active);
  return standing.length === 1 ? standing[0] : -1;
}

/** End-of-limit checkpoint: show the interim standings and ask whether to keep playing. Returns 0 to stop, or the number of extra years to add. */
export async function checkpoint(): Promise<number> {
  const standings = [0, 1].map((p) => `${P(p).name}: ${score(p)} pts  (${owned(p).length} territories, ${p === 0 || ui.spectate ? P(p).energy + " energy" : "? energy"})`).join("\n");
  log(`=== Year ${S!.year} complete ===`, "y");
  const choice = await modal<number>(`YEAR ${S!.year} COMPLETE`, `Current standings:\n${standings}\n\nKeep playing, or stop here for final scoring?`, [
    { label: "Stop — final scoring", value: 0, cls: "primary" },
    { label: "Play 1 more year", value: 1 },
    { label: "Play 3 more years", value: 3 },
    { label: "Play 5 more years", value: 5 },
  ]);
  if (choice) log(`Commanders continue the war for ${choice} more year(s).`, "y");
  return choice;
}

export async function setup() {
  S!.phase = "SETUP"; render();
  for (let i = 0; i < RULES.devastation; i++) devastate(drawTerr("land", true));
  // 2-player rules: the neutral army holds 16 land, 6 water and 6 lunar territories with 3 MODs each
  const place = (type: "land" | "water" | "moon", count: number) => { let k = 0, g = 0; while (k < count && g++ < 500) { const n = drawTerr(type); if (T(n).dev || T(n).owner !== -1) continue; T(n).owner = 2; T(n).mods = 3; k++; } };
  place("land", 16); place("water", 6); place("moon", 6);
  log("Neutral armies hold 16 land, 6 water and 6 lunar territories");
  P(0).pool = P(1).pool = 30;
  // claim the remaining land alternately (each player picks; the human clicks)
  let free = NAMES.filter((n) => MAP[n].type === "land" && !T(n).dev && T(n).owner === -1);
  let p = 0;
  while (free.length) {
    const pick = await choose(p, free, `Claim a territory (${free.length} left). Each claim places 1 MOD.`, (opts) => { const near = opts.filter((o) => MAP[o].adj.some((a) => T(a).owner === p)); return near.length && rnd(4) ? near[rnd(near.length)] : opts[rnd(opts.length)]; });
    T(pick!).owner = p; T(pick!).mods = 1; P(p).pool--; free = free.filter((n) => n !== pick); render(); p = 1 - p;
  }
  // place the remaining MODs
  S!.phase = "PLACEMENT";
  for (const q of [0, 1]) {
    if (P(q).bot) { while (P(q).pool > 0) { const b = borderTerrs(q); T(b[rnd(b.length)]).mods++; P(q).pool--; } continue; }
    ui.mode = "recruit";
    while (P(q).pool > 0) {
      render();
      setPrompt("PLACE STARTING MODS", `${P(q).pool} MODs left. Click one of your territories (+1), or use the buttons on a selected territory.`, [
        { id: "plus5", label: "+5 on selected", disabled: !ui.selected || P(q).pool < 5 }, { id: "all", label: "All on selected", disabled: !ui.selected }]);
      const a = await userInput();
      if (a.type === "terr" && T(a.name).owner === q) { ui.selected = a.name; HL.selected = a.name; T(a.name).mods++; P(q).pool--; }
      if (a.type === "btn" && ui.selected) { const k = a.id === "all" ? P(q).pool : Math.min(5, P(q).pool); T(ui.selected).mods += k; P(q).pool -= k; }
    }
    clearHL(); ui.selected = null;
  }
  // starting Space Station, Land Commander, Diplomat
  for (const q of [0, 1]) {
    const best = owned(q).reduce((b, n) => T(n).mods > T(b).mods ? n : b);
    const st = await choose(q, owned(q, "land"), "Place your starting SPACE STATION", () => best);
    const lc = await choose(q, owned(q), "Place your LAND COMMANDER", () => best);
    const dp = await choose(q, owned(q), "Place your DIPLOMAT", () => { const b = borderTerrs(q); return b[rnd(b.length)]; });
    T(st!).station = true; T(lc!).cmd.Land = true; T(dp!).cmd.Diplomat = true;
    log(`${P(q).name}: Space Station in ${st}, Land Commander in ${lc}, Diplomat in ${dp}`);
  }
  render();
}

export async function bidding() {
  S!.phase = "BIDDING"; S!.cur = -1; render();
  const bids: Record<number, number> = {};
  for (const p of [0, 1]) {
    if (!active(p)) continue;
    if (P(p).bot) {
      const e = P(p).energy, edge = countUnits(p) - countUnits(active(1 - p) ? 1 - p : p);
      const frac = edge > 5 ? 0.7 : edge < -5 ? 0.3 : 0.5;
      bids[p] = e <= 1 ? 0 : rnd(Math.max(1, Math.ceil(e * frac)) + 1);
    } else {
      setPrompt("BIDDING", "Secretly bid energy for turn order. Highest bidder chooses their marker; all bids are spent.", []);
      bids[p] = await modalNumber("SECRET BID", `You have ${P(p).energy} energy. How much do you bid for turn order?`, 0, P(p).energy, 0);
    }
  }
  const ps = Object.keys(bids).map(Number);
  for (const p of ps) P(p).energy -= bids[p];
  const tie: Record<number, number> = {}; for (const p of ps) tie[p] = die(6);
  ps.sort((a, b) => bids[b] - bids[a] || tie[b] - tie[a]);
  log("Bids: " + ps.map((p) => `${P(p).name} ${bids[p]}`).join(", "));
  let markers = ps.map((_, i) => i); const order = new Array(ps.length).fill(-1);
  for (const p of ps) {
    let m: number;
    if (P(p).bot) m = markers[0];
    else m = await modal<number>("CHOOSE TURN ORDER", `You won the choice (bid ${bids[p]}). Which turn marker do you take? Marker #1 acts first this year, #2 second — whoever you leave for your opponent, they get.`, markers.map((k) => ({ label: "#" + (k + 1) + (k === 0 ? " — go first" : " — go second"), value: k })));
    order[m] = p; markers = markers.filter((k) => k !== m);
    log(`${P(p).name} takes turn marker #${m + 1}`);
  }
  S!.order = order;
}

export async function takeTurn(p: number) {
  const pl = P(p); resetTurnFlags(pl); S!.cur = p; S!.inv = null;
  const inc = income(p); pl.pool += inc; pl.energy += inc;
  let stMods = 0; for (const n of owned(p)) if (T(n).station) { T(n).mods++; stMods++; }
  log(`--- ${pl.name}: ${owned(p).length} territories, +${inc} MODs, +${inc} energy${stMods ? `, +${stMods} at stations` : ""}`, "y");
  S!.phase = "RECRUIT"; clearHL(); render();
  if (pl.bot) { setPrompt(pl.name + " IS THINKING", "The bot recruits, invades and fortifies. You will be asked to defend or play reactive cards when it attacks you.", []); await botTurn(p); } else { await humanRecruit(p); await humanAttack(p); await humanFortify(p); }
  await endTurn(p);
}
export async function endTurn(p: number) {
  const pl = P(p);
  for (const n of pl.hidden) if (T(n).owner === p) { pl.energy += 4; log(`${pl.name} collects 4 hidden energy from ${n}`); }
  if (pl.extraction) for (const [k, r] of Object.entries(REGIONS)) if (r.type === "moon" && controls(p, k)) { pl.energy += 7; log(`${pl.name} extracts 7 energy from ${r.name}`); break; }
  for (const q of S!.players) q.jammed = false;
  S!.cur = -1; S!.inv = null; clearHL(); render();
}

export function finalScoring() {
  if (gameEnded()) return; // idempotent: checkElim() may already have ended the game mid-turn
  S!.phase = "GAME OVER";
  log(S!.conquest ? "=== Total conquest ===" : "=== Final scoring ===", "y");
  const res: number[] = [];
  for (const p of [0, 1]) {
    const pl = P(p); let inf = 0;
    if (!pl.elim) pl.hand = pl.hand.filter((id) => { const c = CARD_DEFS[id]; if (c.when === "score" && hasCmd(p, c.deck)) { inf += 3; return false; } return true; });
    pl.final = score(p) + inf;
    log(`${pl.name}: ${owned(p).length} territories + ${regionBonus(p)} bonus + ${inf} influence = ${pl.final}`);
    res.push(p);
  }
  res.sort((a, b) => P(b).final! - P(a).final! || P(b).energy - P(a).energy || countUnits(b) - countUnits(a));
  S!.winner = res[0]; render();
  const title = S!.conquest ? `${P(S!.winner).name} has conquered the world!` : `${P(S!.winner).name} is elected the new world leader!`;
  log(title, "y");
  const body = S!.conquest
    ? `${P(S!.winner).name} has eliminated ${P(1 - S!.winner).name} in Year ${S!.year} — no opposition remains.\n${res.map((p) => `${P(p).name}: ${P(p).final}`).join("   ")}`
    : `${P(S!.winner).name} is elected world leader.\n${res.map((p) => `${P(p).name}: ${P(p).final}`).join("   ")}`;
  showVictory(body);
}
