import { CMDS, LANDING_SITES, MAP, NAMES, REGIONS, RULES } from "./data";
import {
  S, T, P, units, owned, hasCmd, stations, cardCost, canInvade, isBorder, borderTerrs, drawCard, rnd, sleep, ui,
} from "./state";
import { fx, log, render, HL, $ } from "./render";
import { attack, declareInvasion, moveIn } from "./combat";
import { fortifyPath } from "./state";
import { playCard } from "./cards";
import type { CardDef, Commander, Invasion } from "./types";
import { CARD_DEFS } from "./data";
import { controls } from "./state";

export function botReactive(q: number, playable: { c: CardDef; i: number }[], inv: Invasion): number | null {
  const att = units(inv.from), def = units(inv.to);
  const scored: { i: number; v: number }[] = [];
  for (const { c, i } of playable) {
    let v = -1;
    if (c.kind === "stealth" || c.kind === "stealthStation") v = att > def ? (att - def) * 2 : 0.5;
    else if (c.kind === "deathTrap") v = att >= 4 ? att : -1;
    else if (c.kind === "ceaseFire") v = (att >= 6 && P(q).energy >= c.cost + 1) ? att + 2 : -1;
    else if (c.kind === "evacuation") v = (att >= def * 3 && def >= 3) ? att - def : -1;
    if (v >= 0) scored.push({ i, v });
  }
  if (!scored.length) return null;
  scored.sort((a, b) => b.v - a.v);
  return scored[0].i;
}
export function botPlayable(p: number, when: string): number {
  const pl = P(p);
  for (let i = 0; i < pl.hand.length; i++) {
    const c = CARD_DEFS[pl.hand[i]];
    if (c.when !== when || !hasCmd(p, c.deck) || pl.energy - cardCost(p, c) < 1) continue;
    if (c.kind === "armageddon" && !pl.hand.some((id) => CARD_DEFS[id].deck === "Nuclear" && CARD_DEFS[id].cost > 0 && CARD_DEFS[id].kind !== "armageddon")) continue;
    if (c.kind === "extraction" && !Object.entries(REGIONS).some(([k, r]) => r.type === "moon" && controls(p, k))) continue;
    return i;
  }
  return -1;
}
export async function botTurn(p: number) {
  const pl = P(p), D = () => sleep(ui.botDelay);
  // deployment: reinforce whichever border is most outmatched by adjacent enemy strength
  while (pl.pool > 0) {
    const b = borderTerrs(p);
    const scored = b.map((n) => ({ n, threat: MAP[n].adj.filter((m) => T(m).owner >= 0 && T(m).owner !== p).reduce((s, m) => s + units(m), 0) - units(n) }));
    scored.sort((a, b) => b.threat - a.threat);
    const t = scored[0].n;
    const k = Math.min(pl.pool, Math.max(1, Math.ceil(pl.pool / 2)));
    T(t).mods += k; pl.pool -= k; fx.flash(t); render(); await D();
  }
  // economy: cards first (reliably, on a modest budget), then at most one new commander,
  // then rarely a station -- keeps some energy in reserve for next year's bid
  const decks = CMDS.filter((c) => hasCmd(p, c) && S!.decks[c].length);
  if (decks.length && pl.energy > 1 && pl.hand.length < 6) { const n = Math.min(RULES.maxCards, Math.max(1, Math.floor(pl.energy / 2)), pl.energy - 1, 6 - pl.hand.length); let bought = 0; for (let i = 0; i < n; i++) { const d = decks[rnd(decks.length)]; const id = drawCard(d); if (id !== null) { pl.hand.push(id); pl.energy--; bought++; } } if (bought) log(`${pl.name} buys ${bought} command card(s)`); }
  {
    const missing = CMDS.filter((c) => !hasCmd(p, c));
    if (missing.length && pl.energy >= RULES.commanderCost + 3) {
      let c: Commander;
      if (missing.includes("Naval") && owned(p).some((n) => MAP[n].adj.some((m) => MAP[m].type === "water" && T(m).owner !== p))) c = "Naval";
      else if (missing.includes("Space") && stations(p) > 0) c = "Space";
      else if (missing.includes("Land")) c = "Land";
      else c = missing[0];
      const frontiers = borderTerrs(p);
      const t = frontiers.reduce((best, n) => units(n) > units(best) ? n : best, frontiers[0]);
      T(t).cmd[c] = true; pl.energy -= RULES.commanderCost;
      log(`${pl.name} hires a ${c} Commander in ${t}`); render(); await D();
    }
  }
  if (pl.energy >= RULES.stationCost + 4 && stations(p) < RULES.maxStations && rnd(4) === 0) { const spots = owned(p, "land").filter((n) => !T(n).station); if (spots.length) { const t = spots[rnd(spots.length)]; T(t).station = true; pl.energy -= RULES.stationCost; log(`${pl.name} builds a Space Station in ${t}`); } }
  for (let g = 0; g < 8; g++) { const i = botPlayable(p, "before"); if (i < 0) break; await playCard(p, i, "before"); await D(); }
  // attacks
  S!.phase = "ATTACK"; render();
  for (let g = 0; g < 12; g++) {
    const opts: { from: string; to: string; s: number }[] = [];
    for (const from of owned(p)) {
      if (units(from) < 2) continue;
      const targets = new Set(MAP[from].adj); if (T(from).station) LANDING_SITES.forEach((s) => targets.add(s)); if (pl.invadeEarth) targets.add(pl.invadeEarth);
      for (const to of targets) {
        if (canInvade(p, from, to)) continue;
        const du = units(to);
        if (du === 0 || units(from) >= du + 2) {
          let s = units(from) - du + (du === 0 ? 1 : 0) + (T(to).owner === 0 ? 1 : 0);
          const region = MAP[to].region;
          const willComplete = NAMES.filter((n) => MAP[n].region === region && n !== to && !T(n).dev).every((n) => T(n).owner === p);
          if (willComplete) s += REGIONS[region].bonus * 2;
          if (Object.keys(T(to).cmd).length && units(from) < du + 4) s -= 1;
          if (T(to).owner >= 0 && P(T(to).owner).hand.length >= 3) s -= 0.5;
          opts.push({ from, to, s });
        }
      }
    }
    if (!opts.length) break;
    opts.sort((a, b) => b.s - a.s); const o = opts[0];
    await declareInvasion(p, o.from, o.to);
    while (S!.inv && !S!.inv.captured) {
      const u = units(o.from);
      if (u < 2 || (u <= units(o.to) && S!.inv.attackedOnce)) { S!.inv = null; HL.attack = HL.defend = null; break; }
      const before = units(o.from);
      await attack(p, Math.min(3, u - 1));
      if (S!.inv && !S!.inv.captured && units(o.from) <= 2 && before - units(o.from) >= 2) { log(`${pl.name} pulls back from a costly invasion`); S!.inv = null; HL.attack = HL.defend = null; break; }
    }
    if (S!.inv && S!.inv.captured) { const maxMove = units(o.from) - 1, minMove = Math.max(1, Math.min(S!.inv.lastDice, maxMove)); const keep = isBorder(p, o.from) ? Math.min(maxMove, 2) : 0; moveIn(p, Math.max(minMove, maxMove - keep)); await D(); }
    $("combat").classList.remove("show");
  }
  // fortify
  S!.phase = "FORTIFY"; render();
  const i = botPlayable(p, "end"); if (i >= 0) await playCard(p, i, "end");
  for (let m = 0; m <= pl.extraFortify; m++) {
    const interior = owned(p).filter((n) => !isBorder(p, n) && T(n).mods >= 2);
    if (!interior.length) break;
    const from = interior.reduce((x, n) => T(n).mods > T(x).mods ? n : x);
    const dests = borderTerrs(p).filter((n) => n !== from && fortifyPath(p, from, n));
    if (!dests.length) break;
    const scored = dests.map((n) => ({ n, threat: MAP[n].adj.filter((m) => T(m).owner >= 0 && T(m).owner !== p).reduce((s, m) => s + units(m), 0) }));
    scored.sort((a, b) => b.threat - a.threat);
    const to = scored[0].n, k = T(from).mods - 1; T(from).mods -= k; T(to).mods += k;
    log(`${pl.name} fortifies ${k} unit(s) from ${from} to ${to}`); await fx.orb(from, to, "#3d9bff");
  }
  await D();
}
