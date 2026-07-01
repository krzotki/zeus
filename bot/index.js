// Zeus x27 kill-count service. Exposes:
//   GET /kills?steam=<id64|vanity>   -> { "killeater_value": <n>, "name": "..." }
//   GET /inspect?url=<inspect link>  -> { "killeater_value": <n>, ... }   (needs Steam login)
//   GET /                            -> { ready, gc } health
//
// /kills reads the StatTrak count straight out of the owner's PUBLIC Steam inventory
// JSON (the "stattrak_score" tooltip line) — no Steam login, no Game Coordinator. This
// is what the Zeus uses. /inspect is the old Game-Coordinator path and only works if
// STEAM_USERNAME/PASSWORD are set; leave them blank to run login-free.

require('dotenv').config();
const fs = require('fs');
const path = require('path');
const readline = require('readline');
const express = require('express');
const SteamUser = require('steam-user');
const GlobalOffensive = require('globaloffensive');
const SteamTotp = require('steam-totp');

const {
  STEAM_USERNAME,
  STEAM_PASSWORD,
  STEAM_SHARED_SECRET,
  PORT = '3000',
} = process.env;

// Steam login is OPTIONAL now — only needed for the legacy /inspect (GC) route.
// /kills works login-free off the public inventory JSON.
const steamEnabled = !!(STEAM_USERNAME && STEAM_PASSWORD);

const suspicious = (s) => s && (s !== s.trim() || /^["']|["']$/.test(s));
console.log('[config] PORT=%s  steam_login=%s', PORT, steamEnabled ? 'enabled' : 'disabled (/kills only)');
if (steamEnabled) {
  console.log('[config] STEAM_USERNAME=%s  password=set(len %d)  shared_secret=%s',
    JSON.stringify(STEAM_USERNAME), (STEAM_PASSWORD || '').length, STEAM_SHARED_SECRET ? 'set' : 'none');
  if (suspicious(STEAM_USERNAME)) console.warn('[config] WARNING: STEAM_USERNAME has surrounding spaces or quotes — fix .env');
  if (suspicious(STEAM_PASSWORD)) console.warn('[config] WARNING: STEAM_PASSWORD has surrounding spaces or quotes — quote it in .env as STEAM_PASSWORD=\'...\'');
}

const DATA_DIR = path.join(__dirname, 'steam-data');
const TOKEN_FILE = path.join(DATA_DIR, 'refresh-token.txt');
fs.mkdirSync(DATA_DIR, { recursive: true });

const client = new SteamUser({ dataDirectory: DATA_DIR, autoRelogin: true });
const csgo = new GlobalOffensive(client);

let gcReady = false;

// ---- login -----------------------------------------------------------------
function logOn() {
  let savedToken = null;
  try { savedToken = fs.readFileSync(TOKEN_FILE, 'utf8').trim() || null; } catch { /* first run */ }

  if (savedToken) {
    console.log('Logging in with saved refresh token…');
    client.logOn({ refreshToken: savedToken });
  } else {
    console.log('Logging in with account name + password…');
    client.logOn({ accountName: STEAM_USERNAME, password: STEAM_PASSWORD });
  }
}

// Steam Guard: use the shared secret if provided, else prompt on the terminal.
client.on('steamGuard', (domain, callback) => {
  if (STEAM_SHARED_SECRET) {
    console.log('Generating Steam Guard code from shared secret…');
    callback(SteamTotp.generateAuthCode(STEAM_SHARED_SECRET));
    return;
  }
  const where = domain ? `email @${domain}` : 'mobile authenticator';
  const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
  rl.question(`Steam Guard code (${where}): `, (code) => { rl.close(); callback(code.trim()); });
});

client.on('refreshToken', (token) => {
  try { fs.writeFileSync(TOKEN_FILE, token); console.log('Saved refresh token (future restarts skip the Guard code).'); }
  catch (e) { console.warn('Could not save refresh token:', e.message); }
});

client.on('loggedOn', () => {
  console.log(`Logged on as ${client.steamID.getSteamID64()}. Launching CS2…`);
  client.setPersona(SteamUser.EPersonaState.Online);
  client.gamesPlayed([730]); // CS2 app id — required to get a GC session
});

client.on('error', (err) => {
  console.error('Steam error:', err.message || err);
  // A stale refresh token can cause a logon error; drop it so the next run uses the password.
  try { if (fs.existsSync(TOKEN_FILE)) fs.unlinkSync(TOKEN_FILE); } catch {}
  // No in-process retry loop: exit so the run is cancellable. Fix the cause and re-run
  // (under `docker compose up -d` the restart policy handles legitimate transient failures).
  process.exit(1);
});

client.on('disconnected', (eresult, msg) => {
  gcReady = false;
  console.warn(`Disconnected from Steam (${msg || eresult}). Auto-relogin will retry.`);
});

csgo.on('connectedToGC', () => { gcReady = true; console.log('Connected to CS2 Game Coordinator — ready to inspect.'); });
csgo.on('disconnectedFromGC', (reason) => { gcReady = false; console.warn('Lost GC session:', reason); });

// ---- inspect queue ---------------------------------------------------------
// Serialize inspects so we never fire concurrent GC requests.
const queue = [];
let busy = false;

function pump() {
  if (busy || queue.length === 0) return;
  busy = true;
  const job = queue.shift();
  let settled = false;

  const finish = (fn) => { if (settled) return; settled = true; clearTimeout(timer); busy = false; fn(); setImmediate(pump); };
  const timer = setTimeout(() => finish(() => job.reject(new Error('inspect timeout'))), 15000);

  try {
    csgo.inspectItem(job.link, (item) => finish(() => job.resolve(item)));
  } catch (e) {
    finish(() => job.reject(e));
  }
}

function inspect(link) {
  return new Promise((resolve, reject) => { queue.push({ link, resolve, reject }); pump(); });
}

// ---- HTTP ------------------------------------------------------------------
const app = express();

app.get('/', (_req, res) => res.json({ ready: gcReady, gc: csgo.haveGCSession === true }));

app.get('/inspect', async (req, res) => {
  const url = req.query.url;
  if (!url) return res.status(400).json({ error: 'missing url' });

  // Classify: S/A/D links hit the Game Coordinator (live); bare hex is a frozen snapshot.
  const live = /[SM]\d+A\d+D\d+/.test(url);
  const kind = live ? 'S/A/D (live via GC)' : 'masked-hex (STATIC snapshot)';
  console.log(`[inspect] req kind=${kind} url=${url.slice(0, 80)}${url.length > 80 ? '…' : ''}`);

  if (!gcReady || !csgo.haveGCSession) {
    console.warn('[inspect] rejected: GC not ready');
    return res.status(503).json({ error: 'GC not ready' });
  }

  const t0 = Date.now();
  try {
    const item = await inspect(url);
    const out = {
      killeater_value: item.killeatervalue ?? null,
      score_type:      item.killeaterscoretype ?? null,
      defindex:        item.defindex ?? null,
      paintindex:      item.paintindex ?? null,
      paintseed:       item.paintseed ?? null,
      float:           item.paintwear ?? null,
      custom_name:     item.customname ?? null,
    };
    console.log(`[inspect] ok in ${Date.now() - t0}ms  killeater_value=${out.killeater_value}  defindex=${out.defindex}  (${kind})`);
    if (!live) console.warn('[inspect] NOTE: this was a masked/hex link — the value is frozen and will NOT update. Use an S/A/D owned link for live kills.');
    res.json(out);
  } catch (e) {
    console.error(`[inspect] FAILED in ${Date.now() - t0}ms: ${e.message || e}`);
    res.status(504).json({ error: e.message || String(e) });
  }
});

// ---- /kills : read StatTrak count from the public inventory (no Steam login) ----
async function resolveSteamId(input) {
  const s = String(input || '').trim();
  if (/^\d{17}$/.test(s)) return s;                       // already a SteamID64
  // vanity name -> SteamID64 via the public XML endpoint (no API key needed)
  const r = await fetch(`https://steamcommunity.com/id/${encodeURIComponent(s)}/?xml=1`,
    { headers: { 'User-Agent': 'ZeusX27' } });
  const m = (await r.text()).match(/<steamID64>(\d+)<\/steamID64>/);
  if (!m) throw new Error('could not resolve steam id');
  return m[1];
}

async function fetchKills(steamid) {
  const url = `https://steamcommunity.com/inventory/${steamid}/730/2?l=english&count=2000`;
  const r = await fetch(url, { headers: { 'User-Agent': 'Mozilla/5.0 ZeusX27', 'Accept-Encoding': 'identity' } });
  if (r.status === 403) throw new Error('inventory is private');
  if (!r.ok) throw new Error('inventory HTTP ' + r.status);
  const inv = await r.json();
  if (!inv || !Array.isArray(inv.descriptions)) throw new Error('unexpected inventory JSON');

  const d = inv.descriptions.find(x =>
    (x.market_hash_name || '').includes('Zeus x27') && (x.market_hash_name || '').includes('StatTrak'));
  if (!d) throw new Error('no StatTrak Zeus in inventory');

  const line = (d.descriptions || []).find(l => l.name === 'stattrak_score' || /Kills/.test(l.value || ''));
  if (!line) throw new Error('no StatTrak score line on the Zeus');
  const nums = String(line.value).match(/\d+/g);           // last run of digits = the count
  if (!nums) throw new Error('could not parse kill count');
  return { killeater_value: parseInt(nums[nums.length - 1], 10), name: d.market_hash_name };
}

app.get('/kills', async (req, res) => {
  const steam = req.query.steam || req.query.steamid;
  if (!steam) return res.status(400).json({ error: 'missing steam' });
  const t0 = Date.now();
  try {
    const sid = await resolveSteamId(steam);
    const out = await fetchKills(sid);
    console.log(`[kills] ${steam} -> ${sid} -> ${out.killeater_value} in ${Date.now() - t0}ms`);
    res.json(out);
  } catch (e) {
    console.warn(`[kills] ${steam} FAILED in ${Date.now() - t0}ms: ${e.message || e}`);
    res.status(502).json({ error: e.message || String(e) });
  }
});

app.listen(Number(PORT), () => console.log(`Zeus kill-count service listening on :${PORT}`));

if (steamEnabled) logOn();
else console.log('Steam login disabled — /kills is available, /inspect will return 503.');
