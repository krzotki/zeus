// Zeus x27 inspect bot.
// Logs a Steam account into the CS2 Game Coordinator and exposes:
//   GET /inspect?url=<inspect link>  -> { "killeater_value": <n>, ... }
//   GET /                            -> { ready, gc } health
// The Zeus firmware polls /inspect on your LAN instead of a public inspect API.
//
// The StatTrak count (kill_eater) is only readable by inspecting the item through
// Valve's Game Coordinator, which requires a logged-in account. That is what this
// service is for. Use a throwaway/secondary Steam account if you prefer.

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

if (!STEAM_USERNAME || !STEAM_PASSWORD) {
  console.error('Missing STEAM_USERNAME / STEAM_PASSWORD. Copy .env.example to .env and fill it in.');
  process.exit(1);
}

// ---- startup debug: confirm env made it in, without leaking the password -----
const suspicious = (s) => s && (s !== s.trim() || /^["']|["']$/.test(s));
console.log('[config] STEAM_USERNAME=%s  password=%s(len %d)  shared_secret=%s  PORT=%s',
  JSON.stringify(STEAM_USERNAME),
  STEAM_PASSWORD ? 'set' : 'MISSING',
  (STEAM_PASSWORD || '').length,
  STEAM_SHARED_SECRET ? 'set' : 'none',
  PORT);
if (suspicious(STEAM_USERNAME)) console.warn('[config] WARNING: STEAM_USERNAME has surrounding spaces or quotes — fix .env');
if (suspicious(STEAM_PASSWORD)) console.warn('[config] WARNING: STEAM_PASSWORD has surrounding spaces or quotes — quote it in .env as STEAM_PASSWORD=\'...\'');

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
  if (!gcReady || !csgo.haveGCSession) return res.status(503).json({ error: 'GC not ready' });

  try {
    const item = await inspect(url);
    res.json({
      killeater_value: item.killeatervalue ?? null,
      score_type:      item.killeaterscoretype ?? null,
      defindex:        item.defindex ?? null,
      paintindex:      item.paintindex ?? null,
      paintseed:       item.paintseed ?? null,
      float:           item.paintwear ?? null,
      custom_name:     item.customname ?? null,
    });
  } catch (e) {
    res.status(504).json({ error: e.message || String(e) });
  }
});

app.listen(Number(PORT), () => console.log(`Inspect bot HTTP listening on :${PORT}`));

logOn();
