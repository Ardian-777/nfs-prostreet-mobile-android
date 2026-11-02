# Contagem offline de instruções ir3 (sem celular)

Mede o efeito de mudanças no tradutor SPIR-V no código que o Turnip gera, sem
aparelho: o tradutor do Xenia compilado para o host (x86_64 Linux) traduz os
shaders do cache de um jogo (`.xsh`), o `spirv-val` valida cada arquivo e o
Turnip do Mesa (Adreno 830, sobre o `drm-shim` noop do freedreno, sem GPU)
compila pipelines com eles e informa as estatísticas de
`VK_KHR_pipeline_executable_properties` (instruções, NOPs, registradores,
ondas, laços, preâmbulo, stalls estimados).

Serve para decidir o que vale um A/B no aparelho. **Contagem estática não é
tempo de GPU**: ramos pulados em tempo de execução (por exemplo
`spirv_texture_sign_branch`) aparecem como código a mais. O Adreno 825 não
existe no Mesa upstream; o 830 é a GPU a8xx mais próxima, e o Turnip upstream
não é o Gen8 V36 do aparelho.

## Preparação (uma vez)

```bash
sudo apt-get install -y clang ninja-build glslang-tools spirv-tools \
  libvulkan-dev libdrm-dev libexpat1-dev flex bison zlib1g-dev
pip install meson mako pyyaml
git clone https://gitlab.freedesktop.org/mesa/mesa.git
# Medido com 52b557af72fbd2fdbea35a39c7a4d917f7155f34 (2026-09-28).
meson setup mesa/build mesa -Dbuildtype=release -Dvulkan-drivers=freedreno \
  -Dgallium-drivers= -Dfreedreno-kmds=msm -Dtools=drm-shim -Dplatforms= \
  -Dglx=disabled -Degl=disabled -Dgbm=disabled -Dllvm=disabled \
  -Dopengl=false -Dgles1=disabled -Dgles2=disabled -Dvalgrind=disabled \
  -Dlibunwind=disabled -Dzstd=disabled -Dvulkan-layers= -Dbuild-tests=false
ninja -C mesa/build
```

O tradutor precisa do `version.h` de um configure do `emulator-core`
(`./gradlew :emulator-core:configureCMakeRelease[arm64-v8a]`) ou de
`XENIA_GENERATED_DIR`. Depois:

```bash
python3 tools/ir3_offline/gen.py        # gera tools/ir3_offline/build/build.ninja
ninja -C tools/ir3_offline/build        # corpus_tool, ir3stats, shaders parceiros
```

## Uso

O cache de shaders do Forza Horizon usado nas medições está no histórico:
`git show 77c446b0:performance-tests/4D5309C9.xsh > 4D5309C9.xsh` (221 vertex e
262 pixel shaders). No aparelho, o cache de cada jogo fica em
`<cache_root>/shaders/shareable/<TITLE_ID>.xsh`.

```bash
export CORPUS=$PWD/4D5309C9.xsh MESA_BUILD=$PWD/mesa/build
tools/ir3_offline/run_variant.sh base
tools/ir3_offline/run_variant.sh nozero spirv_ps_relaxed_math=1
python3 tools/ir3_offline/ir3cmp.py tools/ir3_offline/work/stats/base.csv \
  tools/ir3_offline/work/stats/nozero.csv FS
```

- Argumentos `cvar=valor` mudam opções do tradutor (sintaxe TOML do valor).
- `IR3STATS_DUMP=<pasta>` grava também o disassembly ir3 de cada shader.
- `ir3cmp.py ... VS` compara os vertex shaders.
- Pixel shaders são traduzidos com a especialização sem alfa quando escrevem a
  cor 0, todos os interpoladores e os alvos que escrevem; o recurso de
  baricêntricas fica desligado (o Turnip não expõe
  `VK_KHR_fragment_shader_barycentric`, então `precise_interpolation` não é
  emitido no aparelho).

## Resultados de referência (Forza Horizon, 262 pixel shaders)

| Variante | Instruções FS |
|---|---:|
| base | 258.944 |
| `spirv_fast_precision_rounding = true` | −6,2% |
| `spirv_ps_relaxed_math = 1` / `2` / `8` / `11` | −4,8% / −13,3% / 0% / −18,7% |
| `spirv_texture_sign_branch = true` (estático) | +4,9% (caminho sem gama: −21,2%) |
| `spirv_vs_math_experiment = 11` (VS, 221 shaders) | −7,8% |

Medidos antes de 30/10/2026, quando o arredondamento de 21 bits depois de
exp, log, sqrt, rsq e rcp saiu (os pixel shaders passaram a usar as operações
do host): `spirv_fast_precision_rounding` não existe mais, e o bit 2 de
`spirv_ps_relaxed_math` não muda mais nada.
