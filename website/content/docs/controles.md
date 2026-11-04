---
title: Controles
description: Controles de toque, controles físicos de P1 a P4, mapeamento de teclas, teste, vibração, giroscópio e o celular como controle.
section: usar
order: 5
conferido: e179885e7
fontes: [app/src/main/java/xendroid/compose/ui/controls/ControlsScreen.kt, app/src/main/java/xendroid/compose/ui/controls/TouchOptions.kt, app/src/main/java/xendroid/compose/gamepad/GamepadLayoutSchema.kt, app/src/main/java/xendroid/compose/gamepad/LayoutPresets.kt, app/src/main/java/xendroid/compose/ui/keymap/KeymapScreen.kt, app/src/main/java/xendroid/compose/companion/CompanionHost.kt, app/src/main/java/xendroid/compose/companion/CompanionNetwork.kt, app/src/main/res/values-pt-rBR/strings_controls.xml]
---

A área **Controles** reúne quem joga como P1 a P4, as opções do toque, os controles físicos, vibração e movimento, os telefones como controle e quatro ferramentas: Mapeamento, Editor de toque, Testar controles e Celular como controle.

{{> print id=controles legenda="Área Controles: quem joga agora, toque, controles físicos, vibração e movimento, telefones e as ferramentas."}}

## Quem joga

Os controles entram como P1 a P4 na ordem em que são usados: o primeiro botão apertado, ou um analógico, gatilho ou direcional empurrado além da metade. Até lá, o controle não ocupa jogador nem manda nada ao jogo. Sem controle físico, o P1 usa os controles de toque. Cada jogador entra com o perfil escolhido em **Perfis → Quem joga** (veja [perfis](doc:interface#perfis-e-saves)).

## Controles de toque

| Opção | Padrão |
|---|---|
| Controles de toque (cada jogo pode mudar na ficha) | ligados; começam desligados se havia um controle físico conectado na primeira abertura |
| Visual | Moderno (vidro escuro, letras coloridas); o Clássico continua como opção |
| Opacidade | 65 %, de 20 a 100 % |
| Esconder sozinho | depois de 8 s; 0 desliga |
| Vibração ao tocar | desligada |
| Esconder enquanto um controle joga como P1 | ligado |
| Tela dividida | desligada; ou em dobrável meio aberto, ou sempre |
| Deslizar entre os botões e até um analógico | desligado |
| Câmera por toque (o lado direito livre gira a câmera) | desligada |

O **Editor de toque** move e muda o tamanho de cada controle, a zona morta e o que aparece, com alinhamento à grade e desfazer. Os **layouts salvos** (até 30) podem valer só para um jogo e por orientação (em pé ou deitado), e dá para exportar e importar.

{{> print id=controles-moderno legenda="Controles de toque no visual Moderno."}}

## Controles físicos

Cada controle conectado aparece com o próprio ID, se tem giroscópio e a própria intensidade de vibração. O cartão **Ajustes do core para controles** traz as zonas mortas dos analógicos e o botão Guia, para todos os jogos (cada jogo pode mudar na ficha).

- **Mapeamento de teclas**: a tecla de cada um dos 16 botões. Uma tecla já usada troca de lugar com a outra; com controle, A escolhe, Y limpa e X troca A/B e X/Y. **Restaurar** volta ao mapa original. Um mapa vale para todos os controles.
- **Testar controles**: o controle desenhado acende o que é apertado, com os analógicos (e a zona morta do core, ajustável ali), os gatilhos, o giroscópio e a vibração. Nada vai para um jogo; segure B por um segundo para sair.

## Vibração e movimento

- **Vibração dos controles**: Desligada, Baixa, Média (padrão) ou Alta.
- **Câmera pelo giroscópio** (desligada por padrão), **Mira pelo giroscópio** (sempre, segurando LT ou segurando LB) e a **sensibilidade**. A calibração fica no menu em jogo. Sem sensor, o app avisa que o telefone não tem giroscópio.
- **Entrada sem buffer**: ligada por padrão, só no Android 11 ou mais novo.

O menu em jogo continua mudando essas opções na hora; aqui ficam os valores com que cada jogo começa.

## Celular como controle {#celular-como-controle}

Experimental. Um segundo telefone entra no jogo como **P2, P3 ou P4** (nunca P1):

1. No telefone do jogo: menu em jogo → **Controles → Telefones como controle**. O app liga um servidor e mostra o endereço (IP:porta) e um código de 6 dígitos.
2. No outro telefone, com o Xendroid+ instalado: **Controles → Celular como controle** (“Usar este telefone como controle”). Digite o endereço e o código e toque em **Conectar**.

Limites:

- só rede local privada (Wi-Fi, ponto de acesso, Ethernet, ancoragem USB ou Bluetooth), nunca dados móveis, VPN ou internet;
- endereço e código novos a cada vez que o servidor liga;
- 10 códigos errados travam o pareamento até desligar e ligar de novo;
- as duas pontas precisam da mesma versão do protocolo.

::: nota Texto de ajuda do app
O texto de ajuda dentro do jogo ainda diz para abrir “Biblioteca → ⋮ → Usar este telefone como controle”, item que o menu da biblioteca não tem mais. O caminho certo é o da área Controles, acima.
:::

## Fora do app, por enquanto

Estas propostas do redesenho da interface ficaram de fora, com o motivo registrado no [documento do redesenho](repo:docs/ui-redesign/app.md):

- um mapa de teclas para cada controle (hoje um vale para todos);
- achar o jogo na rede e ler um QR code no celular como controle.

O [simulador](ferramenta:controls) mostra a área Controles e as quatro ferramentas.
