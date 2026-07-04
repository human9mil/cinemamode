# Cinema Mode — Website

The marketing site for [Cinema Mode](https://github.com/human9mil/cinemamode), a one-click cinema mode app for movie lovers with multi-monitor desks. Plain static HTML/CSS — no framework, no backend, no API keys, no build-time secrets, zero JavaScript shipped to the browser. Vite is used only as a build tool, for asset hashing/minification.

Live structure: Hero → Story → Screenshots → Features → Requirements → Getting It → Footer, all in one `index.html`.

## Project Structure

```
website/
├── public/
│   ├── favicon.svg
│   └── _headers          # Security headers, picked up automatically by Cloudflare Pages
├── src/
│   ├── assets/            # Icon + screenshots (png + webp)
│   └── index.css
├── index.html             # All page markup lives here — no components, no JS
├── vite.config.js
└── package.json
```

## Developing Locally

```bash
cd website
npm install
npm run dev
```

This starts the Vite dev server (with HMR) at the URL printed in the terminal.

To check a local production build before deploying:

```bash
npm run build      # outputs to dist/
npm run preview    # serves the built dist/ folder locally
```

## Commands

| Command | Description |
|---|---|
| `npm run dev` | Start the local dev server with hot reload |
| `npm run build` | Production build, output to `dist/` |
| `npm run preview` | Serve the built `dist/` folder locally, to sanity-check a production build |
| `npm run lint` | Run Oxlint against `src/` |

## Deploying to Cloudflare Pages

This site deploys via Cloudflare Pages' **Git integration** — push to GitHub, Cloudflare builds and deploys automatically. There is no manual `wrangler` CLI deploy step for this project.

Because `website/` is a *subfolder* of the `cinemamode` repo (not its own repo), the one setting that matters most is the **Root directory** below — get that wrong and the build will fail or deploy the wrong thing.

### One-time setup

1. Go to the [Cloudflare dashboard](https://dash.cloudflare.com/) → **Workers & Pages** → **Create** → **Pages** → **Connect to Git**.
2. Select the `human9mil/cinemamode` GitHub repository.
   - If it doesn't show up in the list, the Cloudflare GitHub App likely doesn't have access to this repo yet — grant it access from the GitHub App's settings (under your GitHub account → Settings → Applications → Cloudflare Pages) and retry.
3. Configure build settings:

   | Setting | Value |
   |---|---|
   | Framework preset | Vite (auto-detected, or select manually) |
   | Root directory | `website` |
   | Build command | `npm run build` |
   | Build output directory | `dist` |

4. Environment variables: **none needed.** This is a static site with no backend or API keys.
5. Save and deploy.

### Security headers

`public/_headers` is copied into `dist/_headers` by the Vite build. Cloudflare Pages automatically reads that file from the build output and applies the headers (CSP, `X-Frame-Options`, `X-Content-Type-Options`, `Referrer-Policy`, `Permissions-Policy`) — no extra Cloudflare configuration required.

### Ongoing deploys

Once connected, every push to the production branch (typically `main`) triggers an automatic rebuild and redeploy. Pull requests and other branches get their own preview deployment URLs — this is standard Cloudflare Pages behavior, nothing project-specific.

### Gating the site before it's public

To keep the deployment private while reviewing (before announcing it), put it behind **Cloudflare Access** rather than adding any password logic to the site itself:

1. Cloudflare dashboard → **Zero Trust** → (first time) set up a free team name.
2. **Access → Applications → Add an application**, targeting the Pages deployment's domain (`*.pages.dev`, and any custom domain later).
3. Add a policy — simplest is "Allow" your own email; Access emails a one-time code on each visit, no password to manage.
4. When ready to go public, delete the Access application/policy — no site-side changes needed, since this all happens at Cloudflare's edge, in front of the static files.

This keeps the site itself simple (still zero backend, zero auth code) while giving you a real gate during review.

## Troubleshooting

- **Build fails immediately / "no package.json found"**: The Root directory setting is wrong — it must be `website`, not the repo root.
- **Cloudflare can't find the repo in "Connect to Git"**: The Cloudflare GitHub App needs access granted to `human9mil/cinemamode` (or to all repos) from your GitHub account settings.
- **Site loads but images/fonts are blocked or console shows CSP errors**: Check `public/_headers` — the CSP is intentionally strict (`default-src 'self'`, no external domains). Any new external asset (font CDN, analytics, embed, etc.) needs an explicit addition to that policy.
- **Local dev server won't start**: Delete `node_modules` and re-run `npm install`; confirm you're using a reasonably current Node.js (matching whatever Vite 8 requires).

## Environment Variables

None. This is a fully static site — no `.env` file, no secrets, no runtime configuration.

## License

GPL-3.0-or-later, same as the main Cinema Mode project — see the [repository LICENSE](../LICENSE).
