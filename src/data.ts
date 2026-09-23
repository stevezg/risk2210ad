import type { CardDef, CardWhen, Commander, MapNode, TerrType } from "./types";

// ============================================================================
// DATA: the board graph
// ============================================================================
export const REGIONS: Record<string, { name: string; type: TerrType; bonus: number }> = {
  NA: { name: "North America", type: "land", bonus: 5 }, SA: { name: "South America", type: "land", bonus: 2 }, EU: { name: "Europe", type: "land", bonus: 5 },
  AF: { name: "Africa", type: "land", bonus: 3 }, AS: { name: "Asia", type: "land", bonus: 7 }, AU: { name: "Australia", type: "land", bonus: 2 },
  USP: { name: "US Pacific", type: "water", bonus: 2 }, ASP: { name: "Asia Pacific", type: "water", bonus: 1 }, NAT: { name: "N. Atlantic", type: "water", bonus: 2 },
  SAT: { name: "S. Atlantic", type: "water", bonus: 1 }, IND: { name: "Indian", type: "water", bonus: 2 },
  CRE: { name: "Cresinion", type: "moon", bonus: 2 }, DEL: { name: "Delphot", type: "moon", bonus: 2 }, SAJ: { name: "Sajon", type: "moon", bonus: 3 },
};
// name: [type, region, x%, y%]  (percentages of the board photo; moon nodes are % of the moon inset)
const TDEF: Record<string, [TerrType, string, number, number]> = {
  "Northwestern Oil Emirate": ["land", "NA", 7.83, 9.54], "Nunavut": ["land", "NA", 16.97, 9.16], "Exiled States of America": ["land", "NA", 35.51, 8.78],
  "Alberta": ["land", "NA", 13.84, 16.03], "Canada": ["land", "NA", 19.84, 18.70], "Republique du Quebec": ["land", "NA", 26.63, 20.61],
  "Continental Biospheres": ["land", "NA", 13.05, 25.57], "American Republic": ["land", "NA", 19.06, 29.77], "Mexico": ["land", "NA", 12.79, 37.40],
  "Nuevo Timoto": ["land", "SA", 18.80, 51.76], "Andean Nations": ["land", "SA", 15.14, 58.40], "Amazon Desert": ["land", "SA", 21.93, 60.69], "Argentina": ["land", "SA", 14.88, 73.66],
  "Iceland GRC": ["land", "EU", 39.69, 15.27], "New Avalon": ["land", "EU", 39.95, 23.28], "Jotenheim": ["land", "EU", 49.35, 9.92], "Warsaw Republic": ["land", "EU", 47.52, 24.05],
  "Ukrayina": ["land", "EU", 59.53, 19.85], "Andorra": ["land", "EU", 39.95, 32.82], "Imperial Balkania": ["land", "EU", 50.13, 28.85],
  "Saharan Empire": ["land", "AF", 41.78, 47.33], "Egypt": ["land", "AF", 50.65, 43.51], "Ministry of Djibouti": ["land", "AF", 55.35, 52.29],
  "Zaire Military Zone": ["land", "AF", 50.91, 63.36], "Lesotho": ["land", "AF", 51.96, 76.34], "Madagascar": ["land", "AF", 61.36, 75.95],
  "Middle East": ["land", "AS", 61.88, 41.37], "Afghanistan": ["land", "AS", 67.36, 30.92], "Enclave of the Bear": ["land", "AS", 69.19, 17.94], "Siberia": ["land", "AS", 74.93, 9.16],
  "Sakha": ["land", "AS", 84.07, 8.78], "Pevek": ["land", "AS", 91.38, 9.92], "Alden": ["land", "AS", 82.25, 19.47], "Khan Industrial State": ["land", "AS", 83.03, 26.34],
  "Japan": ["land", "AS", 92.69, 31.68], "Hong Kong": ["land", "AS", 80.42, 35.11], "United Indiastan": ["land", "AS", 73.89, 44.27], "Angkhor Wat": ["land", "AS", 83.03, 47.71],
  "Java Cartel": ["land", "AU", 87.73, 63.74], "New Guinea": ["land", "AU", 96.34, 65.27], "Aboriginal League": ["land", "AU", 86.42, 82.06], "Australian Testing Ground": ["land", "AU", 93.99, 80.15],
  "Poseidon": ["water", "USP", 6.27, 21.76], "Hawaiian Preserve": ["water", "USP", 6.27, 33.97], "New Atlantis": ["water", "USP", 8.62, 45.80],
  "Neo Tokyo": ["water", "ASP", 93.73, 40.84], "Sung Tzu": ["water", "ASP", 94.26, 52.67],
  "Western Ireland": ["water", "NAT", 32.64, 32.06], "New York": ["water", "NAT", 25.33, 37.40], "Nova Brasilia": ["water", "NAT", 28.72, 50.00],
  "Neo Paulo": ["water", "SAT", 31.33, 69.47], "The Ivory Reef": ["water", "SAT", 39.95, 66.41],
  "South Ceylon": ["water", "IND", 69.97, 61.07], "Microcorp": ["water", "IND", 68.67, 73.66], "Akara": ["water", "IND", 76.50, 80.92],
  "Harpalus": ["moon", "CRE", 43, 11], "Sea of Rains": ["moon", "CRE", 44, 30], "Ocean of Storms": ["moon", "CRE", 31, 49], "Bay of Dew": ["moon", "CRE", 21, 34],
  "Aristotle": ["moon", "DEL", 62, 16], "Sea of Serenity": ["moon", "DEL", 64, 37], "Sea of Crisis": ["moon", "DEL", 74, 30], "Sea of Nectar": ["moon", "DEL", 77, 52],
  "Rhaeticus": ["moon", "SAJ", 54, 56], "Byrgius": ["moon", "SAJ", 21, 63], "Sea of Clouds": ["moon", "SAJ", 42, 64], "Straight Wall": ["moon", "SAJ", 64, 70],
  "Marsh of Diseases": ["moon", "SAJ", 32, 77], "Tycho": ["moon", "SAJ", 52, 79],
};
export const LANDING_SITES = ["Sea of Crisis", "Bay of Dew", "Tycho"];
const LINKS: [string, string][] = [
  // --- Earth, transcribed from the official board schematic -------------------
  ["Northwestern Oil Emirate", "Nunavut"], ["Northwestern Oil Emirate", "Alberta"], ["Northwestern Oil Emirate", "Pevek"],
  ["Nunavut", "Alberta"], ["Nunavut", "Canada"], ["Nunavut", "Exiled States of America"], ["Exiled States of America", "Republique du Quebec"], ["Exiled States of America", "Iceland GRC"],
  ["Alberta", "Canada"], ["Alberta", "Continental Biospheres"], ["Canada", "Republique du Quebec"], ["Canada", "Continental Biospheres"], ["Canada", "American Republic"],
  ["Republique du Quebec", "American Republic"], ["Continental Biospheres", "American Republic"], ["Continental Biospheres", "Mexico"], ["American Republic", "Mexico"], ["Mexico", "Nuevo Timoto"],
  ["Nuevo Timoto", "Andean Nations"], ["Nuevo Timoto", "Amazon Desert"], ["Andean Nations", "Amazon Desert"], ["Andean Nations", "Argentina"], ["Amazon Desert", "Argentina"], ["Amazon Desert", "Saharan Empire"],
  ["Iceland GRC", "New Avalon"], ["Iceland GRC", "Jotenheim"], ["New Avalon", "Jotenheim"], ["New Avalon", "Warsaw Republic"], ["New Avalon", "Andorra"], ["Jotenheim", "Warsaw Republic"], ["Jotenheim", "Ukrayina"],
  ["Warsaw Republic", "Andorra"], ["Warsaw Republic", "Imperial Balkania"], ["Warsaw Republic", "Ukrayina"], ["Ukrayina", "Imperial Balkania"], ["Ukrayina", "Enclave of the Bear"], ["Ukrayina", "Afghanistan"], ["Ukrayina", "Middle East"],
  ["Andorra", "Imperial Balkania"], ["Andorra", "Saharan Empire"], ["Imperial Balkania", "Middle East"], ["Imperial Balkania", "Egypt"], ["Imperial Balkania", "Saharan Empire"],
  ["Saharan Empire", "Egypt"], ["Saharan Empire", "Ministry of Djibouti"], ["Saharan Empire", "Zaire Military Zone"], ["Egypt", "Middle East"], ["Egypt", "Ministry of Djibouti"],
  ["Ministry of Djibouti", "Zaire Military Zone"], ["Ministry of Djibouti", "Lesotho"], ["Ministry of Djibouti", "Madagascar"], ["Zaire Military Zone", "Lesotho"], ["Lesotho", "Madagascar"],
  ["Middle East", "Afghanistan"], ["Middle East", "United Indiastan"], ["Afghanistan", "Enclave of the Bear"], ["Afghanistan", "Hong Kong"], ["Afghanistan", "United Indiastan"],
  ["Enclave of the Bear", "Siberia"], ["Enclave of the Bear", "Hong Kong"], ["Siberia", "Sakha"], ["Siberia", "Alden"], ["Siberia", "Khan Industrial State"], ["Siberia", "Hong Kong"],
  ["Sakha", "Pevek"], ["Sakha", "Alden"], ["Pevek", "Alden"], ["Pevek", "Khan Industrial State"], ["Pevek", "Japan"], ["Alden", "Khan Industrial State"],
  ["Khan Industrial State", "Japan"], ["Khan Industrial State", "Hong Kong"], ["Hong Kong", "Angkhor Wat"], ["Hong Kong", "United Indiastan"], ["United Indiastan", "Angkhor Wat"], ["Angkhor Wat", "Java Cartel"],
  ["Java Cartel", "New Guinea"], ["Java Cartel", "Aboriginal League"], ["New Guinea", "Aboriginal League"], ["New Guinea", "Australian Testing Ground"], ["Aboriginal League", "Australian Testing Ground"],
  // water colonies (the two Pacific links wrap around the edge of the board)
  ["Poseidon", "Continental Biospheres"], ["Poseidon", "Hawaiian Preserve"], ["Hawaiian Preserve", "New Atlantis"], ["New Atlantis", "Nuevo Timoto"],
  ["New Atlantis", "Neo Tokyo"], ["Neo Tokyo", "Japan"], ["Neo Tokyo", "Hong Kong"], ["Neo Tokyo", "Sung Tzu"], ["Sung Tzu", "Java Cartel"],
  ["Western Ireland", "New Avalon"], ["Western Ireland", "New York"], ["New York", "American Republic"], ["New York", "Nova Brasilia"], ["Nova Brasilia", "Amazon Desert"],
  ["Neo Paulo", "Amazon Desert"], ["Neo Paulo", "The Ivory Reef"], ["The Ivory Reef", "Saharan Empire"],
  ["South Ceylon", "United Indiastan"], ["South Ceylon", "Microcorp"], ["Microcorp", "Madagascar"], ["Microcorp", "Akara"], ["Akara", "Aboriginal League"],
  // --- the Moon, from the official lunar schematic -----------------------------
  ["Aristotle", "Harpalus"], ["Aristotle", "Sea of Crisis"], ["Aristotle", "Sea of Rains"], ["Aristotle", "Sea of Serenity"],
  ["Bay of Dew", "Byrgius"], ["Bay of Dew", "Harpalus"], ["Bay of Dew", "Ocean of Storms"], ["Bay of Dew", "Sea of Rains"],
  ["Byrgius", "Marsh of Diseases"], ["Byrgius", "Ocean of Storms"], ["Byrgius", "Sea of Clouds"], ["Harpalus", "Sea of Rains"],
  ["Marsh of Diseases", "Sea of Clouds"], ["Marsh of Diseases", "Tycho"], ["Ocean of Storms", "Sea of Clouds"], ["Ocean of Storms", "Sea of Rains"],
  ["Rhaeticus", "Sea of Clouds"], ["Rhaeticus", "Sea of Nectar"], ["Rhaeticus", "Sea of Rains"], ["Rhaeticus", "Sea of Serenity"], ["Rhaeticus", "Straight Wall"],
  ["Sea of Clouds", "Sea of Rains"], ["Sea of Clouds", "Straight Wall"], ["Sea of Clouds", "Tycho"],
  ["Sea of Crisis", "Sea of Nectar"], ["Sea of Crisis", "Sea of Serenity"], ["Sea of Nectar", "Sea of Serenity"], ["Sea of Nectar", "Straight Wall"],
  ["Sea of Rains", "Sea of Serenity"], ["Straight Wall", "Tycho"],
];
export const MAP: Record<string, MapNode> = {};
for (const [n, [type, region, x, y]] of Object.entries(TDEF)) MAP[n] = { name: n, type, region, x, y, adj: [], site: LANDING_SITES.includes(n) };
for (const [a, b] of LINKS) { MAP[a].adj.push(b); MAP[b].adj.push(a); }
export const NAMES = Object.keys(MAP);
export const CMDS: Commander[] = ["Land", "Diplomat", "Naval", "Nuclear", "Space"];
export const CMD_INITIAL: Record<Commander, string> = { Land: "L", Diplomat: "D", Naval: "N", Nuclear: "X", Space: "S" };

// ============================================================================
// DATA: the five base-game command decks (Command Card Summary)
// ============================================================================
type RawCard = [string, Commander, number, number, CardWhen, string, string, string, number?];
const RAW_CARDS: RawCard[] = [
  ["Cease Fire", "Diplomat", 2, 2, "react", "ceaseFire", "land", "Prevent the invasion. The attacker cannot attack any of your territories for the rest of their turn."],
  ["Colony Influence", "Diplomat", 4, 0, "score", "influence", "land", "If your Diplomat Commander is still alive, move your score marker ahead 3 spaces."],
  ["Decoys Revealed", "Diplomat", 2, 0, "before", "decoys", "land", "Move any number of your commanders to any number of territories you control."],
  ["Energy Crisis", "Diplomat", 2, 0, "before", "energyCrisis", "land", "Collect one energy from each opponent."],
  ["Evacuation", "Diplomat", 2, 0, "react", "evacuation", "land", "Move all units from the attacked territory to any territory you occupy."],
  ["MOD Reduction", "Diplomat", 2, 2, "before", "modReduction", "land", "All of your opponents must remove 4 MODs. Then you remove 2 MODs."],
  ["Redeployment", "Diplomat", 3, 0, "end", "redeployment", "land", "Take an extra free move this turn, after you have finished attacking."],
  ["Territorial Station", "Diplomat", 3, 1, "before", "terrStation", "land", "Place a Space Station on any land territory you occupy."],
  ["Assemble MODs", "Land", 3, 1, "before", "assemble", "land", "Place 3 MODs on any one land territory you occupy."],
  ["Colony Influence", "Land", 2, 0, "score", "influence", "land", "If your Land Commander is still alive, move your score marker ahead 3 spaces."],
  ["Frequency Jam", "Land", 2, 0, "before", "jam", "land", "Choose a player. The chosen player cannot play command cards during your turn."],
  ["Land Death Trap", "Land", 1, 3, "react", "deathTrap", "land", "Your opponent must destroy half the units in the invading territory. Round up."],
  ["Reinforcements", "Land", 3, 0, "before", "reinforce", "land", "Place 3 MODs, one each on 3 different land territories you occupy."],
  ["Scout Forces", "Land", 3, 0, "before", "scout", "land", "Draw a land territory card and keep it secret. When you occupy this territory, immediately place 5 MODs there."],
  ["Stealth MODs", "Land", 5, 0, "react", "stealth", "land", "Place 3 additional defending MODs in the defending land territory."],
  ["Stealth Station", "Land", 1, 0, "react", "stealthStation", "land", "Place a Space Station in the defending land territory."],
  ["Assemble MODs", "Naval", 3, 1, "before", "assemble", "water", "Place 3 MODs on any one water territory you occupy."],
  ["Colony Influence", "Naval", 2, 0, "score", "influence", "water", "If your Naval Commander is still alive, move your score marker ahead 3 spaces."],
  ["Frequency Jam", "Naval", 2, 0, "before", "jam", "water", "Choose a player. The chosen player cannot play command cards during your turn."],
  ["Hidden Energy", "Naval", 5, 0, "before", "hiddenEnergy", "water", "Draw a water territory card. If you occupy it at the end of your turn, collect 4 energy."],
  ["Reinforcements", "Naval", 2, 0, "before", "reinforce", "water", "Place 3 MODs, one each on 3 different water territories you occupy."],
  ["Stealth MODs", "Naval", 5, 0, "react", "stealth", "water", "Place 3 additional defending MODs in the defending water territory."],
  ["Water Death Trap", "Naval", 1, 3, "react", "deathTrap", "water", "Your opponent must destroy half the units in the invading territory. Round up."],
  ["Aqua Brother", "Nuclear", 1, 3, "before", "zone", "water", "Roll a 6-sided die. Destroy one unit in each territory of the water colony rolled (1-5; 6 = roll again)."],
  ["Assassin Bomb", "Nuclear", 3, 1, "before", "assassin", "land", "Choose an opponent's commander. Roll an 8-sided die: on a 3 or higher, destroy it."],
  ["Armageddon", "Nuclear", 1, 4, "before", "armageddon", "land", "Nuclear exchange: this turn your nuclear command cards cost no energy."],
  ["The Mother", "Nuclear", 1, 3, "before", "zone", "land", "Roll a 6-sided die. Destroy one unit in each territory of the continent rolled: 1 N.America, 2 S.America, 3 Europe, 4 Africa, 5 Asia, 6 Australia."],
  ["Nicky Boy", "Nuclear", 1, 3, "before", "zone", "moon", "Roll a 6-sided die. Destroy one unit in each territory of the lunar colony rolled: 1-2 Cresinion, 3-4 Delphot, 5-6 Sajon."],
  ["Rocket Strike Land", "Nuclear", 2, 2, "before", "rocket", "land", "Choose an opponent's land territory. Roll a 6-sided die; that many units are destroyed."],
  ["Rocket Strike Moon", "Nuclear", 2, 2, "before", "rocket", "moon", "Choose an opponent's lunar territory. Roll a 6-sided die; that many units are destroyed."],
  ["Rocket Strike Water", "Nuclear", 2, 2, "before", "rocket", "water", "Choose an opponent's water territory. Roll a 6-sided die; that many units are destroyed."],
  ["Scatter Bomb Land", "Nuclear", 3, 1, "before", "scatter", "land", "Turn over 3 land territory cards. Destroy half the opponents' units on those territories (round up).", 3],
  ["Scatter Bomb Moon", "Nuclear", 2, 1, "before", "scatter", "moon", "Turn over 2 lunar territory cards. Destroy half the opponents' units on those territories (round up).", 2],
  ["Scatter Bomb Water", "Nuclear", 2, 1, "before", "scatter", "water", "Turn over 2 water territory cards. Destroy half the opponents' units on those territories (round up).", 2],
  ["Assemble MODs", "Space", 3, 1, "before", "assemble", "moon", "Place 3 MODs on any one lunar territory you control."],
  ["Colony Influence", "Space", 2, 0, "score", "influence", "moon", "If your Space Commander is still alive, move your score marker ahead 3 spaces."],
  ["Energy Extraction", "Space", 1, 1, "before", "extraction", "moon", "If you occupy every territory of a lunar colony at the end of this turn, collect 7 energy."],
  ["Frequency Jam", "Space", 2, 0, "before", "jam", "moon", "Choose a player. The chosen player cannot play command cards during your turn."],
  ["Invade Earth", "Space", 3, 0, "before", "invadeEarth", "moon", "Turn over land territory cards until one you do not occupy. This turn you may attack it from any lunar territory."],
  ["Orbital Mines", "Space", 2, 2, "react", "deathTrap", "moon", "Your opponent must destroy half the units in the invading territory. Round up."],
  ["Reinforcements", "Space", 3, 0, "before", "reinforce", "moon", "Place 3 MODs, one each on 3 different lunar territories you occupy."],
  ["Stealth MODs", "Space", 4, 0, "react", "stealth", "moon", "Place 3 additional defending MODs in the defending lunar territory."],
];
export const CARD_DEFS: CardDef[] = RAW_CARDS.map((c, id) => ({ id, name: c[0], deck: c[1], copies: c[2], cost: c[3], when: c[4], kind: c[5], target: c[6], text: c[7], amount: c[8] || 0 }));
export const WHEN_TEXT: Record<CardWhen, string> = { before: "BEFORE FIRST INVASION", react: "OPPONENT INVADES", end: "END OF TURN", score: "END OF GAME" };

export const RULES = { defaultYears: 5, commanderCost: 3, stationCost: 5, maxStations: 4, cardCost: 1, maxCards: 4, startEnergy: 3, devastation: 4 };
