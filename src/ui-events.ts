import type { ModalOption, UIAction } from "./types";
import { P, rnd, ui } from "./state";
import { $, HL, clearHL, render, setPrompt } from "./render";

export function userInput(): Promise<UIAction> { return new Promise((r) => { ui.resolve = r; }); }
export function emit(a: UIAction) { if (ui.resolve) { const r = ui.resolve; ui.resolve = null; r(a); } }

export function modal<T>(title: string, body: string, buttons: ModalOption<T>[]): Promise<T> {
  return new Promise((res) => {
    $("modal-title").textContent = title; $("modal-body").textContent = body;
    const box = $("modal-btns"); box.innerHTML = "";
    for (const b of buttons) {
      const el = document.createElement("button"); el.textContent = b.label; if (b.cls) el.className = b.cls;
      el.addEventListener("click", () => { $("modal-bg").classList.remove("show"); res(b.value); });
      box.appendChild(el);
    }
    $("modal-bg").classList.add("show");
  });
}
export function modalNumber(title: string, body: string, lo: number, hi: number, def: number): Promise<number> {
  return new Promise((res) => {
    $("modal-title").textContent = title; $("modal-body").textContent = body;
    const box = $("modal-btns"); box.innerHTML = `<input type="number" id="modal-num" min="${lo}" max="${hi}" value="${def}"> `;
    const ok = document.createElement("button"); ok.textContent = "CONFIRM"; ok.className = "primary";
    ok.addEventListener("click", () => { const v = Math.max(lo, Math.min(hi, parseInt(($("modal-num") as HTMLInputElement).value) || lo)); $("modal-bg").classList.remove("show"); res(v); });
    box.appendChild(ok); $("modal-bg").classList.add("show"); setTimeout(() => ($("modal-num") as HTMLInputElement).focus(), 50);
  });
}
/** Human picks one of `options` (territory names) by clicking; bots use `botPick`. */
export async function choose(p: number, options: string[], text: string, botPick?: (opts: string[]) => string): Promise<string | null> {
  if (!options.length) return null;
  if (P(p).bot) return botPick ? botPick(options) : options[rnd(options.length)];
  const prevMode = ui.mode; ui.mode = "choose";
  clearHL(); for (const o of options) HL.option.add(o); render();
  setPrompt("CHOOSE A TERRITORY", text, []);
  while (true) { const a = await userInput(); if (a.type === "terr" && options.includes(a.name)) { ui.mode = prevMode; clearHL(); render(); return a.name; } }
}
export async function choosePlayer(p: number, options: number[], text: string): Promise<number | null> {
  if (!options.length) return null;
  if (P(p).bot) return options.reduce((b, q) => P(q).hand.length > P(b).hand.length ? q : b, options[0]);
  return modal<number>("CHOOSE A PLAYER", text, options.map((q) => ({ label: P(q).name, value: q })));
}
