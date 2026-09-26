// Town Forge cloud saves (2026-09-27).
// One character on every device: sign in with an email + password and the save
// file (/persist/townforge_save.txt, written by the game every 2 s) is kept in a
// Supabase table - uploaded in the background, offered for download when another
// device has newer progress, with one older copy kept as a safety net.
// Needs window.TF_CLOUD = { url, key } (web/cloud-config.js, generated at deploy
// from the SUPABASE_URL / SUPABASE_ANON_KEY environment variables). The anon key
// is a public, browser-safe key: row-level security keeps every save private.
(function () {
  var CFG = window.TF_CLOUD;
  var SAVE = '/persist/townforge_save.txt';
  var LS_SYNC = 'tf-cloud-stamp';        // the cloud copy's updated_at when this device last synced
  var UPLOAD_EVERY = 60 * 1000;          // background upload interval (when changed)
  var PREV_MIN_AGE = 60 * 60 * 1000;     // keep the older cloud copy at most hourly
  var sb = null, user = null, cloud = null, status = 'off', busy = false, paused = false, lastUp = 0, msg = '', lastUpHash = '';

  function fs() { return window.FS || (window.Module && Module.FS); }
  function ready() { return window.Module && Module._TF_persistReady && fs(); }
  function readLocal() { try { return fs().readFile(SAVE, { encoding: 'utf8' }); } catch (e) { return null; } }
  function hash(s) { var h = 5381; for (var i = 0; i < s.length; i++) h = ((h << 5) + h + s.charCodeAt(i)) | 0; return String(h >>> 0) + ':' + s.length; }
  function getStamp() { try { return localStorage.getItem(LS_SYNC) || ''; } catch (e) { return ''; } }
  function setStamp(t) { try { localStorage.setItem(LS_SYNC, t || ''); } catch (e) {} }
  // The save rewrites its timestamp every 2 s - ignore it when asking "did anything change?"
  function contentHash(t) { return hash((t || '').replace(/^lastActiveEpoch=.*$/m, '')); }
  function field(t, k) { var m = t && t.match(new RegExp('^' + k + '=(.*)$', 'm')); return m ? m[1] : ''; }
  function summary(t) {
    if (!t) return null;
    var name = field(t, 'characterName') || 'Unnamed adventurer';
    var gold = field(t, 'gold') || '0';
    var when = +field(t, 'lastActiveEpoch') || 0;
    return { name: name, gold: +gold, when: when * 1000 };
  }
  function ago(ms) {
    if (!ms) return 'unknown time';
    var s = Math.max(0, (Date.now() - ms) / 1000);
    if (s < 90) return 'just now';
    if (s < 3600) return Math.round(s / 60) + ' min ago';
    if (s < 86400 * 2) return Math.round(s / 3600) + ' h ago';
    return Math.round(s / 86400) + ' days ago';
  }
  function esc(s) { return String(s).replace(/[&<>"']/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]; }); }

  // ---- Supabase ----
  function loadLib(cb) {
    if (window.supabase && window.supabase.createClient) return cb();
    var s = document.createElement('script');
    s.src = 'https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2/dist/umd/supabase.min.js';
    s.onload = cb;
    s.onerror = function () { status = 'error'; msg = 'Could not reach the cloud (offline?).'; render(); };
    document.head.appendChild(s);
  }
  function init() {
    if (!CFG || !CFG.url || !CFG.key) return;
    status = 'signedout';
    loadLib(function () {
      sb = window.supabase.createClient(CFG.url, CFG.key, { auth: { persistSession: true, autoRefreshToken: true, detectSessionInUrl: true } });
      sb.auth.onAuthStateChange(function (ev, session) {
        user = session ? session.user : null;
        if (ev === 'PASSWORD_RECOVERY') { open(); showReset(); return; }
        if (user) { status = 'synced'; afterSignIn(); } else { status = 'signedout'; cloud = null; }
        render();
      });
      sb.auth.getSession().then(function (r) { if (r.data && r.data.session) { user = r.data.session.user; status = 'synced'; afterSignIn(); } render(); });
    });
    setInterval(tick, 5000);
    document.addEventListener('visibilitychange', function () { if (document.visibilityState === 'hidden') upload(false); });
  }
  function fetchCloud() {
    return sb.from('saves').select('data, prev_data, updated_at').maybeSingle().then(function (r) {
      if (r.error) throw r.error;
      cloud = r.data || null;
      return cloud;
    });
  }
  // Signed in (or app opened while signed in): decide who has the newest progress.
  // Every upload records the cloud copy's timestamp on this device; if the cloud's
  // timestamp moved since, another device has played - ask before touching anything.
  function afterSignIn() {
    var go = function () {
      if (!ready()) return setTimeout(go, 500);
      fetchCloud().then(function () {
        if (!cloud) { upload(true); return; }
        var stamp = getStamp();
        if (stamp && cloud.updated_at === stamp) { status = 'synced'; upload(false); return; }
        paused = true;
        status = stamp ? 'newer' : 'conflict';
        msg = stamp ? 'Your character was played on another device since this one last synced.'
                    : 'There is already a character in the cloud. Choose which one this device should use.';
        open();
      }).catch(function (e) { status = 'error'; msg = 'Cloud error: ' + (e.message || e); render(); });
    };
    go();
  }
  function upload(force) {
    if (!sb || !user || busy || (paused && !force) || !ready()) return Promise.resolve();
    var local = readLocal();
    if (!local) return Promise.resolve();
    if (!force && contentHash(local) === lastUpHash) return Promise.resolve();
    busy = true; status = 'syncing'; render();
    var check = force ? Promise.resolve(cloud) : fetchCloud();
    return check.then(function (c) {
      if (!force && c && c.updated_at !== getStamp()) { // someone else wrote in the meantime
        busy = false; paused = true; status = 'newer';
        msg = 'Your character was just saved from another device.'; open(); return;
      }
      var row = { user_id: user.id, data: local, updated_at: new Date().toISOString() };
      if (c && c.data && contentHash(c.data) !== contentHash(local) &&
          (!c.updated_at || Date.now() - Date.parse(c.updated_at) > PREV_MIN_AGE)) row.prev_data = c.data;
      return sb.from('saves').upsert(row).then(function (r) {
        busy = false;
        if (r.error) { status = 'error'; msg = 'Upload failed: ' + r.error.message; render(); return; }
        cloud = { data: local, prev_data: row.prev_data || (c && c.prev_data), updated_at: row.updated_at };
        setStamp(row.updated_at); lastUpHash = contentHash(local); lastUp = Date.now(); paused = false;
        status = 'synced'; msg = ''; render();
      });
    }).catch(function (e) { busy = false; status = 'error'; msg = 'Cloud error: ' + (e.message || e); render(); });
  }
  function tick() {
    if (user && !paused && Date.now() - lastUp > UPLOAD_EVERY) upload(false);
    if (box && box.style.display !== 'none' && user) render(); // (never while typing a sign-in)
  }
  // Replace this device's save with a cloud copy, then restart the game on it.
  function restore(text) {
    if (!text || !ready()) return;
    Module._TF_blockSave = 1;               // the game's autosave must not write over it
    fs().writeFile(SAVE, text);
    if (cloud) setStamp(cloud.updated_at); // in step with the cloud again
    fs().syncfs(false, function () { location.reload(); });
  }

  // ---- UI ----
  var box = null;
  function el(html) { var d = document.createElement('div'); d.innerHTML = html; return d.firstChild; }
  function build() {
    if (box) return;
    var css = document.createElement('style');
    css.textContent =
      '#tfcloud{position:fixed;inset:0;display:none;align-items:center;justify-content:center;background:rgba(0,0,0,.55);z-index:20;font-family:arial,sans-serif}' +
      '#tfcloud .p{width:min(92vw,400px);max-height:88vh;overflow:auto;box-sizing:border-box;padding:16px 18px;background:#2a1b10;border:2px solid #b8893c;border-radius:12px;color:#f0dcae;box-shadow:0 10px 30px rgba(0,0,0,.7)}' +
      '#tfcloud h2{margin:0 0 6px;font-size:19px;color:#ffd98a}#tfcloud .s{font-size:13px;color:#c9ab76;margin-bottom:10px}' +
      '#tfcloud input{width:100%;box-sizing:border-box;margin:5px 0;padding:10px;border-radius:7px;border:1px solid #8a6630;background:#1a110a;color:#f6e2b0;font-size:16px}' +
      '#tfcloud button{margin:5px 6px 0 0;padding:9px 13px;border-radius:8px;border:1px solid #b8893c;background:#6b4520;color:#f6e2b0;font-size:14px}' +
      '#tfcloud button.alt{background:#3a2616}#tfcloud .card{border:1px solid #6b4a24;border-radius:8px;padding:8px 10px;margin:8px 0;background:#20150c;font-size:14px}' +
      '#tfcloud .m{color:#ffb38a;font-size:13px;margin:8px 0}#tfcloud a{color:#e8c070}#tfcloud .row{display:flex;flex-wrap:wrap}';
    document.head.appendChild(css);
    box = el('<div id="tfcloud"><div class="p"></div></div>');
    document.body.appendChild(box);
    box.addEventListener('click', function (e) { if (e.target === box) close(); });
  }
  function open() { build(); box.style.display = 'flex'; render(); }
  function close() { if (box) box.style.display = 'none'; }
  function card(title, t, when) {
    var sm = summary(t);
    if (!sm) return '<div class="card"><b>' + title + '</b><br>No save</div>';
    return '<div class="card"><b>' + title + '</b><br>' + esc(sm.name) + ' &middot; ' + sm.gold + ' gold<br><span class="s">played ' + ago(when || sm.when) + '</span></div>';
  }
  function render() {
    if (!box || box.style.display === 'none') return;
    var p = box.firstChild, h = '';
    h += '<h2>&#9729;&#xFE0E; Cloud Save</h2>';
    if (!sb) {
      h += '<div class="s">' + (CFG ? 'Connecting...' : 'Cloud saves are not set up for this build yet.') + '</div>';
    } else if (!user) {
      h += '<div class="s">Keep one character on every device - phone, tablet and computer.</div>' +
           '<input id="tfc-e" type="email" placeholder="Email" autocomplete="email">' +
           '<input id="tfc-p" type="password" placeholder="Password (6+ characters)" autocomplete="current-password">' +
           '<div class="row"><button id="tfc-in">Sign in</button><button id="tfc-up" class="alt">Create account</button></div>' +
           '<div class="s" style="margin-top:8px"><a href="#" id="tfc-f">Forgot password?</a></div>';
    } else {
      var local = readLocal();
      h += '<div class="s">Signed in as ' + esc(user.email || '') + ' &middot; ' +
           (status === 'syncing' ? 'syncing...' : status === 'synced' ? 'up to date &#10003;' : status === 'error' ? 'problem' : 'waiting for you') + '</div>';
      h += card('This device', local, 0);
      h += card('Cloud', cloud && cloud.data, cloud && cloud.updated_at ? Date.parse(cloud.updated_at) : 0);
      if (msg) h += '<div class="m">' + esc(msg) + '</div>';
      h += '<div class="row">';
      h += '<button id="tfc-save">Upload this device</button>';
      if (cloud && cloud.data) h += '<button id="tfc-load" class="alt">Load cloud save</button>';
      if (cloud && cloud.prev_data) h += '<button id="tfc-prev" class="alt">Load older cloud copy</button>';
      h += '</div><div class="row"><button id="tfc-out" class="alt">Sign out</button></div>';
    }
    h += '<div class="row"><button id="tfc-x" class="alt">Close</button></div>';
    var focus = document.activeElement && document.activeElement.id;
    var ev = document.getElementById('tfc-e'), pv = document.getElementById('tfc-p');
    var keepE = ev ? ev.value : '', keepP = pv ? pv.value : '';
    p.innerHTML = h;
    if (document.getElementById('tfc-e')) { document.getElementById('tfc-e').value = keepE; document.getElementById('tfc-p').value = keepP; }
    if (focus && document.getElementById(focus)) document.getElementById(focus).focus();
    bind();
  }
  function on(id, f) { var b = document.getElementById(id); if (b) b.onclick = function (e) { e.preventDefault(); f(); }; }
  function creds() { return { email: (document.getElementById('tfc-e').value || '').trim(), password: document.getElementById('tfc-p').value || '' }; }
  function bind() {
    on('tfc-x', close);
    on('tfc-in', function () {
      var c = creds(); msg = 'Signing in...'; render();
      sb.auth.signInWithPassword(c).then(function (r) { msg = r.error ? r.error.message : ''; render(); });
    });
    on('tfc-up', function () {
      var c = creds(); msg = 'Creating account...'; render();
      sb.auth.signUp(Object.assign(c, { options: { emailRedirectTo: location.origin + location.pathname } })).then(function (r) {
        if (r.error) msg = r.error.message;
        else if (!r.data.session) msg = 'Check your email to confirm the account, then sign in here.';
        else msg = '';
        render();
      });
    });
    on('tfc-f', function () {
      var c = creds();
      if (!c.email) { msg = 'Type your email first.'; render(); return; }
      sb.auth.resetPasswordForEmail(c.email, { redirectTo: location.origin + location.pathname }).then(function (r) {
        msg = r.error ? r.error.message : 'Password reset email sent.'; render();
      });
    });
    on('tfc-save', function () { paused = false; upload(true); });
    on('tfc-load', function () { if (confirm('Replace this device\'s character with the cloud save?')) restore(cloud.data); });
    on('tfc-prev', function () { if (confirm('Replace this device\'s character with the OLDER cloud copy?')) restore(cloud.prev_data); });
    on('tfc-out', function () { sb.auth.signOut(); setStamp(''); });
  }
  function showReset() {
    build();
    box.firstChild.innerHTML = '<h2>Choose a new password</h2><input id="tfc-np" type="password" placeholder="New password">' +
      '<div class="row"><button id="tfc-set">Save password</button></div>';
    on('tfc-set', function () {
      sb.auth.updateUser({ password: document.getElementById('tfc-np').value }).then(function (r) {
        msg = r.error ? r.error.message : 'Password updated.'; render();
      });
    });
  }
  // Typing in the dialog must reach the inputs, not the game (it grabs keys like Backspace).
  ['keydown', 'keyup', 'keypress'].forEach(function (t) {
    window.addEventListener(t, function (e) {
      if (box && box.style.display !== 'none' && e.target && /INPUT|TEXTAREA/.test(e.target.tagName)) e.stopImmediatePropagation();
    }, true);
  });

  window.TFCloud = {
    configured: !!(CFG && CFG.url && CFG.key),
    open: open,
    // 0 not set up, 1 signed out, 2 synced, 3 syncing, 4 needs attention
    state: function () {
      if (!CFG) return 0;
      if (!user) return 1;
      if (status === 'syncing') return 3;
      if (status === 'synced') return 2;
      return 4;
    },
    onReset: function () { // the character was reset: don't push the fresh one over the cloud unasked
      if (!user) return;
      paused = true; status = 'conflict';
      msg = 'You reset your character. Upload the new one, or load the cloud save to get the old one back.';
    },
  };
  init();
})();
