# XenDroid — plano mestre de evolução com cinco referências

**Pesquisa:** 2026-10-01. **Escopo:** X360 Mobile, Bannerlator, DroidDeck, Eden e GameHub,
comparados ao código e ao [status atual do XenDroid](experiencia-em-jogo-status.md).
**Entrega desta pesquisa:** especificação; não implementa novas funcionalidades.
**Atualização 2026-10-01 (auditoria S0):** estado real por item na seção 6.1, base
atualizada na seção 4, próxima fatia na seção 14; achados com evidência em
[auditoria-s0-2026-10-01.md](auditoria-s0-2026-10-01.md).
**Atualização 2026-10-03 (varredura completa):** o histórico inteiro de commits e changelogs do
Bannerlator, DroidDeck, aX360e ("X360E"), X360 Mobile e Eden (nightly) comparado feature a
feature com o XenDroid em [varredura-referencias-2026-10-03.md](varredura-referencias-2026-10-03.md),
com as próximas fatias sugeridas na seção 5 de lá.

Este documento é o backlog consolidado para as próximas implementações e iterações.
O [plano inicial](plano-experiencia-em-jogo.md) preserva decisões anteriores;
o [cloud-goal](cloud-goal-experiencia-em-jogo.md) continua sendo o método operacional.
O novo plano não transforma um recurso compilado em recurso validado no aparelho.

## 1. Decisão de produto e ordem de execução

Construir um emulador Xbox 360 Android com identidade de jogo estável, interface
por controle/toque, dados portáveis, compatibilidade reproduzível e apresentação
medida. **A prioridade imediata é estabilizar as integrações já feitas**, especialmente
armazenamento, HID, lifecycle, caches e FG. Depois ampliar biblioteca, controles,
diagnóstico e multiplayer; promover experimentos a release apenas com evidência.

Ordem recomendada:

1. **P0:** auditoria e testes de integração das funções atuais; identidade de build,
   sessão e cache; correção de falhas de recuperação/ownership encontradas.
2. **P1:** centro de compatibilidade, histórico/recents, storage/portabilidade,
   onboarding, perfis de configuração, controle e updater confiável.
3. **P2:** múltiplos jogadores, companion LAN, ações avançadas de conteúdo,
   editor/presets portáveis, métricas e apresentação adaptativa.
4. **P3:** Bluetooth companion, HDR, perfis FP16/flow, otimizações específicas de
   GPU/JIT e outros experimentos condicionais. Não mudar defaults por benchmarks
   de outro programa, GPU, API gráfica ou jogo.

Não atribuir um percentual fictício de conclusão: cada tarefa tem código,
testes de componente, integração Android, dados reais e, quando aplicável,
benefício medido como estados distintos.

## 2. Fontes, revisões e força da evidência

Recorte de histórico usado: commits até **2026-09-30 23:59:59 UTC** para os três
projetos com código de app. Horários de Eden são frequentemente UTC+02:00;
`2026-10-01 00:28+02:00` ainda está dentro desse recorte UTC. Referências a
releases não significam que seus testes foram reproduzidos por nós.

| Referência | Revisão/release consultado | Evidência utilizável |
|---|---|---|
| **X360 Mobile** | `v0.6.3` (2026-08-17), `v0.6.2-public.hotfix.1`, `v0.6.2`, `v0.6.0` | Repositório oficial declara **closed-source** e distribui releases/guias. Usar comportamentos e especificações das notas; não há código de otimização/companion a transplantar. |
| **Bannerlator** | `f8cd429a0fc1a92d6601411e8ffecd8786d8264a` (2026-09-30); releases 3.1.3/anteriores | Código Kotlin/Java/C++ e commits verificáveis. Licença geral GPL; conferir proveniência por arquivo/dependência, inclusive portes de outros projetos. |
| **DroidDeck** | `941a6c3ef91d0fd81d8045ca6ca99fd85ac85363` (2026-09-30); commits #87/#96/#98/#103 | Código aberto, com backends Steam/Linux/Wine específicos. Adaptar UX, transações, estado e testes; não importar runtime Linux para o Xenia. |
| **Eden** | `2a7e9b53b12711f679238ee738a2ce57cc42c8ed` (2026-09-30 UTC); stable 0.2.1 e 0.2.0 | Código oficial em Forgejo. Arquivos atuais GPL-3.0-or-later e legados GPL-2.0-or-later; auditá-los individualmente. Core Switch difere de PowerPC/Xenos. |
| **GameHub GameSir** | Site oficial + anúncio 6.2.0 reportado em 2026-08-25 | Produto/UX publicamente descritos; não foi localizado fonte pública verificável do cliente/motor. Conteúdo Steam, lojas e ganhos de desempenho são anúncios, não código ou benchmark XenDroid. |

GameHub aqui significa **o app Android da GameSir**, não o projeto Linux homônimo.
O site oficial consultado por busca ainda apresentava 6.0.9; a página completa
deu timeout. A imprensa relata 6.2.0 e há relatos comunitários posteriores.
Não usar páginas não oficiais de download para afirmar a versão mais recente,
obter código ou embasar promessas de desempenho.

## 3. Resultado da investigação por projeto

### 3.1 X360 Mobile: concorrente diretamente comparável

**Notas oficiais [X1–X4]:**

- Companion Android por LAN, até três clientes P2/P3/P4; P1 protegido salvo modo
  remote-only escolhido pelo host; pareamento confirmado, hosts confiáveis,
  recuperação de conexão, rumble e seleção de GamerProfile. Bluetooth é experimental.
- Editor compartilhado de controles, presets/importação/exportação TOML, indicador
  de jogador, placement relativo às bordas e arte de controle.
- Biblioteca controller-first, Blades/Metro, pins, Continue Playing/Last Played/
  Recents, capas/metadados por Title ID, cache persistente, rescan incremental,
  conteúdo instalável/TU/DLC e retorno a frontend externo.
- DocumentsProvider, lixeira interna recuperável, dados/perfis/saves/configuração
  por título portáveis, first-run wizard e cinco idiomas documentados.
- Teclado virtual XAM navegável por controle/toque, tratamento tolerante de texto
  guest e seleção/sign-in do perfil local. Iterar os fluxos existentes no XenDroid.
- Compatibility Center com resultados locais por título, catálogo público em cache,
  flight recorder, estatísticas guest e revisão após sessão. Submissão online estava
  **desabilitada** nas notas; não planejar copiar uma API pública inexistente.
- Atualizações ordenadas por versionCode, canais, verificação de APK/hash/pacote/
  assinatura; documentação de callbacks JNI protegidos no empacotamento.
- Políticas de Vulkan por device/driver, prova de dynamic state, cache binário
  separado por identidade/versão/modo, gravação atômica e checkpoints; pool de
  upload aquecido preservado até pressão de memória real.
- VBlank por deadlines absolutos com rebase após stalls; ADPF `auto|off|on` para
  workload mensurável de command processor; áudio adaptativo com histerese de underruns.

**Aplicação:** referência forte de arquitetura de produto Xbox e casos de teste.
Os resultados em Odin3/Adreno830 não validam Adreno825. Não tratar opções
descritas como implementação disponível nem mudar readback/resolve por semelhança
de nome com o cvar local.

### 3.2 Bannerlator: contrato de opções, diagnóstico e ergonomia

**Código/commits [B1–B6]:**

- Menu por abas, HUD configurável, controles ao vivo e estados de FG.
- Teste visual de controle/bindings e feedback de conexão.
- Atualizador com manifest/versionCode, preferência de incluir prereleases, skip
  e cache. Não confundir essa preferência com os três canais do DroidDeck.
- Portes recentes em `feat/linux-dd-ports-4`: latência/unbuffered input,
  encerramento das próprias tarefas da sessão, recuperação de runtime interrompido
  e offline por jogo. O progress log no pin consultado os registra **não integrados
  à main, não compilados e não testados no aparelho**; são referências de branch,
  não funcionalidades comprovadas da release 3.1.3.
- Seletor `esync/ntsync/fsync/wineserver` baseado em capacidades, fallback e motivo
  de indisponibilidade. **Esses mecanismos não são recursos Xenia**; aproveitar o
  modelo requested/effective/reason, não adicionar switches de Wine ao XenDroid.
- Novos cards Android/Windows, informações de versões de componentes e melhorias
  de transferência de buffers sem limitador: úteis como lição de ownership/status.

**Aplicação:** fazer o menu atual menos extenso e contextual; cada opção explica
escopo/backend e persistência. Diagnóstico deve preservar origem/licença de portes.

### 3.3 DroidDeck: distribuição, sessão e cadência

**Código/commits [D1–D7]:**

- `AppUpdates`/`SelfInstaller`: Stable/Nightly/Test, variante correspondente ao
  pacote, identidade de fonte/build, cache offline do catálogo, hash de download e
  PackageInstaller com resultado explícito.
- `SessionState`/serviço: loading/run/stop/end e status de falha. O objeto do
  DroidDeck é process-wide porque Activity/serviço vivem no mesmo processo; no
  XenDroid, **não usar um singleton Kotlin como IPC entre principal e `:emu`**.
- `DriverPairs`: escolher compositor bionic + runtime glibc em pares compatíveis.
  XenDroid tem um Vulkan nativo: aproveitar catálogo por hardware/proveniência e
  seleção coerente, não criar um par Linux que não existe no seu runtime.
- `Lossless`: DLL/cache identificados por SHA, builds serializados, cache anterior
  mantido após falha, troca de caminho recarrega engine, estados claros. Target FPS
  LSFG 60/90/120, além de multiplicador, mira ligeiramente abaixo dos Hz.
- #87: limite de FPS coordenado com painel, refresh observado durante sessão,
  menos JNI/copiar cursor só quando muda, watchers com backoff e cache Mesa database.
- Workaround de polling KGSL zero-timeout em preload/adapter e clock pin opcional:
  específicos de caminho/driver e com números de demo D3D12. Primeiro provar que
  o problema existe no caminho Vulkan nativo local; nenhum ganho é presumido.

**Aplicação:** updater e contratos de sessão/componentes são P1; governor/target-rate
FG e refresh dinâmico são P0/P2. Proot, gamescope, Flatpak e biblioteca Steam são
arquiteturas diferentes e não entram como dependências do Xbox.

### 3.4 Eden: pipeline de apresentação e integrações Android

**Código/releases/commits [E1–E7]:**

- Quick Settings com indicador global/per-game, sliders/opções e cards de shaders.
- LSFG com `LosslessScalingHelper`, suporte GPU via JNI, multipliers/target rate/
  flow scale, e `FrameGenPacer` com aquecimento, credit/limite, probe e rebase.
- `vk_present_manager` com alvos de fila e reutilização de recursos. Copiar a
  ideia de medir/backpressure; não copiar os limites 8/6 como constantes Android/Xenia.
- Setembro: correções de cache ao mudar driver, tratamento separado de pós-processo
  em applets, readback/fences e buffers. Não confundir Switch NCE com Xbox ARM64 JIT.
- ControllerNavigationGlobalHook e InputOverlay: inputs de controlador com origem
  verificada, A/B invertíveis, lifecycle/foco e layout multi-orientação.
- Driver manager/resolver/metadata e árvore de documentos URI. Há fuzzy matching
  de nomes no resolver consultado: usar para sugestão, **não** para confiar que dois
  ZIPs de driver ou hashes de versão são equivalentes.
- Stable/nightly e múltiplos targets documentados, idiomas e profile/add-on manager.

As releases 0.2.0 (2026-05-13) e 0.2.1 (2026-06-01) dão contexto de distribuição
e correções; o pipeline/LSFG acima foi lido no snapshot de setembro. Não atribuir
automaticamente código posterior a essas releases. As notas de 0.2.1 descrevem um
hotfix sem mudança geral de performance; números de PGO de Eden não são metas Xenia.

**Aplicação:** pacer/governor, layers separadas, driver/caches, SAF e orientação.
Package spoof/Genshin e remoção de readback para NCE não são otimizações portáveis.

### 3.5 GameHub: simplificação do produto

**Site e anúncio reportado [G1–G3]:** emulação PC offline, streaming/cloud com
rede, biblioteca organizada, importação, mods, layouts compartilháveis, cloud saves,
compatibilidade, instalações configuráveis e integração Steam/Epic.

**Aplicação:** modos Jogador/Desenvolvedor, assistente de configuração, coleções,
presets, catálogo de compatibilidade e portabilidade. Amigos/Workshop/achievements
Steam não fornecem Xbox Live; não incluir login GameHub/Steam como requisito para
executar jogos Xbox locais. Mods Xbox exigem semântica/formato próprios.

## 4. O que já existe e deve ser iterado, não refeito

Base observada na pesquisa: build `77011a0c-ux-20260930-debug`, 117 testes JVM,
build nativo, lint e testes de pixels constantes em software Vulkan (histórico).
**Atualizado em 2026-10-01 pela [auditoria S0](auditoria-s0-2026-10-01.md)**: a
coluna do meio descreve o código depois do primeiro lote de correções.

| Subsistema local | Base existente (2026-10-01) | Próximo trabalho real |
|---|---|---|
| `ui/ingame`, `EmulatorHostActivity.kt` | Menu único, presets HUD, FPS por escopo, layout editor, áudio/gyro/display/energia; preparo de armazenamento antes da Surface com diálogo de falha; registro de execução (SessionRunId); cap de FG devolvido também com menu fechado | Estado/IPC e input separados do host monolítico; options contextuais, acessibilidade, callbacks/pause ownership testados no aparelho |
| `settings/SettingContract.kt`, `ConfigStore` | Escopo/apply para todo schema; **todas** as gravações do config global passam por lock + replace atômico (o caminho `openLive()` destrutivo foi removido); `SaveConfig()` nativo desligado no Android | `available` genérico salvo driver; falta matriz real de GPU/API/backend e requested/effective de todas as opções |
| `data/GameLibraryRepository.kt` | Scanner/cache local por URI, ícone, Title/Media ID e formato; ordenação "Recently played" e último jogo/tempo de jogo na ficha a partir dos runs; favoritos por identidade estável (`Game.identityKey`: Title ID + Media ID + disco, URI só sem Title ID) | Biblioteca multi-root/incremental, edição/região, covers persistentes |
| `core/SessionLogs.kt`, `sessions/`, `compatibility/` | ZIPs de app-session, Title IDs; **`SessionRunStore`**: um run por boot, BEGIN/RUNNING/ENDING/ENDED/FAILED/INTERRUPTED, finalização única sob lock entre processos, reconciliação pela causa de saída do Android; resumo de desempenho por run (C02 v1.1, com frametime por quadro guest), flight recorder de eventos do host, rajadas de criação de pipelines e de underrun de áudio, causa de erro fatal (C01 v1), exportação por run revisada pelo usuário (C06 v1) e resultados do usuário por título (C03 v1) | Despejo do anel nativo após crash por sinal, marcadores de cena, catálogo remoto (C04) |
| `saves/SaveBackupStore.kt` | Manifesto, scopes, hashes, staging/journal/rollback; recuperação isolada por transação com relatório; morte em cada fronteira e dentro do rollback coberta por testes; previews abandonados descartados após 24 h | Preview desatualizado, cota/retensão e UI das transações confirmadas e das falhas |
| `saves/BackupSync.kt` | SAF/provider opt-in e réplica por hash | Pular envio sem mudança (hoje exporta e relê o remoto a cada `onStart`), permissões, retries, cancel e providers reais |
| `ui/profile/ProfileManagerViewModel.kt` | Criação/edição com avatar limitado e gravado atomicamente; exclusão com prévia dos saves afetados e **lixeira restaurável** (`saves/ProfileTrash.kt`) | Retensão/limpeza automática da lixeira, perfis P2–P4 |
| `driver/DriverPackageInstaller.kt`, `driver/DriverIdentity.kt` | Pacotes content-addressed/ABI/limites; identidade do driver **realmente carregado** (vendor/device/versão/API/driverID/`pipelineCacheUUID`, loader sistema/custom e SHA-256 do pacote instalado de onde a biblioteca carregou) registrada em cada run | Catálogo offline por hardware, seleção/teste reversível |
| `updater/updater.kt` | Feed do fork só para o pacote `.fork`, ordem por `versionCode` (`XenDroid-v<n>-<sha>`, run number do CI), sem log de corpo HTTP | Canais (R02), download/hash/PackageInstaller (R03), changelog/skip (R04) |
| `vulkan_framegen.inc` | Worker, cópias/refcount, Win-FG/LSFG, presets e timing; política de tempo em `FrameGenerationSchedule` testada com relógio fake; saídas atrasadas puladas e contadas | Testes de integração Vulkan/Android, governor de GPU/latência, rate mode e menos sincronização global (F03) |
| `vulkan_pipeline_cache.cc` | `vulkan_pipeline_cache_file.h`: envelope com XXH3, limite 256 MiB, temporário+rename, identidade vendor/device/`pipelineCacheUUID`, arquivo por driver | Gravação fora da thread da GPU (medir antes), cota global de caches |
| `xe_android_input_driver.cpp` | Estado XInput e rumble; índice de tecla validado no JNI | `GetState/GetCapabilities` rejeitam `user_index != 0`: múltiplos jogadores ainda não existem |

**Conclusão:** passar uma suíte de componentes não satisfaz todos os critérios do
cloud-goal. As pendências abaixo contêm implementação, validação e estabilização.

## 5. Arquitetura proposta

### 5.1 Identidades duráveis

- **BuildIdentity:** canal, variant/package, versionCode monotônico, semver,
  commit de base, identificador do conjunto de mudanças locais, schema ABI e assinatura.
  Não anunciar um SHA limpo quando o APK contém alterações não commitadas.
- **GameIdentity:** Title ID + Media ID/edição + disco; **LaunchSource** é URI/path
  separado. Favoritos/recents/compatibilidade não desaparecem ao mover a ROM.
- **DriverIdentity:** SHA de pacote + driverID/version/API + vendor/device +
  pipelineCacheUUID. Nome de arquivo sozinho não é identidade.
- **SessionRunId:** um por boot/execução guest, com app process session como pai.
  Armazenar BEGIN/RUNNING/ENDING/ENDED/FAILED/INTERRUPTED e concluir exatamente uma vez.

### 5.2 Separação de responsabilidades

Extrair gradualmente do host, preservando fluxo atual: `InputRouter`,
`ControllerSlotManager`, `SessionCoordinator`, `PresentationController`,
`RuntimeOptionRegistry`, `StorageCoordinator`, `DiagnosticsRecorder` e UI de menu.
Evitar refactor geral antes de criar testes de contratos críticos.

IPC entre frontend/`:emu`: Binder/AIDL ou protocolo explícito já conectado por
`EmuProcessLink`, snapshots versionados e eventos com Run ID. Writes de configuração
ficam sob lock/merge atômico; arquivo em disco é persistência, não barramento instantâneo.

### 5.3 Contrato de cada ajuste

`requested`, `effective`, `origin=session|game|global|verified-profile`,
`apply=live|surface-reset|next-launch`, `capability=available|unsupported|unknown`,
`reason`, `measurementSource`, `lastUpdated`. Guardar pedido mesmo quando indisponível,
mas nunca mostrar switch ativo como prova de backend ativo. Um estado unknown não é
supported. Driver/recomendação remota não pode escrever qualquer cvar arbitrário.

### 5.4 Dados, workers e permissões

- Lock interno por domínio com ordem definida: sessão guest/storage, instalação de
  componentes, config e DLL/cache. Não manter locks de UI em operações I/O ou GPU wait.
- Jobs finitos de importação/sync seguem política explícita de cancel/retry; UI não
  deve ocultar falha nem dar sucesso antes de commit/verificação.
- Metadados JSON/TOML com versionamento, cota, migração e preservação de desconhecidos.
- Core não reinicializa na recriação de Activity; desanexar Surface não termina a
  sessão. Em caso de inconsistência, rollback/erro preserva dados de usuário.

## 6. P0 — auditoria e estabilização imediatas

Cada item é uma entrega, não uma afirmação de bug já reproduzido, salvo os pontos
de arquivo/condição confirmados por leitura acima.

| ID | Trabalho | Arquivos principais | Aceite |
|---|---|---|---|
| A01 | Fixar matriz de invariantes e snapshots de regressão | `core/EmulatorSession`, `EmulatorHostActivity`, JNI | Boot single-shot, callbacks, P1, frontend retorno, storage ownership e baseline sem FG testados |
| A02 | Testar scheduler completo, não só síntese isolada | `presenter.cc`, `vulkan_framegen.inc`, `presentation_runtime.h` | Clock/queue fake: input substituído, pausa, epoch/resize, teardown, falha, cap restore e sem catch-up burst |
| A03 | Reduzir riscos de waits/globais de dispatch | Win-FG scratch, LSFG `vkd`, await/device idle | Ordem de locks documentada; nenhum deadlock/destroy in-flight; índice de geração e referência de fonte corretos |
| A04 | Recuperação de restores em todos os passos | `SaveBackupStore`, `StorageAccess`, `ContentLease` | Kill antes/depois de cada rename/journal update/rollback; originais intactos, replay idempotente, diretórios fora do scope nunca tocados |
| A05 | Contexto/cancel em layout e cache | `GamepadLayoutStore`, `LsfgAssets`, viewmodels | Erros visíveis, corrupção preservada, configuração multi-processo sem lost update; failed import mantém cache anterior utilizável |
| A06 | Cache pipeline versionado e atômico | `vulkan_pipeline_cache.cc`, storage writer | Ler com cota/envelope/UUID, commit `.tmp→rename`, falha mantém cache anterior; trocar driver não apaga cache guest compartilhável |
| A07 | Indexação e privacidade de logs | `SessionLogs`, `SanitizedSessionExport` | Run ID/Title context não perdido em startup/shortcut; zip truncado e entry desconhecida não vaza dados, execução atual permanece exportável quando seguro |
| A08 | Testes de packaging/lint/JNI e proveniência | Gradle, manifest, Java/native, notices | APK contém símbolos/callbacks esperados, ABI/minSdk correto, assinaturas conhecidas; sem DLL, ROM, save ou cache proprietário no pacote |
| A09 | Auditar mutações de perfis e avatares | `ProfileManagerViewModel`, `ProfilePaths`, JNI de perfil | Preview explicita saves afetados pela árvore XUID; cancel não altera dados; delete recuperável; decode de avatar limitado e gravação atômica; falha não anuncia sucesso parcial |

Testes de backend devem cobrir fluxo com imagens **em movimento** e descontinuidades,
não apenas cor constante. Virtual display Android/test doubles ajudam sem telefone;
informar explicitamente os caminhos não exercitados.

### 6.1 Estado em 2026-10-01 (auditoria S0, primeiro lote)

Estados: **Impl.** = implementado; **Local** = testado localmente (JVM, lógica nativa
ou build); **Aparelho** = validado no aparelho; **Release** = aprovado para release.
Nenhum item chegou a "Aparelho" nesta sessão. Evidências e IDs de achado em
[auditoria-s0-2026-10-01.md](auditoria-s0-2026-10-01.md).

| ID | Estado | Evidência / arquivos | Próxima ação |
|---|---|---|---|
| A01 | Parcial: Impl. | Boot só depois de `prepareStorage()` (lease + recuperação), diálogo em vez de `finish()` silencioso; `configChanges` completos; hand-off de URI do frontend; run durável por boot | Testes instrumentados de lifecycle (Home, recriação, surface) e roteiro 4 da auditoria no aparelho |
| A02 | Impl. + Local | `FrameGenerationSchedule` em `presentation_runtime.h`; `presentation_runtime_test.cc` (substituição, pausa, epoch, cadência > Hz, atraso, clamp); `GenerationCap` + testes JVM para restauração do cap. 2026-10-02: o laço real (`RunFrameGenerationLoop`) roda nos testes com thread e pintor falso (ordem, LSFG 3×, substituição durante a pintura, shutdown durante a pintura, FG desligado no meio) | A/B no aparelho; epoch/resize com device real |
| A03 | Parcial: Impl. | Ordem de locks documentada no loop; ponteiro estático do Win-FG limpo; vida útil das imagens-fonte conferida por leitura | Medir `AwaitAllSubmissions`/`DeviceWaitIdle` (F03) antes de reduzir; validation layers no aparelho |
| A04 | Impl. + Local | Recuperação isolada por transação, relatório, falhas preservadas; testes de morte em before/after de cada swap e dentro do rollback, replay idempotente, previews antigos | Kill real no aparelho durante restore; UI para transações falhas/confirmadas |
| A05 | Impl. + Local | Config: escritores destrutivos removidos (Kotlin e nativo); cache LSFG já era atômico (hipótese refutada); layout de controle já em DataStore multiprocesso. CFG-02 (lock comum Java/nativo do TOML por jogo) verificado por leitura em 2026-10-02: no Android o core só grava o TOML por jogo pelo diálogo de desempenho ImGui e pelos menus do `emulator_window`, nenhum instanciado nesta UI; o único chamador do backend Vulkan usa `persist=false`. Sem escritor nativo alcançável, não há corrida a travar | Reabrir CFG-02 se algum caminho nativo voltar a gravar o TOML por jogo |
| A06 | Impl. + Local | `vulkan_pipeline_cache_file.h` + `pipeline_cache_file_test.cc` (round trip, bit trocado, truncado, legado, limite, troca de driver e volta, falha de escrita, poda) | Roteiro 7 (troca V36/V37) no aparelho; medir custo da gravação na thread da GPU |
| A07 | Parcial: Impl. + Local | Redator cobre formatos de XUID/gamertag do Xenia; entradas de sobras e itens após tombstone exportados; run ID durável existe | Ligar o export por run (C06) e contexto de run nos ZIPs |
| A08 | Impl. + Local | `configChanges`, identidade de build no APK (`<commit>+local.<digest>`), workflow com `versionCode`; `tools/check-jni.py` (nativos Java × tabelas `RegisterNatives` × `Java_*`, com herança e tipos C++; autoteste com 7 erros plantados) no workflow Checks; `tools/check-apk.py` (ZIP, `JNI_OnLoad` no `libe.so` AArch64, só arm64, bloco de assinatura v2+, licenças, nada de DLL/cache/imagem/perfil, min/target SDK) no job de APK; APK passou a declarar só arm64-v8a (PKG-01) | Comparar certificados conhecidos no CI quando houver chave de teste fixa |
| A09 | Impl. + Local | Prévia de saves afetados, lixeira atômica restaurável, avatar limitado/atômico, falhas sem sucesso parcial | Roteiro 4 no aparelho; retensão da lixeira |
| A10 (novo) | Impl. | Escritores do config: `SaveConfig()` nativo no-op no Android; `SaveGameConfig` atômico; `openLive()` removido | Instrumentados em `ConfigTransactionsInstrumentedTest` (compilados; executar no aparelho) |
| A11 (novo) | Impl. + Local | Privacidade: redator e export (LOG-01/02) | Ampliar corpus com logs reais anonimizados |
| A12 (novo) | Impl. | Entrada: bounds check JNI de tecla; prioridade do stick de toque sobre o gyro | Teste de entrada no aparelho (IN-03) |
| A13 (novo) | Pendente (ensaio feito) | Integrar `origin/main` (`6c772efb6`, 51 commits de desempenho) antes de commitar este trabalho. Ensaio com `git merge-tree` repetido após a PR #12: sem conflitos; novos arquivos sem colisão | Decisão do usuário: branch + commit + `git merge origin/main` |
| A14 (novo) | Candidato | Loop de UI nativo acorda a cada 1 ms (energia) | Trocar por condvar só com validação no aparelho |
| A15 (novo) | Impl. + Local | APC do host (`QueueUserCallback`) entregue à thread que recebe o sinal, sem o slot global que corria com dois alertas simultâneos (achado do aX360e, varredura 2026-10-03) | `threading_apc_test` no aparelho real não se aplica (teste nativo NDK); jogos com esperas alertáveis no roteiro geral |
| A16 (novo) | Impl. + Local | Escala/nitidez, Antialiasing e Dither das Configurações aplicados (o `EmulatorWindow` do fork ignorava os cvars); novos ajustes: filtragem anisotrópica e zonas mortas (Jogador), compilação assíncrona, nitidez CAS/FSR e corte contra faixas (Desenvolvedor) | Conferir no aparelho que cada um muda a imagem/entrada (roteiro 43) |

## 7. P1 — produto, compatibilidade, portabilidade e distribuição

### 7.1 Sessões, Compatibilidade e telemetria

| ID | Recurso | Adaptação / aceite |
|---|---|---|
| C01 | Flight recorder circular | Eventos de boot/input/present/compile/underrun/erro/driver com Run ID; limite de bytes/tempo, dump após crash sem I/O por draw; sem conteúdo guest privado por padrão |
| C02 | Resumo de sessão | FPS guest, frames submitted/synthetic separados, frametime percentis, duração/última cena, thermal/battery e underruns com fonte/N/A; encerrar também por morte/interrupção |
| C03 | Compatibility Center local | Por título + build/GPU/driver/edição, status Unknown/Boot/Intro/In-game/Playable/Regression, avaliação do usuário e data; "Playable" não inferido de abrir menu |
| C04 | Catálogo remoto read-only em cache | Schema assinado/versionado, TTL, offline, origem e hardware correspondentes; não misturar resultados de versões ou dispositivos |
| C05 | Perfis verificados/recomendados | Lista allowlisted de ajustes, prerequisites e justificativa; preview/diff, restore anterior, user override vence. Perfil local de fornecedor/OEM não vindo de qualquer feed |
| C06 | Exportação e submissão opcionais | Pacote por execução sanitizado, consentimento por envio, dados de hardware revisáveis; servidor/API novo é projeto separado, sem inventar endpoint X360 Mobile |
| C07 | Benchmark assistant | Roteiros/captura A/B/ABBA, scene markers manuais, drivers/cap/Hz/temperatura fixados; resultado reprodutível, sem chamar synthetic submitted de scanout |

**Estado 2026-10-01:** base de C01–C03/L04 implementada em `sessions/` — run por boot
com estado durável, finalização única (host no `onDestroy`; frontend/:emu reconciliam
runs órfãos pela causa de saída do Android), heartbeat de 30 s limitando a duração de
runs interrompidos; biblioteca com "Recently played" e último jogo/tempo na ficha.
Testes JVM cobrem finalização concorrente, reconciliação e poda.

- **C02 v1.1** (`sessions/RunPerformance.kt`): por run, distribuição dos FPS médios de
  janelas de 1 s (mediana, 5º percentil — explicitamente **não** "1% low" por quadro),
  submissões ao compositor e sintéticas, segundos com FG ativo e ociosos (pausa,
  fundo, sem quadro novo), bateria início/máx/fim. **Frametime por quadro guest**:
  histograma de 1 ms (último balde = 250 ms ou mais) incrementado uma vez por present
  guest no core (`xe::GuestFrameTimeHistogram`, atômico relaxado), lido por JNI e
  guardado como delta do run; a ficha mostra os percentis 99 e 99,9 como limite
  superior ("sob 34 ms"). Intervalos acima de 1 s (pausa, carregamento) reiniciam a
  medição e não entram. Um segundo só entra no histograma de FPS se chegou quadro
  guest novo (contagem do histograma), não apenas submissões do host. Underruns de
  áudio: não medidos. Gravado a cada heartbeat e no fim.
- **C01 v1** (`sessions/RunEvents.kt`): anel de 200 eventos raros do host por run —
  lifecycle, Surface, foco, pausa/retomada, menu, mudanças de estado de
  apresentação/FG (sem os números por quadro), travamento sem quadro guest por 3 s
  e retomada, status térmico do Android, trim de memória, controles
  conectados/desconectados (vendor/product, nunca o nome Bluetooth), prompts do guest
  (sem texto), erros e saída. Nenhum I/O por quadro: gravado em `<run>.events` a cada
  heartbeat, ao ir para o fundo e em eventos críticos (Surface destruída, térmico
  severo, travamento, erro, troca de disco); a poda remove o log com o run. A ficha do
  jogo mostra a linha do tempo do último run. Lote 3: contadores nativos de criação de
  pipelines (quantidade e tempo em `vkCreateGraphicsPipelines`, incluindo acertos do
  cache) entram no resumo do run (C02) e viram "rajadas" na linha do tempo, com o
  início gravado na hora (um crash do driver durante a compilação fica registrado);
  tempo até os primeiros quadros guest no resumo. Lote 4: underruns de áudio (blocos
  ocultados porque o emulador atrasou, mais xruns do stream AAudio) no resumo e como
  rajadas na linha do tempo; erro fatal do core (ex.: GPU device lost, que aborta sem
  UI) gravado em `<run>.fatal` antes do abort e usado como causa do run na
  reconciliação. Lote 12n: uma falha nativa que nenhum tratador do core resolve deixa no
  `.fatal` a linha "native crash: SIGSEGV (…) at …, thread '…', pc libe.so+0x…", gravada de
  dentro do tratador (async-signal-safe, sem sobrescrever o erro fatal do core) e usada como
  motivo do run. Lote 14e (Impl. + Local; aparelho pendente, roteiro 46): na reconciliação de um
  run morto por crash nativo (API 31+), o tombstone em protobuf do `ApplicationExitInfo` é lido
  (até 8 MiB, leitor de wire format próprio, sem dependência) e o run guarda a thread que caiu, o
  sinal, a causa do Android, a mensagem de abort e até 32 frames (`#00 pc …  libe.so (função+off)
  (BuildId: …)`, só o nome do arquivo); aparece na ficha ("Native crash" + frame do topo) e no
  relatório (texto livre pelo redator). Falta: um anel de eventos nativos para despejar junto
  (hoje só o anel do host) e a validação no aparelho (roteiros 42 e 46).
- **C06 v1 (lote 4):** "Share last run report" na ficha do jogo — prévia do conteúdo,
  ZIP com JSON + linha do tempo, redação de caminhos/contas/endereços, caminho do jogo
  reduzido ao formato, sem logs; sai só pela folha de compartilhamento escolhida pelo
  usuário. Não há servidor nem envio automático (projeto separado, como previsto).
  **v1.1 (lote 14c, Impl. + Local; aparelho pendente, roteiro 44):** os ajustes fora do
  padrão do core com que o run começou (config global + do jogo + quirks, uma linha por
  ajuste com o padrão ao lado e "· this game" quando vêm do jogo; sem perfis, pastas nem
  caminhos) vão no início do log da sessão, no registro do run e no relatório (prévia +
  JSON, cada linha pelo redator). Retrato tirado uma vez no boot, não ao vivo, porque
  alguns cvars de texto mudam em tempo de execução em outras threads.
- **U09 (parte, lote 3):** rótulo de estado sobre a tela preta até o primeiro quadro
  guest (iniciando / preparando gráficos com N pipelines criados / aguardando o
  primeiro quadro, com segundos), a partir dos contadores do core. Lote 12e: "Cancel" no
  aviso de carregamento e saída conforme a origem (`core/GameExit.kt`: biblioteca → biblioteca;
  atalho/frontend → de volta ao frontend, sem tarefa vazia). Falta: validação no aparelho
  (roteiro 39).
- **C03 v1** (`compatibility/CompatibilityStore.kt` + ficha do jogo): resultados
  escolhidos pelo usuário (Doesn't boot … Playable) por Title ID, com build, GPU, chave
  do driver do último run e edição (Media ID)/disco; nunca inferidos. A ficha mostra separadamente a evidência automática
  do último run (como terminou, FPS de 1 s, driver).
- **DriverIdentity** (S4, parte): o presenter registra vendor/device, versão do driver,
  API, driverID/nome/info e `pipelineCacheUUID` do driver realmente carregado
  (`active_driver_identity` via JNI, `driver/DriverIdentity.kt`); gravado no run.
  `VulkanInstance` guarda a biblioteca que o adrenotools carregou de fato; a identidade
  leva `loader=system|custom` e, quando a biblioteca vem de um pacote instalado, o
  SHA-256 do pacote (diretório content-addressed) — só o nome do arquivo e o hash são
  guardados, não o caminho. A chave separa o mesmo build de driver vindo de pacotes
  diferentes; a do driver do sistema não mudou.
- Falta: um anel de eventos nativos no flight recorder (a linha do crash nativo veio no
  lote 12n). C06 v1 e o cliente do C04 já existem (estados abaixo).
- **Estado C07 (2026-10-02, v1, Impl. + Local):** "Compare runs" (`sessions/Benchmark.kt`,
  `ui/benchmark/`): runs marcados A/B, checagem de ordem ABBA, duração mínima, driver, FG e
  temperatura inicial antes de qualquer número, diferenças por par com veredito só quando todos
  concordam; marcador de cena no menu em jogo. Lote 12a: o run grava limite de FPS, taxa da
  tela e cap de VBlank; cada lado precisa de um só setup e A/B mudam no máximo uma dimensão
  (corrige o aviso de driver que impedia comparar drivers). Falta: uso no aparelho (roteiro 34).
- **Estado C05 (2026-10-02, Parcial: mecanismo Impl. + Local):** perfis de ajustes por jogo
  (`compatibility/SettingsProfiles.kt`, `SettingsProfileStore.kt`, cartão em Ajustes do jogo):
  lista permitida de chaves e valores, pré-requisitos de GPU/driver/fabricante/modelo/API/build com
  o motivo quando não se aplicam, motivo e testes de verificação, prévia linha a linha, ajuste do
  jogador sempre mantido, gravação sob trava recusada se o arquivo mudou desde a prévia e "Restore
  previous" que só desfaz o que ainda tem o valor do perfil. Fontes locais apenas: o asset do app
  (exige o teste que verificou; **vazio** — nenhum perfil foi verificado ainda) e arquivos de
  fornecedor/jogador em `settings-profiles/`, marcados como não revisados; nada de feed. Falta: o
  primeiro perfil verificado de verdade (exige jogo + aparelho + C07) e uso no aparelho (roteiro 35).
- **Estado C04 (2026-10-02, Parcial: cliente Impl. + Local, desligado):** catálogo remoto só
  leitura (`compatibility/CompatCatalog.kt`, `CompatCatalogStore.kt`, `CatalogHttp.kt`): envelope
  assinado (ECDSA P-256, chaves fixadas no build, rotação por `keyId`), conteúdo versionado e lido
  estritamente, origem igual à URL configurada, sequência sem rollback, validade/TTL (7 dias após o
  download, "out of date" por 90 dias, depois oculto), cópia offline reverificada a cada leitura,
  download só por pedido do jogador, resultados agrupados por build + GPU + driver sem mistura. Sem
  publicador e sem chave, o build sai com o catálogo desligado (nada aparece). Falta: decidir quem
  publica (servidor/repositório e guarda da chave privada, projeto separado — não inventar endpoint)
  e o uso no aparelho (roteiro 36).

Módulos novos sugeridos `compatibility/`, `sessions/`, `benchmark/`. Reusar coletor
de HUD e evitar lançar outro poller por card. Extrair percentis de buffer limitado
de deltas guest; registrar unavailable/stale e aquecimento. Referências [X2–X4],
[B4], [D4], [E3/E4].

### 7.2 Biblioteca, onboarding e storage

| ID | Recurso | Implementação proposta |
|---|---|---|
| L01 | Assistente inicial | Checagem Vulkan/armazenamento, idioma pt-BR, perfil, driver opcional e modo Jogador; pode concluir offline sem serviço de terceiros |
| L02 | Player / Developer | Jogador mostra Play, perfis e recomendações; Developer revela flags/FG/telemetria. Modo de UI não remove os gates de uma build release |
| L03 | Multi-root + SAF | Repositório de fontes persistentes, permissões explícitas e incremental; suportar URI sem converter cegamente em path. Native mmap/seek exige bridge/provider capaz ou staging controlado |
| L04 | Continue Playing / Recents / tempo de jogo | Baseado em SessionRunId, atualiza normal/crash/interrupted sem duplicar; posição/foco restaurados por identidade, não index |
| L05 | Covers/metadados | Title ID estável, fontes opcionais com chaves no Keystore, cache permanente separado do URI; scan local primeiro, requests bounded/backoff e offline |
| L06 | Ficha e coleções | Título/edição/discos, perfil, versão/TU/DLC/patch, último resultado, saves, logs, atalhos, pins; game files ausentes ficam indicados/removidos da lista, metadados históricos preservados |
| L07 | DocumentsProvider | Acesso user-controlled a saves/config/perfis/logs e componentes documentados; path/URI canonicalização, sem exportar DLL/caches privados ou raiz arbitrária; mutações respeitam lease |
| L08 | Data bundle portável | Export/import de config, layouts, bookmarks/metadados e perfis com schema/migração/dry-run/diff; não incluir ROMs/driver DLL/saves por acidente. Saves usam seu manifesto específico |
| L09 | Smart rescans e escala | Scanner com cancel/depth/cota/progress, fast cached list, extrair só entries novas/modificadas; testar 1k/10k entradas e remoção/movimento de origem |
| L10 | Conteúdo e patches | Disc install-only, múltiplos discos, TU/DLC e catálogos patch atômicos; compatibilidade por Media ID/revisão, preview e rollback |
| L11 | Mods Xbox opt-in | Somente formatos suportados por Xenia/patch/override de arquivos; namespace separado, ordem e conflitos explícitos, não substituir um save/DLC; não portar mods/Workshop Steam |
| L12 | Lixeira de dados gerenciados | Mover somente dados internos elegíveis para namespace isolado sob lease/journal; restauração com preview de conflitos, cota e limpeza explícita; remover referência da biblioteca não exclui ROM externa |

**Estado L11 (2026-10-02, v1, Impl. + Local):** o core só suporta patches `.patch.toml` (não há
override de arquivos), então "mods" = patches do próprio usuário: importados com
`patches/PatchFileCheck.kt` (recusa tudo o que derrubaria o PatchDB, com a linha), em espaço de
nomes próprio, desligados por padrão, removíveis, e conflitos de memória entre patches ligados
mostrados na tela (a ordem do core não é definida). Falta: validação no aparelho (roteiro 26).

**Estado L10 (2026-10-02, v1, Impl. + Local):** catálogo de patches com gravação atômica
e cópias que acompanham o catálogo de cada versão do app (`patches/PatchCatalog.kt`,
`PatchStore.kt`): escolhas casadas por nome, atualização sozinha quando só os interruptores
mudaram, prévia/confirmação quando o arquivo foi editado à mão, desfazer pela cópia anterior.
Lote 12q: cada arquivo diz se é para a versão jogada por último (hashes dos módulos que o
patcher comparou, via JNI, gravados no run). Falta: validação no aparelho (roteiro 25).

**Estado L07 (2026-10-02, v1, Impl. + Local):** o provider que já existia foi endurecido e
movido para `:app` (`DocumentsProvider.kt` + `userdata/UserDataFiles.kt`): só a raiz de dados
do usuário e o que está dentro dela (links e `..` resolvidos), caches do emulador e controles
internos fora, nomes limpos sem substituir, mudanças e escritas sob o lease (achados STO-01 e
STO-02 da auditoria). A DLL do LSFG e o cache dele ficam no armazenamento interno, fora da
raiz. Lote 12r: "Jogos por título" (saves por perfil, DLC/atualizações, config e patches de
cada jogo, em pastas de reunião somente leitura cujas entradas são os arquivos reais) e tudo
somente leitura enquanto o lease está tomado (um jogo roda). Não há cópia instantânea: ler
um save enquanto o jogo grava ainda pode pegar o meio da escrita. Falta: validação no
aparelho (roteiro 24).

**Estado L12 (2026-10-02, v1, Impl. + Local):** lixeira para DLC/TU removidos
(`saves/ContentTrash.kt`), com lease, journal por nome de pasta (`.partial`/`.restoring`) e
recuperação, restauração com prévia de conflito, cota de 4 GiB e limpeza só explícita; perfis
já tinham a sua (A09). Fora por desenho: jogos das pastas da biblioteca (o app não apaga ROM
externa) e caches regeneráveis (shader/LSFG). Falta: validação no aparelho (roteiro 23).

**Estado L09 (2026-10-02, v1, Impl. + Local):** `data/LibraryWalker.kt` com cota (100.000
entradas → lista parcial avisada), profundidade, cancelamento ("Stop") e progresso; lista da
última varredura na abertura; extração só de entradas novas/modificadas, agora também
reaproveitada quando o arquivo só mudou de pasta. Teste de 10.000 arquivos (e o de mover/apagar)
na JVM; o de 1k/10k no aparelho (FUSE é bem mais lento que o disco do contêiner) fica no
roteiro 22.

**Estado L06 (2026-10-02, v1, Impl. + Local):** jogos que saíram da biblioteca ficam
indicados com o motivo (arquivo não encontrado, pasta indisponível, fora das pastas), a partir
de um registro de títulos (`data/TitleRegistry.kt`) e do caminho da última sessão; "Remove"
tira da lista sem apagar tempo de jogo, relatos, saves ou capa, e o jogo volta se o arquivo
reaparecer. Coleções do usuário (`data/GameCollections.kt`) na ficha e como filtro, também no
pacote do L08; a ficha mostra TU instalada, número de DLC e patches ligados, ao lado de título,
discos, Title/Media ID, último resultado, linha do tempo, saves, diagnóstico, atalho e
favorito. Lote 12p: o perfil usado por jogo ("Jogado por último como …", gravado no run e
fora do relatório compartilhado). Lote 14h: "Limpar cache de shaders" na ficha (tamanho e
número de arquivos; só `shaders/shareable/<ID>.*` e `shaders/local/<ID>.*` sob a raiz de cache do
core, respeitando `Storage|cache_root`; nunca as partições cache0/cache1 do jogo nem links;
recusado com o jogo aberto, pelo mesmo lease do armazenamento). Falta: validação no aparelho
(roteiros 20, 21 e 49).

**Estado L05 (2026-10-02, v1 local, Impl. + Local):** capa por Title ID em `files/covers`
(`data/CoverStore.kt`), fora do cache e do URI: cópia do ícone do próprio jogo feita na
varredura e capa escolhida pelo usuário na ficha (leitura limitada, tamanho conferido antes
de decodificar, EXIF, ≤ 512 px, PNG atômico), que vence até "Use the game's own icon"; serve
a todos os discos e sobrevive a mover/renomear o arquivo e à limpeza de cache. Falta: fonte
remota opcional (chave no Keystore, limites/backoff, offline) — sem provedor escolhido, nada
é baixado; validação no aparelho (roteiro 19).

**Lote 15a (Impl. + Local; aparelho pendente, roteiro 52):** biblioteca em carrossel/XMB para
controle e sofá (Bannerlator `16c292d2`/`223b1b8c`, Eden `61ffb309`): modo salvo ao lado da
grade, arte do jogo em foco ao fundo, capa e tempo de jogo no alto, barra de capas com a da vez à
esquerda e a coluna de ações (Jogar, Detalhes, Favoritar) embaixo dela; controle e toque pelo
modelo puro `CarouselNav`. Mesmos filtros, busca e ordem da grade.

**Lote 15b (Impl. + Local; desligado por padrão; aparelho pendente, roteiro 53):** ajustes
compartilhados pela comunidade (Bannerlator `09a319ff`/`8b0eea95`), sem mudar a regra do C05 de que
nada chega sozinho: só num build que nomeia um servidor HTTPS e só quando o jogador busca, vota,
compartilha ou apaga. Cada ajuste vira um perfil C05 de origem `COMMUNITY` (não revisado) e passa
pela mesma lista permitida, prévia, preservação do ajuste do jogador e "Restaurar anterior";
ranking pelo aparelho mais parecido, depois votos e data. Envio com consentimento e lista exata
do que sai; voto sem identificador ligável entre ajustes; token de exclusão só no aparelho.
Servidor de referência (Python + SQLite) com testes próprios, contrato verificado contra o app e
um teste ponta a ponta cliente↔servidor. Protocolo e privacidade: `docs/community-configs.md`.
Não há servidor público: oferecer o recurso exige alguém hospedar e moderar um.

**Lote 15c (Impl. + Local; aparelho pendente, roteiro 54):** tela dividida (Bannerlator
`440d3dc4`): o jogo acima da dobra de um dobrável em postura de mesa (Jetpack WindowManager) ou, se o
jogador escolher "Sempre", no meio de uma tela em retrato/quase quadrada; os controles de toque fora da
imagem (abaixo da dobra como num telefone deitado, ou na tela toda em retrato). Geometria pura em
`gamepad/SplitScreen.kt`; a superfície é redimensionada e o presenter encaixa a imagem como numa
rotação. Desligada por padrão.

**Lote 15d (Impl. + Local, alvo exato; aparelho pendente, roteiro 55):** geração de quadros LSFG
por FPS-alvo (Eden `df05d3de`): 60/90/120 ou a taxa da tela, atendido com o menor multiplicador
inteiro cujo limite alvo÷multiplicador o limite do jogador permite (`core/FrameGenerationTarget.kt`),
com o limite automático já existente. O modo adaptativo do pacer portado do Eden (saídas por quadro
variando, crédito fracionário) segue sem ligar: a thread de FG do XenDroid agenda um número fixo de
saídas por quadro real e o caminho do quadro real não alimenta o motor quando não há quadro gerado —
mudança de apresentação que só o aparelho valida. FG continua experimental, desligado e atrás dos
mesmos gates.

**Lote 15e (Impl. + Local; aparelho pendente, roteiro 56):** tela de carregamento com a arte do jogo
(Bannerlator `e2acda52`) no lugar do rótulo de boot sobre preto — capa, nome, etapa com barra por fase e
aviso de primeira vez lenta — e fim de sessão com "Tentar de novo"/"Compartilhar logs"/"Voltar"
(DroidDeck `02f356f`), o relançamento feito pelo processo principal num `:emu` novo.

**Lote 15f (Impl. + Local, desligado por padrão; aparelho pendente, roteiro 57):** controles de toque
deslizáveis (Bannerlator `72db7f4d`: botão solta ao sair e o próximo aperta; direcional só passa adiante;
analógico por deslize opcional) e zona morta por elemento (Bannerlator `a20fca5a`: centro neutro do
direcional, zona morta própria do analógico), regras puras em `gamepad/TouchSlide.kt`.

**Lote 15g (Impl. + Local; aparelho pendente, roteiro 58):** HUD com memória da GPU pelo KGSL
(DroidDeck `0cc3fe6`, total de todos os apps, assim rotulado) e lugar/tamanho/aparência (caixa, contorno,
texto) guardados por jogo, com o último lugar como padrão (Bannerlator, HUD "Fusion").

**Lote 15h (Impl. + Local; aparelho pendente, roteiro 59):** margem de TV para aparelhos que cortam as
bordas (moldura com 0–10% de cada lado só na saída externa). Conferidos e já cobertos por desenho: o
fallback de vsync (Imediato → Mailbox → FIFO relaxado → FIFO, cada um só se disponível; Eden `abb616c3`) e o
FG através de um redimensionamento da superfície (imagens no tamanho da saída do jogo; DroidDeck `565987e`).

**Lote 15i (Impl. + Local; aparelho pendente, roteiro 60):** flags do Turnip (TU_DEBUG) escolhidas uma a uma
com explicação, preservando as desconhecidas, e filtro de log por subsistema (`log_mask` do Xenia).

**Lote 15j (Impl. + Local; aparelho pendente, roteiro 61):** buffer de áudio AAudio adaptativo com
histerese (cresce no underrun, encolhe após 30 s calmos; política pura testada no host; desligado por
padrão) e vigia térmico consultivo ancorado nos limites do próprio aparelho (headroom térmico do
Android), que avisa e registra, sem agir — mantendo o F04/governador como consultivos.

**Lote 15k (Impl. + Local; custo no aparelho pendente, roteiro 62):** Lanczos-2 como efeito de escala do
presenter (kernel escrito a partir da definição, 4×4 texels, limitado aos 4 mais próximos para não criar
halos), testado com os shaders reais em Vulkan por software. "Formato por aparelho" conferido: já coberto
pelos modos de tela sobre a forma do painel.

**Lote 15l (Impl. + Local; aparelho pendente, roteiro 63):** escala da interface (85–130 %) e do texto
(85–150 % sobre a fonte do sistema) para a biblioteca, as Configurações e o menu em jogo, com limites
que mantêm os layouts (lado menor ≥ 320 dp; fonte × fator entre 0,7 e 2) e sem tocar nos controles de
toque, no HUD e no editor; idioma escolhido dentro do app (aparelho / English / Português) pelo idioma
por app do Android 13+ e, antes dele, aplicado por atividade. Mais idiomas continuam dependendo de
tradução (só en e pt-BR existem).

**Lote 15m (Impl. + Local; aparelho pendente, roteiro 64):** visual de CRT como efeito de escala:
uma passada em qualquer proporção, linhas de varredura com período inteiro de pixels da tela (sem
faixas numa escala fracionária), sulco mais raso no claro e cantos mais escuros; sem curvatura nem
máscara de fósforo. Testado em Vulkan por software; aparência e custo no aparelho pendentes.

**Lote 15n (Impl. + Local; aparelho pendente, roteiro 65):** painel único de desempenho como
terceiro nível do HUD: métricas ao vivo, ritmo dos últimos segundos e da sessão a partir do
histograma de tempos de quadro do núcleo, fontes de travada (pipelines, áudio), calor e os ajustes em
vigor, incluindo os alterados do padrão. Só leitura; nada é ajustado sozinho.

**Lote 15o (Impl. + Local; aparelho pendente, roteiro 66):** controle desenhado na tela de
mapeamento (tocar no botão para mapear), destaque de sem tecla/repetido/alterado, troca automática
quando a tecla já é de outro botão e o atalho A/B-X/Y para o padrão Nintendo.

**Lote 15p (Impl. + Local; aparelho e rede pendentes, roteiro 67):** fontes de driver do usuário
(repositórios do GitHub adicionados e removidos no gerenciador, lidos um a um) e o driver sugerido
para a GPU Adreno pelos nomes dos builds (modelo, família, genérico; nunca outra família), com os
downloads conferidos como antes.

**Lote 15q (Impl. + Local; aparelho pendente, roteiro 68):** modo "Vívido (falso HDR)" no filtro
de cor (saturação +25 % e curva em S suave), testado em Vulkan por software. Presets de um toque e
shaders do usuário continuam fora.

**Lote 15r (Impl. + Local; aparelho pendente, roteiro 69):** grupos no editor de toque (ABXY,
gatilho + bumper, Back + Start, analógico + clique) que se movem e redimensionam juntos mantendo a
forma. O governador probe/backoff segue fora por desenho (F04 consultivo; só com A/B no aparelho).

**Fim do lote 15 (build da nuvem em `d272712f`, APK compilado e não testado no telefone):** os
itens maiores da varredura e os demais de média prioridade viáveis sem aparelho entraram como
Implementado + Testado localmente (15a–15r); nada foi validado no aparelho nem aprovado para release.
O que falta é o roteiro de aparelho da auditoria (itens 52–69), com o APK desta build.

**Primeira rodada do roteiro automatizado no telefone (build `d272712f`):** 43 de 58 testes verdes,
o que passa a parte automatizada de 17 itens do roteiro a "Validado no aparelho" (2, 3, 5, 9/12, 11,
13, 18, 19, 21, 25, 33, 35, 37, 38, 39, 42 e as transações de configuração). As 11 falhas eram um
defeito do app (Configurações com "Essenciais" fora da tela depois do 15l) e dez dos testes (detector
de texto cortado, menu sem rolagem, PNG incompleto, corrida no `pidof`, botão repetido, busca sem
`queryArgs`, lista preguiçosa); todos corrigidos, à espera de nova rodada.

**Estado L03 (2026-10-02, v1, Impl. + Local para caminho real):** várias pastas de jogos
(migração da pasta única, aninhadas varridas uma vez, indisponível não derruba as outras e
mantém o cache, remover não apaga nada; instalação de jogo completo na primeira). Falta SAF
de árvore: exige ponte de leitura no core (mmap/seek) ou staging controlado — bloqueio
técnico registrado, sem converter URI em caminho. Aparelho: roteiro 18.

**Estado L01 (2026-10-02, v1, Impl. + Local):** assistente inicial sobre a biblioteca
(uma vez; reabre no menu): checagem de Vulkan/arm64/Android, pasta de jogos, idioma e região
dos jogos a partir do locale do telefone (pt-BR → pt/BR), perfil, driver opcional e modo
Jogador/Desenvolvedor; offline. A interface em pt-BR veio com o U02 (lotes 10b e 12h–12m).
Falta: validação no aparelho (roteiro 17).

**Estado L02 (2026-10-02, v1, Impl. + Local):** "Interface: Player/Developer" nas
Configurações; Player mostra só "Essentials" (19 ajustes de jogador, também por jogo) e
esconde do menu em jogo FG/LSFG, ADPF, contagem de submissões, política de fundo e modo
sustentado; valores escondidos continuam valendo; nenhum gate de build muda. Padrão Developer
até o assistente (L01) perguntar. Falta: validar no aparelho (roteiro 16).

**Estado L08 (2026-10-02, v1):** "Back up or move settings" no topo de Configurações —
exporta para um arquivo escolhido pelo usuário (SAF) o config global e os por jogo, o
layout dos controles de toque, favoritos/ordenação e os relatos de compatibilidade
(`bundle/DataBundle.kt`: ZIP com `manifest.json` de formato/versão e SHA-256 por
entrada). A importação lê só entradas conhecidas, com limites, confere checksums e
versão, valida o TOML pelo parser do próprio emulador e o layout pelo seu schema antes
de qualquer escrita, mostra a prévia (substituídos/novos/mantidos, favoritos
adicionados, relatos mesclados) e salva o estado atual como backup antes de aplicar.
Caminhos do aparelho (`cache_root`, `content_root`, `storage_root`, `vulkan_lib_path`,
trace/som) nunca saem e, na importação, os valores deste aparelho são mantidos.
Fora do pacote por desenho: saves, perfis, jogos, drivers, LSFG, logs e histórico de
runs. Falta: perfis (P2–P4, com o manifesto próprio dos saves), migração entre versões
de formato quando houver a 2, e teste no aparelho do fluxo SAF.

DocumentsProvider exige `android.provider.DocumentsProvider`, permissão
`MANAGE_DOCUMENTS`, grant URI limitado e snapshots read-only enquanto guest usa
conteúdo. Não implementar como acesso irrestrito aos arquivos do app. L03/L07
devem preservar direct launch e [contrato de frontend externo](frontend-integration.md).

### 7.3 Menu, controle e interface

| ID | Trabalho | Aceite |
|---|---|---|
| U01 | Menu contextual enxuto | Gráficos: apresentação/FG/driver; Sistema: FPS/energia/HUD; Controle: layout/slots/gyro; Sessão: pausa/logs/sair. Opções avançadas em detalhes; cada aba focável e rolável independente |
| U02 | Semantics/localização | `strings.xml` pt-BR/en primeiro; labels, estados, motivo unsupported, a11y actions/focus. Leitor de tela, texto grande/RTL e 4:3/ultrawide sem cortar ação fixa |
| U03 | Contrato requested/effective | GPU/API/features/driver/kernel e apply status no registry; drivers não suportados ficam indisponíveis com causa, configuração persistida não significa ativada |
| U04 | Shared controller navigation | A/B e swap-confirm configuráveis, rolagem/repeat moderados, modais/popup nativos, touch coexistente; não roubar keyboard events nem enviar menu input ao guest |
| U05 | Controller test center | Botões/eixos/deadzone/trigger, gyro e rumble por device/slot, reconnect, input source e nomes; teste de vibração só por ação do usuário |
| U06 | Presets/layout bundles | Perfil por título/dispositivo/orientação, import/export versionado, factory placement de borda, nomes/preview/diff; preservar defaults e unknown fields |
| U07 | Floating area / relative camera | Além do stick adaptativo existente, modo nativo de áreas livres opt-in; buttons/D-pad/menu guardam prioridade, CANCEL/resize/modal/hotplug neutralizam input |
| U08 | Haptics/gyro completos | Intensidade Off/Low/Medium/High por dispositivo, capability API, calibrar sensor e gyro físico; backend não sugere adaptive triggers que XInput Xbox360 não oferece |
| U09 | Direct launch/loading/error | Resolver identidade/perfil antes do boot, estados legíveis, progress compile/cache e cancel seguro; finalizar task conforme origem |
| U10 | Prompts guest e teclado virtual | Reusar bridge XAM existente; navegação por toque/D-pad/stick, atalhos de cursor/páginas/confirmar, UTF-8 inválido e strings limitadas; foco/cancel devolvem input ao dono sem botões presos |
| U11 | Perfis locais e sign-in | Iterar `ProfilesScreen`/viewmodel, GamerTag/idioma/país/avatar, indicação do perfil ativo e erros acionáveis; escolher perfil antes do boot, respeitar saves/XUID existentes e depois integrar atribuição P1–P4 |

**Estado U11 (2026-10-02, v1, Impl. + Local):** "Play as" antes do boot com vários perfis
(`data/ProfilePick.kt`), escolha gravada no slot 0 do config, opção de não perguntar (e de voltar
a perguntar em Perfis), "Signs in as …" na ficha; perfil configurado que sumiu é corrigido em vez
de bootar sem perfil. Lote 12c: perfis de P2–P4 (`data/ProfileSlots.kt`, "Other players" em
Perfis), um perfil por jogador, slots de perfis apagados limpos. Falta: validação no aparelho
(roteiro 33).

**Estado U10 (2026-10-02, v1, Impl. + Local):** teclado do jogo digitável por controle
(`ui/keyboard/KeyboardGrid.kt` no `GuestKeyboardPanel`): grade com destaque, atalhos do 360,
limite em UTF-16 sem partir pares e saneamento de surrogates; toque segue com o IME. O message
box mostra os textos do próprio jogo; o botão de reserva vem dos recursos (lote 12o). Falta:
validação no aparelho (roteiro 32).

**Estado U07 (2026-10-02, v1, Impl. + Local):** câmera por toque opt-in
(`gamepad/TouchCamera.kt` no `GamepadOverlay`): lado direito livre vira analógico direito pela
velocidade do dedo, zerando em repouso/soltar/cancelar; controles mantêm prioridade. Lote 12o:
velocidade e área ajustáveis no editor (Gerais). Falta: a sensação no aparelho (roteiro 31).
Lote 14d (Impl. + Local; aparelho pendente, roteiro 45): os controles de toque saem quando um
controle físico **joga como P1** (primeiro botão, ou stick/gatilho/hat além da metade) e voltam
quando ele desconecta ou passa a jogar outro jogador; tocar na tela não os traz de volta (regra do
Eden `e4dccd5a`); controle de P2–P4 e dispositivo virtual não contam (Bannerlator `f76b16e1`,
DroidDeck `432e197`); mostrar pelo menu vale até o controle sair. Opção "Esconder enquanto um
controle joga como P1" no editor (Gerais), ligada por padrão; o ajuste "Mostrar controle na tela"
continua decidindo antes. `gamepad/TouchOverlayPresence.kt` + `TouchOverlayPresenceTest`.

**Estado U05 (2026-10-02, v1, Impl. + Local):** "Test controllers" na biblioteca
(`ui/controllertest/`, `gamepad/ControllerTest.kt`): botões com checklist, analógicos sobre a
zona morta com o valor que o jogo recebe, gatilhos, hat, giroscópio do controle (API 31+),
vibração só por toque, reconexão pelo descritor; eventos crus capturados só nessa tela. Lote 12o:
o jogador P1–P4 que cada controle seria num jogo aberto agora. Falta: validação no aparelho
(roteiro 30).

**Estado U04 (2026-10-02, v1, Impl. + Local):** `gamepad/MenuButtons.kt` para biblioteca,
menu em jogo e editor: A/B trocáveis (Configurações), Enter/Esc fixos, ritmo de direção
segurada (350 ms + 120 ms) também no analógico/hat; corrigido o Esc que ativava a opção no
menu em jogo. Popups nativos e teclado virtual continuam com o U10. Falta: validação no
aparelho (roteiro 29).

**Estado U02 (2026-10-02, v1, Impl. + Local):** recursos en/pt-BR com o menu em jogo
inteiro, `localeConfig` (idioma por app no Android 13+), descrição/papel do botão do menu e
"selecionado" para o leitor de tela; teste que exige as mesmas chaves e marcadores nas duas
línguas. Lote 12h: biblioteca inteira, diálogos, assistente inicial e navegador de pastas; lote
12i: Configurações (moldura, ajustes do modo Jogador, drivers, pacote de dados, atualizações, perfis
recomendados); lote 12j: Perfis, Saves e Conteúdo (telas e mensagens); lote 12k: patches, teste de
controles, comparação (moldura), companion (motivos estruturados), diagnóstico, sobre, teclas, pausa,
disco, faixa de início, compressão, avisos do host e atualizador; lote 12l: editor de toque e
layouts (recusas estruturadas); lote 12m: veredito e avisos da comparação, eventos de conexão do
teste de controles, linhas de telefones como controle e TV no menu; lote 12s: 31 textos com
contagem viram plurais (919 textos e 31 plurais por língua). Fica em
inglês, de propósito: itens só do modo Desenvolvedor (ADPF, FG/LSFG, governador, cvars), números do
HUD, bloco técnico do último run e logs. Falta: revisão do texto, RTL e texto grande no aparelho
(roteiro 28).

**Estado U01 (2026-10-02, Impl. + Local):** abas Graphics (apresentação/FG), System
(FPS/energia/HUD), Controls (layout/jogadores/gyro) e Session (pausa/som/logs/sair), com o
avançado de cada aba atrás de "More options" (abre no lugar, por aba, sem opções de dev no
Player). Lote 12b: linha do driver na aba Graphics (carregado × ajuste no início e agora).
Lote 14f (Impl. + Local; aparelho pendente, roteiro 47): o HUD completo ganha "Potência e tempo
de bateria" (watts = corrente × tensão com unidade detectada, mA × µA e mV × µV; tempo pela
carga contra a corrente suavizada, ou pela queda da porcentagem sem contador de carga; na
tomada, "full in" pelo Android); FPS e frametime publicados pelo core vão a 0 depois de 1 s sem
quadro do jogo (antes ficavam no último valor durante travamentos e loads).
Falta: validação no aparelho (roteiro 27).

**Estado 2026-10-02:** U03 (parte) — driver pedido × carregado na linha do driver,
pela identidade gravada no run, sem falso alarme para runs anteriores à troca; lote 12b/12f:
a mesma comparação no menu em jogo e, sem KGSL, o ajuste do driver mostrado indisponível com a
causa e o que roda no lugar. Falta no U03: o valor efetivo de cada ajuste (ex.: a escala de
resolução realmente usada) — o core não informa hoje; exige uma leitura por ajuste via JNI e
conferência no aparelho. U06 v1 —
layout de toque por Title ID e orientação ("This game only" no editor dentro do jogo),
com retorno ao compartilhado. U06 v2 (lote 12d) — layouts nomeados (salvar/aplicar com prévia do
que muda/apagar), arquivo `xendroid-touch-layout` versionado com importação validada, e campos e
controles desconhecidos preservados (corrige a perda deles a cada edição). U08 (parte) — intensidade
de rumble Off/Low/Medium/High aplicada ao controle de cada jogador; lote 12g: intensidade
própria por controle (pelo descritor), escolhida em Test controllers; o giroscópio do
telefone já calibra em repouso. Falta no U08: o giroscópio do próprio controle como câmera — a
convenção de eixos dos controles no Android muda com o driver (hid-playstation, hid-nintendo);
medir no aparelho com Test controllers (mostra x/y/z) antes de mapear, para não entregar um
mapeamento chutado. Lote 14h (Impl. + Local; aparelho pendente, roteiro 49): giroscópio
"segurar para mirar" (sempre / segurando LT / segurando LB, guardado; soltar centraliza o stick
uma vez, sem soltar o que um dedo ou controle segura), e entrada sem buffer (sticks de controle
e cada gesto de toque entregues na hora, não uma vez por quadro; Android 11+ para os sticks;
chave A/B no menu do modo Desenvolvedor, ligada por padrão, anotada na linha do tempo). U09 v1 — estado do boot (lote 3), Cancel no aviso de carregamento e saída
conforme a origem (lote 12e); falta só o aparelho (roteiro 39).

Referências concretas [X1/X2], [B3], [D1/D2], [E2/E5]. O visual Blades/Metro pode
inspirar hierarquia própria; não é necessário reproduzir logo/arte proprietária.

### 7.4 Updater, builds e catálogos

**R01** substituir comparação de `VERSION_NAME != latestHash` por manifest de build
e versionCode. **R02** canais Stable/Preview/Nightly/Test com variante, rollout e
cache offline; defaults estáveis, experimentos nunca atualizam um release padrão
sem escolha explícita. **R03** download bounded/hash/size, verificação de package,
versionCode, certificate lineage e PackageInstaller, cancel/falha/retorno do SO.
**R04** changelog legível com ações fixas e links de origem, skip/remind/pre-release.
**R05** CI gera identidade/signature/hash/notices e APK de teste; signing secrets
fora do git. **R06** checks de callbacks JNI/asset/licença/ABI e release otimizado.

**Estado 2026-10-01:** R01 em parte — o feed do fork (`BuildConfig.UPDATE_REPOSITORY`,
`-PxendroidUpdateRepo`) só é consultado pelo pacote regular `.fork`; `.fork.opt`,
debug e sem sufixo não oferecem atualização (antes recebiam builds do upstream
`rfandango/XenDroid`). Ordem por `versionCode` (`-PxendroidVersionCode`, run number no
CI) e tag `XenDroid-v<n>-<sha>` (`updater/ReleaseTags.kt`, testado). Sem `versionCode`
nos dois lados, nada é oferecido.

**Estado 2026-10-02 (R02–R04, R06):** canal Stable/Preview/Off em Configurações (só no
pacote com feed); Preview lê a lista de releases e aceita prereleases, drafts nunca.
Escolha pura e testada (`updater/ReleaseFeed.kt`): mais novo por `versionCode` no canal,
com APK único (`XenDroid_Release_*.apk`), acima do instalado e do "Skip this version".
Download limitado ao tamanho publicado, SHA-256 calculado em streaming contra o
`digest` do asset (sem digest: só a página da release), arquivo temporário + rename.
Antes de instalar, `UpdateInstaller.verify` confere pacote igual, `versionCode` maior e
certificado em comum com o instalado; a instalação é uma sessão `PackageInstaller` que
o sistema confirma com o usuário (permissão `REQUEST_INSTALL_PACKAGES`; sem "instalar
apps desconhecidos" liberado, abre esse ajuste). R06: `tools/check-jni.py` e
`tools/check-apk.py` no CI. Falta: R05 (chaves de release e de teste no CI, decisão do
usuário) e validar o fluxo no aparelho com o pacote `.fork`.
Não trocar signing key arbitrariamente: migração/side-by-side é decisão de release.
Reutilizar conceitos [D3/D4], [B2], [X2/X3], não chaves, package IDs ou servidores.

## 8. P2 — slots, multiplayer local e companion

### 8.1 Native HID antes da rede

**I01:** `ControllerSlotManager` com P1..P4, conexão, device ID, origem e profileXuid.
**I02:** JNI versionada que envia snapshots de estado por jogador, mantendo o
antigo `keyEvent` como adapter P1. **I03:** `AndroidInputDriver` com quatro estados,
packet number só em mudanças e sincronização producer/consumer; `GetCapabilities`,
`GetState`, `SetState` e reset por slot. **I04:** rumble e teste por motor/slot,
com hotplug liberando apenas o dispositivo perdido. **I05:** política de fontes:
hardware/touch/gyro/remote não se sobrepõem sem regra, e input do menu é isolado.

Aceite nativo/JVM: simular dois devices pressionando o mesmo botão, release de um,
desconexão do outro, hat vs keyboard vs gyro, valores limites dos sticks/triggers,
nenhum ghost input e rumble entregue ao dono certo. Antes de anunciar multiplayer,
rodar jogo split-screen com duas entradas e profiles distintos.

**Estado 2026-10-02 (I01–I04, v1):** `AndroidInputDriver` com quatro slots: P1 sempre
presente (pad de toque + primeiro controle, caminho antigo intacto), P2–P4 conectados e
desconectados pelo frontend (`set_slot_connected`, o guest vê o controle entrar/sair via
`NotifyDevicesChanged`), estado/packet/keystrokes por slot e rumble guardado por slot.
`gamepad/ControllerSlots.kt` atribui controles físicos a jogadores pela ordem do primeiro
aperto (#76) e pelo `descriptor` (quem volta recupera o slot se livre; troca de jogadores possível);
`SlotInputRouter` aplica a P2–P4 as mesmas regras do P1 (zona morta 8%, gatilho analógico
0–255 que segue o mapeamento de L2/R2 (#97), hat) com estado por slot, e a desconexão solta só o que aquele controle segurava.
Rumble: menu "Controller rumble" Off/Low/Medium/High; o motor mais forte, escalado, vai ao
vibrador do controle do slot (nunca ao telefone), só com o jogo rodando. Testes JVM de
atribuição, roteamento, release e amplitude; `check-jni` cobre as 3 JNIs novas. Falta: UI
para trocar jogadores (a API existe), perfil por jogador (P2–P4 sem login), e o jogo
split-screen com dois controles no aparelho (gate de anúncio).

### 8.2 Companion LAN e Bluetooth experimental

**I06:** app/control surface companion com modelo de layout exportável, build
separado e protocolVersion. **I07:** pareamento com confirmação/código efêmero,
host descoberto explicitamente, autorização por slot e P1 remoto só opt-in;
bind em LAN escolhida, sem endpoint público/Internet por padrão. **I08:** frames
com sequência, estado completo, ack/heartbeat e timeout que solta input; reconnect
não reemite botões antigos. **I09:** feedback/rumble por slot, latência medida no
host/cliente, layout/perfil compatíveis. **I10:** Bluetooth apenas após LAN passar,
com limitations de hardware/API/permissões documentadas.

Referência [X1] é funcional, não código público. Criar protocolo próprio testado
em dois clientes de teste antes de UI bonita. Companion não é Xbox Live, System
Link ou streaming de vídeo. Networking guest/Xbox Live é uma iniciativa distinta.

**Estado 2026-10-02 (I06–I09, em andamento; protocolo, host e cliente Impl. + Local — os
testes abaixo compilaram e passaram na sessão na nuvem):** protocolo v1 próprio
em `app/src/main/java/xendroid/compose/companion/`: TCP com frames `u16 tamanho | u8 tipo
| payload` (≤ 512 bytes); o host manda CHALLENGE com nonce novo, o cliente responde HELLO
com prova HMAC-SHA256 do código de 6 dígitos (o código nunca trafega), WELCOME/REJECT;
STATE é o estado completo do pad (layout XInput) com sequência u32 (estado antigo é
descartado); PING/PONG a cada 250 ms medem latência e 1 s sem dados solta o input e
libera o slot; reconexão do mesmo cliente substitui a anterior começando solta; RUMBLE
por slot; BYE. O host (`CompanionHost`) escuta só no endereço de LAN que recebe, dá
apenas P2–P4 (`ControllerSlots.connectRemote`, nunca P1), limita 4 handshakes
simultâneos e trava o pareamento após 10 códigos errados (novo código ao desligar e
ligar). O cliente (`CompanionClient`) envia de uma thread própria (o Android proíbe
socket na main thread) e `PadKeys.apply` converte os eventos do pad de toque em estado
completo. Testes JVM escritos: codec/prova/diff/apply (`CompanionProtocolTest`) e host
com clientes reais por loopback (`CompanionHostTest`: dois telefones em P2/P3, código
errado, trava, slots cheios, saída, timeout + sequência, reconexão, rumble, latência,
desligar, limite de handshakes) — 9 + 11 casos passando. **Host integrado (fatia 7b, Impl.
+ Local):** "Phone controllers" na página Controls do menu em jogo (off a cada boot, só com
o jogo rodando), endereço escolhido por `CompanionNetwork` (IPv4 privado de Wi-Fi/hotspot/
Ethernet/tethering; nunca dados móveis, VPN, CGNAT ou 0.0.0.0), liga/desliga ordenado fora
da main thread (`CompanionHostControl`), IP:porta, código, jogadores e latência no menu,
input dos telefones segurado com menu/pausa/fundo, rumble cru do slot a cada telefone só
quando muda (`CompanionRumbleForwarder`). **Cliente (fatia 7c, Impl. + Local):** Biblioteca → ⋮
→ "Use this phone as a controller" (`ui/companion/`): IP:porta + código + nome, conexão fora
da main thread (`CompanionPadLink`, testado com host real), pad de toque em paisagem com o
layout do usuário, latência medida pelo host (vai no PING), vibração com a intensidade do
próprio telefone, `clientId` estável e mensagens de recusa/queda. Falta: o gate em hardware
(dois telefones na mesma rede, jogo split-screen) — roteiro 14 da auditoria.

## 9. P0/P2 — FG, apresentação e energia

### 9.1 Cadência e qualidade antes de novos presets

**F01 — ownership e ordem:** definir timeline de capturar real, gerar/pintar
intermediários, apresentar real e aceitar próxima fonte. Snapshot por frame deve
ter id/timestamp/imagem/extent; não inferir que uma notificação corresponde ao
buffer já consumido do mailbox. Testar replace/drop/pause/resize/engine switch.

**F02 — fila limitada:** deadlines absolutos, atraso reseta/rebase, capacidade
explícita e pressão da fila. Nenhum burst para “recuperar” frames perdidos. Separar
requested multiplier, allowed generations e synthetic realmente submetido.

**F03 — menos waits:** medir `AwaitAllSubmissions` e device-idle do engine; migrar
scratch UBO/descriptor/images para slots fence/timeline onde válido. Nunca retirar
wait sem prova de vida útil; esse é um projeto de sincronização, não um toggle.

**F04 — target-rate e governor:** LSFG 60/90/120/Hz efetivo com credit fracionário,
warm history mesmo em geração zero, limite `target <= refresh` com tolerância
medida, probes de GPU/custo/FPS guest e holdoff/histerese. Recua/desliga em déficit,
fila crescente ou thermal significativo; devolve cap temporário sem desfazer
alteração manual do usuário. Inspiração [D5], [E3/E4], implementação Xenia própria.

**Estado F04 (2026-10-02, fatia 8d, Impl. + Local, consultivo):** `FrameGenerationGovernor`
julga por segundo orçamento de GPU (passada > 80% do intervalo entre saídas), cadência ×
multiplicador > Hz, slots atrasados (> 1 em 5) e térmico SEVERE, com histerese de 3 s; mostra
o veredito no menu e o grava na linha do tempo, **sem agir**. Falta: calibrar os limiares no
aparelho (roteiro 15) e só então deixá-lo recuar/desligar, além de target-rate e credit
fracionário do LSFG. Lote 14i (Impl. + Local; aparelho pendente, roteiro 50): na aba Gráficos
(Desenvolvedor, build debug), antes de ligar, o que a FG faria aqui ("45 FPS × 2 = 90/s acima de
60 Hz: ligar limita o jogo a 30 FPS, base 30 → 60 quadros/s enviados"; a regra é a do
`GenerationCap`), para Win-FG 2× e, com cache, LSFG N×; ligada, "base → enviados (gerados)" por
segundo, medidos com o menu fechado, e o aviso quando FPS × multiplicador passa do Hz. "Enviados"
não é prova de exibição, e o texto diz isso. A FG continua experimental, desligada e atrás dos
mesmos gates.

**F05 — refresh/surface dinâmicos:** observar mudança real de modo externo/telefone,
desanexar/reconectar sem reboot, renegociar FIFO/flags, revalidar cache/engine.
“Requested Hz” e “effective Hz” distintos; não copiar power pin de outro app.

**F06 — LSFG import:** cache imutável por hash + ABI de tradutor + variant/target
SPIR-V, falha deixa cache anterior, select/clear serialized, source/context status
com causa exata. Nenhum Steam login necessário para DLL manual no XenDroid; regra
de propriedade Steam do DroidDeck é específica de seu cliente Steam.

**F07 — flow/FP16:** seleção Auto/qualidade/resolução com probe real, FP32 fallback,
capabilities e benchmark por resolução. SPIR-V válido não prova suporte de todas
as features requeridas pelo device atual. Não importar LSFG-VK v2 ND; observar a
[pesquisa/licenciamento](lsfg-vk-referencia.md).

**F08 — observabilidade:** GPU timestamps com validBits/wrap, falhas/N/A, sample
guest e submitted/synthetic, input-to-frame markers, p95/p99, dropped/pending.
Usar `VK_KHR_present_id/present_wait`, `VK_GOOGLE_display_timing` ou estatística
Android somente quando suportada; câmera/timing externo quando não houver fonte.

**Estado F08 (2026-10-02, fatia 8a, Impl. + Local):** tempo de GPU da geração com
`timestampValidBits` e volta do contador (`TimestampElapsedNs`, testado), falha marcada
como indisponível em vez de valor velho, histograma por passada (0,25 ms) exportado por JNI
e resumido por run (mediana/p95/p99, não cronometradas, saídas atrasadas puladas, quadros
guest substituídos) na ficha e no relatório. Falta: present_id/present_wait ou
`VK_GOOGLE_display_timing` quando o driver tiver (só no aparelho), marcadores
input-to-frame e a fila pendente (F02).

**F09 — qualidade:** checkerboard/câmera/oclusão/HUD/transição/frames duplicados,
scene-cut fallback e movimento determinístico. Imagem constante é só teste básico
de compute. Aplicar FG antes de HUD Android e preservar layers de diálogos.

**Estado F09 (2026-10-02, fatia 8b, Local em Vulkan por software):** teste de movimento
determinístico (`TEST_MOTION`: quadrado liso, textura fina, textura em várias escalas;
vários pares; erro contra o meio ideal e os quadros-fonte) com guardas no
`tools/test-presentation-host.sh`. Win-FG (modelo 3) ganha de repetir um quadro-fonte até
16 px/quadro em 256 px com textura multiescala; não interpola objetos lisos pequenos; com só
detalhe fino perde para "repetir" acima de ~4 px. Medições em
`performance-tests/experiencia-em-jogo-2026-10-02.md`. Falta: oclusão, HUD, corte de cena,
checkerboard e o A/B no aparelho.

**F10 — liberação:** debug-only e off-by-default continuam até Android/integration
e aparelho passarem. Nenhuma versão atual foi comprovada superior no mesmo jogo/
aparelho. 3×/4× mostram capacidade de multiplicar, não ganho da simulação.

### 9.2 Extensões existentes a iterar

- **P01 ADPF:** medir thread correta/target/workduration. Atualmente presenter; avaliar
  command processor separadamente. `auto|off|on`, foreground, API guards, failure
  latch/backoff e thermal low-frequency. Sem sessões apontadas a TIDs de outro processo.
  Lote 14g (Impl. + Local; aparelho pendente, roteiro 48): o app se declara jogo
  (`appCategory="game"` + `game_mode_config`: modos Desempenho/Bateria do aparelho permitidos,
  redução de resolução e limite de FPS do sistema recusados) e diz ao `GameManager` (Android
  13+) carregando → jogando → pausado sob o menu → nada no fundo, para o "game booster" do
  fabricante agir só no jogo; o modo escolhido nas configurações de jogos do sistema vai para a
  linha do tempo do run; janela e tela externa pedem o modo de baixa latência da TV (ALLM). O
  modo sustentado continua opcional e desligado (lição do DroidDeck: ele limita clocks).
- **P02 display externo:** lifecycle state machine e single-surface owner, unplug,
  invalidação de modo/driver/cadência, foco e áudio Android; controle no telefone.
- **P03 áudio:** volume/mute existentes; métricas e buffering adaptativo com histerese,
  route/device change, XMA cross-buffer/reuse e clock drift. DirectAudio/Wine não é backend Xenia.
- **P04 color/postprocess:** lote 14j (Impl. + Local; aparelho pendente, roteiro 51): SGSR 1
  (Qualcomm, BSD-3-Clause, aviso no APK) como efeito de escala do presenter ("sgsr" no ajuste de
  escala e no menu em jogo; uma passada até o tamanho de saída, variante com dither), testado com
  os shaders reais em Vulkan por software contra o bilinear; o custo no Adreno ainda não foi
  medido (A/B do C07 contra FSR). Debanding = o dither do Xenia (ruído azul antes dos 8 bits), que
  vale desde o 14b e agora está no modo Jogador. SDR modes atuais com espaço de cor definido, pure frame
  layer separado de overlay, A/B de custo; HDR opt-in exige cadeia/surface/painel
  compatíveis e teste de cor/transferência, não só flag HDR na UI.
- **P05 economia/background:** manual/auto/never com invariantes de surface/audio/input,
  session lease, retomada exata e opção limitada em hardware que não mantém renderer.
- **P06 gyro:** calibragem/ativador/deadzone/sensibilidade e orientação do display,
  físicas vs virtuais com prioridade; stop solta só a origem gyro, não outro jogador.

## 10. P3 — core e performance específicos

| ID | Pesquisa/experimento | Regra para avançar |
|---|---|---|
| K01 | Dynamic state/static fallback por driver | Probe mínimo topology/cull/frontface + teste jogo exato; pipeline keys incluem subset, não habilitar tudo porque a extensão existe |
| K02 | Cache descriptors/samplers/textures | Estados completos, generations/layouts/ownership e invalidation; counters antes/depois, fallback obrigatório para recurso alterado |
| K03 | Direct recording vs deferred | Audit de comandos que têm metadata/replay pendente; ordering e timestamps preservados; medir CPU só se GPU não estiver saturada |
| K04 | Upload pools / memory pressure | Manter warm pages até trim real e GPU completion; budget por device; não destruir pool só no Home; Android trim levels testados |
| K05 | EDRAM resolve/readback e GMEM | Preservar UMA existente e formatos; testar Alan Wake/Forza/GTA e padrões sintéticos próprios. Não aplicar um perfil X360 Mobile a outro implementation do resolve |
| K06 | Ponto/quad/geometry path | Point sprites e quad conversion gates por título; diagonais e precisão conferidas; alternativa não vira default sem resultado |
| K07 | ARM64 memory/fairness/handoff | Provas de atomics/guest address/endian/concurrency, fairness de waits e XEX secundário. Switch NCE/TLS e DXVK/proot não são patches Xbox |
| K08 | Audio decode / scheduling | Underrun tests e XMA across-buffers; latência mínima saudável e recuperação limitada; não alterar precisão para evitar crash |
| K09 | PGO/ThinLTO | ThinLTO suportado e corpus representativo documentado; no global loop-unroll, PGO só de menu/single demo ou package spoof como benchmark |
| K10 | KGSL polling / clock policy | Demonstrar chamada/problema local por trace antes de interceptar ioctl; opt-in hardware específico, reset no stop/crash e A/B térmico. Demo PC 614→4335 não prevê Xbox FPS |
| K11 | VBlank guest por deadlines absolutos | Referência [X3]: clock guest distinto de deadlines do presenter/FG; rebase após stall sem burst, pausa/retomada/timers/áudio consistentes e cadência Xbox preservada; testes com relógio controlável antes de A/B |

Primeiro benchmark real é controle negativo GPU-bound em Forza/POCO F7. Se a GPU
já opera em ~96–98%, economizar CPU ou gerar mais compute pode não melhorar FPS
guest. Especificar workloads CPU-bound e GPU-bound separados e uma matriz de
portabilidade (driver de sistema/Turnip customizado, Adreno6xx/7xx/8xx, outros
somente se suportados). K11 trata pacing do guest; F02 trata a fila de apresentação.

**Estado K11 (2026-10-02, Parcial: Impl. + Local):** o laço de VBlank do Android já usava prazos
absolutos; a decisão saiu para `xenia/gpu/vblank_pacer.h` (relógio do guest, separado do
presenter/FG) com testes de relógio controlável em `tools/test-native-logic.sh`: cadência exata
(resto carregado: 60 VBlanks por segundo do guest), atraso sem deslocar a fase, travada > 1
intervalo = um VBlank e recomeço (sem rajada), troca de taxa a partir do último prazo, relógio para
trás e taxa zero sem travar; linha de log a cada recomeço em potência de 2. Pausa: `MarkVblank` já
não entrega VBlank pausado e o recomeço evita rajada na volta. Falta: A/B no aparelho (roteiro 37)
e ligar timers/áudio do guest à mesma referência, se o A/B mostrar desvio.

## 11. O que não entra como funcionalidade Xbox

- Runtime Proton/Wine/Steam/FEX/Box64, proot, Flatpak, gamescope e desktop LXQt:
  arquiteturas dos outros produtos, não requisitos do emulador nativo.
- Download/login de lojas PC, Steam Friends/Workshop/Cloud como equivalência de Xbox
  Live ou saves Xbox: contratos/formatos diferentes. Reusar UX de organização.
- Cache binário/shaders proprietários de outro usuário ou driver; LSFG-VK v2 ND sem
  autorização; source/code de X360 Mobile/GameHub que não foi publicado.
- Package spoof para supostos FPS, overclock/root ou correções de readback sem
  diagnóstico: não são padrão de produto e ficam fora do backlog principal.
- Save states: não apresentados aqui como função portável dos cinco projetos;
  serializar Xenia/JIT/GPU é iniciativa própria e muito maior que backup de saves.

## 12. Fases e dependências executáveis

| Fase | Entregas | Dependências | Gate |
|---|---|---|---|
| **S0 — baseline/riscos** | A01–A09, BuildIdentity, contrato de sessão | Checkout atual | JVM/native/lint/package; crash/recovery/interleavings; issues reais documentadas |
| **S1 — Session/Compatibility** | C01–C03, L04, U09 | S0 | Sessão concluída uma vez, crash/interrupted preservados, recents/resultados coerentes |
| **S2 — Storage/Portable** | L03, L07, L08, L12, sync/recovery, U06 | S0, identidade de jogo | SAF/real-path e import/export com diff/cotas/migração, lixeira recuperável, dados intactos em falha |
| **S3 — Product/Registry** | L01/L02/L05/L06/L09, U01–U04/U10/U11, registry completo | S1/S2 | Offline, pt-BR/en, foco/a11y/prompts/perfis, 1k/10k entries, requested/effective verdadeiro |
| **S4 — Drivers/Updates** | cache A06, R01–R06, catálogo por GPU | S0 e BuildIdentity | Troca de driver/cache reversível; updater versão/hash/pacote/assinatura exatos |
| **S5 — Input slots** | I01–I05, U05/U07/U08 | S0 e contratos de input | 4 slots nativos, release por device, rumble/profile correto e sem vazamento de menu |
| **S6 — Companion LAN** | I06–I09 | S5 | 2 clients de teste, timeout/reconnect/seq; jogo split-screen em hardware |
| **S7 — FG estabilizado** | F01–F10, P01/P02/P04 | S0/A02/A03 e S4 | Movimento/surface/device-loss, fila/custo, scanout/latência e A/B em aparelho |
| **S8 — Catálogos/conteúdo** | C04–C07, L10/L11 e bundles de perfis | S1/S2/S4 | Assinatura/allowlist/rollback, version/hardware match e export privacy |
| **S9 — Core/audio** | K01–K09/K11 e P03/P05/P06 | S0 e workloads/C07 | Cada otimização com prova de precisão, fallback, custo e regressões |
| **S10 — opcionais** | I10 Bluetooth, HDR, K10 e servidores externos | Gates anteriores | Teste exato de device/driver, operação opt-in e benefício demonstrado |

Paralelização técnica possível: S2/S4 após S0; identidade/sessões precedem
compatibilidade e recents. FG/CPU podem pesquisar em paralelo, mas release depende
dos gates. Companion não começa pelo socket antes de native HID aceitar P2.

Estimativas de calendário dependem de equipe/aparelhos e devem ser feitas após S0.
Cada etapa vira uma série de commits/PRs pequenos **somente se autorizados**,
com estado de execução no status; não um commit contendo todas as fases.

## 13. Plano de testes e iteração

### Sem telefone

- JVM: config merge/version, update ordering/variant/manifest, archive/parser/
  rollback/cancel/replay, library identity/dedup, snapshots de sessão, slots/gyro,
  layout migrations, lixeira/perfis e pacer/governor com relógio controlável.
- Native: GPU cache envelopes/cotas/UUID, mailbox/scheduler/interleavings e
  lifecycle com fake backend, XML/guest-text tolerante, endian/fairness quando pertinente.
- Vulkan software: motores/postprocess com movimento, cuts, opacity/color spaces,
  timestamps/resource reuse e readback. Usar validation layers quando disponíveis;
  ausência delas registrada, sem declarar toda sincronização correta.
- Android emulator/Robolectric/instrumentação conforme ABI disponível: services,
  permission/URI flows e UI/root windows. Emulador x86 sem `libe` ARM64 não executa
  o core por mágica; mocks/compilation e engine reais são resultados separados.
- `:app:testDebugUnitTest :app:assembleDebug :app:assembleDebugAndroidTest
  :app:lintDebug`; para release, variante/pacote/licenças e callbacks testados.
  Reusar [BUILD.md](../BUILD.md) e `tools/test-presentation-host.sh`.

### Com telefone / dispositivos reais

- Controle touch/USB/Bluetooth, hotplug, P1..P4, guest prompts, texto grande/a11y,
  4:3/16:9/ultrawide, Android29/35, external launcher/task return e rotação.
- Saves descartáveis reais: export/restore, perfil existente/diferente, kill em
  commit, lixeira/recovery, armazenamento cheio, URI revogada e provider cloud
  offline; hashes antes/depois. Teclado/prompts e sign-in devem usar perfis de teste.
- Direct launch/library/bootstrap: medir cold/warm time, títulos com XEX secundário,
  multi-disc, disc install-only e conteúdo que altera startup.
- A/B/ABBA de gameplay: build/source/driver/UUID, scene/cap/Hz/resolução/temperatura,
  duração e 1% lows/FPS real/custo GPU/latência/cadência/térmica/artefatos.
- FG: mesmos parâmetros para Off/Win-FG2×/LSFG2×; 3×/4× e FP16 em ensaios próprios.
  Uplift de outputs não conta como uplift de simulação. Forza saturado é controle negativo.
- Companion: dois devices em LAN, jitter/perda/pausa/reconnect, hosts/slot inválidos,
  per-player rumble e perfil; Bluetooth separado com intervalo/limitações explícitos.

### Metas iniciais a calibrar no baseline

- Off path: diferenças dentro da variabilidade medida; investigar regressão
  sustentada >3%, p99 pior ou crescimento de memória/filas ao longo de 20–30 min.
- Scan/index: cold mostra snapshot útil primeiro, UI thread não percorre arquivos
  nem processa imagens/ZIP; operações canceláveis sem recomeçar toda extração.
- FG positivo 30 FPS/60Hz: preservar ≥95% do FPS guest do baseline e medir ≥1,8×
  outputs efetivos sem fila crescente/latência excessiva, após definir métrica confiável.
  Se falhar, reduzir custo/limite ou não liberar para essa configuração.
- Qualquer archive/config/profile falho deixa destino íntegro ou rollback recuperável.
  Zero writes fora do scope em corpus adversarial; retries não duplicam Run IDs.

Registrar `hipótese → mudança → verificações → resultado → decisão → próximo
ensaio`, atualizar status em cada gate e ler código novamente após commits de referência.
Não repetir suíte aprovada sem mudança/falha ou preocupação nova justificada.

## 14. Definição de pronto e próxima implementação

Uma tarefa só é **implementada** se tem backend acionado/estado verdadeiro, não
um card mock. **Testada localmente** identifica quais testes e ambiente. **Validada
no aparelho** exige resultado daquela versão. **Release aprovada** exige gate,
default/fallback, documentação e migração corretos. Dados e licenças preservados.

**Próxima fatia recomendada (revisada em 2026-10-01):** o primeiro lote S0 (A02, A04,
A06, A09, A10–A12) e a base de SessionRunId/L04 estão implementados e testados
localmente, não no aparelho. Ordem seguinte:

1. **Validação no aparelho** do roteiro da auditoria (seção 4 de
   [auditoria-s0-2026-10-01.md](auditoria-s0-2026-10-01.md)) e execução dos testes
   instrumentados — é o gate que falta para fechar S0. Desde 2026-10-03 os grupos A (sem
   jogo), B (arquivo de jogo, sem boot) e C (o jogo roda) do roteiro são testes instrumentados
   no pacote próprio `xendroid.compose.uitest` (seção 4.0 da auditoria;
   `./gradlew :app:connectedUitestAndroidTest`, com `gameDir` para B e C), escritos e
   compilados na nuvem e ainda não rodados: rodar no telefone é o que transforma esses itens
   em "Validado no aparelho". O grupo D (frontends reais, dois telefones, controle Bluetooth,
   partes visuais…) continua manual. Sem aparelho, os candidatos vêm da
   [varredura de 2026-10-03](varredura-referencias-2026-10-03.md), seção 5 (APC sem slot
   global no core, cvars por jogo como filtragem anisotrópica e área segura, ajustes alterados
   no log, controles de toque que somem ao usar um controle físico, backtrace do tombstone).
2. **A13**: integrar `origin/main` (desempenho) antes de commitar. O ensaio com
   `git merge-tree` mesclou sem conflitos (o nome do cache por `ir3_debug` do upstream
   passa a ser o arquivo principal do `pipeline_cache_file::Store`).
3. **S1**: C01 v1 (eventos do host + rajadas de criação de pipelines), C02 v1.1
   (frametime por quadro guest, pipelines, tempo até o primeiro quadro), C03 v1,
   GameIdentity e o estado do boot (U09, parte) estão implementados e testados
   localmente (ciclo 5 no pod de build: 176 testes JVM, lint sem erros). Lote 4:
   áudio, causa de erro fatal e exportação por run (C06 v1). Segue o resto do U09
   (retorno conforme a origem do launch, depende do teste SES-03 no aparelho).
4. **S4**: DriverIdentity em runtime implementado e ligado ao SHA do pacote instalado;
   faltam catálogo por hardware e updater R02/R03.
5. **S5** (slots P1–P4 nativos) antes do companion; **S7** só com A/B no aparelho.

## 15. Referências concretas e links fixos

### X360 Mobile — notas oficiais, sem fonte do app

- **[X1]** [v0.6.3 / Companion](https://github.com/Ashnar2602/X360-Mobile---OFFICIAL/releases/tag/v0.6.3).
- **[X2]** [v0.6.0: biblioteca/storage/compatibilidade](https://github.com/Ashnar2602/X360-Mobile---OFFICIAL/releases/tag/v0.6.0).
- **[X3]** [v0.6.2: performance/políticas](https://github.com/Ashnar2602/X360-Mobile---OFFICIAL/releases/tag/v0.6.2).
- **[X4]** [v0.6.2-public.hotfix.1](https://github.com/Ashnar2602/X360-Mobile---OFFICIAL/releases/tag/v0.6.2-public.hotfix.1) e [propósito closed-source](https://github.com/Ashnar2602/X360-Mobile---OFFICIAL).

### Bannerlator — GPL, conferir arquivos e dependências

- **[B1]** [`XServerDrawer.kt`](https://github.com/The412Banner/Bannerlator/blob/f8cd429a0fc1a92d6601411e8ffecd8786d8264a/app/src/main/java/com/winlator/star/ui/XServerDrawer.kt).
- **[B2]** [`UpdateManager.kt`](https://github.com/The412Banner/Bannerlator/blob/f8cd429a0fc1a92d6601411e8ffecd8786d8264a/app/src/main/java/com/winlator/star/core/UpdateManager.kt).
- **[B3]** [`SettingsControllerTestDialog.kt`](https://github.com/The412Banner/Bannerlator/blob/f8cd429a0fc1a92d6601411e8ffecd8786d8264a/app/src/main/java/com/winlator/star/ui/controllertest/SettingsControllerTestDialog.kt) e [`ControllerTestVisualizer.kt`](https://github.com/The412Banner/Bannerlator/blob/f8cd429a0fc1a92d6601411e8ffecd8786d8264a/app/src/main/java/com/winlator/star/ui/controllertest/ControllerTestVisualizer.kt).
- **[B4]** [`PerformanceHudModels.kt`](https://github.com/The412Banner/Bannerlator/blob/f8cd429a0fc1a92d6601411e8ffecd8786d8264a/app/src/main/java/com/winlator/star/widget/perfhud/PerformanceHudModels.kt).
- **[B5]** [`SyncSupport.kt`](https://github.com/The412Banner/Bannerlator/blob/f8cd429a0fc1a92d6601411e8ffecd8786d8264a/app/src/main/java/com/winlator/star/core/SyncSupport.kt): exemplo de capabilities/fallback, não feature Xenia.
- **[B6]** [input/unbuffered](https://github.com/The412Banner/Bannerlator/commit/3b08d65ec3f289c1a15051ce626b9405514279a5), [session hygiene](https://github.com/The412Banner/Bannerlator/commit/5ec9b25dab40b1d02d43245ae69359e9d0e4c9e6), [offline per-entry](https://github.com/The412Banner/Bannerlator/commit/e0cfc5c87eb713124ee7495ed378e815c30c33a1) e [progress log](https://github.com/The412Banner/Bannerlator/blob/f8cd429a0fc1a92d6601411e8ffecd8786d8264a/PROGRESS_LOG.md). Os três commits são da branch de portes não integrada no recorte consultado; o log explicita ausência de build/teste.

### DroidDeck — GPL, portes com proveniência individual

- **[D1]** [`SessionOverlay.kt`](https://github.com/Droid-Deck/DroidDeck/blob/941a6c3ef91d0fd81d8045ca6ca99fd85ac85363/app/src/main/java/com/droiddeck/launcher/ui/SessionOverlay.kt) e [`SessionState.kt`](https://github.com/Droid-Deck/DroidDeck/blob/941a6c3ef91d0fd81d8045ca6ca99fd85ac85363/app/src/main/java/com/droiddeck/launcher/session/SessionState.kt).
- **[D2]** [`OnScreenControls.kt`](https://github.com/Droid-Deck/DroidDeck/blob/941a6c3ef91d0fd81d8045ca6ca99fd85ac85363/app/src/main/java/com/droiddeck/launcher/input/OnScreenControls.kt) e [`FocusGlide.kt`](https://github.com/Droid-Deck/DroidDeck/blob/941a6c3ef91d0fd81d8045ca6ca99fd85ac85363/app/src/main/java/com/droiddeck/launcher/ui/FocusGlide.kt).
- **[D3]** [`AppUpdates.kt`](https://github.com/Droid-Deck/DroidDeck/blob/941a6c3ef91d0fd81d8045ca6ca99fd85ac85363/app/src/main/java/com/droiddeck/launcher/update/AppUpdates.kt).
- **[D4]** [`SelfInstaller.kt`](https://github.com/Droid-Deck/DroidDeck/blob/941a6c3ef91d0fd81d8045ca6ca99fd85ac85363/app/src/main/java/com/droiddeck/launcher/update/SelfInstaller.kt).
- **[D5]** [`Lossless.kt`](https://github.com/Droid-Deck/DroidDeck/blob/941a6c3ef91d0fd81d8045ca6ca99fd85ac85363/app/src/main/java/com/droiddeck/launcher/gpu/Lossless.kt) e [commit #96/cache/target rate](https://github.com/Droid-Deck/DroidDeck/commit/6c5d6cda73a2a65ba89affc1e617c65f63a40714).
- **[D6]** [`DriverPairs.kt`](https://github.com/Droid-Deck/DroidDeck/blob/941a6c3ef91d0fd81d8045ca6ca99fd85ac85363/app/src/main/java/com/droiddeck/launcher/gpu/DriverPairs.kt).
- **[D7]** [commit #87/performance](https://github.com/Droid-Deck/DroidDeck/commit/8ee1cbe3ebe3224500c92055462a7dc2087745ed), incluindo diff e contexto do workaround KGSL.

### Eden — fonte oficial Forgejo, GPL por arquivo

- **[E1]** [stable 0.2.1](https://git.eden-emu.dev/eden-emu/eden/releases/tag/v0.2.1), [0.2.0](https://git.eden-emu.dev/eden-emu/eden/releases/tag/v0.2.0) e [downloads/canais](https://eden-emu.dev/downloads).
- **[E2]** [`QuickSettings.kt`](https://git.eden-emu.dev/eden-emu/eden/src/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed/src/android/app/src/main/java/org/yuzu/yuzu_emu/dialogs/QuickSettings.kt).
- **[E3]** [`frame_gen_pacer.cpp`](https://git.eden-emu.dev/eden-emu/eden/src/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed/src/video_core/renderer_vulkan/present/frame_gen_pacer.cpp).
- **[E4]** [`vk_present_manager.cpp`](https://git.eden-emu.dev/eden-emu/eden/src/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed/src/video_core/renderer_vulkan/vk_present_manager.cpp) e [pasta present/LSFG](https://git.eden-emu.dev/eden-emu/eden/src/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed/src/video_core/renderer_vulkan/present).
- **[E5]** [`ControllerNavigationGlobalHook.kt`](https://git.eden-emu.dev/eden-emu/eden/src/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed/src/android/app/src/main/java/org/yuzu/yuzu_emu/utils/ControllerNavigationGlobalHook.kt) e [InputOverlay](https://git.eden-emu.dev/eden-emu/eden/src/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed/src/android/app/src/main/java/org/yuzu/yuzu_emu/overlay/InputOverlay.kt).
- **[E6]** [`DriverResolver.kt`](https://git.eden-emu.dev/eden-emu/eden/src/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed/src/android/app/src/main/java/org/yuzu/yuzu_emu/utils/DriverResolver.kt), [`LosslessScalingHelper.kt`](https://git.eden-emu.dev/eden-emu/eden/src/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed/src/android/app/src/main/java/org/yuzu/yuzu_emu/utils/LosslessScalingHelper.kt), [`DocumentsTree.kt`](https://git.eden-emu.dev/eden-emu/eden/src/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed/src/android/app/src/main/java/org/yuzu/yuzu_emu/utils/DocumentsTree.kt).
- **[E7]** [cache/driver fix](https://git.eden-emu.dev/eden-emu/eden/commit/2a7e9b53b12711f679238ee738a2ce57cc42c8ed), [postprocess/app-layer](https://git.eden-emu.dev/eden-emu/eden/commit/cb73a4dcc7710a7fea1643797b17e39858f4ae6a) e [buffer/fence change](https://git.eden-emu.dev/eden-emu/eden/commit/f273423b2b83f1a37cc7c1a7a92601f26e0c12ae).

### GameHub — produto oficial e relato secundário, sem código de engine verificado

- **[G1]** [site oficial](https://gamehub.xiaoji.com/) / [hub.xiaoji.com](https://hub.xiaoji.com/en-us): plataforma/import/streaming/cloud; página completa deu timeout nesta pesquisa, informações acessíveis no índice de busca.
- **[G2]** [GameSir Software Downloads](https://gamesir.com/support/downloads): identificação do produto/cliente.
- **[G3]** [Android Authority, anúncio GameHub 6.2.0 em 2026-08-25](https://www.androidauthority.com/gamehub-update-mod-support-3702532/): mods/layout cloud/biblioteca/plataformas. O próprio artigo distingue anúncio e interpretação; não valida tecnologia interna nem FPS.

Os sources Forgejo foram consultados por API `contents` e decodificados com
base64 quando o endpoint raw respondeu 403. Na revisão final, endpoints de commit
também responderam 403; os três commits [E7] foram confirmados pela API
`commits?limit=1&sha=<SHA>`, com SHA, data, mensagem e arquivos alterados. A árvore
recursiva Eden foi truncada a 1000 entradas; diretórios relevantes foram enumerados
pela API de conteúdo, não se afirma uma auditoria integral do core. Código está
fixado por SHA; notas de release e páginas de produto podem mudar. Re-checar
licença, regressões e testes antes de qualquer port.
