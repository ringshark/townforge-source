// Multiplayer zone server address (server/mp, a Cloudflare Worker).
// Empty = multiplayer off. After `wrangler deploy`, set it to the Worker's address, e.g.
//   window.TF_MP_URL = 'wss://townforge-mp.<your-subdomain>.workers.dev';
window.TF_MP_URL = window.TF_MP_URL || '';
