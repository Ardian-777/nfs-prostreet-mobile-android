// Página inicial, em inglês (raiz) e em português (pt-br/). Números e fatos vêm dos dados
// extraídos do código (versão estável); o que não dá para conferir no repositório não entra.
import { html, raw, icon } from '../lib/html.mjs';
import { rel } from '../lib/paths.mjs';

function tourPanels(site, L) {
  const { tx } = L;
  const s = site.stats;
  const doc = (slug, hash = '') => site.docPath(L.id, slug) + hash;
  const sim = hash => `${L.simPath}#${hash}`;
  const seeSim = tx('See it in the simulator', 'Ver no simulador');
  return [
    {
      id: 'biblioteca', tab: tx('Library and details', 'Biblioteca e ficha'), shot: 'ficha',
      title: tx('Your games in a library that understands the Xbox 360', 'Os seus jogos numa biblioteca que entende o Xbox 360'),
      points: tx([
        'Finds ISO, ZAR, GOD, folders with default.xex and XBLA packages in your folders, with search by name or Title ID, favorites, collections and a format filter.',
        'Each game’s details page brings together Play, Start with… (profile, driver, executable, without patches or without the game’s settings, for that launch only), sessions, patches and saves.',
        'With a controller connected, the interface switches to controller mode by itself: a cover carousel, tabs on LB and RB and button hints always in view.',
      ], [
        'Encontra ISO, ZAR, GOD, pastas com default.xex e pacotes XBLA nas suas pastas, com busca por nome ou Title ID, favoritos, coleções e filtro por formato.',
        'A ficha de cada jogo junta Jogar, Iniciar com… (perfil, driver, executável, sem patches ou sem os ajustes do jogo, só nesta abertura), as sessões, os patches e os saves.',
        'Com um controle conectado, a interface troca sozinha para o modo controle: carrossel de capas, abas por LB e RB e dicas de botão sempre à vista.',
      ]),
      links: [[doc('interface'), tx('App interface', 'Interface do app')], [sim('library'), seeSim]],
    },
    {
      id: 'menu', tab: tx('In-game menu and HUD', 'Menu em jogo e HUD'), shot: 'menu-imagem',
      title: tx('Settings in the middle of a game, saved for that game', 'Ajustes no meio do jogo, guardados para ele'),
      points: tx([
        'The menu slides in from the left edge, with Back or the controller’s Guide button, and pauses the game by default.',
        'Five categories: Image, Performance, HUD, Controls and Session. Scaling, antialiasing, sharpness and dither change right away.',
        'What you change is saved for that game, with Undo this session’s changes and Use these changes in every game. The HUD shows FPS, frame time, CPU, GPU, memory, temperatures and battery.',
      ], [
        'O menu abre deslizando da borda esquerda, pelo Voltar ou pelo botão Guia do controle, e pausa o jogo por padrão.',
        'Cinco categorias: Imagem, Desempenho, HUD, Controles e Sessão. Escala, antisserrilhamento, nitidez e pontilhado mudam na hora.',
        'O que você muda fica guardado para aquele jogo, com Desfazer as mudanças da sessão e Usar em todos os jogos. O HUD mostra FPS, tempo de quadro, CPU, GPU, memória, temperaturas e bateria.',
      ]),
      links: [[doc('interface', tx('#in-game-menu', '#menu-em-jogo')), tx('In-game menu', 'Menu em jogo')], [sim('ingame'), seeSim]],
    },
    {
      id: 'ajustes', tab: tx('Settings', 'Ajustes'), shot: 'configuracoes',
      title: tx(`${s.settings} settings, each one saying when it applies`, `${s.settings} ajustes, cada um dizendo quando vale`),
      points: tx([
        `Three levels of detail: Essential (${s.essential}), Advanced (${s.advanced}) and All (${s.settings}). Each row says whether the value is the default, whether it changed and whether it applies right away or at the next launch.`,
        'The same settings apply to every game in Settings, or to one game from its details page, saved in config/<Title ID>.config.toml, the file the core reads.',
        'The Performance, Balanced and Quality presets apply several settings at once, with Undo.',
      ], [
        `Três níveis de detalhe: Essencial (${s.essential}), Avançado (${s.advanced}) e Tudo (${s.settings}). Cada linha diz se o valor é o padrão, se mudou e se vale na hora ou na próxima abertura.`,
        'Os mesmos ajustes valem para todos os jogos em Configurações ou só para um jogo na ficha, gravados em config/<Title ID>.config.toml, o arquivo que o núcleo lê.',
        'Predefinições Desempenho, Equilíbrio e Qualidade aplicam vários ajustes de uma vez, com Desfazer.',
      ]),
      links: [[doc('referencia-de-ajustes'), tx('Settings reference', 'Referência de ajustes')], [sim('settings'), seeSim]],
    },
    {
      id: 'drivers', tab: 'Drivers', shot: 'drivers',
      title: tx('Turnip drivers per game, checked before they are installed', 'Drivers Turnip por jogo, conferidos antes de instalar'),
      points: tx([
        'Download packages from sources on GitHub (the default is K11MCH1/AdrenoToolsDrivers) or import a ZIP; the app checks the arm64 Vulkan library and the SHA-256 when the release publishes one.',
        'Pick a driver for every game, one per game or one just for the next launch; the system driver is always one tap away.',
        `The ${s.turnipFlags} Turnip flags (TU_DEBUG) are listed one by one, with sysmem as the default. Custom drivers only load on Adreno GPUs.`,
      ], [
        'Baixe pacotes de fontes no GitHub (a padrão é K11MCH1/AdrenoToolsDrivers) ou importe um ZIP; o app confere a biblioteca Vulkan arm64 e o SHA-256 quando a release publica um.',
        'Escolha um driver para todos os jogos, um por jogo ou um só para a próxima abertura; o driver do sistema fica sempre a um toque.',
        `As ${s.turnipFlags} flags do Turnip (TU_DEBUG) aparecem uma a uma, com sysmem como padrão. Drivers personalizados só carregam em GPUs Adreno.`,
      ]),
      links: [[doc('drivers'), tx('GPU drivers', 'Drivers da GPU')], [sim('drivers'), seeSim]],
    },
    {
      id: 'controles', tab: tx('Controls', 'Controles'), shot: 'controles-moderno',
      title: tx('Touch, a physical controller and even another phone', 'Toque, controle físico e até outro celular'),
      points: tx([
        'Touch controls in the Modern or Classic look, with a layout editor, layouts saved per game and per orientation, touch camera and gyroscope.',
        'Physical controllers play as P1 to P4, with mapping for the 16 buttons, rumble per controller and a screen to test every button.',
        'Phone as controller (experimental): another phone on the same local network joins as P2 to P4 with a 6-digit code.',
      ], [
        'Controles de toque no visual Moderno ou Clássico, com editor de layout, layouts salvos por jogo e por orientação, câmera por toque e giroscópio.',
        'Controles físicos jogam como P1 a P4, com mapeamento dos 16 botões, vibração por controle e uma tela para testar cada botão.',
        'Celular como controle (experimental): outro telefone na mesma rede local entra como P2 a P4 com um código de 6 dígitos.',
      ]),
      links: [[doc('controles'), tx('Controls', 'Controles')], [sim('controls'), seeSim]],
    },
    {
      id: 'diagnostico', tab: tx('Diagnostics', 'Diagnóstico'), shot: 'diagnostico',
      title: tx('Every session leaves a trail you can share', 'Cada sessão deixa um rastro que dá para compartilhar'),
      points: tx([
        `The app keeps the latest sessions (${s.logSessions} by default), with the emulator log, the app’s logcat and how each one ended.`,
        'Sharing produces a cleaned copy: paths, gamertags, XUIDs, IP addresses, e-mail addresses, passwords and tokens are removed first.',
        'At the end of a session with problems, the app suggests up to three changes with the reason in numbers, and Compare runs measures one change at a time (A B B A).',
      ], [
        `O app guarda as últimas sessões (${s.logSessions} por padrão), com o log do emulador, o logcat do app e como cada uma terminou.`,
        'Compartilhar gera uma cópia limpa: caminhos, gamertags, XUIDs, endereços IP, e-mails, senhas e tokens saem antes.',
        'No fim de uma sessão com problema, o app sugere até três mudanças com o motivo em números, e Comparar execuções mede uma mudança por vez (A B B A).',
      ]),
      links: [[doc('problemas'), tx('Troubleshooting and logs', 'Problemas e logs')], [sim('diagnostics'), seeSim]],
    },
  ];
}

export function landing(site, L) {
  const { tx } = L;
  const PATH = L.home;
  const { stable } = site.v;
  const name = stable ? stable.label.toLowerCase() : null;
  const page = {
    path: PATH,
    lang: L.id,
    alternates: site.alternatesOf('home'),
    section: 'inicio',
    title: tx('Xbox 360 emulator for Android', 'emulador de Xbox 360 para Android'),
    description: tx(
      `Xbox 360 emulator for Android, focused on Snapdragon and Adreno GPUs. Download ${name || 'the latest version'}, check the requirements and limitations, the documentation and an interface simulator.`,
      `Emulador de Xbox 360 para Android, focado em Snapdragon e GPUs Adreno. Baixe a ${name || 'versão mais recente'}, veja requisitos e limitações, a documentação e um simulador da interface.`),
    head: jsonLd(site, L),
  };
  const r = to => rel(PATH, to);
  const doc = (slug, hash = '') => r(site.docPath(L.id, slug) + hash);
  const s = site.stats;
  const est = site.ch.est;
  const panels = tourPanels(site, L);
  const app = est.app;
  const fromCode = name ? tx(`the code of ${name}`, `o código da ${name}`) : tx('the published code', 'o código publicado');

  const content = html`
<section class="hero">
  <div class="wrap">
    <div class="hero-copy">
      <span class="eyebrow">${tx('Xbox 360 emulator', 'Emulador de Xbox 360')} · Android ${app.minAndroid}+ · ${app.abi}</span>
      <h1>Xendroid<span class="plus">+</span></h1>
      <p class="lead">${tx(
        html`Xbox 360 emulation on Android, tuned for <strong>Snapdragon and Adreno GPUs</strong>. It continues XenDroid on the Xenia core, with an interface for touch and controllers, per-game settings applied at launch and a diagnosis of every session on the device itself.`,
        html`Emulação de Xbox 360 no Android, ajustada para <strong>Snapdragon e GPUs Adreno</strong>. Continua o XenDroid sobre o núcleo do Xenia, com uma interface para toque e controle, ajustes por jogo aplicados na abertura e o diagnóstico de cada sessão no próprio aparelho.`)}</p>
      ${stable && stable.apk ? html`<div class="download">
        <a class="button primary" href="${stable.apk.url}">${icon('download', 22)}${tx(`Download ${name}`, `Baixar a ${name}`)}</a>
        <p class="download-meta">
          <span>APK · ${L.bytes(stable.apk.size)}</span>
          <span>${tx('published', 'publicada em')} ${L.date(stable.date)}</span>
          ${stable.apk.sha256 ? html`<span>SHA-256 <span class="u-mono" title="${stable.apk.sha256}">${stable.apk.sha256.slice(0, 12)}…</span> <button class="button quiet small" type="button" data-copy="${stable.apk.sha256}" data-copied="${tx('SHA-256 copied', 'SHA-256 copiado')}">${tx('Copy', 'Copiar')}</button></span>` : ''}
        </p>
        <p class="download-meta"><a href="${stable.url}" rel="noopener">${tx('Release notes', 'Notas desta versão')}</a><a href="${site.config.repoUrl}/releases" rel="noopener">${tx('All versions', 'Todas as versões')}</a><a href="${doc('instalacao')}">${tx('How to install and update', 'Como instalar e atualizar')}</a></p>
      </div>` : html`<p class="hero-note">${icon('warn', 18)}<span>${tx(html`No published release was found in this build. See <a href="${site.config.repoUrl}/releases" rel="noopener">the releases on GitHub</a>.`, html`Nenhuma release publicada foi encontrada neste build. Veja <a href="${site.config.repoUrl}/releases" rel="noopener">as releases no GitHub</a>.`)}</span></p>`}
      <p class="hero-note">${icon('warn', 18)}<span>${tx(
        html`<b>Experimental.</b> How each game runs depends on the game, the device and the GPU driver: some titles run well, others still have glitches or do not boot.`,
        html`<b>Experimental.</b> Como cada jogo roda depende do jogo, do aparelho e do driver da GPU: alguns títulos rodam bem, outros ainda têm falhas ou não iniciam.`)}</span></p>
      <p class="hero-links"><a href="${doc('primeiros-passos')}">${icon('book', 18)}${tx('Getting started', 'Primeiros passos')}</a><a href="${r(L.simPath)}">${icon('phone', 18)}${tx('Explore the interface in the simulator', 'Explorar a interface no simulador')}</a></p>
    </div>
    <div class="hero-shot">${site.shot(PATH, 'biblioteca', { eager: true, lang: L.id, caption: tx(raw('<b>The library in touch mode.</b> Image generated by the app’s screen tests, with sample covers and games.'), raw('<b>A biblioteca no modo toque.</b> Imagem gerada pelos testes de tela do app, com capas e jogos de exemplo.')) })}</div>
  </div>
</section>

<section class="section" aria-labelledby="tour-t">
  <div class="wrap">
    <div class="section-head">
      <span class="eyebrow">${tx('Inside the app', 'Por dentro do app')}</span>
      <h2 id="tour-t">${tx('The Xendroid+ screens', 'As telas do Xendroid+')}</h2>
      <p>${tx(
        'Images generated from the interface code by the project’s screen tests, which run with the app in Portuguese. The games, covers and numbers in them are sample data.',
        'Imagens geradas a partir do código da interface pelos testes de tela do projeto. Os jogos, capas e números nelas são dados de exemplo.')}</p>
    </div>
    <div class="tour" data-tabs>
      <div class="tour-tabs" role="tablist" aria-label="${tx('App screens', 'Telas do app')}" hidden>
        ${panels.map((p, i) => html`<button type="button" role="tab" id="tab-${p.id}" aria-controls="painel-${p.id}" aria-selected="${i === 0}" tabindex="${i === 0 ? 0 : -1}">${p.tab}</button>`)}
      </div>
      ${panels.map(p => html`<div class="tour-panel" id="painel-${p.id}" role="tabpanel" aria-labelledby="tab-${p.id}">
        ${site.shot(PATH, p.shot, { lang: L.id })}
        <div>
          <h3>${p.title}</h3>
          <ul>${p.points.map(t => html`<li>${t}</li>`)}</ul>
          <p class="more-links">${p.links.map(([href, t]) => html`<a href="${r(href)}">${t}</a>`)}</p>
        </div>
      </div>`)}
    </div>
  </div>
</section>

<section class="section" aria-labelledby="diff-t">
  <div class="wrap">
    <div class="section-head">
      <span class="eyebrow">${tx('What is different', 'O que é diferente')}</span>
      <h2 id="diff-t">${tx('Made for the device, checked against the code', 'Feito para o aparelho, conferido no código')}</h2>
      <p>${tx(`Every number below is counted in ${fromCode} when this site is generated.`, `Cada número abaixo é contado n${fromCode.slice(1)} quando este site é gerado.`)}</p>
    </div>
    <ol class="facts">
      <li>
        <span class="fig">${s.quirkTitles}<small>${tx('games with fixes', 'jogos com correções')}</small></span>
        <div><h3>${tx('Per-game fixes applied at launch', 'Correções por jogo aplicadas na abertura')}</h3><p>${tx(
          `The core carries ${s.quirkEntries} settings for ${s.quirkTitles} specific games, such as ${L.list(s.quirkExampleNames)}. They kick in when the title starts, with the priority of a game’s own setting, without you configuring anything.`,
          `O núcleo traz ${s.quirkEntries} ajustes para ${s.quirkTitles} jogos específicos, como ${L.list(s.quirkExampleNames)}. Eles entram quando o título inicia, com a prioridade de um ajuste do próprio jogo, sem você configurar nada.`)}</p></div>
        <a class="src" href="${doc('compatibilidade', tx('#automatic-fixes', '#correcoes-automaticas'))}">${tx('See the list', 'Ver a lista')}</a>
      </li>
      <li>
        <span class="fig">${s.settings}<small>${tx('settings', 'ajustes')}</small></span>
        <div><h3>${tx('Every setting comes from the app’s schema', 'Cada ajuste vem do esquema do app')}</h3><p>${tx(
          'Image, performance, audio, controls, compatibility, driver, system and debugging, global or per game. The per-game file is written in the format the core reads, and this site’s simulator generates the same file.',
          'Imagem, desempenho, áudio, controles, compatibilidade, driver, sistema e depuração, globais ou por jogo. O arquivo por jogo é gravado no formato que o núcleo lê, e o simulador deste site gera o mesmo arquivo.')}</p></div>
        <a class="src" href="${doc('referencia-de-ajustes')}">${tx('Full reference', 'Referência completa')}</a>
      </li>
      <li>
        <span class="fig">${s.patchTitles}<small>${tx('games with patches', 'jogos com patches')}</small></span>
        <div><h3>${tx('Game patches inside the APK', 'Patches de jogos dentro do APK')}</h3><p>${tx(
          `The ${s.patchFiles} Xenia Canary patch files ship with the app. The details page separates the patches made for your version of the game from the ones for other versions.`,
          `Os ${s.patchFiles} arquivos de patch do Xenia Canary vão no app. A ficha separa os patches feitos para a sua versão do jogo dos que valem para outras versões.`)}</p></div>
        <a class="src" href="${doc('jogos-e-arquivos', '#patches')}">${tx('How they work', 'Como funcionam')}</a>
      </li>
      <li>
        <span class="fig">${s.turnipFlags}<small>${tx('Turnip flags', 'flags do Turnip')}</small></span>
        <div><h3>${tx('Adreno driver manager', 'Gerenciador de drivers Adreno')}</h3><p>${tx(
          'Turnip packages downloaded from GitHub or imported, checked before installing, chosen for every game, per game or for a single launch.',
          'Pacotes Turnip baixados do GitHub ou importados, conferidos antes de instalar, escolhidos para todos os jogos, por jogo ou só numa abertura.')}</p></div>
        <a class="src" href="${doc('drivers')}">${tx('GPU drivers', 'Drivers da GPU')}</a>
      </li>
      <li>
        <span class="fig">A/B<small>${tx('compare runs', 'comparar execuções')}</small></span>
        <div><h3>${tx('Measure before you change', 'Medir antes de mudar')}</h3><p>${tx(
          'Every session records FPS per second, frame time, pipelines created, audio glitches and temperature. Compare runs only gives a verdict when the runs follow the A B B A order and differ in a single thing.',
          'Cada sessão grava FPS por segundo, tempo de quadro, pipelines criados, falhas de áudio e temperatura. Comparar execuções só dá um veredito quando as execuções seguem a ordem A B B A e diferem em uma coisa só.')}</p></div>
        <a class="src" href="${doc('desempenho')}">${tx('Performance', 'Desempenho')}</a>
      </li>
    </ol>
  </div>
</section>

${measured(site, L)}

<section class="section" aria-labelledby="req-t">
  <div class="wrap">
    <div class="section-head">
      <span class="eyebrow">${tx('Before you install', 'Antes de instalar')}</span>
      <h2 id="req-t">${tx('Requirements and limitations', 'Requisitos e limitações')}</h2>
    </div>
    <div class="req-grid">
      <div>
        <h3>${tx('What the device needs', 'O que o aparelho precisa')}</h3>
        <dl class="spec">
          <dt>Android</dt><dd>${tx(`${app.minAndroid} or newer. Choosing the games folder requires Android 11.`, `${app.minAndroid} ou mais novo. Escolher a pasta de jogos exige Android 11.`)}</dd>
          <dt>${tx('Processor', 'Processador')}</dt><dd>${tx(html`64-bit ARM (<span class="u-mono">${app.abi}</span>). The APK does not install on other architectures.`, html`ARM de 64 bits (<span class="u-mono">${app.abi}</span>). O APK não instala em outras arquiteturas.`)}</dd>
          <dt>GPU</dt><dd>${tx('Vulkan, required. The focus is Adreno 7xx and 8xx with the Turnip driver; a Snapdragon 8 Gen 2 or newer is recommended.', 'Vulkan, obrigatório. O foco são Adreno 7xx e 8xx com o driver Turnip; recomenda-se um Snapdragon 8 Gen 2 ou mais novo.')}</dd>
          <dt>${tx('Games', 'Jogos')}</dt><dd>${tx('Your own, dumped from your discs or consoles: ISO, ZAR, GOD, folders with default.xex and XBLA packages.', 'Os seus, extraídos de discos ou consoles seus: ISO, ZAR, GOD, pastas com default.xex e pacotes XBLA.')}</dd>
          <dt>${tx('Files', 'Arquivos')}</dt><dd>${tx('All files access, so the library can read the game folders.', 'Acesso a todos os arquivos, para a biblioteca ler as pastas de jogos.')}</dd>
        </dl>
        <p class="u-muted" style="margin-top:12px"><a href="${doc('requisitos')}">${tx('Requirements in detail', 'Requisitos em detalhe')}</a></p>
      </div>
      <div>
        <h3>${tx('What still limits it', 'O que ainda limita')}</h3>
        <ul class="limits">
          <li>${icon('warn', 18)}<span>${tx(html`<b>Compatibility varies.</b> There is no public Xendroid+ compatibility list: each game is rated on your own device.`, html`<b>Compatibilidade varia.</b> Não há lista pública de compatibilidade do Xendroid+: a avaliação de cada jogo fica no seu aparelho.`)}</span></li>
          <li>${icon('warn', 18)}<span>${tx(html`<b>Custom drivers only on Adreno.</b> On other GPUs games run on the system’s Vulkan driver.`, html`<b>Drivers personalizados só em Adreno.</b> Em outras GPUs os jogos rodam no driver Vulkan do sistema.`)}</span></li>
          <li>${icon('warn', 18)}<span>${tx(html`<b>Frame generation is experimental.</b> Win-FG 2× and LSFG start off at every launch; LSFG needs your own Lossless.dll.`, html`<b>Geração de quadros é experimental.</b> Win-FG 2× e LSFG começam desligados a cada abertura; o LSFG precisa do seu próprio Lossless.dll.`)}</span></li>
          <li>${icon('warn', 18)}<span>${tx(html`<b>On Android 10</b> the app installs, but only opens games launched from a frontend: choosing folders requires Android 11.`, html`<b>No Android 10</b> o app instala, mas só abre jogos vindos de um frontend: escolher pastas exige Android 11.`)}</span></li>
          <li>${icon('warn', 18)}<span>${tx(html`<b>Community settings and the compatibility catalog</b> exist in the code, but are off in the published versions: there is no server.`, html`<b>Ajustes da comunidade e catálogo de compatibilidade</b> existem no código, mas ficam desligados nas versões publicadas: não há servidor.`)}</span></li>
        </ul>
        <div class="planned">
          <h4><span class="tag plain">${tx('Not in the app', 'Fora do app')}</span>${tx('Recorded proposals, not implemented yet', 'Propostas registradas, ainda não implementadas')}</h4>
          <ul>
            ${tx([
              'A key map for each controller (today one map applies to all of them).',
              'Finding the game on the network and scanning a QR code on the phone used as a controller.',
              'Looking for the disc file when disc swapping finds none.',
            ], [
              'Um mapa de teclas para cada controle (hoje um mapa vale para todos).',
              'Achar o jogo na rede e ler um QR code no celular como controle.',
              'Procurar o arquivo do disco quando a troca de disco não acha nenhum.',
            ]).map(t => html`<li>${t}</li>`)}
          </ul>
          <p class="u-muted" style="margin-top:8px;font-size:var(--step--1)">${tx('Recorded in', 'Registradas em')} <a href="${site.config.repoUrl}/blob/main/docs/ui-redesign/app.md#propostas-do-prot%C3%B3tipo-que-ficaram-de-fora" rel="noopener">docs/ui-redesign/app.md</a>${tx(' (in Portuguese), with no planned date.', ', sem data prevista.')}</p>
        </div>
      </div>
    </div>
  </div>
</section>

<section class="section" aria-labelledby="go-t">
  <div class="wrap">
    <div class="section-head">
      <span class="eyebrow">${tx('Next steps', 'Próximos passos')}</span>
      <h2 id="go-t">${tx('Documentation and simulator', 'Documentação e simulador')}</h2>
    </div>
    <div class="split">
      <section>
        <h3>${icon('book', 22)}${tx('Documentation', 'Documentação')}</h3>
        <p>${tx(`From installation to collecting logs, written from ${fromCode}, with search and links to every section.`, `Da instalação à coleta de logs, escrita a partir d${fromCode.slice(1)}, com busca e links para cada seção.`)}</p>
        <ul>
          ${[['primeiros-passos', tx('Getting started', 'Primeiros passos')], ['instalacao', tx('Install and update', 'Instalar e atualizar')], ['ajustes', tx('Settings and effects', 'Ajustes e efeitos')], ['problemas', tx('Troubleshooting and logs', 'Problemas e logs')], ['compilar-e-contribuir', tx('Build and contribute', 'Compilar e contribuir')]]
            .map(([slug, t]) => html`<li><a href="${doc(slug)}">${t}${icon('arrowR', 18)}</a></li>`)}
        </ul>
      </section>
      <section>
        <h3>${icon('phone', 22)}${tx('Interface simulator', 'Simulador da interface')}</h3>
        <p>${tx(
          `Browse the app’s screens in the browser, in touch or controller mode, with a keyboard or a real controller. The settings are the ${s.settings} of ${name || 'the published version'}, and a game’s config file comes out in the format the core reads.`,
          `Navegue pelas telas do app no navegador, no modo toque ou no modo controle, com teclado ou um controle de verdade. Os ajustes são os ${s.settings} da ${name || 'versão publicada'}, e o arquivo de configuração de um jogo sai no formato que o núcleo lê.`)}</p>
        <p><a class="button" href="${r(L.simPath)}">${icon('play', 18)}${tx('Open the simulator', 'Abrir o simulador')}</a></p>
        <p class="u-muted" style="font-size:var(--step--1)">${tx('The simulator’s games, covers, profiles and sessions are examples; nothing runs a game.', 'Jogos, capas, perfis e sessões do simulador são exemplos; nada roda um jogo.')}</p>
      </section>
    </div>
  </div>
</section>

<section class="section" aria-labelledby="proj-t">
  <div class="wrap project">
    <div>
      <span class="eyebrow">${tx('The project', 'O projeto')}</span>
      <h2 id="proj-t" style="margin-bottom:16px">${tx('Open source, built on the device', 'Código aberto, feito no aparelho')}</h2>
      <p>${tx(
        html`Xendroid+ is created and maintained by <a href="https://github.com/phforner0" rel="noopener">phforner0</a>. It continues XenDroid, the original Android port by <a href="https://github.com/rfandango" rel="noopener">rfandango</a>, and uses the core of <a href="https://github.com/xenia-project/xenia" rel="noopener">Xenia</a> and Xenia Canary.`,
        html`O Xendroid+ é criado e mantido por <a href="https://github.com/phforner0" rel="noopener">phforner0</a>. Dá continuidade ao XenDroid, o port original para Android de <a href="https://github.com/rfandango" rel="noopener">rfandango</a>, e usa o núcleo do <a href="https://github.com/xenia-project/xenia" rel="noopener">Xenia</a> e do Xenia Canary.`)}</p>
      <p>${tx(
        html`Performance work is measured frame by frame on real devices, with methods and results in <a href="${site.config.repoUrl}/tree/main/performance-tests" rel="noopener">performance-tests/</a>, and every published version comes with notes on what changed. Third-party components keep their own licenses, listed in <a href="${site.config.repoUrl}/blob/main/THIRD-PARTY-NOTICES.md" rel="noopener">THIRD-PARTY-NOTICES.md</a>.`,
        html`O trabalho de desempenho é medido quadro a quadro em aparelhos reais, com métodos e resultados em <a href="${site.config.repoUrl}/tree/main/performance-tests" rel="noopener">performance-tests/</a>, e cada versão publicada traz as notas do que mudou. Componentes de terceiros mantêm as próprias licenças, listadas no <a href="${site.config.repoUrl}/blob/main/THIRD-PARTY-NOTICES.md" rel="noopener">THIRD-PARTY-NOTICES.md</a>.`)}</p>
    </div>
    <ul class="link-list">
      <li><a href="${site.config.repoUrl}" rel="noopener">${icon('code', 20)}<span>${tx('Source code', 'Código-fonte')}<small>${site.config.repo} ${tx('on GitHub', 'no GitHub')}</small></span>${icon('external', 16)}</a></li>
      <li><a href="${site.config.repoUrl}/releases" rel="noopener">${icon('tag', 20)}<span>Releases<small>${tx('APKs and notes for every build', 'APKs e notas de cada build')}</small></span>${icon('external', 16)}</a></li>
      <li><a href="${doc('historico')}">${icon('history', 20)}<span>${tx('What’s new and release history', 'Novidades e histórico')}<small>${tx('what changed in every version', 'o que mudou em cada versão, em português')}</small></span>${icon('arrowR', 16)}</a></li>
      <li><a href="${doc('compilar-e-contribuir')}">${icon('wrench', 20)}<span>${tx('Build and contribute', 'Compilar e contribuir')}<small>${tx('JDK 21, Android SDK and NDK, tests and CI', 'JDK 21, Android SDK e NDK, testes e CI')}</small></span>${icon('arrowR', 16)}</a></li>
      <li><a href="${site.config.discord}" rel="noopener">${icon('chat', 20)}<span>${tx('Community', 'Comunidade')}<small>${tx('chat on Discord', 'conversa no Discord')}</small></span>${icon('external', 16)}</a></li>
      <li><a href="${site.config.support}" rel="noopener">${icon('heart', 20)}<span>${tx('Support the project', 'Apoie o projeto')}<small>${tx('voluntary, on Buy Me a Coffee', 'voluntário, no Buy Me a Coffee')}</small></span>${icon('external', 16)}</a></li>
    </ul>
  </div>
</section>
`;
  return { page, content };
}

function measured(site, L) {
  const { tx } = L;
  const perf = site.perf(L.id);
  if (!perf || !perf.rows.length) return '';
  const cols = perf.header;
  return html`<section class="section" aria-labelledby="perf-t">
  <div class="wrap measured">
    <div class="section-head" style="margin-bottom:8px">
      <span class="eyebrow">${tx('Measured performance', 'Desempenho medido')}</span>
      <h2 id="perf-t">${tx('Measurements published by the project', 'Medições publicadas pelo projeto')}</h2>
      <p>${perf.context.replace(/\*\*/g, '')}</p>
    </div>
    <div class="table-wrap" tabindex="0" role="region" aria-label="${tx('Performance measurements', 'Medições de desempenho')}">
      <table>
        <thead><tr>${cols.map(c => html`<th scope="col">${c}</th>`)}</tr></thead>
        <tbody>${perf.rows.map(row => html`<tr>${cols.map((c, i) => (i === 0 ? html`<th scope="row">${row[c]}</th>` : html`<td>${row[c] || '—'}</td>`))}</tr>`)}</tbody>
      </table>
    </div>
    <p class="context">
      <span>${tx('Source', 'Fonte')}: <a href="${site.config.repoUrl}/blob/main/${perf.source}#${tx('performance-snapshot', 'desempenho')}" rel="noopener">${perf.source}</a> ${tx('and', 'e')} <a href="${site.config.repoUrl}/tree/main/performance-tests" rel="noopener">performance-tests/</a>.</span>
      <span>${tx('They are not a guarantee: your device, the driver and the game version change these numbers.', 'Não são garantia: o seu aparelho, o driver e a versão do jogo mudam estes números.')}</span>
    </p>
  </div>
</section>`;
}

/** Dados estruturados da aplicação (só fatos que o build conhece). */
function jsonLd(site, L) {
  const { stable } = site.v;
  const app = site.ch.est.app;
  const d = {
    '@context': 'https://schema.org',
    '@type': 'SoftwareApplication',
    name: 'Xendroid+',
    applicationCategory: 'GameApplication',
    operatingSystem: `Android ${app.minAndroid}+`,
    processorRequirements: app.abi,
    url: site.config.url + L.home,
    codeRepository: site.config.repoUrl,
    inLanguage: ['en', 'pt-BR'],
    isAccessibleForFree: true,
    offers: { '@type': 'Offer', price: '0', priceCurrency: 'USD' },
    author: { '@type': 'Person', name: 'phforner0', url: 'https://github.com/phforner0' },
  };
  if (stable && stable.apk) {
    d.softwareVersion = `build ${stable.build}`;
    d.downloadUrl = stable.apk.url;
    d.fileSize = `${Math.round(stable.apk.size / 1024 / 1024)}MB`;
    d.datePublished = stable.date;
  }
  return raw(`<script type="application/ld+json">${JSON.stringify(d).replace(/</g, '\\u003c')}</script>`);
}
