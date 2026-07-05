// Fake Zeus kill-count bot for reproducing the LevelUp reset without a real kill.
// The counter climbs +1 every 10 seconds on a timer, so as long as the device polls
// more often than that, each poll sees a higher value -> "increase" -> plays LevelUp
// -> hits the path that resets the Zeus. (Set the device to SET interval 1.)
// Point the device at it: SET server http://<this-PC-LAN-IP>:2137
// Mirrors the real bot's response shape:
//   GET /kills?steam=<anything> -> { "killeater_value": N, "name": "Zeus x27" }
// Control endpoints:
//   GET /set?v=N   -> force the counter to N
//   GET /peek      -> read current
// Run: node fakebot.js
const http = require('http');
const PORT = 2137;
const STEP_MS = 10000;   // increment every 10 s
let n = 1;               // starts low so the device caches a low value on the first poll

setInterval(() => { n += 1; console.log(`[fake] tick -> n=${n}`); }, STEP_MS);

http.createServer((req, res) => {
  const u = new URL(req.url, 'http://x');
  res.setHeader('Content-Type', 'application/json');

  if (u.pathname === '/set') {
    n = parseInt(u.searchParams.get('v') || '1', 10);
    console.log(`[fake] set n=${n}`);
    return res.end(JSON.stringify({ ok: true, n }));
  }
  if (u.pathname === '/peek') {
    return res.end(JSON.stringify({ killeater_value: n, name: 'Zeus x27' }));
  }
  if (u.pathname === '/kills') {
    console.log(`[fake] /kills -> ${n}  from ${req.socket.remoteAddress}`);
    return res.end(JSON.stringify({ killeater_value: n, name: 'Zeus x27' }));
  }
  res.statusCode = 404;
  res.end(JSON.stringify({ error: 'not found' }));
}).listen(PORT, '0.0.0.0', () => console.log(`[fake] listening on 0.0.0.0:${PORT} (n +1 every ${STEP_MS/1000}s)`));
