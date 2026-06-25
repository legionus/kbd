"use strict";

const catalogUrl = "data/models/catalog.json";
const keymapCatalogUrl = "data/keymaps/catalog.json";

const modifierWeights = new Map([
  ["Shift", 1],
  ["AltGr", 2],
  ["Control", 4],
  ["Alt", 8],
  ["ShiftL", 16],
  ["ShiftR", 32],
  ["CtrlL", 64],
  ["CtrlR", 128],
  ["CapsShift", 256],
]);

const modifierKeyIds = new Map([
  ["Shift", "LFSH"],
  ["AltGr", "RALT"],
  ["Control", "LCTL"],
  ["Alt", "LALT"],
  ["ShiftL", "LFSH"],
  ["ShiftR", "RTSH"],
  ["CtrlL", "LCTL"],
  ["CtrlR", "RCTL"],
  ["CapsShift", "CAPS"],
]);

const symbolAliases = new Map([
  ["space", "␠"],
  ["Tab", "⇥"],
  ["Escape", "Esc"],
  ["Delete", "⌦"],
  ["BackSpace", "⌫"],
  ["Return", "↵"],
  ["Linefeed", "↵"],
  ["Find", "Home"],
  ["Select", "End"],
  ["Prior", "PgUp"],
  ["Next", "PgDn"],
  ["Insert", "Ins"],
  ["Remove", "Del"],
  ["Backtab", "⇤"],
  ["KP_Multiply", "×"],
  ["KP_Divide", "÷"],
  ["KP_Subtract", "−"],
  ["KP_Add", "+"],
  ["KP_Enter", "↵"],
  ["KP_Period", "."],
  ["one", "1"],
  ["two", "2"],
  ["three", "3"],
  ["four", "4"],
  ["five", "5"],
  ["six", "6"],
  ["seven", "7"],
  ["eight", "8"],
  ["nine", "9"],
  ["zero", "0"],
  ["exclam", "!"],
  ["at", "@"],
  ["numbersign", "#"],
  ["dollar", "$"],
  ["percent", "%"],
  ["asciicircum", "^"],
  ["ampersand", "&"],
  ["asterisk", "*"],
  ["parenleft", "("],
  ["parenright", ")"],
  ["minus", "-"],
  ["underscore", "_"],
  ["equal", "="],
  ["plus", "+"],
  ["bracketleft", "["],
  ["bracketright", "]"],
  ["braceleft", "{"],
  ["braceright", "}"],
  ["backslash", "\\"],
  ["bar", "|"],
  ["semicolon", ";"],
  ["colon", ":"],
  ["apostrophe", "'"],
  ["quotedbl", "\""],
  ["grave", "`"],
  ["asciitilde", "~"],
  ["comma", ","],
  ["less", "<"],
  ["period", "."],
  ["greater", ">"],
  ["slash", "/"],
  ["question", "?"],
  ["Left", "←"],
  ["Right", "→"],
  ["Up", "↑"],
  ["Down", "↓"],
]);

const symbolPrefixes = [
  ["Meta_", "M-"],
  ["Control_", "C-"],
];

const state = {
  catalog: null,
  keymapCatalog: null,
  keymap: null,
  keymapByKeycode: new Map(),
  modelGroups: new Map(),
  keymapGroups: new Map(),
  keys: new Map(),
  selected: null,
  heldModifiers: new Map(),
  lockedModifiers: new Map(),
  lockActions: [],
};

const elements = {
  modelSelect: document.querySelector("#model-select"),
  keymapSelect: document.querySelector("#keymap-select"),
  lockSequences: document.querySelector("#lock-sequences"),
  keyboard: document.querySelector("#keyboard"),
};

async function main() {
  const [catalog, keymapCatalog] = await Promise.all([
    fetchJson(catalogUrl),
    fetchJson(keymapCatalogUrl),
  ]);
  state.catalog = catalog;
  state.keymapCatalog = keymapCatalog;

  for (const model of state.catalog.models) {
    const group = modelGroup(model);
    const option = document.createElement("option");
    option.value = model.id;
    option.textContent = model.name;
    group.append(option);
  }
  for (const keymap of state.keymapCatalog.keymaps) {
    const group = keymapGroup(keymap);
    const option = document.createElement("option");
    option.value = keymap.id;
    option.textContent = keymap.name;
    group.append(option);
  }

  elements.modelSelect.addEventListener("change", () => loadModel(elements.modelSelect.value));
  elements.keymapSelect.addEventListener("change", () => loadKeymap(elements.keymapSelect.value));
  await Promise.all([
    loadKeymap(defaultKeymapId()),
    loadModel(state.catalog.models[0].id),
  ]);
}

function modelGroup(model) {
  const label = model.group ?? "Other";
  let group = state.modelGroups.get(label);
  if (!group) {
    group = document.createElement("optgroup");
    group.label = label;
    elements.modelSelect.append(group);
    state.modelGroups.set(label, group);
  }
  return group;
}

function keymapGroup(keymap) {
  const label = keymap.group ?? "Other";
  let group = state.keymapGroups.get(label);
  if (!group) {
    group = document.createElement("optgroup");
    group.label = label;
    elements.keymapSelect.append(group);
    state.keymapGroups.set(label, group);
  }
  return group;
}

async function loadModel(modelId) {
  clearSelection();
  const entry = state.catalog.models.find((model) => model.id === modelId);
  if (!entry) {
    throw new Error(`unknown model: ${modelId}`);
  }

  elements.modelSelect.value = modelId;

  const base = new URL(catalogUrl, window.location.href);
  const [model, svg] = await Promise.all([
    fetchJson(new URL(entry.json, base)),
    fetchText(new URL(entry.svg, base)),
  ]);

  state.keys = new Map(model.keys.map((key) => [key.id, key]));
  elements.keyboard.innerHTML = svg;
  elements.keyboard.querySelectorAll("g[data-key-id]").forEach((node) => {
    node.addEventListener("click", () => pressKey(node.dataset.keyId));
  });
  updateLockActions();
  renderKeymapLegends();
  renderHeldModifiers();
}

async function loadKeymap(keymapId) {
  const entry = state.keymapCatalog.keymaps.find((keymap) => keymap.id === keymapId);
  if (!entry) {
    throw new Error(`unknown keymap: ${keymapId}`);
  }

  const base = new URL(keymapCatalogUrl, window.location.href);
  const keymap = await fetchJson(new URL(entry.json, base));
  state.keymap = keymap;
  state.keymapByKeycode = new Map(keymap.keys.map((key) => [key.kbd_keycode, key]));
  elements.keymapSelect.value = keymapId;
  clearHeldModifiers();
  clearSelection();
  updateLockActions();
  renderKeymapLegends();
  renderHeldModifiers();
}

function defaultKeymapId() {
  return state.keymapCatalog.keymaps.some((keymap) => keymap.id === "i386/qwerty/defkeymap")
    ? "i386/qwerty/defkeymap"
    : state.keymapCatalog.keymaps[0].id;
}

function pressKey(keyId) {
  const key = state.keys.get(keyId);
  const action = key ? actionForKey(key) : null;
  if (action) {
    applyKeyAction(key, action);
  }
  selectKey(keyId);
}

function selectKey(keyId) {
  clearSelection();

  const node = elements.keyboard.querySelector(`g[data-key-id="${CSS.escape(keyId)}"]`);
  if (!node) {
    return;
  }

  state.selected = node;
  node.classList.add("selected");
}

function clearSelection() {
  if (state.selected) {
    state.selected.classList.remove("selected");
  }
  state.selected = null;
}

function clearHeldModifiers() {
  state.heldModifiers.clear();
  state.lockedModifiers.clear();
}

function applyKeyAction(key, action) {
  const lock = lockModifierForSymbol(action.symbol);
  if (lock) {
    activateLockAction({lock});
    return;
  }

  const modifier = modifierWeights.get(action.symbol);
  if (modifier) {
    toggleHeldModifier(key, modifier);
  }
}

function toggleHeldModifier(key, modifier) {
  if (state.heldModifiers.has(key.id)) {
    state.heldModifiers.delete(key.id);
  } else {
    state.heldModifiers.set(key.id, modifier);
  }
  renderHeldModifiers();
  renderKeymapLegends();
}

function toggleLockedModifier(modifier) {
  const name = modifierName(modifier);
  if (state.lockedModifiers.has(name)) {
    state.lockedModifiers.delete(name);
  } else {
    state.lockedModifiers.set(name, modifier);
  }
}

function activateLockAction(action) {
  toggleLockedModifier(action.lock);
  state.heldModifiers.clear();
  renderHeldModifiers();
  renderKeymapLegends();
  renderLockSequences();
}

function updateLockActions() {
  state.lockActions = findLockActions();
  renderLockSequences();
}

function findLockActions() {
  if (!state.keymap || !state.keys.size) {
    return [];
  }

  const actions = [];
  const seen = new Set();
  for (const key of state.keys.values()) {
    if (key.kbd_keycode === null) {
      continue;
    }

    const keymapKey = state.keymapByKeycode.get(key.kbd_keycode);
    if (!keymapKey) {
      continue;
    }

    for (const entry of keymapKey.entries) {
      const lock = lockModifierForSymbol(entry.symbol);
      if (!lock) {
        continue;
      }

      const modifiers = decomposeKeymap(entry.keymap).filter((weight) => weight !== lock);
      const id = `${modifiers.join("+")}:${key.id}:${entry.symbol}`;
      if (seen.has(id)) {
        continue;
      }
      seen.add(id);
      actions.push({
        keyId: key.id,
        kbdKeycode: key.kbd_keycode,
        keymap: entry.keymap,
        lock,
        modifiers,
        symbol: entry.symbol,
      });
    }
  }

  return actions.sort((left, right) =>
    left.symbol.localeCompare(right.symbol)
    || left.keymap - right.keymap
    || left.keyId.localeCompare(right.keyId)
  );
}

function renderLockSequences() {
  elements.lockSequences.replaceChildren();
  if (!state.lockActions.length) {
    const empty = document.createElement("span");
    empty.classList.add("lock-sequences-empty");
    empty.textContent = "none";
    elements.lockSequences.append(empty);
    return;
  }

  for (const action of state.lockActions) {
    const sequence = [...modifierKeyIdsForWeights(action.modifiers), action.keyId];
    const chip = document.createElement("button");
    chip.type = "button";
    chip.classList.add("lock-sequence");
    if (state.lockedModifiers.has(modifierName(action.lock))) {
      chip.classList.add("active");
    }
    chip.textContent = sequence.join(" + ");
    chip.title = kbdSwitchSyntax(action);
    chip.addEventListener("click", () => activateLockAction(action));
    elements.lockSequences.append(chip);
  }
}

function kbdSwitchSyntax(action) {
  return [...kbdModifierNames(action.modifiers), "keycode", String(action.kbdKeycode)].join(" ");
}

function renderHeldModifiers() {
  elements.keyboard.querySelectorAll("g[data-key-id].held").forEach((node) => {
    node.classList.remove("held");
  });
  for (const keyId of state.heldModifiers.keys()) {
    const node = elements.keyboard.querySelector(`g[data-key-id="${CSS.escape(keyId)}"]`);
    node?.classList.add("held");
  }
}

function renderKeymapLegends() {
  const overlay = elements.keyboard.querySelector("#overlay-legends");
  if (!overlay || !state.keymap) {
    return;
  }

  overlay.replaceChildren();
  for (const key of state.keys.values()) {
    const entry = legendForKey(key);
    if (!entry || entry.symbol === "VoidSymbol") {
      continue;
    }

    const label = document.createElementNS("http://www.w3.org/2000/svg", "text");
    label.classList.add("keymap-legend");
    label.setAttribute("x", formatSvgNumber(key.x + key.w / 2));
    label.setAttribute("y", formatSvgNumber(key.y + key.h / 2));
    label.setAttribute("font-size", formatSvgNumber(legendFontSize(key, entry)));
    label.setAttribute("dominant-baseline", "middle");
    label.append(svgTitle(entry.symbol));
    label.append(document.createTextNode(displayEntry(entry)));
    if (key.rotation) {
      const [originX, originY] = key.rotation.origin;
      label.setAttribute("transform", `rotate(${key.rotation.angle} ${originX} ${originY})`);
    }
    overlay.append(label);
  }
}

function legendForKey(key) {
  if (key.kbd_keycode === null) {
    return null;
  }
  const keymapKey = state.keymapByKeycode.get(key.kbd_keycode);
  if (!keymapKey) {
    return null;
  }

  const activeKeymap = activeKeymapColumn();
  return (
    keymapKey.entries.find((entry) => entry.keymap === activeKeymap)
    ?? keymapKey.entries.find((entry) => entry.keymap === 0)
    ?? null
  );
}

function actionForKey(key) {
  return legendForKey(key);
}

function activeKeymapColumn() {
  const keymap = rawActiveKeymapColumn();
  return state.keymap?.keymaps.includes(keymap) ? keymap : 0;
}

function rawActiveKeymapColumn() {
  return activeModifierWeights().reduce((keymap, weight) => keymap + weight, 0);
}

function decomposeKeymap(keymap) {
  const weights = [];
  let remainder = keymap;
  for (const weight of modifierWeights.values()) {
    if (remainder & weight) {
      weights.push(weight);
      remainder -= weight;
    }
  }
  if (remainder) {
    weights.push(remainder);
  }
  return weights;
}

function heldModifierWeights() {
  return [...new Set(state.heldModifiers.values())];
}

function lockedModifierWeights() {
  return [...new Set(state.lockedModifiers.values())];
}

function activeModifierWeights() {
  return [...new Set([...heldModifierWeights(), ...lockedModifierWeights()])];
}

function modifierNames(weights) {
  return weights.map(modifierName);
}

function modifierKeyIdsForWeights(weights) {
  return modifierNames(weights).map((name) => modifierKeyIds.get(name) ?? name);
}

function kbdModifierNames(weights) {
  return modifierNames(weights).map((name) => name.toLowerCase());
}

function modifierName(weight) {
  for (const [name, value] of modifierWeights) {
    if (value === weight) {
      return name;
    }
  }
  return String(weight);
}

function lockModifierForSymbol(symbol) {
  if (!symbol.endsWith("_Lock")) {
    return null;
  }
  return modifierWeights.get(symbol.slice(0, -"_Lock".length)) ?? null;
}

function displayEntry(entry) {
  return entry.unicode ?? unicodeCharacter(entry) ?? compactSymbol(entry.symbol);
}

function unicodeCharacter(entry) {
  if (!entry.numeric.startsWith("U+")) {
    return null;
  }
  return String.fromCodePoint(entry.numeric_value);
}

function compactSymbol(symbol) {
  let value = symbol.startsWith("+") ? symbol.slice(1) : symbol;
  let prefix = "";
  let changed = true;
  while (changed) {
    changed = false;
    for (const [source, replacement] of symbolPrefixes) {
      if (value.startsWith(source)) {
        prefix += replacement;
        value = value.slice(source.length);
        changed = true;
      }
    }
  }
  return `${prefix}${aliasForSymbol(value) ?? value}`;
}

function aliasForSymbol(symbol) {
  return symbolAliases.get(symbol) ?? symbolAliases.get(symbol.toLowerCase()) ?? null;
}

function legendFontSize(key, entry) {
  const compact = displayEntry(entry);
  const maxSize = 0.28;
  const minSize = 0.12;
  const maxWidth = key.w * 0.8;
  const estimatedSize = maxWidth / Math.max(compact.length * 0.55, 1);
  return Math.max(minSize, Math.min(maxSize, estimatedSize));
}

function svgTitle(text) {
  const title = document.createElementNS("http://www.w3.org/2000/svg", "title");
  title.textContent = text;
  return title;
}

async function fetchJson(url) {
  const response = await fetch(url);
  if (!response.ok) {
    throw new Error(`${url}: ${response.status}`);
  }
  return response.json();
}

async function fetchText(url) {
  const response = await fetch(url);
  if (!response.ok) {
    throw new Error(`${url}: ${response.status}`);
  }
  return response.text();
}

function formatSvgNumber(value) {
  return Number(value).toFixed(3).replace(/\.?0+$/, "");
}

main().catch((error) => {
  console.error(error);
});
