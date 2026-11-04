// HTML com escape por padrão: html`<p>${texto}</p>` escapa o texto; raw(...) passa marcação pronta.

const ESC = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' };
export const esc = s => String(s ?? '').replace(/[&<>"']/g, c => ESC[c]);

class Raw {
  constructor(s) { this.s = String(s); }
  toString() { return this.s; }
}
export const raw = s => (s instanceof Raw ? s : new Raw(s));

function piece(v) {
  if (v == null || v === false) return '';
  if (v instanceof Raw) return v.s;
  if (Array.isArray(v)) return v.map(piece).join('');
  return esc(v);
}

export function html(strings, ...values) {
  let out = strings[0];
  for (let i = 0; i < values.length; i++) out += piece(values[i]) + strings[i + 1];
  return new Raw(out);
}

/**
 * Texto de um dado do app na página de idioma `lang` ('en' ou 'pt'): no inglês, o texto em
 * inglês; no português, o texto do aparelho em pt-BR, marcado com lang="en" quando o app
 * ainda não tem a tradução.
 */
export function langText(t, lang = 'pt') {
  if (!t) return '';
  if (typeof t === 'string') return esc(t);
  if (lang === 'en') return esc(t.en ?? t.text);
  if (!t.lang || t.lang === 'pt-BR') return esc(t.text);
  return `<span lang="${esc(t.lang)}">${esc(t.text)}</span>`;
}

/** Ícones de traço do app (mesmo desenho do protótipo e do XdIcons), 24px de viewBox. */
export const ICONS = {
  download: '<path d="M12 4v11M7 10.5l5 5 5-5M5 20h14"/>',
  arrowR: '<path d="M5 12h14M13 6l6 6-6 6"/>',
  chevD: '<path d="M6 9.5l6 6 6-6"/>',
  external: '<path d="M14 5h5v5M19 5l-8 8"/><path d="M18 14v4.5a1.5 1.5 0 0 1-1.5 1.5h-11A1.5 1.5 0 0 1 4 18.5v-11A1.5 1.5 0 0 1 5.5 6H10"/>',
  search: '<circle cx="11" cy="11" r="6.5"/><path d="M20 20l-4.3-4.3"/>',
  menu: '<path d="M4 7h16M4 12h16M4 17h16"/>',
  x: '<path d="M6.5 6.5l11 11M17.5 6.5l-11 11"/>',
  book: '<path d="M5 4.5h9.5a3 3 0 0 1 3 3V20H8a3 3 0 0 1-3-3z"/><path d="M5 17a3 3 0 0 1 3-3h9.5"/>',
  gamepad: '<path d="M7.5 7.5h9a4.5 4.5 0 0 1 4.4 5.4l-.6 3a2.6 2.6 0 0 1-4.6 1.1L14.2 15H9.8l-1.5 2a2.6 2.6 0 0 1-4.6-1.1l-.6-3A4.5 4.5 0 0 1 7.5 7.5z"/><path d="M8 10.3v3M6.5 11.8h3"/><circle cx="15.3" cy="11" r=".9" fill="currentColor" stroke="none"/><circle cx="17.3" cy="13" r=".9" fill="currentColor" stroke="none"/>',
  code: '<path d="M9 7.5L4.5 12 9 16.5M15 7.5l4.5 4.5-4.5 4.5"/>',
  chip: '<rect x="6.5" y="6.5" width="11" height="11" rx="2"/><path d="M9.5 3v3.5M14.5 3v3.5M9.5 17.5V21M14.5 17.5V21M3 9.5h3.5M3 14.5h3.5M17.5 9.5H21M17.5 14.5H21"/>',
  shield: '<path d="M12 3.5l7 2.8v5.4c0 4.4-3 7.6-7 8.8-4-1.2-7-4.4-7-8.8V6.3z"/><path d="M9 12l2.2 2.2L15.5 10"/>',
  chart: '<path d="M4 4v16h16"/><path d="M8.5 16v-4.5M12.5 16V8.5M16.5 16v-6"/>',
  sliders: '<path d="M4 7h9M17 7h3M4 17h3M11 17h9"/><circle cx="15" cy="7" r="2"/><circle cx="9" cy="17" r="2"/>',
  info: '<circle cx="12" cy="12" r="8.5"/><path d="M12 11v5.2M12 7.7v.2"/>',
  warn: '<path d="M12 4.2l8.8 15.3H3.2z"/><path d="M12 10v4.2M12 17v.2"/>',
  check: '<path d="M5 12.5l4.5 4.5L19 7.5"/>',
  clock: '<circle cx="12" cy="12" r="8.5"/><path d="M12 7.5V12l3 2"/>',
  flask: '<path d="M9.5 3.5h5M10.5 3.5v6l-5 8.6A1.6 1.6 0 0 0 6.9 20.5h10.2a1.6 1.6 0 0 0 1.4-2.4l-5-8.6v-6"/><path d="M7.8 15h8.4"/>',
  layers: '<path d="M12 4l8.5 4.2L12 12.4 3.5 8.2z"/><path d="M3.5 12.2L12 16.4l8.5-4.2"/><path d="M3.5 16.2L12 20.4l8.5-4.2"/>',
  chat: '<path d="M4 5.5h16a1 1 0 0 1 1 1v9.5a1 1 0 0 1-1 1H9.5L5 20.5V17H4a1 1 0 0 1-1-1V6.5a1 1 0 0 1 1-1z"/><path d="M7.5 10h9M7.5 13h6"/>',
  heart: '<path d="M12 20s-7.5-4.6-8.7-9.2A4.6 4.6 0 0 1 12 8a4.6 4.6 0 0 1 8.7 2.8C19.5 15.4 12 20 12 20z"/>',
  tag: '<path d="M3.5 12.5V4.5a1 1 0 0 1 1-1h8l8 8-9 9z"/><circle cx="8" cy="8" r="1.4"/>',
  copy: '<rect x="8.5" y="8.5" width="11" height="11" rx="2"/><path d="M15.5 8.5V6A1.5 1.5 0 0 0 14 4.5H6A1.5 1.5 0 0 0 4.5 6v8A1.5 1.5 0 0 0 6 15.5h2.5"/>',
  link: '<path d="M10 14a4 4 0 0 0 5.7 0l3-3a4 4 0 0 0-5.7-5.7l-1 1"/><path d="M14 10a4 4 0 0 0-5.7 0l-3 3a4 4 0 0 0 5.7 5.7l1-1"/>',
  sun: '<circle cx="12" cy="12" r="4"/><path d="M12 2.8v2.4M12 18.8v2.4M2.8 12h2.4M18.8 12h2.4M5.5 5.5l1.7 1.7M16.8 16.8l1.7 1.7M5.5 18.5l1.7-1.7M16.8 7.2l1.7-1.7"/>',
  moon: '<path d="M19.5 14.5A8 8 0 0 1 9.5 4.5a8 8 0 1 0 10 10z"/>',
  monitor: '<rect x="3" y="4.5" width="18" height="12" rx="2"/><path d="M8.5 20h7M12 16.5V20"/>',
  phone: '<rect x="7" y="3" width="10" height="18" rx="2.2"/><path d="M11 17.5h2"/>',
  bug: '<rect x="7" y="7.5" width="10" height="12" rx="5"/><path d="M12 7.5V19.5M3.5 13H7M17 13h3.5M5 8.5l2.5 1.5M19 8.5l-2.5 1.5M5 18l2.5-1.5M19 18l-2.5-1.5M9.5 7.5l-1-3M14.5 7.5l1-3"/>',
  wrench: '<path d="M14.7 6.3a4 4 0 0 0-5.4 5.4L4 17l3 3 5.3-5.3a4 4 0 0 0 5.4-5.4l-2.4 2.4-2.6-.4-.4-2.6z"/>',
  play: '<path d="M8 5.5v13l10.5-6.5z" fill="currentColor" stroke="none"/>',
  globe: '<circle cx="12" cy="12" r="8.5"/><path d="M3.5 12h17M12 3.5c2.3 2.5 3.5 5.3 3.5 8.5s-1.2 6-3.5 8.5c-2.3-2.5-3.5-5.3-3.5-8.5s1.2-6 3.5-8.5z"/>',
  history: '<path d="M4.5 12a7.5 7.5 0 1 0 2.2-5.3"/><path d="M4.5 4.5v4h4"/><path d="M12 8v4l2.5 2"/>',
};

export function icon(name, size = 20, cls = '') {
  const p = ICONS[name];
  if (!p) throw new Error(`ícone desconhecido: ${name}`);
  return raw(`<svg class="xi${cls ? ' ' + cls : ''}" width="${size}" height="${size}" viewBox="0 0 24 24" aria-hidden="true" focusable="false">${p}</svg>`);
}

/** Bytes em MB/KB no formato brasileiro. */
export function fmtBytes(n) {
  if (n == null) return '';
  if (n >= 1024 * 1024) return `${(n / 1024 / 1024).toLocaleString('pt-BR', { maximumFractionDigits: 1 })} MB`;
  return `${Math.round(n / 1024).toLocaleString('pt-BR')} KB`;
}

/** Data ISO como "05/10/2026". */
export function fmtDate(iso) {
  if (!iso) return '';
  const d = new Date(iso);
  return d.toLocaleDateString('pt-BR', { timeZone: 'America/Sao_Paulo', day: '2-digit', month: '2-digit', year: 'numeric' });
}
