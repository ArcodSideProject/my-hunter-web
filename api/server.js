// my-hunter scoreboard API. Plain Node HTTP server, no dependencies,
// backed by a single JSON file (scoreboard.json) in this directory.
//
// Data shape: { "<name>": { "best": <int>, "tries": <int> } }
//
// Endpoints:
//   GET  /api/score/:name      -> { name, best, tries } (best=0, tries=0 if unknown)
//   POST /api/score            -> body { name, score }
//                                 records one more try for `name`, bumps
//                                 best if score > current best.
//                                 returns { name, best, tries }
//   GET  /api/scoreboard       -> full list, sorted by best desc, capped at 100
//
// Runs on 127.0.0.1:8090, meant to sit behind Caddy at
// hunter.mister-esman.uk/api/*.

const http = require('http');
const fs = require('fs');
const path = require('path');

const DB_PATH = path.join(__dirname, 'scoreboard.json');
const PORT = 8091;
const MAX_NAME_LEN = 20;

// Minimal per-IP rate limiting for the POST endpoint (the only one that
// writes) -- caps abuse without needing an external dependency. Window
// is intentionally short; this is meant to stop a script hammering the
// file, not to be a general-purpose limiter.
const RATE_LIMIT_WINDOW_MS = 10_000;
const RATE_LIMIT_MAX = 10;
const hitsByIp = new Map();

function isRateLimited(ip) {
    const now = Date.now();
    let entry = hitsByIp.get(ip);
    if (!entry || now - entry.windowStart > RATE_LIMIT_WINDOW_MS) {
        entry = { windowStart: now, count: 0 };
        hitsByIp.set(ip, entry);
    }
    entry.count += 1;
    return entry.count > RATE_LIMIT_MAX;
}

// Periodic cleanup so hitsByIp doesn't grow unbounded over a long uptime.
setInterval(() => {
    const now = Date.now();
    for (const [ip, entry] of hitsByIp) {
        if (now - entry.windowStart > RATE_LIMIT_WINDOW_MS * 5) hitsByIp.delete(ip);
    }
}, 60_000).unref();

function loadDb() {
    try {
        const raw = fs.readFileSync(DB_PATH, 'utf8');
        return JSON.parse(raw);
    } catch (e) {
        return {};
    }
}

// Simple write queue so concurrent requests can't interleave a
// read-modify-write and lose an update (single file, no real DB).
let writeChain = Promise.resolve();
function saveDb(db) {
    writeChain = writeChain.then(() => new Promise((resolve, reject) => {
        const tmp = DB_PATH + '.tmp';
        fs.writeFile(tmp, JSON.stringify(db), (err) => {
            if (err) return reject(err);
            fs.rename(tmp, DB_PATH, (err2) => err2 ? reject(err2) : resolve());
        });
    }));
    return writeChain;
}

function sanitizeName(name) {
    if (typeof name !== 'string') return null;
    const trimmed = name.trim().slice(0, MAX_NAME_LEN);
    // Restrict to a safe charset: letters (incl. basic accented), digits,
    // space, underscore, hyphen, apostrophe (default pseudos include
    // possessive combos like "Annie's Bear"). Blocks control chars /
    // HTML / anything that could matter if this is ever rendered
    // unescaped client-side, and keeps the JSON file's keys predictable.
    if (!/^[\p{L}\p{N} _'-]+$/u.test(trimmed)) return null;
    if (trimmed.length === 0) return null;
    return trimmed;
}

const ALLOWED_ORIGIN = 'https://hunter.mister-esman.uk';

function sendJson(res, status, obj) {
    const body = JSON.stringify(obj);
    res.writeHead(status, {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(body),
        'Access-Control-Allow-Origin': ALLOWED_ORIGIN,
    });
    res.end(body);
}

function readBody(req, cb) {
    let data = '';
    let size = 0;
    req.on('data', (chunk) => {
        size += chunk.length;
        if (size > 4096) { req.destroy(); return; } // guard against abuse
        data += chunk;
    });
    req.on('end', () => cb(data));
}

// Bound to 0.0.0.0 because it's reached through the Caddy container via
// the docker bridge (host.docker.internal), not literal 127.0.0.1 from
// inside that container. Externally this is still not directly
// reachable: ufw only allows this port from the docker bridge subnet
// (172.20.0.0/16), matching the same pattern used for this VPS's other
// container->host reverse-proxy targets (diepcustom, lucy-portal).
const HOST = '0.0.0.0';

const server = http.createServer((req, res) => {
    if (req.method === 'OPTIONS') {
        res.writeHead(204, {
            'Access-Control-Allow-Origin': ALLOWED_ORIGIN,
            'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type',
        });
        return res.end();
    }

    const url = new URL(req.url, 'http://localhost');

    if (req.method === 'GET' && url.pathname === '/api/scoreboard') {
        const db = loadDb();
        const list = Object.entries(db)
            .map(([name, v]) => ({ name, best: v.best, tries: v.tries }))
            .sort((a, b) => b.best - a.best)
            .slice(0, 100);
        return sendJson(res, 200, list);
    }

    if (req.method === 'GET' && url.pathname.startsWith('/api/score/')) {
        const name = sanitizeName(decodeURIComponent(url.pathname.slice('/api/score/'.length)));
        if (!name) return sendJson(res, 400, { error: 'invalid name' });
        const db = loadDb();
        const entry = db[name] || { best: 0, tries: 0 };
        return sendJson(res, 200, { name, best: entry.best, tries: entry.tries });
    }

    if (req.method === 'POST' && url.pathname === '/api/score') {
        const ip = req.socket.remoteAddress || 'unknown';
        if (isRateLimited(ip)) return sendJson(res, 429, { error: 'too many requests' });

        return readBody(req, (raw) => {
            let body;
            try { body = JSON.parse(raw); } catch (e) { return sendJson(res, 400, { error: 'invalid json' }); }
            const name = sanitizeName(body.name);
            const score = Number.isFinite(body.score) ? Math.max(0, Math.floor(body.score)) : null;
            if (!name || score === null) return sendJson(res, 400, { error: 'invalid name or score' });
            if (score > 1_000_000) return sendJson(res, 400, { error: 'score out of range' });

            const db = loadDb();
            const entry = db[name] || { best: 0, tries: 0 };
            // Cap total distinct names so a flood of unique names can't
            // grow the JSON file unbounded (disk-fill DoS).
            const MAX_DISTINCT_NAMES = 5000;
            if (!db[name] && Object.keys(db).length >= MAX_DISTINCT_NAMES) {
                return sendJson(res, 503, { error: 'scoreboard full' });
            }
            entry.tries += 1;
            if (score > entry.best) entry.best = score;
            db[name] = entry;

            saveDb(db).then(() => {
                sendJson(res, 200, { name, best: entry.best, tries: entry.tries });
            }).catch(() => {
                sendJson(res, 500, { error: 'write failed' });
            });
        });
    }

    sendJson(res, 404, { error: 'not found' });
});

server.listen(PORT, HOST, () => {
    console.log(`my-hunter scoreboard API listening on ${HOST}:${PORT}`);
});
