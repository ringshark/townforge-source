// Build variants (2026-10-19, Mark: "can we split them? a 2D version and a 3D version"):
// the web game is built twice from this source, a 2D-only game and a 3D-only game, each with
// the view switch removed, plus a backup code in Options. This checks what can go wrong
// without compiling: the source switches, the build script, the packaging of each variant,
// and above all that the two sites (one web address) never delete each other's cached copy.
const fs = require('fs'), path = require('path'), os = require('os'), cp = require('child_process');
const root = path.join(__dirname, '..');
const read = (f) => fs.readFileSync(path.join(root, f), 'utf8').replace(/\r\n/g, '\n');
let failures = 0;
const check = (ok, label) => { if (ok) console.log('PASS: ' + label); else { failures++; console.error('FAIL: ' + label); } };

// --- The game source ---------------------------------------------------------
const main = read('main.cpp');
check(/#if defined\(TF_VARIANT_2D\)\nstatic constexpr int kVariantView = 2;\n#elif defined\(TF_VARIANT_3D\)\nstatic constexpr int kVariantView = 3;\n#else\nstatic constexpr int kVariantView = 0;\n#endif/.test(main), 'main.cpp: 2 = 2D only, 3 = 3D only, 0 = the combined build');
check(/if \(kVariantView == 2\) state\.optClassic2D = true;[^\n]*\n\s*else if \(kVariantView == 3\) state\.optClassic2D = false;[^\n]*\n\s*bool v3 = !state\.optClassic2D;/.test(main), 'each build forces its own view in the one place the view is derived each frame');
const vKeys = main.split('\n').filter((l) => l.includes('IsKeyPressed(KEY_V)'));
check(vKeys.length >= 4 && vKeys.every((l) => l.includes('kVariantView == 0 && IsKeyPressed(KEY_V)')), 'the V key only switches views in the combined build (' + vKeys.length + ' places)');
check(/if \(kVariantView == 0\) toggle\("Classic 2D view"/.test(main), 'the Options row for the view only exists in the combined build');
check(/EM_JS\(int, JS_BackupSave/.test(main) && /EM_JS\(int, JS_RestoreSave/.test(main) && /"Back up my save"/.test(main) && /"Restore from a backup"/.test(main), 'Options has the backup and restore rows (web builds)');

// --- The backup code itself: the same JavaScript the game runs ---------------
const js = (name) => { const m = main.match(new RegExp('EM_JS\\(int, ' + name + ', \\(\\), \\{\\n([\\s\\S]*?)\\n\\}\\);')); return m ? m[1] : ''; };
const backup = js('JS_BackupSave'), restore = js('JS_RestoreSave');
function run(body, env) {
  const files = env.files, alerts = [];
  const FS = { readFile: (p) => { if (!(p in files)) throw new Error('ENOENT'); return files[p]; }, writeFile: (p, t) => { files[p] = t; }, syncfs: (_f, cb) => cb() };
  const window = { prompt: env.prompt || (() => null), confirm: env.confirm || (() => true) };
  const navigator = env.clipboard === undefined ? {} : { clipboard: { writeText: (t) => { env.copied = t; return { then: (ok) => ok() }; } } };
  const Module = {}, location = { reload: () => { env.reloaded = true; } };
  const result = new Function('FS', 'window', 'navigator', 'Module', 'location', 'alert', 'btoa', 'atob', 'unescape', 'escape', body)(
    FS, window, navigator, Module, location, (m) => alerts.push(m), (s) => Buffer.from(s, 'binary').toString('base64'), (s) => Buffer.from(s, 'base64').toString('binary'), unescape, escape);
  return { result, alerts, Module };
}
const save = 'version=1\ncharacterName=Mark\ngold=1234\nnote=café ✓\n';
const env1 = { files: { '/persist/townforge_save.txt': save }, clipboard: true };
const b = run(backup, env1);
check(b.result === 1 && /^TFSAVE1:[0-9a-f]+:[A-Za-z0-9+\/=]+$/.test(env1.copied), 'a backup code is "TFSAVE1:" + a checksum + base64, and it goes to the clipboard');
const env2 = { files: { '/persist/townforge_save.txt': 'version=1\ngold=5\n' }, prompt: () => env1.copied, confirm: () => true };
const r = run(restore, env2);
check(r.result === 1 && env2.files['/persist/townforge_save.txt'] === save && env2.reloaded === true, 'restoring writes the saved text back exactly (accents and symbols too) and reloads');
check(env2.files['/persist/townforge_save.prev.txt'] === 'version=1\ngold=5\n' && r.Module._TF_blockSave === 1, 'the replaced save is kept as a spare and the autosave is blocked until the reload');
const bad = (code, why) => { const e = { files: { '/persist/townforge_save.txt': save }, prompt: () => code, confirm: () => true }; const x = run(restore, e); check(x.result === 0 && e.files['/persist/townforge_save.txt'] === save && !e.reloaded && x.alerts.length === 1, 'restore refuses ' + why + ' and changes nothing'); };
bad('hello', 'text that is not a code');
bad(env1.copied.slice(0, -12), 'a code with its end cut off');
bad(env1.copied.replace(/^TFSAVE1:[0-9a-f]+/, 'TFSAVE1:0'), 'a code whose checksum does not match');
const fake = 'not a save\nat all\n';
let sum = 0; for (let i = 0; i < fake.length; i++) sum = (sum * 31 + fake.charCodeAt(i)) >>> 0;
bad('TFSAVE1:' + sum.toString(16) + ':' + Buffer.from(fake).toString('base64'), 'a valid code that does not hold a Town Forge save');
const spaced = env1.copied.replace(/(.{20})/g, '$1\n ');
const env3 = { files: {}, prompt: () => spaced, confirm: () => true };
check(run(restore, env3).result === 1 && env3.files['/persist/townforge_save.txt'] === save, 'a code that was wrapped or spaced when pasted still restores');
const env4 = { files: { '/persist/townforge_save.txt': 'version=1\n' }, prompt: () => env1.copied, confirm: () => false };
check(run(restore, env4).result === 0 && env4.files['/persist/townforge_save.txt'] === 'version=1\n' && !env4.reloaded, 'saying no to "replace your progress?" leaves the save alone');
const env5 = { files: {} };
check(run(backup, env5).result === 0, 'with no save yet, the backup says so instead of failing');
const env6 = { files: { '/persist/townforge_save.txt': save }, prompt: null };
let shown = ''; env6.prompt = (_m, v) => { shown = v; return null; };
run(backup, env6);
check(/^TFSAVE1:/.test(shown), 'with no clipboard the code is shown in a box to copy by hand');

// --- The build script -----------------------------------------------------------
const bash = (cmd, env) => cp.spawnSync('bash', ['-c', cmd], { cwd: root, env: Object.assign({}, process.env, env), encoding: 'utf8' });
const lf = (f) => read(f);
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'tf-variants-'));
const scripts = {};
for (const f of ['build_web.sh', 'deploy_web.sh']) { scripts[f] = path.join(root, 'web', '_lf_' + f); fs.writeFileSync(scripts[f], lf('web/' + f)); }
try {
  const bad = bash('bash web/_lf_build_web.sh', { RAYLIB_DIR: '/nowhere', TF_VARIANT: 'sideways' });
  check(bad.status === 1 && /TF_VARIANT must be 2d, 3d or empty/.test(bad.stderr), 'the build script rejects an unknown variant before it compiles anything');
  const bs = read('web/build_web.sh');
  check(/2d\) define=-DTF_VARIANT_2D/.test(bs) && /3d\) define=-DTF_VARIANT_3D/.test(bs) && /out=build-web\$\{variant:\+-\$variant\}/.test(bs), 'a variant compiles with its own define into build-web-2d / build-web-3d (the combined build still goes to build-web)');

  // --- Packaging each variant ------------------------------------------------------
  const build = path.join(tmp, 'build'); fs.mkdirSync(build);
  fs.writeFileSync(path.join(build, 'index.html'), '<!doctypehtml><title>Town Forge</title><meta content="Town Forge"name=apple-mobile-web-app-title>');
  for (const f of ['townforge.js', 'townforge.wasm', 'townforge.data']) fs.writeFileSync(path.join(build, f), 'x');
  const sites = {};
  for (const v of ['', '2d', '3d']) {
    const site = path.join(tmp, 'site-' + (v || 'combined'));
    const out = bash('bash web/_lf_deploy_web.sh "' + build.replace(/\\/g, '/') + '" "' + site.replace(/\\/g, '/') + '"', { TF_VARIANT: v });
    check(out.status === 0 && /deployed web app version/.test(out.stdout), 'packaging "' + (v || 'combined') + '" works' + (out.status ? ': ' + out.stderr.slice(0, 200) : ''));
    sites[v] = site;
  }
  const manifest = (v) => JSON.parse(fs.readFileSync(path.join(sites[v], 'manifest.webmanifest'), 'utf8'));
  const html = (v) => fs.readFileSync(path.join(sites[v], 'index.html'), 'utf8');
  check(manifest('3d').name === 'Town Forge 3D' && manifest('3d').short_name === 'Town Forge 3D' && /<title>Town Forge 3D<\/title>/.test(html('3d')) && /content="Town Forge 3D"name=apple-mobile-web-app-title/.test(html('3d')), 'the 3D site is named Town Forge 3D (home-screen icon, page title)');
  check(manifest('2d').name === 'Town Forge' && /classic top-down/.test(manifest('2d').description) && /<title>Town Forge<\/title>/.test(html('2d')), 'the 2D site stays Town Forge, described as the classic top-down game');
  check(manifest('').name === 'Town Forge' && /living 3D world/.test(manifest('').description), 'the combined build is packaged as it always was');

  // --- The service workers never delete each other's caches ---------------------
  const sw = (v) => fs.readFileSync(path.join(sites[v], 'sw.js'), 'utf8').replace(/\r\n/g, '\n');
  const policy = (v) => {
    const lines = sw(v).split('\n').filter((l) => /^const (VARIANT|PREFIX|CACHE|VERSION|legacy|mine)\b/.test(l)).join('\n');
    return new Function(lines + '\nreturn { CACHE, mine, VARIANT };')();
  };
  const legacyKey = 'townforge-20261001093000-1c27c40', k2 = 'townforge-2d-20261001093000-aaaaaaa', k3 = 'townforge-3d-20261001093000-bbbbbbb', other = 'something-else';
  const p2 = policy('2d'), p3 = policy('3d'), pc = policy('');
  check(p2.VARIANT === '2d' && p3.VARIANT === '3d' && pc.VARIANT === '' && /^townforge-2d-\d{14}-/.test(p2.CACHE) && /^townforge-3d-\d{14}-/.test(p3.CACHE), 'each service worker names its caches after its variant');
  check(!p2.mine(k3) && !p2.mine(legacyKey) && !p2.mine(other) && p2.mine(k2), 'the 2D site only ever deletes its own caches');
  check(!p3.mine(k2) && !p3.mine(other) && p3.mine(k3), 'the 3D site never deletes the 2D site\'s cache');
  check(p3.mine(legacyKey), 'the 3D site (which grew out of the combined one) clears the old untagged caches');
  check(!pc.mine(k2) && !pc.mine(k3) && pc.mine(legacyKey), 'the combined build leaves both variants\' caches alone');
  check(sw('2d').includes("'./townforge.data'") && sw('3d').includes("source-version.txt"), 'the rest of the service worker is unchanged');
} finally {
  for (const f of Object.values(scripts)) try { fs.unlinkSync(f); } catch (e) {}
  fs.rmSync(tmp, { recursive: true, force: true });
}
console.log('Failures: ' + failures);
process.exit(failures ? 1 : 0);
