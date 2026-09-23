import { MAP, NAMES, RULES, REGIONS, CARD_DEFS } from "./data";
import {
  T, P, units, active, owned, hasCmd, cardCost, isBorder, checkElim, destroyUnits, drawTerr, die, rnd, stations,
} from "./state";
import { fx, log, render } from "./render";
import { choose, choosePlayer } from "./ui-events";
import type { CardDef, Commander } from "./types";

export async function playCard(p: number, idx: number, when: string) {
  const pl = P(p), c = CARD_DEFS[pl.hand[idx]];
  if (c.when !== when || !hasCmd(p, c.deck) || pl.energy < cardCost(p, c) || (when === "before" && pl.invaded)) return;
  pl.energy -= cardCost(p, c); pl.hand.splice(idx, 1);
  log(`${pl.name} plays ${c.name}`); render();
  await applyCard(p, c);
  render();
}
export function zoneRoll(type: string): string {
  if (type === "land") return ["NA", "SA", "EU", "AF", "AS", "AU"][die(6) - 1];
  if (type === "moon") return ["CRE", "CRE", "DEL", "DEL", "SAJ", "SAJ"][die(6) - 1];
  let r; do r = die(6); while (r === 6); return ["USP", "ASP", "NAT", "SAT", "IND"][r - 1];
}
export async function applyCard(p: number, c: CardDef) {
  const pl = P(p);
  const botBorder = (opts: string[]) => { const b = opts.filter((n) => isBorder(p, n)); return (b.length ? b : opts)[rnd((b.length ? b : opts).length)]; };
  switch (c.kind) {
    case "reinforce": { let opts = owned(p, c.target); for (let i = 0; i < Math.min(3, opts.length); i++) { const t = await choose(p, opts, "Place 1 reinforcement MOD", botBorder); if (!t) break; T(t).mods++; fx.flash(t); opts = opts.filter((n) => n !== t); render(); } break; }
    case "assemble": { const t = await choose(p, owned(p, c.target), "Place 3 MODs", botBorder); if (t) { T(t).mods += 3; fx.flash(t); } break; }
    case "energyCrisis": { let got = 0; for (const q of [0, 1]) if (q !== p && active(q) && P(q).energy > 0) { P(q).energy--; got++; } pl.energy += got; log(`${pl.name} collects ${got} energy`); break; }
    case "decoys": { for (const n of owned(p)) for (const k of Object.keys(T(n).cmd) as Commander[]) { const to = await choose(p, owned(p), `Move your ${k} Commander to`, botBorder); if (to && to !== n && T(n).cmd[k]) { delete T(n).cmd[k]; T(to).cmd[k] = true; if (units(n) === 0 && !T(n).station) T(n).owner = -1; await fx.orb(n, to); } } break; }
    case "modReduction": { const rm = (q: number, k: number) => { let r = 0; while (r < k) { const cands = owned(q).filter((n) => T(n).mods > 0 && units(n) > 1); if (!cands.length) break; const b = cands.reduce((x, n) => T(n).mods > T(x).mods ? n : x); T(b).mods--; r++; } if (r) log(`${P(q).name} removes ${r} MOD(s)`); }; for (const q of [0, 1]) if (q !== p && active(q)) rm(q, 4); rm(p, 2); break; }
    case "redeployment": pl.extraFortify++; break;
    case "terrStation": { if (stations(p) >= RULES.maxStations) break; const t = await choose(p, owned(p, "land").filter((n) => !T(n).station), "Place a Space Station in", botBorder); if (t) { T(t).station = true; log(`${pl.name} deploys a Space Station to ${t}`); } break; }
    case "jam": { const q = await choosePlayer(p, [0, 1].filter((x) => x !== p && active(x)), "Jam which player's command cards this turn?"); if (q !== undefined && q !== null) { P(q).jammed = true; log(`${P(q).name} cannot play command cards during ${pl.name}'s turn`); } break; }
    case "scout": { const t = drawTerr("land", true); if (T(t).owner === p) { T(t).mods += 5; log(`${pl.name} places 5 scout MODs on ${t}`); } else { pl.scout = t; log(pl.bot ? `${pl.name} has scouts waiting somewhere` : `Your scouts wait in ${t}`); } break; }
    case "hiddenEnergy": { const t = drawTerr("water"); pl.hidden.push(t); log(pl.bot ? `${pl.name} draws a hidden energy site` : `Hold ${t} at the end of your turn to collect 4 energy`); break; }
    case "zone": { const r = zoneRoll(c.target); log(`${c.name} strikes ${REGIONS[r].name}!`); const aff = new Set<number>(); for (const n of NAMES) if (MAP[n].region === r && !T(n).dev && units(n) > 0) { aff.add(T(n).owner); destroyUnits(n, 1); } for (const q of aff) checkElim(q); break; }
    case "assassin": { const opts = NAMES.filter((n) => T(n).owner >= 0 && T(n).owner !== p && Object.keys(T(n).cmd).length); if (!opts.length) { log("No enemy commander to target"); break; } const t = await choose(p, opts, "Target the commander in", (o) => o[0]); if (!t) break; const k = (["Nuclear", "Space", "Naval", "Land", "Diplomat"] as Commander[]).find((x) => T(t).cmd[x])!; const r = die(8); if (r >= 3) { const o = T(t).owner; delete T(t).cmd[k]; log(`Assassin Bomb rolls ${r}: ${P(o).name}'s ${k} Commander in ${t} is destroyed`); fx.shock(t, "#ff5040"); if (units(t) === 0 && !T(t).station) T(t).owner = -1; checkElim(o); } else log(`Assassin Bomb rolls ${r}: the commander survives`); break; }
    case "armageddon": pl.armageddon = true; log(`${pl.name} launches Armageddon: nuclear cards are free this turn`); break;
    case "rocket": { const opts = NAMES.filter((n) => MAP[n].type === c.target && T(n).owner >= 0 && T(n).owner !== p && units(n) > 0); if (!opts.length) { log("No target for the rocket strike"); break; } const t = await choose(p, opts, "Rocket strike target", (o) => o.reduce((b, n) => units(n) > units(b) ? n : b)); if (!t) break; const r = die(6), o = T(t).owner; log(`Rocket strike on ${t} rolls ${r}`); destroyUnits(t, r); checkElim(o); break; }
    case "scatter": { for (let i = 0; i < c.amount; i++) { const t = drawTerr(c.target as "land" | "water" | "moon"); const s = T(t); if (s.owner < 0 || s.owner === p || s.dev) continue; const k = Math.ceil(units(t) / 2), o = s.owner; log(`Scatter bomb hits ${t}: ${k} unit(s) destroyed`); destroyUnits(t, k); checkElim(o); } break; }
    case "invadeEarth": { for (let g = 0; g < 60; g++) { const t = drawTerr("land", true); if (T(t).owner !== p) { pl.invadeEarth = t; log(`${pl.name} may invade ${t} from the Moon this turn`); break; } } break; }
    case "extraction": pl.extraction = true; break;
  }
}
