// Configuração do site. Endereços e nomes do projeto ficam aqui; o resto vem do repositório.
// SITE_URL e SITE_BASE_PATH podem ser definidos pelo GitHub Actions (actions/configure-pages).

const repo = process.env.SITE_REPO || 'phforner0/Xendroid-Plus';
const [owner, name] = repo.split('/');
const defaultUrl = `https://${owner.toLowerCase()}.github.io/${name}/`;
const url = (process.env.SITE_URL || defaultUrl).replace(/\/?$/, '/');

export default {
  name: 'Xendroid+',
  repo,
  repoUrl: `https://github.com/${repo}`,
  /** Endereço público do site, usado só em metadados (canonical, Open Graph, sitemap). */
  url,
  /** Caminho do site no domínio (ex.: /Xendroid-Plus/); usado pela página 404, que não pode usar links relativos. */
  basePath: (process.env.SITE_BASE_PATH || new URL(url).pathname).replace(/\/?$/, '/'),
  discord: 'https://discord.gg/AT8Bswv62',
  support: 'https://buymeacoffee.com/phforner0',
  /** Seções da documentação, na ordem da barra lateral. */
  docSections: [
    { id: 'comecar', title: { en: 'Get started', pt: 'Começar' } },
    { id: 'usar', title: { en: 'Use', pt: 'Usar' } },
    { id: 'referencia', title: { en: 'Reference', pt: 'Referência' } },
    { id: 'projeto', title: { en: 'Project', pt: 'Projeto' } },
  ],
  /** Caminhos do repositório que alimentam o site (o workflow do Pages observa os mesmos). */
  sparsePaths: [
    '/website/',
    '/README.md', '/README.pt-BR.md', '/BUILD.md', '/GAME_COMPAT.md', '/THIRD-PARTY-NOTICES.md', '/local.properties.example',
    '/app/build.gradle', '/app/src/main/',
    '/emulator-core/build.gradle',
    '/emulator-core/src/main/assets/',
    '/emulator-core/src/main/java/',
    '/emulator-core/src/main/cpp/*.cpp', '/emulator-core/src/main/cpp/*.h',
    '/emulator-core/src/main/cpp/xenia/LICENSE',
    '/emulator-core/src/main/cpp/xenia/src/xenia/',
    '/emulator-core/src/main/cpp/xenia/assets/game-compatibility/',
    '/patches/xenia-canary/patches/',
    '/docs/',
    '/performance-tests/',
    '/tools/',
    '/.github/',
  ],
};
