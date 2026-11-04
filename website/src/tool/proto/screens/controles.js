/* Lote 3: Controles. Uma área para o toque, os controles físicos, vibração e giroscópio e os telefones,
   com o mapeamento de teclas, o editor de toque, o teste de controles e o celular como controle. */
'use strict';

Object.assign(NOTES, {
  ctlhub: ['app', 'Controles é uma área do app: toque, controles físicos, vibração e giroscópio e telefones, com as ferramentas de mapeamento, editor de toque, teste e celular como controle.'],
  slots: ['app', 'Quem joga como P1 a P4, na ordem do primeiro botão apertado. Sem controle, o P1 são os controles de toque.'],
  touchone: ['app', 'Um interruptor só para os controles de toque; um jogo pode mudar isso na própria ficha.'],
  touchglob: ['app', 'Os ajustes gerais do toque valem para o controle inteiro: visual, opacidade, esconder sozinho, vibração ao tocar, tela dividida, deslizar o dedo e câmera por toque.'],
  layouts: ['app', 'Layouts salvos (até 30, nome com até 40 caracteres): aplicar com a prévia do que muda, exportar, importar um arquivo conferido (até 256 KB) e apagar.'],
  ctlmotion: ['app', 'Vibração padrão, giroscópio e entrada sem buffer: os valores com que cada jogo começa; o menu em jogo muda na hora.'],
  padrumble: ['app', 'Cada controle pode ter a própria intensidade de vibração, guardada pelo descritor do Android.'],
  hidcore: ['app', 'Ajustes do núcleo para controles, para todos os jogos; cada jogo pode mudar na ficha.'],
  kmdraw: ['app', 'O controle desenhado: tocar num botão escolhe a tecla dele. O mapa vale para todos os controles e o teclado.'],
  kmstate: ['app', 'Tecla mudada, tecla em dois botões (um deles não faz nada) e botão sem tecla.'],
  kmswap: ['app', 'Trocar A/B e X/Y, para controles no padrão da Nintendo; tocar de novo desfaz.'],
  kmcap: ['app', 'Esperando a tecla: vale botão de controle ou de teclado. Se a tecla já era de outro botão, os dois trocam e o app avisa.'],
  kmsim: ['sim', 'No navegador, as teclas do seu teclado e um controle conectado (Gamepad API) servem para escolher a tecla.'],
  tegrid: ['app', 'Grade quadrada (20 casas no lado menor da tela); com “Alinhar à grade”, o controle encaixa ao soltar.'],
  tebar: ['app', 'Barra do editor: orientação, grade, grupo, só este jogo (aberto pelo menu do jogo), layouts, gerais, restaurar, salvar e sair. Recolhe num botão.'],
  teinsp: ['app', 'Tamanho do controle escolhido, zona morta do direcional e dos analógicos e mostrar ou esconder.'],
  teundo: ['app', 'Desfazer a última mudança sem sair do editor.'],
  tec: ['app', 'Editar com o controle: LB/RB escolhem o controle, o direcional move uma casa, A abre os ajustes dele.'],
  ptcheck: ['app', 'Os botões, com marca quando já foram apertados.'],
  ptstick: ['app', 'Analógicos com a zona morta e o que o jogo recebe.'],
  ptdz: ['app', 'A zona morta do núcleo se ajusta aqui, vendo o analógico.'],
  ptrumble: ['app', 'A vibração de cada controle e o teste de 0,3 s, só quando pedido.'],
  ptleave: ['app', 'Segure B por um segundo para sair usando só o controle; nada do que é apertado aqui chega a um jogo.'],
  ptsim: ['sim', 'Em “Controle real”, o simulador lê um controle conectado ao computador pela Gamepad API do navegador.'],
  pcform: ['app', 'Endereço, código de 6 dígitos, nome e vibração deste telefone; ele joga como P2 a P4 no outro. Experimental.'],
  pcplay: ['app', 'Conectado: o controle de toque ocupa a tela, com o jogador, a latência e Sair.'],
  pcerr: ['app', 'O motivo da recusa: código errado, vagas ocupadas, outra versão do app ou pareamento fechado.'],
  pchelp: ['dif', 'O texto de ajuda dentro do jogo manda procurar “Biblioteca → ⋮ → Usar este telefone como controle”, mas esse item não existe no menu da biblioteca; o caminho real é Controles → Celular como controle.'],
});

/* ---------- ajustes do app para toque e controles (guardados pelo app, nunca por jogo) ---------- */
const CTLSET = [
  { k: '@touch.style', ty: 'list', def: 'modern', o: [['modern', 'Moderno'], ['classic', 'Clássico']], t: 'Visual', d: 'Moderno: botões de vidro escuro com letras coloridas. Clássico: os botões coloridos de antes.' },
  { k: '@touch.opacity', ty: 'int', def: 65, min: 20, max: 100, step: 5, unit: '%', t: 'Opacidade', d: 'Vale para todos os controles de toque.' },
  { k: '@touch.autohide', ty: 'int', def: 8, min: 0, max: 20, step: 1, unit: ' s', zero: 'Desligado', t: 'Esconder sozinho', d: 'Depois de tantos segundos sem tocar na tela; um toque traz de volta.' },
  { k: '@touch.haptics', ty: 'bool', def: false, t: 'Vibração ao tocar', d: 'Uma vibração curta do telefone a cada botão apertado.' },
  { k: '@touch.hidepad', ty: 'bool', def: true, t: 'Esconder enquanto um controle joga como P1', d: 'O primeiro botão ou movimento do stick esconde os controles; eles voltam quando o controle desconecta ou pelo menu. Tocar na tela não os traz de volta.' },
  { k: '@touch.split', ty: 'list', def: 'off', o: [['off', 'Desligada'], ['tabletop', 'Dobrável meio aberto'], ['always', 'Sempre']], t: 'Tela dividida', d: 'O jogo vai para a parte de cima e os controles ficam fora dele: na dobra de um dobrável em pé meio aberto, ou no meio de uma tela em retrato ou quase quadrada (“Sempre”).' },
  { k: '@touch.slidebtn', ty: 'bool', def: false, t: 'Deslizar entre os botões e o direcional', d: 'Sem levantar o dedo: um botão solta quando o dedo sai dele e o próximo em que ele entra é apertado.' },
  { k: '@touch.slidestick', ty: 'bool', def: false, t: 'Deslizar até um analógico para pegá-lo', d: 'Um dedo livre que desliza até um analógico passa a movê-lo.' },
  { k: '@touch.camera', ty: 'bool', def: false, t: 'Câmera por toque', d: 'No lado direito, longe dos botões, deslize o dedo para girar a câmera; ela para quando o dedo para.' },
  { k: '@touch.camspeed', ty: 'int', def: 100, min: 50, max: 200, step: 10, unit: '%', t: 'Velocidade da câmera por toque', d: 'Quanto a câmera gira para o mesmo movimento do dedo.' },
  { k: '@touch.camarea', ty: 'int', def: 55, min: 30, max: 70, step: 5, unit: '%', t: 'Área da câmera por toque', d: 'Quanto da tela, a partir da direita, gira a câmera.' },
  { k: '@ctl.rumble', ty: 'list', def: 'medium', o: [['off', 'Desligada'], ['low', 'Baixa'], ['medium', 'Média'], ['high', 'Alta']], t: 'Vibração dos controles', d: 'A força da vibração nos controles sem intensidade própria; cada controle pode ter a sua. Se os jogos vibram é o ajuste do core em Controles físicos.' },
  { k: '@ctl.gyrocam', ty: 'bool', def: false, t: 'Câmera pelo giroscópio', d: 'Girar o telefone gira a câmera.' },
  { k: '@ctl.gyroaim', ty: 'list', def: 'always', o: [['always', 'Sempre'], ['lt', 'Segurando LT'], ['lb', 'Segurando LB']], t: 'Mira pelo giroscópio', d: 'Quando o giroscópio move a mira: sempre, ou só enquanto o P1 segura LT ou LB.' },
  { k: '@ctl.gyrosens', ty: 'list', def: 'normal', o: [['low', 'Baixa'], ['normal', 'Normal'], ['high', 'Alta']], t: 'Sensibilidade do giroscópio', d: 'Quanto a câmera gira para o mesmo movimento do telefone.' },
  { k: '@ctl.unbuffered', ty: 'bool', def: true, t: 'Entrada sem buffer', d: 'Entrega toques e botões ao jogo assim que chegam, sem esperar o próximo quadro da tela: menos atraso. Android 11 ou mais novo.' },
];
for (const d of CTLSET) DEF[d.k] = Object.assign({ g: 'ctl', lvl: 1, vt: 'app', ap: 'live' }, d);
const TOUCH_GROUPS = [['Geral', ['HID.show_touch_overlay', '@touch.style', '@touch.opacity', '@touch.autohide', '@touch.haptics', '@touch.hidepad']], ['Tela dividida', ['@touch.split']], ['Deslizar o dedo', ['@touch.slidebtn', '@touch.slidestick']], ['Câmera por toque', ['@touch.camera', '@touch.camspeed', '@touch.camarea']]];
const TOUCH_DEFAULT = JSON.parse(JSON.stringify(TOUCH_LAYOUT));
const RUMBLE_T = { off: 'Desligada', low: 'Baixa', medium: 'Média', high: 'Alta' };
const padRumble = p => p.rumble || globalOf('@ctl.rumble');
const padSlot = p => PADS.indexOf(p);

function touchGlobalsHTML(v) { return `<div data-note="touchglob">${TOUCH_GROUPS.map(([t, keys], i) => `<h4 class="gh">${t}</h4><div${i ? '' : ' data-note="touchone"'}>${appRows(keys, v)}</div>`).join('')}</div>`; }
function lpDiff(d) { if (!d) return ''; const [m, r, sh, hd] = d, parts = []; if (m) parts.push(`${m} ${m === 1 ? 'movido' : 'movidos'}`); if (r) parts.push(`${r} com outro tamanho`); if (sh) parts.push(`${sh} ${sh === 1 ? 'mostrado' : 'mostrados'}`); if (hd) parts.push(`${hd} ${hd === 1 ? 'escondido' : 'escondidos'}`); return parts.join(', ') || 'Nada muda'; }
function layoutsHTML(pfx) {
  return `<div class="list" data-note="layouts">${LAYOUTS.length ? LAYOUTS.map(l => drow({ icon: 'hand', t: esc(l.name), s: `${[l.land && 'paisagem', l.port && 'retrato'].filter(Boolean).join(' e ')} · salvo em ${l.when}`, acts: `<button class="btn sm" data-act="lp-ask" data-v="${l.id}" data-k="${pfx}-ap-${l.id}">Aplicar…</button><button class="btn sm ghost" data-act="toast" data-msg="No app, o Android pergunta onde salvar “${esc(l.name)}”." data-k="${pfx}-ex-${l.id}">Exportar</button><button class="ibtn" data-act="lp-del" data-v="${l.id}" data-k="${pfx}-rm-${l.id}" aria-label="Apagar ${esc(l.name)}">${ic('trash', 17)}</button>` })).join('') : '<p class="note">Nenhum layout salvo ainda.</p>'}</div>
    <div class="row"><input class="txt plain" id="lp-name" type="text" placeholder="Nome" data-k="${pfx}-name" maxlength="40" autocomplete="off" aria-label="Nome do layout"><button class="btn sm" data-act="lp-save" data-k="${pfx}-save">${ic('save', 15)} Salvar o atual</button><button class="btn sm ghost" data-act="toast" data-msg="No app, abre o seletor de arquivos do Android. O arquivo tem até 256 KB e é conferido antes." data-k="${pfx}-imp" title="Importar um arquivo de layout">${ic('download', 15)} Importar…</button></div>`;
}
action('lp-ask', el => { S.modal = 'lp-apply'; S.mp = { id: el.dataset.v }; render(); });
action('lp-del', el => { const i = LAYOUTS.findIndex(l => l.id === el.dataset.v); if (i >= 0) { const l = LAYOUTS.splice(i, 1)[0]; toast(`“${l.name}” apagado.`); } });
action('lp-save', () => { const inp = document.getElementById('lp-name'), name = (inp && inp.value || '').trim(); if (!name) { toast('Um layout precisa de um nome com 1 a 40 caracteres'); return; } const ex = LAYOUTS.find(l => l.name === name); if (ex) { ex.when = 'hoje'; toast(`“${name}” substituído.`); return; } if (LAYOUTS.length >= 30) { toast('No máximo 30 layouts; apague um antes'); return; } LAYOUTS.push({ id: 'u' + Date.now(), name, when: 'hoje', land: [0, 0, 0, 0], port: [0, 0, 0, 0] }); toast(`“${name}” salvo.`); });
function applyLayout(id) {
  const tgt = S.route.name === 'touchedit' && TE.work ? TE.work : TOUCH_LAYOUT, l = LAYOUTS.find(x => x.id === id);
  if (S.route.name === 'touchedit') teSnap();
  const tf = { big: e => { if (['stick', 'abxy', 'dpad'].includes(e.k)) e.s = 1.2; }, race: e => { if (e.id === 'lt' || e.id === 'rt') e.y = 58; if (e.id === 'lb' || e.id === 'rb') e.y = 44; if (e.id === 'rs') e.vis = false; }, tab: e => { e.s = .85; } }[id] || (() => {});
  for (const o of ['land', 'port']) if (l[o]) tgt[o].forEach(tf);
  if (S.route.name === 'touchedit') TE.dirty = true;
  S.modal = null; toast(`“${l.name}” aplicado.`);
}
modal('lp-apply', mp => {
  const l = LAYOUTS.find(x => x.id === mp.id), o = isPortrait() ? 'port' : 'land', other = o === 'land' ? 'port' : 'land';
  const lines = [`${o === 'land' ? 'Paisagem' : 'Retrato'}: ${lpDiff(l[o])}.`];
  if (!l[o]) lines[0] = `Ele não tem layout em ${o === 'land' ? 'paisagem' : 'retrato'}; esta orientação fica como está.`;
  if (l[other]) lines.push(`O layout em ${other === 'land' ? 'paisagem' : 'retrato'} também é substituído.`);
  return { html: sheetHead(`Aplicar “${esc(l.name)}”?`, S.route.name === 'touchedit' && TE.perGame ? 'Salvar guarda o layout deste jogo; aplicar o substitui.' : 'Salvar guarda o layout compartilhado; aplicar o substitui.') + `<div class="card">${lines.map(x => `<p style="font-size:13.5px">${esc(x)}</p>`).join('')}</div><div class="acts"><button class="btn ghost" data-act="close" data-k="la-x">Cancelar</button><button class="btn primary" data-act="lp-apply" data-v="${l.id}" data-k="la-ok" data-autofocus>Aplicar</button></div>` };
});
action('lp-apply', el => applyLayout(el.dataset.v));

/* ---------- área Controles ---------- */
function ctlOverviewHTML(v) {
  const km = kmStats(), slots = [0, 1, 2, 3].map(i => PADS[i]);
  const tile = (go, icon, t, s, k) => v === 'c'
    ? `<button class="crow" data-act="go" data-v="${go}" data-k="${k}"><span class="t"><b>${t}</b><small>${s}</small></span>${ic('chevR', 18)}</button>`
    : `<button class="tile" data-act="go" data-v="${go}" data-k="${k}">${ic(icon, 24)}<b>${t}</b><small>${s}</small></button>`;
  return `<div class="stack" data-note="ctlhub">
    <section class="card" data-note="slots"><h3>${ic('gamepad', 15)} Quem joga agora<span class="r">na ordem do primeiro botão apertado</span></h3>
      <div class="slots">${slots.map((p, i) => p ? `<div class="slot"><span class="p">P${i + 1}</span><b>${esc(p.name)}</b><small>${p.conn} · vibração ${RUMBLE_T[padRumble(p)].toLowerCase()}</small></div>` : `<div class="slot free"><span class="p">P${i + 1}</span><b>Livre</b><small>${i === 2 ? 'Outro controle ou um telefone' : 'Livre'}</small></div>`).join('')}</div>
      <p class="note">Os controles de toque são do P1 e ${globalOf('@touch.hidepad') ? 'somem enquanto um controle físico joga como P1' : 'continuam na tela mesmo com um controle físico'}.</p></section>
    <div class="${v === 'c' ? 'c-rows' : 'tiles'}">
      ${tile('keymap', 'keyboard', 'Mapeamento de teclas', `${km.chg} ${km.chg === 1 ? 'tecla mudada' : 'teclas mudadas'}${km.shared ? ` · ${km.shared} em dois botões` : ''}`, 'ct-km')}
      ${tile('touchedit', 'move', 'Editar o layout de toque', `${globalOf('HID.show_touch_overlay') ? 'Na tela' : 'Escondidos'} · opacidade ${globalOf('@touch.opacity')}% · ${LAYOUTS.length} layouts salvos`, 'ct-te')}
      ${tile('padtest', 'gamepad', 'Testar controles', `${PADS.length} conectados · botões, analógicos, vibração`, 'ct-pt')}
      ${tile('phonepad', 'phone', 'Usar este telefone como controle', 'Para jogar num jogo aberto em outro telefone', 'ct-pc')}
    </div>
    <div class="grid2">
      <section class="card"><h3>${ic('hand', 15)} Na tela</h3><dl class="kv"><dt>Controles de toque</dt><dd>${label(DEF['HID.show_touch_overlay'], globalOf('HID.show_touch_overlay'))}</dd><dt>Esconder sozinho</dt><dd>${label(DEF['@touch.autohide'], globalOf('@touch.autohide'))}</dd><dt>Tela dividida</dt><dd>${label(DEF['@touch.split'], globalOf('@touch.split'))}</dd><dt>Câmera por toque</dt><dd>${label(DEF['@touch.camera'], globalOf('@touch.camera'))}</dd></dl>
        <div class="row"><button class="btn sm" data-act="sec" data-key="controls" data-v="touch" data-k="ov-touch">Ajustes de toque</button></div></section>
      <section class="card"><h3>${ic('vibrate', 15)} Vibração e movimento</h3><dl class="kv"><dt>Vibração padrão</dt><dd>${label(DEF['@ctl.rumble'], globalOf('@ctl.rumble'))}</dd>${PADS.filter(p => p.rumble).map(p => `<dt>${esc(p.name.split(' ')[0])}</dt><dd>${RUMBLE_T[p.rumble]} (própria)</dd>`).join('')}<dt>Mira pelo giroscópio</dt><dd>${label(DEF['@ctl.gyroaim'], globalOf('@ctl.gyroaim'))}</dd><dt>Entrada sem buffer</dt><dd>${label(DEF['@ctl.unbuffered'], globalOf('@ctl.unbuffered'))}</dd></dl>
        <div class="row"><button class="btn sm" data-act="sec" data-key="controls" data-v="motion" data-k="ov-motion">Abrir</button></div></section>
    </div>
  </div>`;
}
function ctlTouchHTML(v) {
  return `<div class="stack">${touchGlobalsHTML(v)}
    <section class="card"><h3>${ic('layers', 15)} Layouts salvos<span class="r">${LAYOUTS.length}</span></h3>${layoutsHTML('hl')}<p class="note">Aplicar daqui muda o layout compartilhado; um jogo com layout próprio continua com o dele.</p></section>
    <div class="row"><button class="btn primary" data-act="go" data-v="touchedit" data-k="tc-edit">${ic('move', 16)} Editar o layout de toque</button></div></div>`;
}
function padCardHTML(p) {
  const opts = [['', `Padrão (${RUMBLE_T[globalOf('@ctl.rumble')]})`], ['off', 'Desligada'], ['low', 'Baixa'], ['medium', 'Média'], ['high', 'Alta']];
  return `<section class="card" data-note="padrumble"><h3>${ic('gamepad', 15)} P${padSlot(p) + 1} · ${esc(p.name)}<span class="r">${p.conn}</span></h3>
    <dl class="kv"><dt>Identificação</dt><dd class="mono">${p.vidpid} · ${p.src.join(', ')}</dd><dt>Giroscópio</dt><dd>${p.gyro ? 'Tem' : 'O Android não informa'}</dd></dl>
    <div class="row"><span class="note" style="flex:1 1 120px">Vibração no jogo</span><div class="seg" role="group" aria-label="Vibração de ${esc(p.name)}">${opts.map(([v, t]) => `<button data-act="pad-rumble" data-p="${p.id}" data-v="${v}" aria-pressed="${(p.rumble || '') === v}" data-k="pr-${p.id}-${v || 'def'}">${t}</button>`).join('')}</div></div>
    <div class="row"><button class="btn sm" data-act="go" data-v="padtest" data-k="pc-test-${p.id}">${ic('gamepad', 15)} Testar</button><button class="btn sm ghost" data-act="pt-vib" data-v="${p.id}" data-k="pc-vib-${p.id}"${padRumble(p) === 'off' ? ' disabled' : ''}>${ic('vibrate', 15)} Vibrar 0,3 s</button></div></section>`;
}
function ctlPadsHTML(v) {
  const core = SET.filter(d => d.g === 'input');
  return `<div class="stack">${PADS.map(padCardHTML).join('')}
    <section class="card"><h3>${ic('keyboard', 15)} Outros dispositivos</h3><div class="list">${INPUT_OTHER.map(d => drow({ icon: 'keyboard', t: esc(d.name), s: `${d.conn} · usa o mapeamento de teclas; não entra na ordem P1–P4`, acts: `<button class="btn sm ghost" data-act="go" data-v="keymap" data-k="io-km">Mapeamento</button>` })).join('')}</div></section>
    <section class="card" data-note="hidcore"><h3>${ic('sliders', 15)} Ajustes do core para controles<span class="r">todos os jogos</span></h3>${v === 'c' ? `<div class="c-rows">${core.map(d => cRow(d, null)).join('')}</div>` : `<div class="setp-list">${core.map(d => settingRow(d, null, 'full')).join('')}</div>`}<p class="note">Cada jogo pode mudar estes na própria ficha, em Controles.</p></section>
  </div>`;
}
function ctlMotionHTML(v) {
  return `<div data-note="ctlmotion"><h4 class="gh">Vibração</h4>${appRows(['@ctl.rumble'], v)}
    <h4 class="gh">Giroscópio do telefone</h4>${appRows(['@ctl.gyrocam', '@ctl.gyroaim', '@ctl.gyrosens'], v)}
    <p class="note" style="margin-top:8px">Para calibrar, use o menu do jogo (Controles) com o telefone parado.</p>
    <h4 class="gh">Entrada</h4>${appRows(['@ctl.unbuffered'], v)}
    <p class="note" style="margin-top:10px">O menu em jogo continua mudando estes na hora; aqui ficam os valores com que cada jogo começa.</p></div>`;
}
function ctlPhonesHTML() {
  return `<div class="grid2">
    <section class="card" data-note="pchelp"><h3>${ic('phone', 15)} Este telefone como controle</h3><p style="font-size:13.5px">Para jogar num jogo aberto em outro telefone, na mesma rede Wi-Fi ou no mesmo ponto de acesso. Este telefone vira um controle de toque e entra como P2–P4.</p><div class="row"><button class="btn sm primary" data-act="go" data-v="phonepad" data-k="ph-open">Usar este telefone como controle</button></div></section>
    <section class="card"><h3>${ic('wifi', 15)} Telefones jogando aqui</h3><p style="font-size:13.5px">Com um jogo aberto: menu do jogo → Controles → Telefones como controle. Aparecem o endereço e o código de 6 dígitos para digitar no outro telefone.</p><dl class="kv"><dt>Rede agora</dt><dd>Wi-Fi · 192.168.0.12 (exemplo)</dd></dl></section>
    <section class="card span2"><p class="note">O jogo só escuta na rede local dele, nunca na internet, e cada vez que é ligado usa um endereço e um código novos. Dez códigos errados travam o pareamento até os telefones como controle serem desligados e ligados de novo.</p></section>
  </div>`;
}
action('pad-rumble', el => { const p = PADS.find(x => x.id === el.dataset.p); p.rumble = el.dataset.v || null; toast(`${p.name}: vibração ${p.rumble ? RUMBLE_T[p.rumble].toLowerCase() : 'padrão'}`); });

screen('controls', {
  title: 'Controles', secKey: 'controls', globalScope: true,
  render() {
    const secs = [
      { group: 'Controles', id: 'ov', t: 'Visão geral', icon: 'gamepad', title: 'Controles', body: ctlOverviewHTML },
      { id: 'touch', t: 'Na tela (toque)', icon: 'hand', title: 'Controles de toque', lead: 'Valem para o controle inteiro em todos os jogos; posição e tamanho de cada botão ficam no editor.', body: ctlTouchHTML },
      { id: 'pads', t: 'Controles físicos', icon: 'gamepad', n: PADS.length, body: ctlPadsHTML },
      { id: 'motion', t: 'Vibração e movimento', icon: 'vibrate', body: ctlMotionHTML },
      { id: 'phones', t: 'Telefones', icon: 'phone', title: 'Telefones como controle', body: ctlPhonesHTML },
      { group: 'Ferramentas', go: 'keymap', t: 'Mapeamento', icon: 'keyboard' },
      { go: 'touchedit', t: 'Editor de toque', icon: 'move' },
      { go: 'padtest', t: 'Testar controles', icon: 'gamepad' },
      { go: 'phonepad', t: 'Celular como controle', icon: 'phone' },
    ];
    return sectioned({ key: 'controls', title: 'Controles', sub: `${PADS.length} controles conectados · toque ${globalOf('HID.show_touch_overlay') ? 'ligado' : 'desligado'}`, csub: `${PADS.length} conectados`, plainSub: true, icon: 'gamepad', sections: secs });
  },
});

/* ---------- controle desenhado ---------- */
const PD_POS = { LT: [190, 26, 'trig'], RT: [450, 26, 'trig'], LB: [192, 62, 'bump'], RB: [448, 62, 'bump'], L3: [172, 156, 'stick'], R3: [400, 234, 'stick'], UP: [250, 204, 'dp'], DOWN: [250, 262, 'dp'], LEFT: [221, 233, 'dp'], RIGHT: [279, 233, 'dp'], BACK: [282, 156, 'sm'], START: [358, 156, 'sm'], Y: [478, 116, 'face'], X: [440, 154, 'face'], B: [516, 154, 'face'], A: [478, 192, 'face'] };
const PD_LBL = { UP: '↑', DOWN: '↓', LEFT: '←', RIGHT: '→', BACK: 'Back', START: 'Start' };
const PD_BODY = '<svg viewBox="0 0 640 400" aria-hidden="true"><path class="pd-body" d="M205 86C255 74 385 74 435 86C515 96 560 124 588 186C616 254 632 326 600 358C572 386 528 366 494 326C470 298 440 284 400 284L240 284C200 284 170 298 146 326C112 366 68 386 40 358C8 326 24 254 52 186C80 124 125 96 205 86Z"/><circle class="pd-guide" cx="320" cy="116" r="17"/></svg>';
const kmName = id => (KM_BTNS.find(b => b[0] === id) || [id, id])[1];
function padDraw(o) {
  return `<div class="pd ${o.cls || ''}"${o.note ? ` data-note="${o.note}"` : ''}${o.pad ? ` data-pad="${o.pad}"` : ''}>${PD_BODY}${Object.entries(PD_POS).map(([id, [x, y, k]]) => {
    const st = o.state ? o.state(id) : '', kb = o.kb ? o.kb(id) : '';
    return `<button class="pd-b ${k} ${id.length === 1 ? id : ''} ${st}" style="left:${(x / 6.4).toFixed(2)}%;top:${(y / 4).toFixed(2)}%" data-pd="${id}" ${o.attrs ? o.attrs(id) : 'tabindex="-1"'} aria-label="${esc(kmName(id))}">${k === 'stick' ? `<i></i>${id}` : PD_LBL[id] || id}${kb ? `<span class="kb">${esc(kb)}</span>` : ''}</button>`;
  }).join('')}</div>`;
}

/* ---------- mapeamento de teclas ---------- */
/* o app guarda um mapa só, para todos os controles e o teclado (KeymapStore.kt) */
const KM = { maps: { all: Object.assign({}, KM_DEF, { BACK: 'BUTTON_MODE', RT: 'BUTTON_R1' }) }, scope: 'all', swapped: false, cap: null, capPrev: null };
const kmMap = () => KM.maps[KM.scope] || KM.maps.all;
function kmState(id, m = kmMap()) { const c = m[id]; if (!c) return 'unb'; if (Object.keys(m).some(o => o !== id && m[o] === c)) return 'shared'; return c !== KM_DEF[id] ? 'chg' : ''; }
function kmStats() { const m = KM.maps.all, codes = KM_BTNS.map(b => m[b[0]]).filter(Boolean); return { chg: KM_BTNS.filter(b => m[b[0]] !== KM_DEF[b[0]]).length, shared: new Set(codes.filter((c, i) => codes.indexOf(c) !== i)).size }; }
const keyText = c => c || '(sem tecla)';
const PAD_CODES = ['BUTTON_A', 'BUTTON_B', 'BUTTON_X', 'BUTTON_Y', 'BUTTON_L1', 'BUTTON_R1', 'BUTTON_L2', 'BUTTON_R2', 'BUTTON_SELECT', 'BUTTON_START', 'BUTTON_THUMBL', 'BUTTON_THUMBR', 'DPAD_UP', 'DPAD_DOWN', 'DPAD_LEFT', 'DPAD_RIGHT', 'BUTTON_MODE'];
function keyFromEvent(e) {
  const c = e.code || '';
  if (/^Key[A-Z]$/.test(c)) return c.slice(3);
  if (/^Digit\d$/.test(c)) return c.slice(5);
  const m = { Space: 'SPACE', Enter: 'ENTER', Tab: 'TAB', Backspace: 'DEL', ShiftLeft: 'SHIFT_LEFT', ShiftRight: 'SHIFT_RIGHT', ControlLeft: 'CTRL_LEFT', ControlRight: 'CTRL_RIGHT', AltLeft: 'ALT_LEFT', AltRight: 'ALT_RIGHT', ArrowUp: 'DPAD_UP', ArrowDown: 'DPAD_DOWN', ArrowLeft: 'DPAD_LEFT', ArrowRight: 'DPAD_RIGHT', Comma: 'COMMA', Period: 'PERIOD', Slash: 'SLASH', Semicolon: 'SEMICOLON', Minus: 'MINUS', Equal: 'EQUALS', BracketLeft: 'LEFT_BRACKET', BracketRight: 'RIGHT_BRACKET', Backquote: 'GRAVE', Quote: 'APOSTROPHE', Backslash: 'BACKSLASH' };
  return m[c] || (c ? c.toUpperCase() : 'UNKNOWN');
}
function kmAssign(id, code) {
  const m = kmMap(); S.modal = null; KM.cap = null;
  if (m[id] === code) { render(); return; }
  const from = code ? Object.keys(m).find(o => o !== id && m[o] === code) : null;
  if (from) { m[from] = m[id]; m[id] = code; toast(`${kmName(from)} tinha essa tecla, então ficou com a que ${kmName(id)} tinha antes.`); }
  else { m[id] = code; toast(`${kmName(id)}: ${keyText(code)}`); }
}
function kmBody(v) {
  const m = kmMap();
  const shared = KM_BTNS.filter(b => kmState(b[0]) === 'shared').map(b => b[0]);
  const scope = '';
  const draw = padDraw({ note: 'kmdraw', state: id => kmState(id), kb: id => (kmState(id) ? (m[id] || '—').replace('BUTTON_', '') : ''), attrs: id => `data-act="km-cap" data-v="${id}" data-k="kmd-${id}"${id === 'A' ? ' data-autofocus' : ''}` });
  const list = KM_BTNS.map(([id, name]) => { const s = kmState(id); return `<div class="km-row ${s}"><button class="km-pick" data-act="km-cap" data-v="${id}" data-k="kml-${id}"><b>${esc(name)}</b><small>${esc(keyText(m[id]))}${s === 'shared' ? ' · também em outro botão' : ''}</small></button><button class="btn sm ghost" data-act="km-clear" data-v="${id}" data-k="kmc-${id}"${m[id] ? '' : ' disabled'}>Limpar</button></div>`; }).join('');
  return `<div class="km">
    <div class="km-draw">${scope}${draw}
      <div class="legend2" data-note="kmstate"><span class="chg"><i></i>Mudada</span><span class="shared"><i></i>Em dois botões</span><span class="unb"><i></i>Sem tecla</span></div>
      <p class="note" data-note="kmsim">Toque num botão do controle para dar uma tecla a ele, ou escolha na lista.</p>
      ${shared.length ? `<p class="errline">${ic('warn', 15)} Uma tecla aciona dois botões, então um deles não faz nada: ${shared.map(kmName).join(', ')}.</p>` : ''}
      <div class="row" data-note="kmswap"><button class="btn sm${KM.swapped ? ' primary' : ''}" data-act="km-swap" data-k="km-swap" aria-pressed="${KM.swapped}">${ic('refresh', 15)} Trocar A/B e X/Y</button><span class="note">Para controles no padrão da Nintendo, com o A à direita.</span></div>
    </div>
    <div class="km-list">${list}</div>
  </div>`;
}
action('km-cap', el => { KM.cap = el.dataset.v; KM.capPrev = null; S.modal = 'km-cap'; S.mp = {}; render(); });
action('km-assign', el => kmAssign(KM.cap, el.dataset.v));
action('km-clear', el => { kmMap()[el.dataset.v] = ''; toast(`${kmName(el.dataset.v)}: sem tecla`); });
action('km-reset', () => { const m = kmMap(); Object.assign(m, KM_DEF); KM.swapped = false; toast('Teclas restauradas'); });
action('km-swap', () => { const m = kmMap(); [[ 'A', 'B'], ['X', 'Y']].forEach(([a, b]) => { const t = m[a]; m[a] = m[b]; m[b] = t; }); KM.swapped = !KM.swapped; toast(KM.swapped ? 'A/B e X/Y trocados' : 'A/B e X/Y de volta'); });
modal('km-cap', () => { const id = KM.cap, m = kmMap(); return { html: sheetHead(`Aperte uma tecla para ${esc(kmName(id))}`) + `<div class="cap" data-note="kmcap"><span class="pulse">${ic('gamepad', 30)}</span><b>Esperando um botão do controle ou do teclado…</b><small>Agora: ${esc(keyText(m[id]))}</small></div><div class="acts"><button class="btn ghost" data-act="km-assign" data-v="" data-k="kc-none">Deixar sem tecla</button><button class="btn" data-act="close" data-k="kc-x" data-autofocus>Cancelar</button></div>` }; });

screen('keymap', {
  title: 'Mapeamento de teclas', globalScope: true,
  render() {
    return single({ key: 'keymap', title: 'Mapeamento de teclas', sub: 'Todos os controles e o teclado', icon: 'keyboard',
      actions: `<button class="btn sm ghost" data-act="km-reset" data-k="km-reset">${ic('reset', 15)} Restaurar</button>`,
      body: kmBody, hints: [['A', 'Escolher a tecla', 'a'], ['Y', 'Limpar', 'y'], ['X', 'Trocar A/B e X/Y', 'x'], ['B', 'Voltar', 'back']] });
  },
  onRawKey(e) {
    if (S.modal !== 'km-cap') return false;
    e.preventDefault();
    if (e.key === 'Escape') { S.modal = null; KM.cap = null; render(); return true; }
    if (!e.repeat) kmAssign(KM.cap, keyFromEvent(e));
    return true;
  },
  capturePad(p) {
    if (S.modal !== 'km-cap') return false;
    const now = p.buttons.map(b => !!b.pressed);
    if (!KM.capPrev) { KM.capPrev = now; return true; }
    const i = now.findIndex((b, n) => b && !KM.capPrev[n]); KM.capPrev = now;
    if (i >= 0 && PAD_CODES[i]) kmAssign(KM.cap, PAD_CODES[i]);
    return true;
  },
  onKey(k) {
    if (S.modal) return false;
    const a = document.activeElement, id = a && a.dataset && (a.dataset.act === 'km-cap' ? a.dataset.v : null);
    if (k === 'y' && id) { kmMap()[id] = ''; toast(`${kmName(id)}: sem tecla`); return true; }
    if (k === 'x') { ACT['km-swap'](); return true; }
    return false;
  },
});

/* ---------- editor de toque ---------- */
const TE = { work: null, sel: null, snap: true, group: false, perGame: false, collapsed: false, cOpen: false, undo: [], dirty: false };
const TE_GROUPS = [['lt', 'lb'], ['rt', 'rb'], ['back', 'start']];
const teFromGame = () => { const p = S.stack[S.stack.length - 1]; return !!p && p.name === 'ingame'; };
const teOr = () => isPortrait() ? 'port' : 'land';
const teList = () => TE.work[teOr()];
const teGroup = id => TE.group ? (TE_GROUPS.find(g => g.includes(id)) || [id]) : [id];
function teInit() { if (!TE.work) { TE.work = JSON.parse(JSON.stringify(TOUCH_LAYOUT)); for (const o of ['land', 'port']) TE.work[o].forEach(e => { if (e.s == null) e.s = 1; if (e.dz == null) e.dz = e.k === 'dpad' ? 33 : e.k === 'stick' ? 0 : null; }); TE.undo = []; TE.dirty = false; } }
function teSnap() { TE.undo.push(JSON.stringify(TE.work)); if (TE.undo.length > 40) TE.undo.shift(); }
function teGrid() { const s = document.getElementById('screen'), w = s.clientWidth, h = s.clientHeight, cell = Math.min(w, h) / 20; return [Math.max(2, Math.round(w / cell)), Math.max(2, Math.round(h / cell))]; }
function teSnapTo(id) {
  const [gx, gy] = teGrid(), l = teList(), e = l.find(x => x.id === id);
  const nx = clamp(Math.round(e.x / 100 * gx), 1, gx - 1) / gx * 100, ny = clamp(Math.round(e.y / 100 * gy), 1, gy - 1) / gy * 100, dx = nx - e.x, dy = ny - e.y;
  for (const gid of teGroup(id)) { const o = l.find(x => x.id === gid); o.x = clamp(o.x + dx, 2, 98); o.y = clamp(o.y + dy, 2, 98); }
}
function teMove(dx, dy) { const [gx, gy] = teGrid(), l = teList(); teSnap(); for (const gid of teGroup(TE.sel)) { const o = l.find(x => x.id === gid); o.x = clamp(o.x + dx * 100 / gx, 2, 98); o.y = clamp(o.y + dy * 100 / gy, 2, 98); } TE.dirty = true; render(); }
function teInspector() {
  const e = teList().find(x => x.id === TE.sel); if (!e) return '';
  const g = teGroup(e.id).filter(x => x !== e.id).map(id => teList().find(x => x.id === id).t);
  return `<aside class="te-insp card tight" data-note="teinsp"><header><b>${esc(e.t)}</b><button class="ibtn" style="width:30px;height:30px" data-act="te-sel" data-v="" data-k="te-insp-x" aria-label="Fechar">${ic('x', 16)}</button></header>
    <div class="te-r"><span>Tamanho</span><span class="stp"><button data-act="te-size" data-d="-10" data-k="te-s-" aria-label="Menor">${ic('minus', 15)}</button><b>${Math.round(e.s * 100)}%</b><button data-act="te-size" data-d="10" data-k="te-s+" aria-label="Maior">${ic('plus', 15)}</button></span></div>
    ${e.dz != null ? `<div class="te-r"><span>Zona morta</span><span class="stp"><button data-act="te-dz" data-d="-5" data-k="te-dz-" aria-label="Menos zona morta">${ic('minus', 15)}</button><b>${e.dz}%</b><button data-act="te-dz" data-d="5" data-k="te-dz+" aria-label="Mais zona morta">${ic('plus', 15)}</button></span></div>` : ''}
    <div class="te-r"><span>Visível</span><button class="tg" role="switch" aria-checked="${e.vis !== false}" data-act="te-vis" data-k="te-vis" aria-label="Visível"></button></div>
    ${g.length ? `<p class="note">Com Grupo, move e muda de tamanho junto com ${g.map(esc).join(', ')}.</p>` : ''}</aside>`;
}
function teBar() {
  if (isC() ? !TE.cOpen : TE.collapsed) return `<button class="te-pill" data-act="te-collapse" data-k="te-open">${ic('move', 15)} Ferramentas de edição${isC() ? ' ' + glyph('⧉') : ''}</button>`;
  const o = teOr(), g = curGame();
  return `<div class="te-bar" data-note="tebar">
    <div class="seg" role="group" aria-label="Orientação"><button data-act="te-orient" data-v="land" aria-pressed="${o === 'land'}" data-k="te-land">Paisagem</button><button data-act="te-orient" data-v="port" aria-pressed="${o === 'port'}" data-k="te-port">Retrato</button></div>
    <button class="chip" data-act="te-snapt" aria-pressed="${TE.snap}" data-k="te-snap">Alinhar à grade</button>
    <button class="chip" data-act="te-group" aria-pressed="${TE.group}" data-k="te-group">Grupo</button>
    ${teFromGame() ? `<button class="chip" data-act="te-pergame" aria-pressed="${TE.perGame}" data-k="te-pg">Só ${esc(g.name)}</button>` : ''}
    <span class="sepv"></span>
    <button class="ibtn" data-act="te-undo" data-k="te-undo" data-note="teundo" aria-label="Desfazer" title="Desfazer"${TE.undo.length ? '' : ' disabled'}>${ic('reset', 17)}</button>
    <button class="btn sm ghost" data-act="modal" data-v="te-layouts" data-k="te-lp">Layouts</button>
    <button class="btn sm ghost" data-act="modal" data-v="te-globals" data-k="te-gl">Gerais</button>
    <button class="btn sm ghost" data-act="te-reset" data-k="te-reset">Restaurar</button>
    <button class="ibtn" data-act="te-collapse" data-k="te-col" aria-label="Recolher" title="Recolher">${ic('chevD', 17)}</button>
    <span class="sepv"></span>
    <button class="btn sm ghost" data-act="te-cancel" data-k="te-cancel">Cancelar</button>
    <button class="btn sm primary" data-act="te-save" data-k="te-save">Salvar e sair</button>
  </div>`;
}
action('te-sel', el => { TE.sel = el.dataset.v || null; render(); });
action('te-size', el => { const e = teList().find(x => x.id === TE.sel); if (!e) return; teSnap(); const s = clamp(Math.round(e.s * 100 + Number(el.dataset.d)), 50, 300) / 100; for (const gid of teGroup(e.id)) teList().find(x => x.id === gid).s = s; TE.dirty = true; render(); });
action('te-dz', el => { const e = teList().find(x => x.id === TE.sel); if (!e || e.dz == null) return; teSnap(); const [a, b] = e.k === 'dpad' ? [15, 60] : [0, 50]; e.dz = clamp(e.dz + Number(el.dataset.d), a, b); TE.dirty = true; render(); });
action('te-vis', () => { const e = teList().find(x => x.id === TE.sel); if (!e) return; teSnap(); e.vis = e.vis === false; TE.dirty = true; render(); });
action('te-orient', el => { if (S.orient === el.dataset.v) return; S.full = false; S.orient = el.dataset.v; persist(); fit(); render(); });
action('te-snapt', () => { TE.snap = !TE.snap; render(); });
action('te-group', () => { TE.group = !TE.group; render(); });
action('te-pergame', () => { TE.perGame = !TE.perGame; toast(TE.perGame ? `Editando o layout só de ${curGame().name}` : 'Editando o layout compartilhado'); });
action('te-undo', () => { if (!TE.undo.length) return; TE.work = JSON.parse(TE.undo.pop()); TE.dirty = true; render(); });
action('te-reset', () => { teSnap(); TE.work[teOr()] = JSON.parse(JSON.stringify(TOUCH_DEFAULT[teOr()])).map(e => Object.assign(e, { s: 1, dz: e.k === 'dpad' ? 33 : e.k === 'stick' ? 0 : null })); TE.dirty = true; toast('Layout de fábrica nesta orientação; Desfazer volta'); });
action('te-collapse', () => { if (isC()) TE.cOpen = !TE.cOpen; else TE.collapsed = !TE.collapsed; render(); });
const teLeave = () => { const a = document.activeElement; if (a && a.blur) a.blur(); back(); };
action('te-cancel', () => { TE.work = null; teLeave(); });
action('te-save', () => { if (TE.dirty) { for (const o of ['land', 'port']) TOUCH_LAYOUT[o] = JSON.parse(JSON.stringify(TE.work[o])); } const msg = TE.dirty ? (TE.perGame ? `Layout salvo só para ${curGame().name}` : 'Layout salvo') : ''; TE.work = null; teLeave(); if (msg) toast(msg); });
modal('te-layouts', () => ({ wide: true, html: sheetHead('Layouts', TE.perGame ? 'Salvar guarda o layout deste jogo; aplicar o substitui.' : 'Salvar guarda o layout compartilhado; aplicar o substitui.') + layoutsHTML('tl') + `<p class="note">Os layouts salvos são mantidos com Salvar e sair.</p><div class="acts"><button class="btn primary" data-act="close" data-k="tl-ok" data-autofocus>Pronto</button></div>` }));
modal('te-globals', () => ({ wide: true, html: sheetHead('Gerais', 'Valem para o controle inteiro, não só para o botão escolhido.') + touchGlobalsHTML(isC() ? 'c' : 'b') + `<div class="acts"><button class="btn primary" data-act="close" data-k="tg-ok" data-autofocus>Pronto</button></div>` }));

screen('touchedit', {
  title: 'Editor de toque', globalScope: true,
  render() {
    teInit();
    const g = curGame(), [gx, gy] = teGrid(), l = teList();
    return `<div class="te" style="--gx:${gx};--gy:${gy}"><img class="ig-scene" src="${g.scene || ''}" alt="">
      ${TE.snap ? '<div class="te-grid" data-note="tegrid"></div>' : ''}
      <div class="tcl" id="te-l" style="--op:${Math.max(.5, touchOp())}">${l.map(e => touchEl(e, `data-te="${e.id}" role="button" aria-label="${esc(e.t)}"`, `${TE.sel === e.id ? 'picked' : ''} ${e.vis === false ? 'hid' : ''}`)).join('')}</div>
      ${TE.sel ? '' : `<p class="te-tip">${isC() ? 'LB/RB escolhem um controle; o direcional move uma casa da grade.' : 'Arraste um controle para mover. Toque nele para ver o tamanho e a zona morta.'}</p>`}
      ${teInspector()}${teBar()}
      ${isC() ? hints([['LB/RB', 'Escolher', 'tabs'], ['A', 'Ajustes dele', 'a'], ['X', 'Layouts', 'x'], ['Y', 'Gerais', 'y'], ['⧉', 'Ferramentas', 'view'], ['≡', 'Salvar e sair', 'start'], ['B', 'Cancelar', 'back']], 'tec') : ''}
    </div>`;
  },
  after() {
    const layer = document.getElementById('te-l'); if (!layer) return;
    layer.onpointerdown = e => {
      const el = e.target.closest('[data-te]'); if (!el) return;
      e.preventDefault();
      const id = el.dataset.te, l = teList(), scr = document.getElementById('screen').getBoundingClientRect();
      if (TE.sel !== id) { TE.sel = id; layer.querySelectorAll('.tc.picked').forEach(n => n.classList.remove('picked')); el.classList.add('picked'); }
      const grp = teGroup(id).map(gid => { const o = l.find(x => x.id === gid); return [o, o.x, o.y]; });
      const sx = e.clientX, sy = e.clientY; let moved = false; teSnap();
      try { el.setPointerCapture(e.pointerId); } catch (err) { /* sem captura */ }
      const mv = ev => {
        const dx = (ev.clientX - sx) / scr.width * 100, dy = (ev.clientY - sy) / scr.height * 100;
        if (Math.abs(dx) + Math.abs(dy) > .4) moved = true;
        for (const [o, ox, oy] of grp) { o.x = clamp(ox + dx, 2, 98); o.y = clamp(oy + dy, 2, 98); const n = layer.querySelector(`[data-te="${o.id}"]`); if (n) { n.style.left = o.x + '%'; n.style.top = o.y + '%'; } }
      };
      const up = () => { el.removeEventListener('pointermove', mv); el.removeEventListener('pointerup', up); el.removeEventListener('pointercancel', up); if (moved) { if (TE.snap) teSnapTo(id); TE.dirty = true; } else TE.undo.pop(); render(); };
      el.addEventListener('pointermove', mv); el.addEventListener('pointerup', up); el.addEventListener('pointercancel', up);
    };
  },
  onBack() {
    const a = document.activeElement;
    if (a && a.closest && a.closest('.te-insp, .te-bar')) { a.blur(); return true; }
    TE.work = null; return false;
  },
  onKey(k) {
    if (S.modal) return false;
    const a = document.activeElement, inUi = a && a.closest && a.closest('.te-insp, .te-bar');
    const l = teList();
    if (k === 'lb' || k === 'rb' || k === 'tabs') { if (k === 'tabs') k = 'rb'; let i = l.findIndex(e => e.id === TE.sel); i = (i + (k === 'rb' ? 1 : -1) + l.length) % l.length; TE.sel = l[i].id; if (inUi) a.blur(); render(); return true; }
    if (!inUi && ['up', 'down', 'left', 'right'].includes(k)) { if (!TE.sel) TE.sel = l[0].id; teMove(k === 'left' ? -1 : k === 'right' ? 1 : 0, k === 'up' ? -1 : k === 'down' ? 1 : 0); return true; }
    if (!inUi && k === 'a') { if (!TE.sel) { TE.sel = l[0].id; render(); } setNav(true); focusKey('te-s+'); return true; }
    if (k === 'x') { S.modal = 'te-layouts'; render(); return true; }
    if (k === 'y') { S.modal = 'te-globals'; render(); return true; }
    if (k === 'start') { ACT['te-save'](); return true; }
    if (k === 'view') { ACT['te-collapse'](); return true; }
    return false;
  },
});

/* ---------- teste de controles ---------- */
const PT = { src: 'demo', seen: {}, raf: 0, t0: 0, holdB: 0, gyro: [0, 0, 0] };
const PT_CHECK = ['A', 'B', 'X', 'Y', 'LB', 'RB', 'LT', 'RT', 'L3', 'R3', 'Start', 'Back', '↑', '↓', '←', '→'];
const PT_PD = { A: 'A', B: 'B', X: 'X', Y: 'Y', LB: 'LB', RB: 'RB', LT: 'LT', RT: 'RT', L3: 'L3', R3: 'R3', Start: 'START', Back: 'BACK', '↑': 'UP', '↓': 'DOWN', '←': 'LEFT', '→': 'RIGHT' };
const PT_STD = ['A', 'B', 'X', 'Y', 'LB', 'RB', 'LT', 'RT', 'Back', 'Start', 'L3', 'R3', '↑', '↓', '←', '→'];
const dzOf = side => Math.max(.08, Number(globalOf(`HID.${side}_stick_deadzone_percentage`)));
function ptDevices() {
  if (PT.src === 'none') return [];
  if (PT.src === 'real') { let l = []; try { l = Array.from(navigator.getGamepads ? navigator.getGamepads() : []).filter(Boolean); } catch (e) { l = []; } return l.map((gp, i) => { const m = /Vendor: (\w{4}) Product: (\w{4})/.exec(gp.id) || /^(\w{4})-(\w{4})-/.exec(gp.id); return { id: 'gp' + gp.index, name: gp.id.replace(/\s*\(.*\)\s*/, '').replace(/^\w{4}-\w{4}-/, '') || 'Controle', vidpid: m ? `${m[1]}:${m[2]}`.toUpperCase() : '—', src: ['gamepad', 'joystick'], motor: !!gp.vibrationActuator, gyro: false, real: gp.index, rumble: null }; }); }
  return PADS;
}
function ptInput(p, t) {
  if (p.real != null) {
    let gp = null; try { gp = navigator.getGamepads()[p.real]; } catch (e) { gp = null; }
    if (!gp) return null;
    const b = i => gp.buttons[i] ? gp.buttons[i].value || (gp.buttons[i].pressed ? 1 : 0) : 0, pressed = {};
    PT_STD.forEach((n, i) => { if (b(i) > .5) pressed[n] = true; });
    return { pressed, lx: gp.axes[0] || 0, ly: gp.axes[1] || 0, rx: gp.axes[2] || 0, ry: gp.axes[3] || 0, lt: b(6), rt: b(7) };
  }
  const k = padSlot(p), pressed = {};
  const step = Math.floor(t / (k ? .62 : .45)), idx = (step + k * 5) % 16;
  if ((t % (k ? .62 : .45)) < (k ? .38 : .3)) pressed[PT_CHECK[idx]] = true;
  const r = k ? .05 : .92;
  return { pressed, lx: Math.cos(t * 1.3) * r, ly: Math.sin(t * 1.3) * r, rx: k ? Math.sin(t * .9) * .85 : .04, ry: k ? Math.cos(t * .7) * .5 : -.03, lt: Math.max(0, Math.sin(t * 1.7)), rt: Math.max(0, Math.sin(t * 1.7 + 2.2)), gyro: p.gyro ? [Math.sin(t * 2) * .41, Math.cos(t * 1.4) * .22, Math.sin(t * .8) * .09] : null };
}
const n2 = v => (v < 0 ? '−' : '') + Math.abs(v).toFixed(2).replace('.', DEC);
function ptCardHTML(p) {
  const dl = dzOf('left'), dr = dzOf('right'), opts = PT.src === 'real' ? [] : [['', 'padrão'], ['off', 'desligada'], ['low', 'baixa'], ['medium', 'média'], ['high', 'alta']];
  return `<section class="card pt" data-pad="${p.id}">
    <div class="pt-h"><div><b>${esc(p.name)}</b><small>${p.vidpid} · ${p.src.join(', ')}</small></div><span class="badge acc">Num jogo aberto agora: P${padSlot(p) >= 0 ? padSlot(p) + 1 : 1}</span></div>
    <div class="pt-grid">
      <div>${padDraw({ cls: 'test', pad: p.id })}<div class="pt-checks" data-note="ptcheck">${PT_CHECK.map(n => `<span class="chip" data-ck="${n}">${n}</span>`).join('')}</div></div>
      <div class="stack" style="gap:12px">
        <div class="pt-sticks" data-note="ptstick"><div class="pt-stick" data-st="l" style="--dz:${dl}"><i class="dz"></i><i class="dot"></i><span>Esquerdo</span></div><div class="pt-stick" data-st="r" style="--dz:${dr}"><i class="dz"></i><i class="dot"></i><span>Direito</span></div><div class="pt-read" data-rd="1"></div></div>
        <div style="margin-top:12px"><div class="pt-trig"><span>LT</span><span class="bar"><i data-tg="lt" style="width:0"></i></span><span data-tv="lt">0,00</span></div><div class="pt-trig" style="margin-top:6px"><span>RT</span><span class="bar"><i data-tg="rt" style="width:0"></i></span><span data-tv="rt">0,00</span></div></div>
        <div class="pt-dz" data-note="ptdz"><b>Zona morta do core</b>${['left', 'right'].map(s => `<span>${s === 'left' ? 'Esquerdo' : 'Direito'}</span><span class="stp"><button data-act="step" data-key="HID.${s}_stick_deadzone_percentage" data-d="-1" aria-label="Menos zona morta, ${s === 'left' ? 'esquerdo' : 'direito'}" data-k="ptdz-${p.id}-${s}-">${ic('minus', 14)}</button><b>${esc(label(DEF[`HID.${s}_stick_deadzone_percentage`], globalOf(`HID.${s}_stick_deadzone_percentage`)))}</b><button data-act="step" data-key="HID.${s}_stick_deadzone_percentage" data-d="1" aria-label="Mais zona morta, ${s === 'left' ? 'esquerdo' : 'direito'}" data-k="ptdz-${p.id}-${s}+">${ic('plus', 14)}</button></span>`).join('')}</div>
        ${p.gyro ? `<div class="pt-read" data-gy="1">Giroscópio: mova o controle</div>` : ''}
        ${p.motor ? `<div class="row" data-note="ptrumble"><button class="btn sm" data-act="pt-rumble" data-v="${p.id}" data-k="ptr-${p.id}"${PT.src === 'real' ? ' disabled' : ''}>Vibração no jogo: ${p.rumble ? RUMBLE_T[p.rumble].toLowerCase() : `padrão (menu do jogo)`}</button><button class="btn sm ghost" data-act="pt-vib" data-v="${p.id}" data-k="ptv-${p.id}"${padRumble(p) === 'off' ? ' disabled' : ''}>${ic('vibrate', 15)} Vibrar 0,3 s</button></div>` : `<p class="note">O Android não informa motor de vibração.</p>`}
      </div>
    </div></section>`;
}
function ptFrame() {
  const t = (performance.now() - PT.t0) / 1000;
  for (const p of ptDevices()) {
    const card = app.querySelector(`.pt[data-pad="${p.id}"]`); if (!card) continue;
    const s = ptInput(p, t); if (!s) continue;
    const seen = PT.seen[p.id] = PT.seen[p.id] || {};
    if (s.lt > .5) s.pressed.LT = true; if (s.rt > .5) s.pressed.RT = true;
    for (const n of PT_CHECK) {
      const on = !!s.pressed[n]; if (on) seen[n] = true;
      const b = card.querySelector(`.pd-b[data-pd="${PT_PD[n]}"]`); if (b) { b.classList.toggle('on', on); b.classList.toggle('seen', !!seen[n] && !on); }
      const c = card.querySelector(`[data-ck="${n}"]`); if (c) { c.classList.toggle('on', on); c.classList.toggle('seen', !!seen[n] && !on); c.textContent = n + (seen[n] && !on ? ' ✓' : ''); }
    }
    const dl = dzOf('left'), dr = dzOf('right'), gv = (v, dz) => (!isFinite(v) || Math.abs(v) < dz ? 0 : clamp(v, -1, 1));
    const sl = card.querySelector('[data-st="l"]'), sr = card.querySelector('[data-st="r"]');
    if (sl) { sl.style.setProperty('--x', s.lx.toFixed(3)); sl.style.setProperty('--y', s.ly.toFixed(3)); sl.style.setProperty('--dz', dl); sl.classList.toggle('dead', Math.hypot(s.lx, s.ly) < dl); }
    if (sr) { sr.style.setProperty('--x', s.rx.toFixed(3)); sr.style.setProperty('--y', s.ry.toFixed(3)); sr.style.setProperty('--dz', dr); sr.classList.toggle('dead', Math.hypot(s.rx, s.ry) < dr); }
    const st = card.querySelectorAll('.pd-b.stick'); if (st[0]) { st[0].style.setProperty('--x', s.lx.toFixed(3)); st[0].style.setProperty('--y', s.ly.toFixed(3)); } if (st[1]) { st[1].style.setProperty('--x', s.rx.toFixed(3)); st[1].style.setProperty('--y', s.ry.toFixed(3)); }
    const rd = card.querySelector('[data-rd]'); if (rd) rd.innerHTML = `O jogo recebe<br>E ${n2(gv(s.lx, dl))}; ${n2(gv(s.ly, dl))}<br>D ${n2(gv(s.rx, dr))}; ${n2(gv(s.ry, dr))}`;
    for (const k of ['lt', 'rt']) { const bar = card.querySelector(`[data-tg="${k}"]`), tv = card.querySelector(`[data-tv="${k}"]`), pb = card.querySelector(`.pd-b[data-pd="${k.toUpperCase()}"]`); if (bar) bar.style.width = (s[k] * 100).toFixed(1) + '%'; if (tv) tv.textContent = n2(s[k]) + (s[k] > .5 ? ' (apertado)' : ''); if (pb) pb.style.setProperty('--v', s[k].toFixed(3)); }
    const gy = card.querySelector('[data-gy]'); if (gy && s.gyro) gy.textContent = `Giroscópio (rad/s) x ${n2(s.gyro[0])} · y ${n2(s.gyro[1])} · z ${n2(s.gyro[2])}`;
  }
}
action('pt-rumble', el => { const p = PADS.find(x => x.id === el.dataset.v); if (!p) return; const order = [null, 'off', 'low', 'medium', 'high']; p.rumble = order[(order.indexOf(p.rumble) + 1) % order.length]; render(); });
action('pt-vib', el => {
  const id = el.dataset.v, p = PADS.find(x => x.id === id);
  let done = false;
  if (PT.src === 'real' || !p) { try { const gp = Array.from(navigator.getGamepads()).filter(Boolean)[0]; const amp = { low: .3, medium: .6, high: 1 }[p ? padRumble(p) : 'medium'] || .6; if (gp && gp.vibrationActuator) { gp.vibrationActuator.playEffect('dual-rumble', { duration: 300, strongMagnitude: amp, weakMagnitude: amp }); done = true; } } catch (e) { /* sem vibração */ } }
  const d = app.querySelector(`.pt[data-pad="${id}"] .pd`); if (d) { d.classList.remove('shake'); void d.offsetWidth; d.classList.add('shake'); }
  if (!done) toast(`${p ? p.name : 'Controle'}: vibrando 0,3 s (${RUMBLE_T[p ? padRumble(p) : 'medium'].toLowerCase()})`);
});

screen('padtest', {
  title: 'Testar controles', globalScope: true,
  variants: [{ label: 'Origem', list: [['demo', 'Exemplo animado'], ['real', 'Controle real (Gamepad API)'], ['none', 'Nenhum controle']], get: () => PT.src, set: v => { PT.src = v; PT.seen = {}; } }],
  render() {
    const devs = ptDevices();
    const body = () => `<div class="stack">
      <div class="pt-intro" data-note="ptleave"><span data-note="ptsim"></span>${ic('info', 18)}<p class="note">Aperte todos os botões e mova todos os analógicos. O que você aperta aqui não chega a nenhum jogo. Segure B por um segundo, ou use Voltar, para sair. Num jogo, os controles pegam os jogadores na ordem do primeiro botão apertado.</p></div>
      ${devs.length ? devs.map(ptCardHTML).join('') : `<section class="card"><p style="font-size:14px">${PT.src === 'real' ? 'Nenhum controle visto pelo navegador. Conecte um e aperte um botão.' : 'Nenhum controle conectado. Pareie um por Bluetooth ou ligue pelo cabo.'}</p></section>`}
      ${PT.src === 'demo' ? `<section class="card"><h3>${ic('timeline', 15)} Conexões</h3><ul class="pt-ev">${PAD_EVENTS.map(([n, k]) => `<li>${esc(n)}: ${k}</li>`).join('')}</ul></section>` : ''}
    </div>`;
    return single({ key: 'padtest', title: 'Testar controles', sub: devs.length ? `${devs.length} ${devs.length === 1 ? 'controle' : 'controles'}` : '', icon: 'gamepad', body, hints: [['B', 'Segure para sair', 'back']] });
  },
  after() { cancelAnimationFrame(PT.raf); PT.t0 = PT.t0 || performance.now(); const loop = () => { if (S.route.name !== 'padtest') return; ptFrame(); PT.raf = requestAnimationFrame(loop); }; PT.raf = requestAnimationFrame(loop); },
  capturePad(p) {
    const b = !!(p.buttons[1] && p.buttons[1].pressed), now = performance.now();
    if (b) { if (!PT.holdB) PT.holdB = now; else if (now - PT.holdB > 1000) { PT.holdB = 0; back(); } } else PT.holdB = 0;
    return true;
  },
});

/* ---------- celular como controle ---------- */
const PC = { state: 'form', addr: '', code: '', name: 'Pixel 7a', vib: 'medium', err: '', timer: 0 };
const PC_ERR = { code: 'Código errado', full: 'Todas as vagas de jogador estão ocupadas', version: 'O jogo roda outra versão do Xendroid+', closed: 'O pareamento está fechado no jogo (desligue e ligue os telefones como controle lá para ter um código novo)', unreachable: 'Não deu para alcançar o jogo em 192.168.1.20:41234. Os dois telefones precisam estar no mesmo Wi-Fi ou ponto de acesso, com Telefones como controle ligado no menu do jogo (um endereço novo a cada vez que é ligado).' };
function pcFormHTML(v) {
  const busy = PC.state === 'connecting', D = busy ? ' disabled' : '';
  return `<div class="pc">
    <p class="note span2">No telefone que roda o jogo: abra o menu dele (Voltar), Controles → Telefones como controle. Digite o endereço e o código que ele mostra. Os dois telefones precisam estar no mesmo Wi-Fi ou ponto de acesso. Este telefone joga como P2, P3 ou P4 com o controle de toque. Experimental.</p>
    <section class="card" data-note="pcform">
      <label class="fld">Endereço do jogo (IP:porta)<input class="txt" id="pc-addr" type="text" inputmode="decimal" placeholder="192.168.1.20:41234" value="${esc(PC.addr)}" data-k="pc-addr" autocomplete="off" spellcheck="false"${D}></label>
      <label class="fld">Código (6 dígitos)<input class="txt pc-code" id="pc-code" type="text" inputmode="numeric" maxlength="6" placeholder="000000" value="${esc(PC.code)}" data-k="pc-code" autocomplete="one-time-code"${D}></label>
      <label class="fld">Nome mostrado no jogo (opcional)<input class="txt plain" id="pc-name" type="text" maxlength="24" value="${esc(PC.name)}" data-k="pc-name" autocomplete="off"${D}></label>
      <div class="fld">Vibração neste telefone<div class="seg" role="group" aria-label="Vibração neste telefone">${Object.entries(RUMBLE_T).map(([k, t]) => `<button data-act="pc-vib" data-v="${k}" aria-pressed="${PC.vib === k}" data-k="pc-vib-${k}"${D}>${t}</button>`).join('')}</div></div>
      ${PC.err ? `<p class="errline" data-note="pcerr">${ic('warn', 15)} ${esc(PC.err)}</p>` : ''}
      ${busy ? `<div class="row"><span class="spin" style="display:inline-grid">${ic('refresh', 18)}</span><b>Conectando…</b><span class="sp"></span><button class="btn sm ghost" data-act="pc-cancel" data-k="pc-cancel">Cancelar</button></div>` : `<button class="btn primary wide" data-act="pc-connect" data-k="pc-go"${PC.addr && PC.code ? '' : ''}>Conectar</button>`}
    </section>
    <p class="note span2">O jogo só escuta na rede local dele, nunca na internet. Dez códigos errados travam o pareamento até os telefones como controle serem desligados e ligados de novo lá (um código novo).</p>
  </div>`;
}
function pcPlayHTML() {
  return `<div class="pc-play" data-note="pcplay">${touchOverlay(1.25)}<div class="pc-pill"><b class="mono" style="color:var(--acc)">P2</b><span class="mono">18 ms</span><button class="btn sm" data-act="pc-leave" data-k="pc-leave">${ic('exit', 15)} Sair</button></div></div>`;
}
action('input:pc-addr', el => { PC.addr = el.value; });
action('input:pc-code', el => { el.value = el.value.replace(/\D/g, '').slice(0, 6); PC.code = el.value; });
action('input:pc-name', el => { PC.name = el.value; });
action('pc-vib', el => { PC.vib = el.dataset.v; render(); });
action('pc-cancel', () => { clearTimeout(PC.timer); PC.state = 'form'; render(); });
action('pc-leave', () => { PC.state = 'form'; PC.err = ''; toast('Desconectado do jogo'); });
action('pc-connect', () => {
  const a = PC.addr.trim(), c = PC.code.trim();
  if (!/^\d{1,3}(\.\d{1,3}){3}:\d{2,5}$/.test(a)) { PC.err = 'Digite o endereço que o jogo mostra, como 192.168.1.20:41234'; render(); return; }
  if (!/^\d{6}$/.test(c)) { PC.err = 'O código são os 6 dígitos que o jogo mostra'; render(); return; }
  PC.err = ''; PC.state = 'connecting'; render();
  PC.timer = setTimeout(() => { if (S.route.name !== 'phonepad' || PC.state !== 'connecting') return; if (c === '482913') { PC.state = 'playing'; } else { PC.state = 'form'; PC.err = PC_ERR.code; } render(); }, 1300);
});

screen('phonepad', {
  title: 'Celular como controle', globalScope: true,
  variants: [{ label: 'Estado', list: [['form', 'Formulário'], ['error', 'Código errado'], ['unreachable', 'Jogo fora de alcance'], ['connecting', 'Conectando'], ['playing', 'Conectado']], get: () => PC.state === 'form' && PC.err ? (PC.err === PC_ERR.code ? 'error' : PC.err === PC_ERR.unreachable ? 'unreachable' : 'form') : PC.state, set: v => { clearTimeout(PC.timer); PC.err = v === 'error' ? PC_ERR.code : v === 'unreachable' ? PC_ERR.unreachable : ''; PC.state = v === 'error' || v === 'unreachable' ? 'form' : v; if (v !== 'form') { PC.addr = '192.168.1.20:41234'; PC.code = v === 'error' ? '482931' : '482913'; } } }],
  render() {
    if (PC.state === 'playing') return pcPlayHTML();
    return single({ key: 'phonepad', title: 'Usar este telefone como controle', sub: 'Para um jogo aberto em outro telefone', icon: 'phone', body: pcFormHTML });
  },
  onBack() { if (PC.state === 'playing') { PC.state = 'form'; render(); return true; } clearTimeout(PC.timer); if (PC.state === 'connecting') PC.state = 'form'; return false; },
});
