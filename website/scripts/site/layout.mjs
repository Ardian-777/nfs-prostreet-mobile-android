// Moldura comum das páginas: <head> com metadados, cabeçalho, rodapé e a busca, no idioma da
// página (page.lang: 'en' na raiz, 'pt' em pt-br/). Todos os links são relativos à página
// (rel), então o site funciona em qualquer subdiretório.
import { html, raw, icon } from '../lib/html.mjs';
import { rel } from '../lib/paths.mjs';
import { lang as langOf, LANGS, LANG_IDS, DEFAULT_LANG } from './i18n.mjs';

/** Script do tema antes da pintura: só lê a escolha guardada (auto, claro ou escuro). */
const THEME_BOOT = "try{var t=localStorage.getItem('xdr-theme');if(t==='light'||t==='dark')document.documentElement.dataset.theme=t}catch(e){}";

export function shell(site, page, content) {
  const L = langOf(page.lang || DEFAULT_LANG);
  const { tx } = L;
  // a 404 é servida em qualquer endereço, então usa caminhos absolutos a partir da base do site
  const r = to => (page.absolute ? site.config.basePath + to.replace(/^\.\/?$/, '') : rel(page.path, to));
  const a = file => `${r('assets/' + file)}?v=${site.assetVersion}`;
  const url = site.config.url + page.path;
  const title = page.path === L.home ? `${site.config.name}: ${page.title}` : `${page.title} · ${site.config.name}`;
  const stable = site.v.stable;
  const desc = page.description;
  const ogImage = site.config.url + 'assets/brand/og-image.png';
  const alt = page.alternates || {};
  const other = LANG_IDS.find(x => x !== L.id);
  const nav = [
    { id: 'docs', href: L.docs, text: tx('Documentation', 'Documentação') },
    { id: 'simulador', href: L.simPath, text: tx('Simulator', 'Simulador') },
    { id: 'historico', href: site.docPath(L.id, 'historico'), text: tx('What’s new', 'Novidades') },
  ];
  return html`<!doctype html>
<html lang="${L.html}" data-root="${page.absolute ? site.config.basePath : r('')}" data-page="${page.path}" data-search="${r(`assets/search-${L.id}.json`)}"${alt[other] != null ? raw(` data-alt="${r(alt[other])}"`) : ''}>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>${title}</title>
<meta name="description" content="${desc}">
<link rel="canonical" href="${url}">
${LANG_IDS.filter(x => alt[x] != null).map(x => raw(`<link rel="alternate" hreflang="${LANGS[x].html}" href="${site.config.url + alt[x]}">\n`))}${alt[DEFAULT_LANG] != null ? raw(`<link rel="alternate" hreflang="x-default" href="${site.config.url + alt[DEFAULT_LANG]}">\n`) : ''}<meta name="color-scheme" content="dark light">
<meta name="theme-color" content="#0f1214" media="(prefers-color-scheme: dark)">
<meta name="theme-color" content="#f2f4f1" media="(prefers-color-scheme: light)">
<meta property="og:type" content="${page.ogType || 'website'}">
<meta property="og:site_name" content="${site.config.name}">
<meta property="og:locale" content="${L.og}">
${alt[other] != null ? raw(`<meta property="og:locale:alternate" content="${LANGS[other].og}">\n`) : ''}<meta property="og:title" content="${title}">
<meta property="og:description" content="${desc}">
<meta property="og:url" content="${url}">
<meta property="og:image" content="${ogImage}">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta property="og:image:alt" content="${tx('Xendroid+ logo and the text: Xbox 360 emulation on Android, tuned for Snapdragon and Adreno.', 'Logo do Xendroid+ e o texto: Emulação de Xbox 360 no Android, ajustada para Snapdragon e Adreno.')}">
<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="${title}">
<meta name="twitter:description" content="${desc}">
<meta name="twitter:image" content="${ogImage}">
<link rel="icon" href="${r('favicon.ico')}" sizes="32x32">
<link rel="icon" href="${r('assets/brand/favicon-32.png')}" type="image/png">
<link rel="apple-touch-icon" href="${r('assets/brand/apple-touch-icon.png')}">
<link rel="manifest" href="${r('site.webmanifest')}">
<link rel="preload" href="${r('assets/fonts/barlow-400.woff2')}" as="font" type="font/woff2" crossorigin>
<link rel="preload" href="${r('assets/fonts/barlow-semi-condensed-700.woff2')}" as="font" type="font/woff2" crossorigin>
<link rel="stylesheet" href="${a('css/site.css')}">
${page.styles ? page.styles.map(s => raw(`<link rel="stylesheet" href="${a(s)}">\n`)) : ''}
<script>${raw(THEME_BOOT)}</script>
${page.head || ''}
</head>
<body class="${page.bodyClass || ''}">
<a class="skip-link" href="#conteudo">${tx('Skip to content', 'Pular para o conteúdo')}</a>
<header class="site-header">
  <div class="wrap">
    <a class="brand" href="${r(L.home)}" aria-label="${tx('Xendroid+, home page', 'Xendroid+, página inicial')}">
      <img src="${r('assets/brand/mark-64.webp')}" width="34" height="34" alt="">
      <span>Xendroid<span class="plus">+</span></span>
    </a>
    <nav class="site-nav" id="site-nav" aria-label="${tx('Main', 'Principal')}">
      ${nav.map(n => html`<a href="${r(n.href)}"${page.section === n.id ? raw(' aria-current="page"') : ''}>${n.text}</a>`)}
      <a class="ext" href="${site.config.repoUrl}" rel="noopener">${icon('code', 18)}${tx('Source code', 'Código-fonte')}</a>
    </nav>
    <div class="header-tools">
      <button class="search-btn" type="button" data-open-search aria-haspopup="dialog" aria-controls="busca" aria-label="${tx('Search', 'Buscar')}">${icon('search', 18)}<span class="label">${tx('Search', 'Buscar')}</span><kbd>/</kbd></button>
      ${alt[other] != null ? html`<a class="lang-btn" href="${r(alt[other])}" hreflang="${LANGS[other].html}" lang="${LANGS[other].html}" data-lang-switch="${other}">${icon('globe', 18)}<span class="visually-hidden">${LANGS[other].name}: </span>${LANGS[other].short}</a>` : ''}
      <button class="icon-btn" type="button" id="theme-btn" aria-label="${tx('Theme: automatic', 'Tema: automático')}" title="${tx('Theme: automatic', 'Tema: automático')}">${icon('monitor', 20)}</button>
      ${stable && stable.apk ? html`<a class="button primary small header-dl" href="${stable.apk.url}">${icon('download', 18)}${tx('Download', 'Baixar')}</a>` : ''}
      <button class="icon-btn menu-btn" type="button" aria-expanded="false" aria-controls="site-nav" aria-label="${tx('Menu', 'Menu')}">${icon('menu', 22)}</button>
    </div>
  </div>
</header>
<main id="conteudo" tabindex="-1">
${content}
</main>
${footer(site, page, r, L)}
<dialog class="search-dialog" id="busca" aria-label="${tx('Search the documentation', 'Buscar na documentação')}">
  <div class="search-field">
    ${icon('search', 20)}
    <label class="visually-hidden" for="busca-q">${tx('Search the documentation', 'Buscar na documentação')}</label>
    <input id="busca-q" type="search" placeholder="${tx('Search the docs and the settings', 'Buscar na documentação e nos ajustes')}" autocomplete="off" spellcheck="false" aria-controls="busca-res" aria-describedby="busca-dica">
    <button class="icon-btn" type="button" data-close-search aria-label="${tx('Close search', 'Fechar a busca')}">${icon('x', 20)}</button>
  </div>
  <div class="search-results" id="busca-res" aria-live="polite"><p class="search-hint" id="busca-dica">${tx('Type to search every documentation page, the app settings and the TOML keys.', 'Digite para buscar em todas as páginas da documentação, nos ajustes do app e nas chaves do TOML.')}</p></div>
  <div class="search-foot"><span><kbd>↑</kbd> <kbd>↓</kbd> ${tx('choose', 'escolher')}</span><span><kbd>Enter</kbd> ${tx('open', 'abrir')}</span><span><kbd>Esc</kbd> ${tx('close', 'fechar')}</span></div>
</dialog>
<script type="module" src="${a('js/site.js')}"></script>
${page.scripts || ''}
</body>
</html>
`;
}

function footer(site, page, r, L) {
  const { tx } = L;
  const { stable, dev } = site.v;
  const doc = slug => r(site.docPath(L.id, slug));
  const alt = page.alternates || {};
  return html`<footer class="site-footer">
  <div class="wrap">
    <div class="pins" aria-hidden="true"></div>
    <div class="footer-cols">
      <section>
        <h2>Xendroid+</h2>
        <ul>
          ${stable && stable.apk ? html`<li><a href="${stable.apk.url}">${tx(`Download ${stable.label.toLowerCase()} (APK)`, `Baixar a ${stable.label.toLowerCase()} (APK)`)}</a></li>` : ''}
          <li><a href="${site.config.repoUrl}/releases" rel="noopener">${tx('All releases', 'Todas as releases')}</a></li>
          <li><a href="${doc('historico')}">${tx('What’s new and release history', 'Novidades e histórico')}</a></li>
          <li><a href="${r(L.simPath)}">${tx('Interface simulator', 'Simulador da interface')}</a></li>
        </ul>
      </section>
      <section>
        <h2>${tx('Documentation', 'Documentação')}</h2>
        <ul>
          <li><a href="${doc('primeiros-passos')}">${tx('Getting started', 'Primeiros passos')}</a></li>
          <li><a href="${doc('requisitos')}">${tx('Requirements', 'Requisitos')}</a></li>
          <li><a href="${doc('problemas')}">${tx('Troubleshooting and logs', 'Problemas e logs')}</a></li>
          <li><a href="${doc('referencia-de-ajustes')}">${tx('Settings reference', 'Referência de ajustes')}</a></li>
        </ul>
      </section>
      <section>
        <h2>${tx('Project', 'Projeto')}</h2>
        <ul>
          <li><a href="${site.config.repoUrl}" rel="noopener">${tx('Source code on GitHub', 'Código-fonte no GitHub')}</a></li>
          <li><a href="${doc('compilar-e-contribuir')}">${tx('Build and contribute', 'Compilar e contribuir')}</a></li>
          <li><a href="${site.config.discord}" rel="noopener">${tx('Community on Discord', 'Comunidade no Discord')}</a></li>
          <li><a href="${site.config.support}" rel="noopener">${tx('Support the project', 'Apoie o projeto')}</a></li>
          <li><a href="${doc('dados-do-site')}">${tx('Where the site data comes from', 'De onde vêm os dados do site')}</a></li>
        </ul>
      </section>
    </div>
    ${Object.keys(alt).length > 1 ? html`<p class="footer-lang"><span>${tx('Language', 'Idioma')}:</span>${LANG_IDS.filter(x => alt[x] != null).map(x => html`<a href="${r(alt[x])}" hreflang="${LANGS[x].html}" lang="${LANGS[x].html}"${x === L.id ? raw(' aria-current="true"') : ''}>${LANGS[x].name}</a>`)}</p>` : ''}
    <div class="footer-legal">
      ${tx(
        html`<p>Xendroid+ is an open-source, non-commercial project created and maintained by <a href="https://github.com/phforner0" rel="noopener">phforner0</a>. It continues XenDroid, by rfandango, and uses the <a href="https://github.com/xenia-project/xenia" rel="noopener">Xenia</a> core. It is not affiliated with or endorsed by Microsoft; Xbox and Xbox 360 are trademarks of Microsoft Corporation.</p>
      <p>No games, disc images, firmware, system files or keys are included or distributed. Only use copies of games you own.</p>`,
        html`<p>O Xendroid+ é um projeto de código aberto, sem fins comerciais, criado e mantido por <a href="https://github.com/phforner0" rel="noopener">phforner0</a>. Continua o XenDroid, de rfandango, e usa o núcleo do <a href="https://github.com/xenia-project/xenia" rel="noopener">Xenia</a>. Não tem vínculo com a Microsoft nem é endossado por ela; Xbox e Xbox 360 são marcas da Microsoft Corporation.</p>
      <p>Nenhum jogo, imagem de disco, firmware, arquivo de sistema ou chave é incluído ou distribuído. Use apenas cópias de jogos que você possui.</p>`)}
    </div>
    <p class="footer-meta">${tx(
      `Generated on ${L.date(site.builtAt)} from commit ${dev.short}.${stable ? ` Stable: ${stable.label.toLowerCase()} (${stable.short}).` : ''}${site.data.sameChannels ? ' The current main has the same settings and data.' : ''}`,
      `Gerado em ${L.date(site.builtAt)} a partir do commit ${dev.short}.${stable ? ` Estável: ${stable.label.toLowerCase()} (${stable.short}).` : ''}${site.data.sameChannels ? ' O main atual tem os mesmos ajustes e dados.' : ''}`)}</p>
  </div>
</footer>`;
}
