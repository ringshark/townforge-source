# Town Forge multiplayer server (phase 1: presence + chat)

A Cloudflare Worker with one Durable Object per zone (`wild`, `town0`..`town3`).
Players in the same zone see each other move and can chat. It uses the WebSocket
hibernation API, so an idle zone costs nothing, and it fits the free Workers plan.

## Test locally

```
npm install
npx wrangler dev --port 8787 --var ALLOWED_ORIGINS:""   # open to any origin while testing
npm test                                                 # two scripted players: join, move, chat, leave
```

## Deploy

Needs `CLOUDFLARE_API_TOKEN` (an "Edit Cloudflare Workers" token) and
`CLOUDFLARE_ACCOUNT_ID` in the environment, and network access to
`api.cloudflare.com`.

```
npx wrangler deploy
```

It prints the address, e.g. `https://townforge-mp.<subdomain>.workers.dev`. Then
deploy the game with that address as a WebSocket URL:

```
TF_MP_URL=wss://townforge-mp.<subdomain>.workers.dev bash web/deploy_web.sh <build> <site>
```

`web/deploy_web.sh` writes it into the site's `mp-config.js`, and keeps it on later
deploys. Only the origins in `wrangler.toml` (`ALLOWED_ORIGINS`) may connect.
