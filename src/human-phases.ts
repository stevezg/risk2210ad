import { CMDS, MAP, NAMES, RULES } from "./data";
import { S, T, P, units, owned, hasCmd, stations, canInvade, fortifyPath, ui, drawCard, gameEnded } from "./state";
import { $, HL, clearHL, fx, log, render, setPrompt, setStatus } from "./render";
import { userInput } from "./ui-events";
import { declareInvasion, moveIn, attack } from "./combat";
import { playCard } from "./cards";
import type { Button, Commander } from "./types";

export async function humanRecruit(p: number) {
  const pl = P(p); ui.mode = "recruit"; ui.selected = null; ui.hireType = null; ui.cart = [];
  while (true) {
    if (gameEnded()) break;
    render();
    const btns: Button[] = [];
    if (pl.pool > 0) {
      btns.push({ id: "plus5", label: "+5 on selected", disabled: !ui.selected || pl.pool < 5 }, { id: "all", label: "All on selected", disabled: !ui.selected });
      setPrompt("RECRUIT", `${pl.pool} MODs to deploy. Click a territory you control (+1).`, btns);
    } else {
      for (const c of CMDS) btns.push({ id: "hire:" + c, label: `Hire ${c}`, disabled: hasCmd(p, c) || pl.energy < RULES.commanderCost, cls: ui.hireType === c ? "toggled" : "" });
      btns.push({ id: "hire:station", label: "Space Station (5E)", disabled: pl.energy < RULES.stationCost || stations(p) >= RULES.maxStations, cls: ui.hireType === "station" ? "toggled" : "" });
      for (const c of CMDS) {
        const inCart = ui.cart.filter((x) => x === c).length;
        btns.push({
          id: "buy:" + c, label: `+${c} card (${S!.decks[c].length})`, cls: "small",
          disabled: !hasCmd(p, c) || inCart >= S!.decks[c].length || ui.cart.length + pl.cardsBought >= RULES.maxCards || ui.cart.length >= pl.energy,
        });
      }
      if (ui.cart.length) btns.push({ id: "buy", label: `Buy ${ui.cart.length}: ${ui.cart.map((c) => c[0]).join(" ")}`, cls: "primary" }, { id: "clear", label: "Clear", cls: "small" });
      btns.push({ id: "end", label: "End recruitment", cls: "primary" });
      setPrompt("RECRUIT", `Energy ${pl.energy}. Hire (3E) / station (5E): pick an option, then click a territory. Cards cost 1E (max 4). Glowing cards can be played now.`, btns);
    }
    const a = await userInput();
    if (a.type === "terr") {
      const n = a.name;
      if (T(n).owner !== p) { setStatus("Choose a territory you control"); continue; }
      if (pl.pool > 0) { ui.selected = n; HL.selected = n; T(n).mods++; pl.pool--; fx.flash(n); }
      else if (ui.hireType === "station") { if (T(n).station) setStatus("Already has a station"); else { pl.energy -= RULES.stationCost; T(n).station = true; ui.hireType = null; clearHL(); log(`${pl.name} builds a Space Station in ${n}`); } }
      else if (ui.hireType) { pl.energy -= RULES.commanderCost; T(n).cmd[ui.hireType as Commander] = true; log(`${pl.name} hires a ${ui.hireType} Commander in ${n}`); ui.hireType = null; clearHL(); }
      else { ui.selected = n; HL.selected = n; }
    } else if (a.type === "btn") {
      if (a.id === "plus5" || a.id === "all") { const k = a.id === "all" ? pl.pool : Math.min(5, pl.pool); T(ui.selected!).mods += k; pl.pool -= k; }
      else if (a.id.startsWith("hire:")) { const h = a.id.slice(5); ui.hireType = ui.hireType === h ? null : h; clearHL(); if (ui.hireType) for (const n of owned(p)) if (h !== "station" || (MAP[n].type === "land" && !T(n).station)) HL.option.add(n); }
      else if (a.id.startsWith("buy:")) ui.cart.push(a.id.slice(4));
      else if (a.id === "clear") ui.cart = [];
      else if (a.id === "buy") {
        let bought = 0;
        for (const c of ui.cart) { const id = drawCard(c); if (id === null) continue; pl.hand.push(id); pl.energy -= RULES.cardCost; pl.cardsBought++; bought++; }
        log(`${pl.name} buys ${bought} command card(s)`); ui.cart = [];
      } else if (a.id === "end") break;
    } else if (a.type === "card") await playCard(p, a.idx, "before");
  }
  clearHL(); ui.mode = "idle";
}

export async function humanAttack(p: number) {
  if (gameEnded()) return;
  const pl = P(p); ui.mode = "attack"; ui.selected = null; ui.moveTarget = null; S!.phase = "ATTACK"; clearHL();
  while (true) {
    if (gameEnded()) break;
    render();
    const inv = S!.inv;
    if (!inv) {
      setPrompt("ATTACK", ui.selected ? `From ${ui.selected}: click an orange target.` : "Click one of your territories to attack from, then a highlighted target. Cards can still be played until your first invasion.", [{ id: "end", label: "End attacks", cls: "primary" }]);
      const a = await userInput();
      if (a.type === "btn" && a.id === "end") break;
      if (a.type === "card") { await playCard(p, a.idx, "before"); continue; }
      if (a.type !== "terr") continue;
      if (T(a.name).owner === p) { ui.selected = a.name; clearHL(); HL.selected = a.name; for (const n of NAMES) if (!canInvade(p, a.name, n)) HL.target.add(n); setStatus(""); }
      else if (ui.selected) { const why = canInvade(p, ui.selected, a.name); if (why) setStatus(why); else { clearHL(); await declareInvasion(p, ui.selected, a.name); } }
    } else if (!inv.captured) {
      const maxD = Math.min(3, units(inv.from) - 1);
      setPrompt("COMBAT", `${inv.from} (${units(inv.from)}) vs ${inv.to} (${units(inv.to)}). Roll up to ${maxD} dice.`, [
        ...[1, 2, 3].map((d) => ({ id: "roll" + d, label: "Roll " + d, cls: "danger", disabled: d > maxD })), { id: "retreat", label: "Retreat", disabled: !inv.attackedOnce }]);
      const a = await userInput(); if (a.type !== "btn") continue;
      if (a.id === "retreat") { S!.inv = null; HL.attack = HL.defend = null; ui.selected = null; $("combat").classList.remove("show"); }
      else await attack(p, +a.id.slice(4));
    } else {
      const f = T(inv.from);
      const optional = CMDS.filter((c) => f.cmd[c] && !inv.mustMove.includes(c));
      const mandatoryCount = inv.mustMove.filter((c) => f.cmd[c]).length;
      if (ui.moveTarget !== inv.to) { ui.moveTarget = inv.to; ui.moveCmds = Object.fromEntries(optional.map((c) => [c, true])); ui.moveMods = null; }
      const chosenOptional = optional.filter((c) => ui.moveCmds[c]).length;
      const totalCmd = mandatoryCount + chosenOptional;
      const maxTotal = units(inv.from) - 1, minTotal = Math.max(1, Math.min(inv.lastDice, maxTotal));
      const maxMods = Math.max(0, Math.min(f.mods, maxTotal - totalCmd));
      const minMods = Math.max(0, minTotal - totalCmd);
      if (ui.moveMods === null || ui.moveMods > maxMods) ui.moveMods = maxMods;
      if (ui.moveMods < minMods) ui.moveMods = minMods;
      const btns: Button[] = [
        { id: "mv-5", label: "-5 MODs", disabled: ui.moveMods - 5 < minMods },
        { id: "mv-1", label: "-1 MOD", disabled: ui.moveMods <= minMods },
        { id: "mv+1", label: "+1 MOD", disabled: ui.moveMods >= maxMods },
        { id: "mv+5", label: "+5 MODs", disabled: ui.moveMods + 5 > maxMods },
      ];
      for (const c of optional) btns.push({ id: "mv-cmd:" + c, label: (ui.moveCmds[c] ? "✓ " : "") + c, cls: ui.moveCmds[c] ? "toggled" : "", tip: ui.moveCmds[c] ? `${c} Commander will advance into ${inv.to}.` : `${c} Commander stays behind in ${inv.from}.` });
      btns.push({ id: "mv-go", label: `Move in ${ui.moveMods + totalCmd} unit(s)`, cls: "primary" });
      const mandatoryTxt = mandatoryCount ? ` ${inv.mustMove.filter((c) => f.cmd[c]).join(", ")} rolled 8s and must advance.` : "";
      setPrompt("TERRITORY CAPTURED", `${inv.to} is yours! Choose MODs and which commanders come along (min ${minTotal}, max ${maxTotal} total).${mandatoryTxt}`, btns);
      const a = await userInput();
      if (a.type === "btn") {
        if (a.id === "mv-5") ui.moveMods = Math.max(minMods, ui.moveMods - 5);
        else if (a.id === "mv-1") ui.moveMods = Math.max(minMods, ui.moveMods - 1);
        else if (a.id === "mv+1") ui.moveMods = Math.min(maxMods, ui.moveMods + 1);
        else if (a.id === "mv+5") ui.moveMods = Math.min(maxMods, ui.moveMods + 5);
        else if (a.id.startsWith("mv-cmd:")) { const c = a.id.slice(7) as Commander; ui.moveCmds[c] = !ui.moveCmds[c]; }
        else if (a.id === "mv-go") {
          moveIn(p, ui.moveMods + totalCmd, ui.moveCmds);
          ui.moveTarget = null;
          ui.selected = inv.to; clearHL(); HL.selected = inv.to; for (const m of NAMES) if (!canInvade(p, inv.to, m)) HL.target.add(m);
        }
      }
    }
  }
  if (S!.inv && !S!.inv.captured) S!.inv = null;
  if (S!.inv && S!.inv.captured) moveIn(p, Math.max(1, Math.min(S!.inv.lastDice, units(S!.inv.from) - 1)));
  $("combat").classList.remove("show"); HL.attack = HL.defend = null; clearHL(); ui.mode = "idle";
}

export async function humanFortify(p: number) {
  if (gameEnded()) return;
  const pl = P(p); ui.mode = "fortify"; S!.phase = "FORTIFY"; ui.selected = null; ui.fortDest = null; clearHL();
  let moves = 0;
  while (true) {
    if (gameEnded()) break;
    render();
    if (!ui.selected) {
      setPrompt("FORTIFY", "One free move along a chain of your territories (Space Station <-> landing sites count). Click the source territory, or skip.", [{ id: "skip", label: moves ? "End turn" : "Skip fortify", cls: "primary" }]);
      const a = await userInput();
      if (a.type === "btn") break;
      if (a.type === "card") { await playCard(p, a.idx, "end"); continue; }
      if (a.type !== "terr" || T(a.name).owner !== p) continue;
      ui.selected = a.name; clearHL(); HL.selected = a.name;
      for (const n of owned(p)) if (n !== a.name && fortifyPath(p, a.name, n)) HL.fortify.add(n);
    } else if (!ui.fortDest) {
      setPrompt("FORTIFY", `From ${ui.selected}: click a blue destination, or pick a different source.`, [{ id: "skip", label: "Cancel", cls: "small" }]);
      const a = await userInput();
      if (a.type === "btn") { ui.selected = null; clearHL(); continue; }
      if (a.type === "card") { await playCard(p, a.idx, "end"); continue; }
      if (a.type !== "terr" || T(a.name).owner !== p) continue;
      if (a.name === ui.selected) continue;
      if (!fortifyPath(p, ui.selected, a.name)) { setStatus("No path of friendly territories connects them"); continue; }
      const f = T(ui.selected);
      if (units(ui.selected) - 1 < 1) { setStatus("Nothing to move"); continue; }
      ui.fortDest = a.name;
      ui.fortCmds = Object.fromEntries(CMDS.filter((c) => f.cmd[c]).map((c) => [c, false]));
      ui.fortMods = null;
    } else {
      const f = T(ui.selected), to = ui.fortDest;
      const cmdsPresent = CMDS.filter((c) => f.cmd[c]);
      const chosenCmd = cmdsPresent.filter((c) => ui.fortCmds[c]).length;
      const maxTotal = units(ui.selected) - 1;
      const maxMods = Math.max(0, Math.min(f.mods, maxTotal - chosenCmd));
      if (ui.fortMods === null || ui.fortMods > maxMods) ui.fortMods = maxMods;
      const total = ui.fortMods + chosenCmd;
      const btns: Button[] = [
        { id: "ft-5", label: "-5 MODs", disabled: ui.fortMods - 5 < 0 },
        { id: "ft-1", label: "-1 MOD", disabled: ui.fortMods <= 0 },
        { id: "ft+1", label: "+1 MOD", disabled: ui.fortMods >= maxMods },
        { id: "ft+5", label: "+5 MODs", disabled: ui.fortMods + 5 > maxMods },
      ];
      for (const c of cmdsPresent) btns.push({ id: "ft-cmd:" + c, label: (ui.fortCmds[c] ? "✓ " : "") + c, cls: ui.fortCmds[c] ? "toggled" : "", tip: ui.fortCmds[c] ? `${c} Commander will move to ${to}.` : `${c} Commander stays in ${ui.selected}.` });
      btns.push({ id: "ft-go", label: `Move ${total} unit(s)`, cls: "primary", disabled: total < 1 }, { id: "ft-back", label: "Back", cls: "small" });
      setPrompt("FORTIFY", `${ui.selected} → ${to}: choose MODs and which commanders come along.`, btns);
      const a = await userInput();
      if (a.type !== "btn") continue;
      if (a.id === "ft-back") { ui.fortDest = null; continue; }
      if (a.id === "ft-5") ui.fortMods = Math.max(0, ui.fortMods - 5);
      else if (a.id === "ft-1") ui.fortMods = Math.max(0, ui.fortMods - 1);
      else if (a.id === "ft+1") ui.fortMods = Math.min(maxMods, ui.fortMods + 1);
      else if (a.id === "ft+5") ui.fortMods = Math.min(maxMods, ui.fortMods + 5);
      else if (a.id.startsWith("ft-cmd:")) { const c = a.id.slice(7) as Commander; ui.fortCmds[c] = !ui.fortCmds[c]; }
      else if (a.id === "ft-go") {
        let moved = ui.fortMods; f.mods -= ui.fortMods; T(to!).mods += ui.fortMods;
        for (const c of cmdsPresent) if (ui.fortCmds[c]) { delete f.cmd[c]; T(to!).cmd[c] = true; moved++; }
        log(`${pl.name} fortifies ${moved} unit(s) from ${ui.selected} to ${to}`); await fx.orb(ui.selected!, to!, "#ff8090"); moves++;
        clearHL(); ui.selected = null; ui.fortDest = null;
        if (pl.extraFortify > 0) { pl.extraFortify--; setStatus("Redeployment: you may fortify again"); continue; }
        break;
      }
    }
  }
  clearHL(); ui.mode = "idle";
}
