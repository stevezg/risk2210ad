export type TerrType = "land" | "water" | "moon";
export type Commander = "Land" | "Diplomat" | "Naval" | "Nuclear" | "Space";
export type CardWhen = "before" | "react" | "end" | "score";

export interface MapNode {
  name: string;
  type: TerrType;
  region: string;
  x: number;
  y: number;
  adj: string[];
  site: boolean;
}

export interface TerritoryState {
  owner: number;
  mods: number;
  cmd: Partial<Record<Commander, boolean>>;
  station: boolean;
  dev: boolean;
}

export interface CardDef {
  id: number;
  name: string;
  deck: Commander;
  copies: number;
  cost: number;
  when: CardWhen;
  kind: string;
  target: string;
  text: string;
  amount: number;
}

export interface PlayerState {
  id: number;
  name: string;
  color: string;
  bot: boolean;
  energy: number;
  pool: number;
  hand: number[];
  elim: boolean;
  neutral: boolean;
  // per-turn flags, reset by resetTurnFlags()
  cardsBought: number;
  captures: number;
  bonusClaimed: boolean;
  invaded: boolean;
  invasions: number;
  invadeEarth: string | null;
  extraFortify: number;
  jammed: boolean;
  armageddon: boolean;
  extraction: boolean;
  hidden: string[];
  ceaseFire: number[];
  scout: string | null;
  bid: number;
  final?: number;
}

export interface Invasion {
  attacker: number;
  from: string;
  to: string;
  defender: number;
  contested: boolean;
  attackedOnce: boolean;
  captured: boolean;
  lastDice: number;
  mustMove: Commander[];
}

export interface GameState {
  year: number;
  yearLimit: number;
  phase: string;
  cur: number;
  order: number[];
  winner: number;
  conquest: boolean;
  players: PlayerState[];
  T: Record<string, TerritoryState>;
  inv: Invasion | null;
  decks: Record<string, number[]>;
  tdecks: { land: string[]; water: string[]; moon: string[] };
  tpos: { land: number; water: number; moon: number };
}

export type UIAction =
  | { type: "terr"; name: string }
  | { type: "btn"; id: string }
  | { type: "card"; idx: number };

export interface UIState {
  resolve: ((a: UIAction) => void) | null;
  mode: string;
  selected: string | null;
  hireType: string | null;
  cart: string[];
  botDelay: number;
  spectate: boolean;
  view: "earth" | "moon";
  yearLimit: number;
  psig: string;
  rsig: string;
  hsig: string;
  moveTarget: string | null;
  moveCmds: Partial<Record<Commander, boolean>>;
  moveMods: number | null;
  fortDest: string | null;
  fortCmds: Partial<Record<Commander, boolean>>;
  fortMods: number | null;
}

export interface Button {
  id: string;
  label: string;
  cls?: string;
  disabled?: boolean;
  tip?: string;
}

export interface ModalOption<T> {
  label: string;
  value: T;
  cls?: string;
}

export interface Highlights {
  option: Set<string>;
  target: Set<string>;
  fortify: Set<string>;
  selected: string | null;
  attack: string | null;
  defend: string | null;
}
