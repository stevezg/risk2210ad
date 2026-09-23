import { CARD_DEFS, CMDS, RULES } from "./data";
import {
  S, T, P, units, active, stations, checkElim, owned, cardCost, attackD8, reactivePlayable, drawCard, hasCmd,
  destroyUnits, die, sleep, ui, focusMap,
} from "./state";
import { $, fx, HL, log, render, setPrompt } from "./render";
import { choose, modal, userInput } from "./ui-events";
import { botReactive } from "./bot";
import type { Commander, Invasion } from "./types";

/** `contested` = the target already has defenders; drives whether reactive cards get a window. */
export async function declareInvasion(p: number, from: string, to: string) {
  const inv: Invasion = { attacker: p, from, to, defender: T(to).owner, contested: units(to) > 0, attackedOnce: false, captured: false, lastDice: 0, mustMove: [] };
  S!.inv = inv; P(p).invaded = true; P(p).invasions++;
  HL.attack = from; HL.defend = to; focusMap(to); render();
  log(`${P(p).name} invades ${to} from ${from}`);
  if (inv.contested) {
    // reactive cards: the defender first, then a bystander may help -- but only when the
    // defender is a real player. Nobody gets a "help the neutral army" prompt.
    const bystanders = active(inv.defender) ? [0, 1].filter((x) => x !== inv.defender && x !== p) : [];
    for (const q of [inv.defender, ...bystanders]) {
      if (!active(q)) continue;
      for (let guard = 0; guard < 6 && S!.inv; guard++) {
        const playable = P(q).hand.map((id, i) => ({ c: CARD_DEFS[id], i })).filter((x) => reactivePlayable(q, x.c));
        if (!playable.length) break;
        let pick: number | null = null;
        if (P(q).bot) pick = botReactive(q, playable, inv);
        else {
          ui.mode = "react"; render();
          setPrompt("OPPONENT INVADES", `${P(p).name} invades ${to} from ${from} (${units(from)} vs ${units(to)}). Play a glowing reactive card, or pass.`, [{ id: "pass", label: "Pass" }]);
          const a = await userInput(); ui.mode = "idle";
          if (a.type === "card") pick = a.idx;
        }
        if (pick === null) break;
        await applyReactive(q, pick);
      }
      if (!S!.inv) break;
    }
    if (!S!.inv) { HL.attack = HL.defend = null; render(); return; }
    if (units(to) === 0) occupyEmpty();
  } else occupyEmpty();
  render();
}
export function occupyEmpty() {
  const inv = S!.inv!, d = T(inv.to), old = d.owner; d.owner = inv.attacker;
  if (old >= 0 && old !== inv.attacker && d.station && stations(inv.attacker) > RULES.maxStations) d.station = false;
  inv.captured = true; inv.lastDice = 1; inv.attackedOnce = true;
  log(`${P(inv.attacker).name} occupies ${inv.to}`);
}
export async function applyReactive(q: number, idx: number) {
  const inv = S!.inv!, c = CARD_DEFS[P(q).hand[idx]]; P(q).energy -= cardCost(q, c); P(q).hand.splice(idx, 1);
  log(`${P(q).name} plays ${c.name}`);
  switch (c.kind) {
    case "stealth": T(inv.to).mods += 3; fx.flash(inv.to); break;
    case "stealthStation": T(inv.to).station = true; break;
    case "deathTrap": {
      const n = Math.ceil(units(inv.from) / 2);
      log(`${P(inv.attacker).name} loses ${n} unit(s) in ${inv.from} to the trap`);
      destroyUnits(inv.from, n); checkElim(inv.attacker);
      if (units(inv.from) < 2 || !active(inv.attacker)) { S!.inv = null; log("The invasion collapses"); }
      break;
    }
    case "ceaseFire": P(inv.attacker).ceaseFire.push(q); S!.inv = null; log(`Cease Fire: ${P(inv.attacker).name} may not attack ${P(q).name} again this turn`); break;
    case "evacuation": {
      const dests = owned(q).filter((n) => n !== inv.to);
      const to = await choose(q, dests, "Evacuate all units to", (o) => o.reduce((b, n) => units(n) > units(b) ? n : b));
      const f = T(inv.to), d = T(to!); const moved = units(inv.to);
      d.mods += f.mods; f.mods = 0;
      for (const k of CMDS) if (f.cmd[k]) { delete f.cmd[k]; d.cmd[k] = true; }
      if (!f.station) f.owner = -1;
      log(`${P(q).name} evacuates ${moved} unit(s) to ${to}`); await fx.orb(inv.to, to!);
      break;
    }
  }
  render();
}
export async function attack(p: number, nDice: number) {
  const inv = S!.inv!, f = T(inv.from), d = T(inv.to);
  const maxD = Math.min(2, units(inv.to));
  let dDice = maxD;
  if (maxD === 2 && active(inv.defender) && !P(inv.defender).bot) dDice = await modal<number>("DEFEND", `${P(p).name} attacks ${inv.to} (${units(inv.to)} units) from ${inv.from} (${units(inv.from)}) with ${nDice} dice. Defend with how many?`, [{ label: "1 die", value: 1 }, { label: "2 dice", value: 2 }]);
  const which = attackD8(inv.from, inv.to, nDice), a8 = which.length, d8 = d.station ? dDice : Math.min(dDice, Object.keys(d.cmd).length);
  const ar = Array.from({ length: nDice }, (_, i) => die(i < a8 ? 8 : 6)).sort((x, y) => y - x);
  const dr = Array.from({ length: dDice }, (_, i) => die(i < d8 ? 8 : 6)).sort((x, y) => y - x);
  let aLost = 0, dLost = 0; for (let i = 0; i < Math.min(nDice, dDice); i++) { if (ar[i] > dr[i]) dLost++; else aLost++; }
  inv.attackedOnce = true; inv.lastDice = nDice;
  showCombat(inv, ar, dr, a8, d8, aLost, dLost);
  log(`  ${P(p).name} rolls ${ar.join(" ")}${a8 ? ` (${a8}xd8)` : ""} vs ${dr.join(" ")}${d8 ? ` (${d8}xd8)` : ""} -> attacker -${aLost}, defender -${dLost}`);
  const quiet = (n: string, k: number) => { const t = T(n); let r = Math.min(k, t.mods); t.mods -= r; for (const c of CMDS) if (r < k && t.cmd[c]) { delete t.cmd[c]; r++; } };
  quiet(inv.from, aLost); quiet(inv.to, dLost);
  if (aLost) fx.flash(inv.from); if (dLost) fx.flash(inv.to); if (aLost + dLost) fx.shake();
  if (units(inv.to) === 0) {
    inv.captured = true; inv.mustMove = which.filter((c) => f.cmd[c]);
    if (d.station && stations(p) >= RULES.maxStations) { d.station = false; log(`  the Space Station in ${inv.to} is destroyed`); }
    const defender = d.owner; d.owner = p; fx.shock(inv.to, p === 0 ? "#ff4d5a" : "#3d9bff");
    log(`  ${P(p).name} captures ${inv.to}`);
    const pl = P(p); if (inv.contested) pl.captures++;
    if (pl.captures >= 3 && !pl.bonusClaimed) {
      pl.bonusClaimed = true; pl.energy++;
      const opts = CMDS.filter((c) => hasCmd(p, c) && S!.decks[c].length);
      log(`  3-territory bonus: ${pl.name} gains 1 energy and a command card`);
      if (opts.length) { const deck = pl.bot ? opts[0] : await modal<Commander>("3-TERRITORY BONUS", "You gain 1 energy and a command card. Draw from which deck?", opts.map((c) => ({ label: c, value: c }))); pl.hand.push(drawCard(deck)!); }
    }
    checkElim(defender);
  }
  render();
  await sleep(P(p).bot ? ui.botDelay : 250);
}
/** `chosenCmds` (optional) is the set of non-mandatory commanders the player picked to bring along
 *  (e.g. {Naval:true}); commanders whose d8 forced them to advance move automatically regardless.
 *  If omitted, every present commander comes along (used by the bot, and as a safe fallback). */
export function moveIn(p: number, k: number, chosenCmds?: Partial<Record<Commander, boolean>>) {
  const inv = S!.inv!, f = T(inv.from), d = T(inv.to);
  const maxMove = units(inv.from) - 1; k = Math.max(1, Math.min(k, maxMove));
  let moved = 0;
  for (const c of inv.mustMove) if (f.cmd[c]) { delete f.cmd[c]; d.cmd[c] = true; moved++; } // commanders whose d8 must advance (rulebook)
  for (const c of CMDS) if (f.cmd[c] && (chosenCmds ? chosenCmds[c] : true)) { delete f.cmd[c]; d.cmd[c] = true; moved++; }
  const m = Math.min(Math.max(0, k - moved), f.mods); f.mods -= m; d.mods += m; moved += m; // MODs fill whatever's left
  d.owner = p; log(`  ${P(p).name} moves ${moved} unit(s) into ${inv.to}`); fx.orb(inv.from, inv.to, p === 0 ? "#ff4d5a" : "#3d9bff");
  if (P(p).scout === inv.to) { d.mods += 5; P(p).scout = null; log("  scout forces arrive: +5 MODs"); }
  S!.inv = null; HL.attack = HL.defend = null; $("combat").classList.remove("show"); render();
}
export function showCombat(inv: Invasion, ar: number[], dr: number[], a8: number, d8: number, aLost: number, dLost: number) {
  const el = $("combat"); el.classList.add("show");
  const dice = (rolls: number[], cls: string, n8: number, lost: number[]) => rolls.map((r, i) => `<div class="die ${i < n8 ? "d8" : cls} ${lost.includes(i) ? "lost" : ""}">${r}</div>`).join("");
  const pairs = Math.min(ar.length, dr.length), aL: number[] = [], dL: number[] = []; for (let i = 0; i < pairs; i++) (ar[i] > dr[i] ? dL : aL).push(i);
  el.innerHTML = `<div class="vs"><span style="color:${P(inv.attacker).color}">${P(inv.attacker).name}</span> ${inv.from} &rarr; ${inv.to} <span style="color:${inv.defender >= 0 ? P(inv.defender).color : "#888"}">${inv.defender >= 0 ? P(inv.defender).name : "empty"}</span></div>
    <div class="dice">${dice(ar, "a", a8, aL)}</div><div class="dice">${dice(dr, "d", d8, dL)}</div>
    <div class="result">attacker loses ${aLost} · defender loses ${dLost}${units(inv.to) - dLost <= 0 ? " · CAPTURED" : ""}</div>`;
}
