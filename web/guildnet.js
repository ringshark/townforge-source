// Town Forge online guilds + War Week (2026-09-27).
// Talks to the tf_* functions in web/supabase.sql through the cloud-save
// sign-in (cloud.js). The game polls TFGuildNet.state(): plain "key=value"
// lines it can parse without JSON.
//   ready=0|1  signed in and cloud configured     cfg=0|1  cloud set up at all
//   busy=0|1   msg=<last error/notice>             my=<guild id>|<name>|<tag>
//   g=<id>|<name>|<tag>|<members>|<week points>   (every guild, best first)
//   m=<char name>|<power>|<week points>|<user id>|<rank>|<seconds since seen>  (my roster, best first)
//   me=<my user id>|<my rank>   motd=<message of the day>   rank: 0 member, 1 officer, 2 leader
//   war=<guild id>|<name>|<tag>|<out|in|both>     (wars we declared / declared on us)
// Guild Hall (2026-09-29, tf_guild_hub):
//   hall=<level>|<funds>|<member cap>|<build secs left, -1 none>|<build total secs>|<hands>|<I lent 0|1>|<recommended tech>
//   mine=<merit>|<donations left>|<secs to next>|<contribution this week>|<help merit today>
//   tech=<id>|<level>|<progress>      help=<id>|<who>|<what>|<helps>|<max>|<mine 0|1>|<I helped 0|1>
//   gift=<id>|<kind>|<from>           board=<name>|<contribution this week>
//   done=<seq>|<action>|<result>      (the latest finished action: bought / opened / helped / donated)
(function () {
  var busy = false, msg = '', my = null, guilds = [], roster = [], wars = [], motd = '', myRank = 0, lastWeek = '', lastRefresh = 0;
  var city = null, cityAck = null;
  var hub = null, doneSeq = 0, done = '';
  function sb() { return window.TFCloud && TFCloud.client && TFCloud.client(); }
  function me() { return window.TFCloud && TFCloud.user && TFCloud.user(); }
  function clean(s) { return String(s == null ? '' : s).replace(/[|\r\n]/g, ' '); }
  function fail(e) { busy = false; msg = (e && (e.message || e.error_description)) || String(e); }

  function refresh(week) {
    var c = sb(), u = me();
    if (!c || !u || busy) return;
    busy = true; lastWeek = week || lastWeek; lastRefresh = Date.now();
    Promise.all([
      c.rpc('tf_standings', { p_week: lastWeek }),
      c.from('guild_members').select('guild_id').eq('user_id', u.id).maybeSingle(),
    ]).then(function (r) {
      if (r[0].error) throw r[0].error;
      if (r[1].error) throw r[1].error;
      guilds = r[0].data || [];
      var gid = r[1].data && r[1].data.guild_id;
      my = null; roster = []; wars = []; motd = ''; myRank = 0; hub = null; city = null;
      if (!gid) { busy = false; return; }
      guilds.forEach(function (g) { if (g.id === gid) my = g; });
      if (!my) my = { id: gid, name: '?', tag: '?' };
      return Promise.all([
        c.from('guild_members').select('user_id, char_name, power, rank, seen_at').eq('guild_id', gid),
        c.from('war_scores').select('user_id, points').eq('guild_id', gid).eq('week', lastWeek),
        c.from('guilds').select('motd').eq('id', gid).maybeSingle(),
        c.from('guild_wars').select('guild_id, target_id').or('guild_id.eq.' + gid + ',target_id.eq.' + gid),
        c.rpc('tf_guild_hub'),
        c.rpc('tf_city_hub'),
      ]).then(function (q) {
        // A failed wars/score query must not look like peace or zero progress.
        q.forEach(function (result) { if (result.error) throw result.error; });
        var pts = {};
        (q[1].data || []).forEach(function (w) { pts[w.user_id] = (pts[w.user_id] || 0) + w.points; });
        var now = Date.now();
        roster = (q[0].data || []).map(function (m) {
          if (m.user_id === u.id) myRank = m.rank || 0;
          return { name: m.char_name, power: m.power, points: pts[m.user_id] || 0, id: m.user_id, rank: m.rank || 0,
                   seen: m.seen_at ? Math.max(0, Math.round((now - Date.parse(m.seen_at)) / 1000)) : -1 };
        });
        motd = (q[2] && q[2].data && q[2].data.motd) || '';
        hub = q[4].data || null; city = q[5].data || null;
        var byId = {}; guilds.forEach(function (g) { byId[g.id] = g; });
        var w = {};
        ((q[3] && q[3].data) || []).forEach(function (r) {
          var other = r.guild_id === gid ? r.target_id : r.guild_id, dir = r.guild_id === gid ? 'out' : 'in';
          w[other] = w[other] && w[other] !== dir ? 'both' : dir;
        });
        wars = Object.keys(w).map(function (id) { var g = byId[id] || { name: '?', tag: '?' }; return { id: id, name: g.name, tag: g.tag, dir: w[id] }; });
        roster.sort(function (a, b) { return b.points - a.points || b.power - a.power; });
        busy = false;
      });
    }).catch(fail);
  }
  function call(fn, args, after) {
    var c = sb();
    if (!c || !me()) { msg = 'Sign in with Cloud save first.'; return; }
    if (busy) return;
    busy = true; msg = '';
    c.rpc(fn, args).then(function (r) {
      busy = false;
      if (r.error) { msg = r.error.message; return; }
      if (after) after(r.data);
      refresh(lastWeek);
    }, fail);
  }
  function finished(action, result) { doneSeq++; done = action + '|' + clean(result); }

  window.TFGuildNet = {
    cityAction: function(action,target) {
      if(busy) return;
      if(!window.TFCloud || !TFCloud.guildAction) { msg='Update the cloud client to use guild progression.'; return; }
      busy=true; msg='';
      TFCloud.guildAction(action,target).then(function(r) {
        busy=false; cityAck=r;
        finished('city',[r.request,r.wood,r.ore,r.leather,r.gold,clean(r.summary)].join(','));
        refresh(lastWeek);
      },fail);
    },
    cityAck: function(request) { if(cityAck && cityAck.request===request) TFCloud.guildAck(request,cityAck.stamp); },
    refresh: refresh,
    create: function (name, tag, ch, power) { call('tf_create_guild', { p_name: name, p_tag: tag, p_char: ch, p_power: power | 0 }, function () { msg = 'Guild founded!'; }); },
    join: function (id, ch, power) { call('tf_join_guild', { p_guild: id, p_char: ch, p_power: power | 0 }, function () { msg = 'Joined!'; }); },
    leave: function () { call('tf_leave_guild', {}, function () { msg = 'You left the guild.'; }); },
    setMotd: function (t) { call('tf_set_motd', { p_text: t }, function () { msg = 'Message updated!'; }); },
    kick: function (id) { call('tf_kick', { p_user: id }, function () { msg = 'Member removed.'; }); },
    setRank: function (id, r) { call('tf_set_rank', { p_user: id, p_rank: r | 0 }, function () { msg = r === 2 ? 'Leadership handed over!' : 'Rank changed!'; }); },
    declareWar: function (id) { call('tf_declare_war', { p_target: id }, function () { msg = 'War declared!'; }); },
    endWar: function (id) { call('tf_end_war', { p_target: id }, function () { msg = 'You made peace.'; }); },
    // The game reports its running total for the day; the server keeps the best.
    submit: function (week, day, points, ch, power, snap) {
      var c = sb();
      if (!c || !me()) return;
      c.rpc('tf_submit', { p_week: week, p_day: day | 0, p_points: points | 0, p_char: ch, p_power: power | 0, p_snap: snap || null })
        .then(function (r) { if (r.error) msg = r.error.message; else if (Date.now() - lastRefresh > 20000) refresh(week); });
    },
    // Guild Hall (2026-09-29)
    donate: function (t) { call('tf_donate', { p_tech: t }, function (lv) { finished('donated', t + ':' + lv); }); },
    recommend: function (t) { call('tf_recommend', { p_tech: t }, function () { msg = t ? 'Research recommended!' : 'Recommendation cleared!'; }); },
    projectWork: function (kind, request) { call('tf_project_work', { p_kind: kind, p_request: request }, function (result) { finished('project', result); msg = 'Construction order delivered!'; }); },
    hallUpgrade: function () { call('tf_hall_upgrade', {}, function () { msg = 'The builders start on the Hall!'; }); },
    lendHand: function () { call('tf_lend_hand', {}, function () { msg = 'You lend a hand - the Hall rises faster!'; }); },
    requestHelp: function (label, total) { call('tf_request_help', { p_label: label, p_total: total | 0 }, function () { msg = 'Your guild has been asked for help!'; finished('asked', label); }); },
    closeHelp: function () { var c = sb(); if (c && me()) c.rpc('tf_close_help', {}); },
    helpAll: function () { call('tf_help_all', {}, function (n) { finished('helped', n | 0); }); },
    shopBuy: function (item) { call('tf_shop_buy', { p_item: item }, function (left) { finished('bought', item); }); },
    sendGift: function (kind) { var c = sb(); if (c && me()) c.rpc('tf_send_gift', { p_kind: kind | 0 }); },
    openGifts: function () { call('tf_open_gifts', {}, function (kinds) { finished('opened', (kinds || []).join(',')); }); },
    clearMsg: function () { msg = ''; },
    state: function () {
      var out = [];
      out.push('cfg=' + (window.TFCloud && TFCloud.configured ? 1 : 0));
      out.push('ready=' + (sb() && me() ? 1 : 0));
      out.push('busy=' + (busy ? 1 : 0));
      out.push('msg=' + clean(msg));
      if (my) out.push('my=' + [my.id, clean(my.name), clean(my.tag)].join('|'));
      guilds.slice(0, 40).forEach(function (g) { out.push('g=' + [g.id, clean(g.name), clean(g.tag), g.members, g.points].join('|')); });
      roster.slice(0, 30).forEach(function (m) { out.push('m=' + [clean(m.name), m.power, m.points, m.id, m.rank, m.seen].join('|')); });
      if (my) { var u = me(); out.push('me=' + (u ? u.id : '') + '|' + myRank); out.push('motd=' + clean(motd)); }
      wars.forEach(function (w) { out.push('war=' + [w.id, clean(w.name), clean(w.tag), w.dir].join('|')); });
      if (hub) {
        if (hub.project) out.push('project=' + [hub.project.timber,hub.project.ore,hub.project.tools,hub.project.contracts,hub.project.worked ? 1 : 0].join('|'));
        out.push('hall=' + [hub.hall, hub.funds, hub.cap, hub.build_left, hub.build_secs, hub.hands, hub.lent ? 1 : 0, clean(hub.rec)].join('|'));
        out.push('mine=' + [hub.merit, hub.charges, hub.next_charge, hub.contrib, hub.help_merit].join('|'));
        (hub.tech || []).forEach(function (t) { out.push('tech=' + [clean(t.t), t.l, t.p].join('|')); });
        (hub.helps || []).forEach(function (h) { out.push('help=' + [h.id, clean(h.who), clean(h.what), h.n, h.max, h.mine ? 1 : 0, h.done ? 1 : 0].join('|')); });
        (hub.gifts || []).forEach(function (g) { out.push('gift=' + [g.id, g.k, clean(g.from)].join('|')); });
        (hub.board || []).slice(0, 30).forEach(function (b) { out.push('board=' + [clean(b.n), b.c].join('|')); });
      }
      if (window.TFCloud && TFCloud.guildPending && TFCloud.guildPending()) out.push('citypending=1');
      if(city && city.stock) {
        out.push('citystock='+[city.stock.wood||0,city.stock.ore||0,city.stock.leather||0,city.contracts||0].join('|'));
        (city.buildings||[]).forEach(function(b){out.push('citybuilding='+[clean(b.kind),b.level,b.left].join('|'));});
        (city.ledger||[]).forEach(function(l){out.push('citylog='+[clean(l.char_name),clean(l.summary),clean(l.created_at)].join('|'));});
        (city.runs||[]).forEach(function(r){out.push('cityrun='+[r.id,clean(r.kind),r.goal,r.progress,r.left,r.finished?1:0,r.success?1:0,r.joined?1:0,r.claimed?1:0].join('|'));});
        (city.battles||[]).forEach(function(b){out.push('citybattle='+[b.id,clean(b.enemy),b.starts,b.ends,b.actions,b.claimed?1:0,b.mine,b.enemy_score].join('|'));});
      }
      if (doneSeq) out.push('done=' + doneSeq + '|' + done);
      return out.join('\n');
    },
  };
})();
