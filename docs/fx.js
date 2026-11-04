// PixiJS backdrop: catalog fogScene, full-viewport, no CDN.
const C = { v4jbTeal: 0x6fc3b8, v4jbOrange: 0xf0a030, v4jbIndigo: 0x4a3f8f };
const reduceMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
const clamp = (v, lo, hi) => Math.min(hi, Math.max(lo, v));
const rand = (lo, hi) => lo + Math.random() * (hi - lo);

const pointer = { x: 0.5, y: 0.5 };
addEventListener('pointermove', (e) => {
  pointer.x = e.clientX / innerWidth;
  pointer.y = e.clientY / innerHeight;
}, { passive: true });

start().catch((err) => {
  console.info('fx: animated backdrop disabled:', err?.message || err);
  document.getElementById('fx')?.remove();
});

async function start() {
  const PIXI = await import('./vendor/pixi.min.mjs');
  const app = new PIXI.Application();
  await app.init({
    canvas: document.getElementById('fx'),
    resizeTo: window,
    backgroundAlpha: 0,
    antialias: true,
    autoDensity: true,
    resolution: Math.min(devicePixelRatio || 1, 2),
    preference: 'webgl',
  });

  const glow = glowTexture(PIXI);
  const { width: w0, height: h0 } = app.screen;
  const scene = fogScene(PIXI, glow, w0, h0);
  app.stage.addChild(scene.root);
  bootTicker(app, scene);
}

function bootTicker(app, scene) {
  if (reduceMotion) {
    app.ticker.stop();
    const still = () => {
      const { width: w, height: h } = app.screen;
      scene.update(8, w, h, 0);
      app.render();
    };
    addEventListener('resize', () => requestAnimationFrame(still));
    still();
    return;
  }

  let t = 0;
  app.ticker.add((ticker) => {
    const dt = Math.min(ticker.deltaMS / 1000, 0.05);
    t += dt;
    const { width: w, height: h } = app.screen;
    const ease = Math.min(1, dt * 2.5);
    scene.root.x += ((0.5 - pointer.x) * 40 - scene.root.x) * ease;
    scene.root.y += ((0.5 - pointer.y) * 24 - scene.root.y) * ease;
    scene.update(t, w, h, dt);
  });

  document.addEventListener('visibilitychange', () => {
    if (document.hidden) app.ticker.stop(); else app.ticker.start();
  });
}

function glowTexture(PIXI, size = 64) {
  const c = document.createElement('canvas');
  c.width = c.height = size;
  const g = c.getContext('2d');
  const r = size / 2;
  const grd = g.createRadialGradient(r, r, 0, r, r, r);
  grd.addColorStop(0, 'rgba(255,255,255,1)');
  grd.addColorStop(0.25, 'rgba(255,255,255,0.5)');
  grd.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = grd;
  g.fillRect(0, 0, size, size);
  return PIXI.Texture.from(c);
}

function glowSprite(PIXI, tex, tint) {
  const s = new PIXI.Sprite(tex);
  s.anchor.set(0.5);
  s.tint = tint;
  s.blendMode = 'add';
  return s;
}

function wrap(s, w, h, m = 40) {
  if (s.x < -m) s.x += w + 2 * m; else if (s.x > w + m) s.x -= w + 2 * m;
  if (s.y < -m) s.y += h + 2 * m; else if (s.y > h + m) s.y -= h + 2 * m;
}

function fogScene(PIXI, glow, w0, h0) {
  const root = new PIXI.Container();
  const bars = new PIXI.Graphics();
  bars.blendMode = 'add';
  root.addChild(bars);

  const fog = Array.from({ length: 22 }, (_, i) => {
    const s = glowSprite(PIXI, glow, [C.v4jbTeal, C.v4jbIndigo, 0x8a6fd1][i % 3]);
    s.x = rand(0, w0); s.y = rand(0, h0);
    s.scale.set(rand(6, 12));
    s.alpha = rand(0.05, 0.11);
    s.vx = rand(-12, 12); s.vy = rand(-6, 6);
    root.addChild(s);
    return s;
  });

  const sparks = Array.from({ length: 40 }, () => {
    const s = glowSprite(PIXI, glow, C.v4jbOrange);
    s.x = rand(0, w0); s.y = rand(0, h0);
    s.scale.set(rand(0.08, 0.22));
    s.vy = rand(-30, -10); s.ph = rand(0, 6.28);
    root.addChild(s);
    return s;
  });

  const N = clamp(Math.round(w0 / 7), 60, 260);
  const partials = Array.from({ length: N }, () => ({ sp: rand(0.2, 1.1), ph: rand(0, 6.28) }));

  return {
    root,
    update(t, w, h, dt) {
      for (const s of fog) {
        s.x += s.vx * dt; s.y += s.vy * dt;
        wrap(s, w, h, 300);
      }
      for (const s of sparks) {
        s.y += s.vy * dt;
        s.x += Math.sin(t * 0.7 + s.ph) * 10 * dt;
        s.alpha = 0.35 + 0.45 * Math.sin(t * 2 + s.ph);
        wrap(s, w, h);
      }

      bars.clear();
      const bw = w / N;
      const center = 0.45 + 0.35 * Math.sin(t * 0.09);
      for (let i = 0; i < N; i++) {
        const u = i / N, p = partials[i];
        const tilt = 1 / (1 + i * 0.025);
        const bump = Math.exp(-((u - center) ** 2) / 0.015);
        const amp = 0.35 + 0.65 * (0.5 + 0.5 * Math.sin(t * p.sp + p.ph));
        const bh = h * 0.6 * (0.55 * tilt + 0.6 * bump) * amp;
        const x = i * bw;
        bars.rect(x + bw * 0.2, h - bh, bw * 0.6, bh).fill({ color: C.v4jbTeal, alpha: 0.12 });
        bars.rect(x + bw * 0.1, h - bh - 2, bw * 0.8, 3).fill({ color: C.v4jbTeal, alpha: 0.7 });
      }
    },
  };
}
