/**
 * Evaluates the P palette and SPRITES object from the JSX file and writes
 * sprites_data.json — an array of { name, w, h, pixels: [[x,y,"#rrggbb"],...] }
 *
 * Handles all pixel formats:
 *   [x, y, P.KEY]         – palette reference (resolved to hex)
 *   [x, y, "#rrggbb"]     – literal hex string
 *   ...Array.from(...)    – programmatically generated rows
 */
const fs   = require('fs');
const path = require('path');

const jsxPath  = path.resolve(__dirname, '..', 'assets', 'lights_out_christmas_sprites_v3.jsx');
const outPath  = path.resolve(__dirname, 'sprites_data.json');

const content = fs.readFileSync(jsxPath, 'utf8');

// ── Extract source blocks ────────────────────────────────────────────────────
function sliceBlock(src, startToken) {
  const start = src.indexOf(startToken);
  if (start === -1) throw new Error(`Token not found: ${startToken}`);
  let depth = 0, inStr = false, strCh = '';
  for (let i = start; i < src.length; i++) {
    const ch = src[i];
    if (inStr) {
      if (ch === strCh && src[i - 1] !== '\\') inStr = false;
    } else if (ch === '"' || ch === "'") {
      inStr = true; strCh = ch;
    } else if (ch === '{') {
      depth++;
    } else if (ch === '}') {
      depth--;
      if (depth === 0) return src.slice(start, i + 1);
    }
  }
  throw new Error('Unmatched braces for ' + startToken);
}

const pCode       = sliceBlock(content, 'const P = {').replace(/^const /, 'var ');
const spritesCode = sliceBlock(content, 'const SPRITES = {').replace(/^const /, 'var ');

// ── Evaluate in an isolated function scope ───────────────────────────────────
// Using Function() so the var declarations are local but accessible via return.
const TRANSPARENT = '#060914'; // P._ – skipped in the original renderer

const fn = new Function(`
  ${pCode}
  ${spritesCode}

  // The React renderer skips pixels where c === P._ or !c
  const SKIP = P._;

  const result = [];
  for (const [name, spr] of Object.entries(SPRITES)) {
    const pixels = [];
    for (const entry of spr.pixels) {
      const [x, y, c] = entry;
      if (!c || c === SKIP) continue;
      pixels.push([x, y, c]);
    }
    result.push({ name, w: spr.w, h: spr.h, pixels });
  }
  return JSON.stringify(result);
`);

const json = fn();
fs.writeFileSync(outPath, json);

const sprites = JSON.parse(json);
console.log(`Extracted ${sprites.length} sprites → ${outPath}`);
sprites.forEach(s => process.stdout.write(`  ${s.name}: ${s.w}x${s.h}  (${s.pixels.length} px)\n`));
