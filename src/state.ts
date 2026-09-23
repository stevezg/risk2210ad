import { CARD_DEFS, CMDS, MAP, NAMES, REGIONS, RULES } from "./data";
import type { CardDef, Commander, GameState, PlayerState, TerritoryState, UIState } from "./types";
import { fx, log, setView } from "./render";

export let S: GameState | null = null;

export const ui: UIState = {
  resolve: null, mode: "idle", selected: null, hireType: null, cart: [], botDelay: 350, spectate: false, view: "earth", yearLimit: 5,
  psig: "", rsig: "", hsig: "", moveTarget: null, moveCmds: {}, moveMods: null, fortDest: null, fortCmds: {}, fortMods: null,
};

export const rnd = (n: number) => Math.floor(Math.random() * n);
export const die = (sides: number) => 1 + rnd(sides);
export const sleep = (ms: number) => new Promise<void>((r) => setTimeout(r, ms));
export const shuffle = <T,>(a: T[]): T[] => { for (let i = a.length - 1; i > 0; i--) { const j = rnd(i + 1); [a[i], a[j]] = [a[j], a[i]]; } return a; };

/** Brings the map holding `n` into view (only for the human commander's own actions). */
export function focusMap(n: string, force?: boolean) {
  if (!S) return;
  if (!force && (ui.spectate || S.cur !== 0)) return;
  const want = MAP[n].type === "moon" ? "moon" : "earth";
  if (ui.view !== want) setView(want);
}

export function newGame(spectate: boolean) {
  ui.spectate = spectate;
  S = {
    year: 0, yearLimit: ui.yearLimit || RULES.defaultYears, phase: "SETUP", cur: -1, order: [], winner: -1, conquest: false,
    players: [
      // turn-flag fields (cardsBought, captures, ...) are filled in immediately below by
      // resetTurnFlags(); this partial shape is completed before anything else touches it.
      { id: 0, name: "RED", color: "var(--red)", bot: spectate, energy: RULES.startEnergy, pool: 0, hand: [], elim: false, neutral: false } as unknown as PlayerState,
      { id: 1, name: "BLUE", color: "var(--blue)", bot: true, energy: RULES.startEnergy, pool: 0, hand: [], elim: false, neutral: false } as unknown as PlayerState,
      { id: 2, name: "GRAY", color: "var(--gray)", bot: true, energy: 0, pool: 0, hand: [], elim: false, neutral: true } as unknown as PlayerState,
    ],
    T: {}, inv: null,
    decks: {}, tdecks: { land: [], water: [], moon: [] }, tpos: { land: 0, water: 0, moon: 0 },
  };
  for (const n of NAMES) S.T[n] = { owner: -1, mods: 0, cmd: {}, station: false, dev: false };
  for (const d of CMDS) S.decks[d] = shuffle(CARD_DEFS.filter((c) => c.deck === d).flatMap((c) => Array(c.copies).fill(c.id)));
  for (const t of ["land", "water", "moon"] as const) S.tdecks[t] = shuffle(NAMES.filter((n) => MAP[n].type === t));
  for (const p of S.players) resetTurnFlags(p);
}
export function resetTurnFlags(p: PlayerState) {
  Object.assign(p, {
    cardsBought: 0, captures: 0, bonusClaimed: false, invaded: false, invasions: 0, invadeEarth: null, extraFortify: 0,
    jammed: false, armageddon: false, extraction: false, hidden: [], ceaseFire: [], scout: p.scout || null, bid: -1,
  });
}

// ---------- queries ----------
export const T = (n: string): TerritoryState => S!.T[n];
export const units = (n: string) => T(n).mods + Object.keys(T(n).cmd).length;
export const P = (i: number): PlayerState => S!.players[i];
export const active = (i: number) => i >= 0 && i < 2 && !P(i).elim;
export const owned = (p: number, type?: string) => NAMES.filter((n) => T(n).owner === p && (!type || MAP[n].type === type));
export const hasCmd = (p: number, c: Commander) => NAMES.some((n) => T(n).owner === p && T(n).cmd[c]);
export const stations = (p: number) => owned(p).filter((n) => T(n).station).length;
export const countUnits = (p: number) => owned(p).reduce((s, n) => s + units(n), 0);
export const controls = (p: number, r: string) => { const ts = NAMES.filter((n) => MAP[n].region === r && !T(n).dev); return ts.length > 0 && ts.every((n) => T(n).owner === p); };
export const regionBonus = (p: number) => Object.entries(REGIONS).reduce((s, [k, r]) => s + (controls(p, k) ? r.bonus : 0), 0);
export const income = (p: number) => Math.max(3, Math.floor(owned(p).length / 3)) + regionBonus(p);
export const score = (p: number) => owned(p).length + regionBonus(p);
export const isBorder = (p: number, n: string) => MAP[n].adj.some((m) => !T(m).dev && T(m).owner !== p);
export const borderTerrs = (p: number) => { const b = owned(p).filter((n) => isBorder(p, n)); return b.length ? b : owned(p); };
export const cardCost = (p: number, c: CardDef) => (c.deck === "Nuclear" && P(p).armageddon) ? 0 : c.cost;
export const drawTerr = (type: "land" | "water" | "moon", skipDev?: boolean): string => {
  for (let g = 0; g < 200; g++) {
    if (S!.tpos[type] >= S!.tdecks[type].length) { shuffle(S!.tdecks[type]); S!.tpos[type] = 0; }
    const n = S!.tdecks[type][S!.tpos[type]++];
    if (!skipDev || !T(n).dev) return n;
  }
  return S!.tdecks[type][0];
};
export const drawCard = (deck: string): number | null => S!.decks[deck].length ? S!.decks[deck].pop()! : null;

export function canInvade(p: number, from: string | null, to: string | null): string | null {
  if (!from || !to || from === to) return "pick two territories";
  const f = T(from), d = T(to);
  if (f.owner !== p) return "you do not control the attacking territory";
  if (d.owner === p) return "you already control the target";
  if (f.dev || d.dev) return "devastated territories are impassable";
  if (units(from) < 2) return "need at least 2 units to invade";
  if (d.owner >= 0 && P(p).ceaseFire.includes(d.owner)) return "a Cease Fire protects " + P(d.owner).name + " this turn";
  const ft = MAP[from].type, tt = MAP[to].type;
  if ((ft === "water" || tt === "water") && !hasCmd(p, "Naval")) return "a Naval Commander must be in play to invade into or out of water";
  if ((ft === "moon" || tt === "moon") && !hasCmd(p, "Space")) return "a Space Commander must be in play to invade into or out of the Moon";
  if (ft === "moon" && tt !== "moon") return P(p).invadeEarth === to ? null : "Earth can only be invaded from the Moon with Invade Earth";
  if (ft !== "moon" && tt === "moon") { if (!f.station) return "lunar invasions launch from a Space Station"; return MAP[to].site ? null : "from Earth you may only invade a landing site (Sea of Crisis, Bay of Dew, Tycho)"; }
  return MAP[from].adj.includes(to) ? null : "territories are not adjacent";
}
export function attackD8(from: string, to: string, nDice: number): Commander[] {
  const f = T(from), ft = MAP[from].type, tt = MAP[to].type, which: Commander[] = [];
  const ok: Record<Commander, boolean> = { Nuclear: true, Land: ft === "land" || tt === "land", Naval: ft === "water" || tt === "water", Space: ft === "moon" || tt === "moon", Diplomat: false };
  for (const c of CMDS) if (f.cmd[c] && ok[c] && which.length < nDice) which.push(c);
  return which;
}
export function fortifyPath(p: number, from: string, to: string): boolean {
  if (T(from).owner !== p || T(to).owner !== p) return false;
  const sts = owned(p).filter((n) => T(n).station), sites = owned(p).filter((n) => MAP[n].site);
  const seen = new Set([from]), q = [from];
  while (q.length) {
    const c = q.shift()!;
    if (c === to) return true;
    const nexts = [...MAP[c].adj];
    if (T(c).station) nexts.push(...sites);
    if (MAP[c].site) nexts.push(...sts);
    for (const n of nexts) if (T(n).owner === p && !T(n).dev && !seen.has(n)) { seen.add(n); q.push(n); }
  }
  return false;
}
export function reactivePlayable(p: number, c: CardDef): boolean {
  const inv = S!.inv;
  if (!inv || c.when !== "react" || !hasCmd(p, c.deck) || P(p).energy < cardCost(p, c) || P(p).jammed) return false;
  const tt = MAP[inv.to].type;
  switch (c.kind) {
    case "stealth": return c.target === tt && inv.defender >= 0;
    case "stealthStation": return tt === "land" && inv.defender === p && !T(inv.to).station && stations(p) < RULES.maxStations;
    case "deathTrap": return c.target === tt && inv.defender === p;
    case "ceaseFire": return inv.defender === p;
    case "evacuation": return inv.defender === p && owned(p).length > 1;
  }
  return false;
}

// ---------- mutation helpers ----------
export function destroyUnits(n: string, k: number): number {
  const t = T(n); let removed = Math.min(k, t.mods); t.mods -= removed;
  for (const c of CMDS) if (removed < k && t.cmd[c]) { delete t.cmd[c]; removed++; }
  if (units(n) === 0 && !t.station) t.owner = -1;
  if (removed > 0) fx.shock(n, "#ff5040");
  return removed;
}
export function devastate(n: string) {
  S!.T[n] = { owner: -1, mods: 0, cmd: {}, station: false, dev: true };
  log(MAP[n].name + " is devastated");
  fx.shock(n, "#ff2020");
}
export function checkElim(p: number) {
  if (!active(p) || countUnits(p) > 0) return;
  P(p).elim = true; P(p).hand = [];
  for (const n of owned(p)) { T(n).station = false; T(n).owner = -1; }
  log(P(p).name + " has been eliminated!", "y");
}
