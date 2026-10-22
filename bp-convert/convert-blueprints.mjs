#!/usr/bin/env node
// Converts Satisfactory blueprints between map types (normal, PEP/Skyblock, Random World
// Generator) by rewriting the level name inside them and swapping the matching name prefix.
import fs from 'fs';
import path from 'path';
import readline from 'readline';
import {Parser} from '@etothepii/satisfactory-file-parser';

// The game stamps every object and connection path with the level name of the map it was
// saved in, and does not translate it on load, so a mismatch drops wires and some belts.
// Adding a map type takes one entry here: its level name and the file-name prefix it gets.
const FORMATS = {
  normal: {level: 'Persistent_Level', prefix: '', label: 'vanilla and SatisfactoryPlus maps'},
  pep: {level: 'SkyblockLevel', prefix: 'PEP - ', label: 'Platform Expansion Program and Skyblock maps'},
  rwg: {level: 'ProceduralWorldLevel', prefix: 'RWG - ', label: 'Random World Generator maps'},
};

const USAGE = `usage: node convert-blueprints.mjs <blueprint-folder> --to ${Object.keys(FORMATS).join('|')} [options] [name ...]

${Object.entries(FORMATS).map(([k, f]) =>
  `  --to ${k.padEnd(9)} ${f.label}: ${f.level}, ${f.prefix ? `"${f.prefix}" prefix` : 'no prefix'}`).join('\n')}
  --replace      move the originals to a backup folder (default keeps them in place)
  --dry-run      list what would change, write nothing
  name ...       only these blueprints (file name without .sbp); default is the whole folder`;

const args = process.argv.slice(2);
const flag = name => args.includes(name);
const toIndex = args.indexOf('--to');
const target = toIndex >= 0 ? args[toIndex + 1] : undefined;
const positional = args.filter((a, i) => !a.startsWith('--') && i !== toIndex + 1);
const [dirArg, ...onlyNames] = positional;
if (!dirArg || !FORMATS[target]) {
  console.error(USAGE);
  process.exit(2);
}
const dryRun = flag('--dry-run');
const replace = flag('--replace');
const srcDir = path.resolve(dirArg);
const backupDir = `${srcDir.replace(/[\\/]+$/, '')}-backup-before-${target}`;

// The parser takes ArrayBuffers; Node Buffers can share a larger pool, so slice out the exact bytes.
const toArrayBuffer = b => b.buffer.slice(b.byteOffset, b.byteOffset + b.byteLength);

// Windows (and the game under Proton) treats names differing only in case as one file.
const sameFile = (a, b) => a.toLowerCase() === b.toLowerCase();
const PREFIXES = Object.values(FORMATS).map(f => f.prefix).filter(Boolean);
// Removes one known map prefix, matched without case.
const stripPrefix = name => {
  const found = PREFIXES.find(p => name.toLowerCase().startsWith(p.toLowerCase()));
  return found ? name.slice(found.length) : name;
};
// Writes the target prefix in its listed case and keeps the rest of the name as it was.
const targetName = name => FORMATS[target].prefix + stripPrefix(name);

// Returns a FORMATS key, 'mixed' or 'unknown' from the level names of the blueprint objects.
function detectFormat(bp) {
  const levels = new Set(bp.objects.map(o => o.rootObject));
  const known = Object.keys(FORMATS).filter(f => levels.has(FORMATS[f].level));
  if (known.length === 1 && levels.size === 1) return known[0];
  return known.length > 1 ? 'mixed' : 'unknown';
}

// Swaps the level name in every string of the parsed objects and returns the replacement count.
function renameLevel(node, from, to) {
  let count = 0;
  const walk = obj => {
    if (!obj || typeof obj !== 'object') return;
    for (const key of Object.keys(obj)) {
      const value = obj[key];
      if (typeof value !== 'string') { walk(value); continue; }
      if (value === from) { obj[key] = to; count++; }
      else if (value.startsWith(from + ':')) { obj[key] = to + value.slice(from.length); count++; }
    }
  };
  walk(node);
  return count;
}

// Counts object references that point at an object missing from the blueprint.
function countDanglingRefs(bp) {
  const names = new Set(bp.objects.map(o => o.instanceName));
  let bad = 0;
  const walk = obj => {
    if (!obj || typeof obj !== 'object') return;
    if (typeof obj.pathName === 'string' && /:PersistentLevel\./.test(obj.pathName) && !names.has(obj.pathName)) bad++;
    for (const key in obj) walk(obj[key]);
  };
  bp.objects.forEach(o => walk(o.properties));
  return bad;
}

function parse(name, sbp, cfg) {
  return Parser.ParseBlueprintFiles(name, toArrayBuffer(sbp), toArrayBuffer(cfg));
}

const allNames = fs.readdirSync(srcDir).filter(f => f.endsWith('.sbp')).map(f => f.slice(0, -4)).sort();

// Builds the converted files for one blueprint, or returns a reason it is skipped.
function plan(base) {
  const sbpIn = fs.readFileSync(path.join(srcDir, base + '.sbp'));
  const cfgIn = fs.readFileSync(path.join(srcDir, base + '.sbpcfg'));
  const bp = parse(base, sbpIn, cfgIn);
  const format = detectFormat(bp);
  const outBase = targetName(base);

  if (format === 'mixed' || format === 'unknown') return {base, skip: `level names are ${format}`};
  if (format === target && outBase === base) return {base, skip: 'already converted'};
  const clash = allNames.find(n => n !== base && sameFile(n, outBase));
  if (clash) return {base, skip: `"${clash}" already exists`};

  // Already in the target format but named wrong: rename only, contents untouched.
  if (format === target) return {base, outBase, replaced: 0, sbp: sbpIn, cfg: cfgIn};

  const objectCount = bp.objects.length;
  const danglingBefore = countDanglingRefs(bp);
  const from = FORMATS[format].level;
  const replaced = renameLevel(bp.objects, from, FORMATS[target].level);

  const chunks = [];
  let header;
  const written = Parser.WriteBlueprintFiles(bp, h => { header = h; }, c => chunks.push(c));
  const sbp = Buffer.concat([Buffer.from(header), ...chunks.map(c => Buffer.from(c))]);
  const cfg = Buffer.from(written.configFileBinary);

  // Re-read the output: same object count, only the target level name, no added dangling references.
  const check = parse(outBase, sbp, cfg);
  if (check.objects.length !== objectCount) throw new Error('object count changed');
  if (detectFormat(check) !== target) throw new Error('level names did not convert cleanly');
  if (JSON.stringify(check).includes(from)) throw new Error('source level name remains');
  if (countDanglingRefs(check) > danglingBefore) throw new Error('conversion added dangling references');
  return {base, outBase, replaced, sbp, cfg};
}

function ask(question) {
  const rl = readline.createInterface({input: process.stdin, output: process.stdout});
  return new Promise(resolve => rl.question(question, a => { rl.close(); resolve(a.trim().toLowerCase()); }));
}

// Writes via a temp file and rename so an interrupted run never leaves a half-written blueprint.
function writeAtomic(target, data) {
  const tmp = target + '.tmp';
  fs.writeFileSync(tmp, data);
  fs.renameSync(tmp, target);
}

const missing = onlyNames.filter(n => !allNames.includes(n));
if (missing.length) {
  console.error('Not found in folder:\n' + missing.map(n => '  ' + n).join('\n'));
  process.exit(1);
}
const names = onlyNames.length ? onlyNames : allNames;
console.log(`Checking ${names.length} blueprint(s) in ${srcDir} for conversion to ${target} ...`);

const jobs = [];
const skipped = [];
const failed = [];
const claimed = new Set();
for (const base of names) {
  try {
    const job = plan(base);
    if (job.skip) { skipped.push(job); continue; }
    // Two sources can map to one name, e.g. "PEP - X" and "RWG - X" both converting to normal.
    if (claimed.has(job.outBase.toLowerCase())) { skipped.push({base, skip: `"${job.outBase}" is produced twice`}); continue; }
    claimed.add(job.outBase.toLowerCase());
    jobs.push(job);
  } catch (e) {
    failed.push(`${base}: ${e.message}`);
  }
}

const already = skipped.filter(s => s.skip === 'already converted').length;
const otherSkips = skipped.filter(s => s.skip !== 'already converted');
console.log(`${jobs.length} to convert, ${already} already ${target}, ${otherSkips.length} skipped, ${failed.length} failed.`);
jobs.forEach(j => console.log(`  ${j.replaced ? String(j.replaced).padStart(5) + ' refs' : ' rename   '}  ${j.base}  ->  ${j.outBase}`));
if (otherSkips.length) console.log('Skipped:\n' + otherSkips.map(s => `  ${s.base}: ${s.skip}`).join('\n'));
if (failed.length) console.log('Failed (left untouched):\n' + failed.map(f => '  ' + f).join('\n'));
if (jobs.length === 0) process.exit(0);
if (dryRun) {
  console.log('Dry run: nothing was changed.');
  process.exit(0);
}

// A job that keeps its name (ignoring case) overwrites the original in place, so it is always backed up.
const backups = jobs.filter(j => replace || sameFile(j.outBase, j.base));
for (const j of backups) {
  for (const ext of ['.sbp', '.sbpcfg']) {
    if (fs.existsSync(path.join(backupDir, j.base + ext))) {
      console.error(`Backup already holds "${j.base}${ext}" in ${backupDir}. Move it first.`);
      process.exit(1);
    }
  }
}

console.log('\nClose Satisfactory first; the game rewrites blueprint folders when it exits.');
const action = (replace
  ? `Move ${backups.length} original(s) to ${backupDir} and write ${jobs.length} converted blueprint(s)?`
  : `Write ${jobs.length} converted blueprint(s) next to the originals?`)
  + (!replace && backups.length ? ` ${backups.length} keep their name and are backed up to ${backupDir} first.` : '');
const answer = await ask(action + ' [y/N] ');
if (answer !== 'y' && answer !== 'yes') {
  console.log('Cancelled: nothing was changed.');
  process.exit(0);
}

if (backups.length) {
  fs.mkdirSync(backupDir, {recursive: true});
  for (const j of backups) {
    for (const ext of ['.sbp', '.sbpcfg']) {
      fs.copyFileSync(path.join(srcDir, j.base + ext), path.join(backupDir, j.base + ext), fs.constants.COPYFILE_EXCL);
    }
  }
}
for (const j of jobs) {
  // A case-only rename removes the old name first; on Windows both names are the same file.
  if (sameFile(j.outBase, j.base) && j.outBase !== j.base) {
    fs.unlinkSync(path.join(srcDir, j.base + '.sbp'));
    fs.unlinkSync(path.join(srcDir, j.base + '.sbpcfg'));
  }
  writeAtomic(path.join(srcDir, j.outBase + '.sbp'), j.sbp);
  writeAtomic(path.join(srcDir, j.outBase + '.sbpcfg'), j.cfg);
  // With --replace the original goes away once its backup and the converted copy both exist.
  if (replace && !sameFile(j.outBase, j.base)) {
    fs.unlinkSync(path.join(srcDir, j.base + '.sbp'));
    fs.unlinkSync(path.join(srcDir, j.base + '.sbpcfg'));
  }
}
console.log(`Done. Wrote ${jobs.length} blueprint(s)` + (backups.length ? `; originals are in ${backupDir}.` : '.'));
