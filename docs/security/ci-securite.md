# Enhance — Jobs sécurité de la CI (à implémenter par devops)

Statut : v1.0 du 2026-10-21 (secdevops). Remplace l'emplacement `security` de `.github/workflows/ci.yml` (commit 939e704). Références : ADR-027, ADR-029, ADR-035, `threat-model.md`, `zero-reseau.md`, `licences.md`.

Principes communs à tous les jobs :
- `permissions: contents: read` au niveau du workflow (déjà en place) ; aucun job n'a besoin de plus. Jamais `pull_request_target`, jamais de `secrets.*`, aucun runner auto-hébergé.
- **Toute action épinglée par SHA de commit** (40 hex) avec la version en commentaire ; `actions/checkout` avec `persist-credentials: false` (déjà en place) et `fetch-depth: 0` pour les jobs qui lisent l'historique.
- Tout outil téléchargé en CI : version exacte + **SHA-256 vérifiée** dans le workflow avant exécution ; jamais `curl | sh`.
- `timeout-minutes` sur chaque job ; les jobs de sécurité ne sont **pas** soumis à `paths-ignore` (un e-mail ou un secret dans `docs/` doit être vu) → les placer dans un workflow séparé `security.yml` déclenché sur `push` (toutes branches) et `pull_request`, sans `paths-ignore`.
- Les mêmes contrôles tournent en local dans `scripts/precommit-check.ps1` (hook) pour échouer **avant** le commit.

## 1. Vue d'ensemble

| Job | Runner | Durée visée | Bloquant | Contrôle |
|---|---|---|---|---|
| `identity` | ubuntu-24.04 | < 1 min | Oui | Auteurs, committers et trailers de **tout l'historique** |
| `forbidden-files` | ubuntu-24.04 | < 1 min | Oui | `git ls-files` contre la liste des fichiers interdits, taille, binaires |
| `secrets` | ubuntu-24.04 | < 1 min | Oui | gitleaks (règles par défaut + e-mail + chemins personnels) sur l'historique complet |
| `zero-network-src` | ubuntu-24.04 | < 1 min | Oui | C1 de `zero-reseau.md` (sources, CMake, `vcpkg.json`, QML) |
| `zero-network-bin` | windows-2025 (`needs: core`) | 2–3 min | Oui | C2/C3/C4 sur l'artefact du build `ci` |
| `deps` | ubuntu-24.04 | < 2 min | Oui (manifeste) / non (CVE) | Manifeste vcpkg (baseline, features interdites), registre des licences à jour, veille CVE |
| `workflow-lint` | ubuntu-24.04 | < 1 min | Oui | Actions épinglées, permissions, déclencheurs (zizmor optionnel) |
| `sast` | windows-2025 | 5–10 min | HIGH = oui | cppcheck + règles semgrep locales ; MSVC `/analyze` dans le preset `ci` |
| `cve-watch` (planifié, hebdomadaire) | ubuntu-24.04 | < 2 min | Non (ouvre un rapport) | Versions FFmpeg / Qt / tiers comparées aux CVE connues |

## 2. `forbidden-files`

Étend la vérification déjà présente dans `ci.yml`. Sur `git ls-files` (index de la révision testée) :
- **Chemins interdits** : `^\.claude/`, `(^|/)CLAUDE\.md$`, `(^|/)LISEZMOI-EQUIPE\.md$`, `(^|/)KICKOFF\.txt$`, `^_claude-setup/`, `^third_party/`, `^(build|out|dist|vcpkg_installed|\.vs|logs|tests/clips)/`, `(^|/)\.env($|\.)` (sauf `.env.example`), `(^|/)CMakeUserPresets\.json$`.
- **Extensions interdites** : binaires `dll exe lib pdb obj ilk exp msi msix zip 7z`, modèles `onnx engine plan trt pth pt ckpt safetensors pkl`, médias `mp4 mkv mov avi webm mxf y4m yuv ts m2ts mpg mpeg vob wmv flv 3gp h264 h265 hevc ivf`, secrets `pfx p12 pem key cer crt jks`, journaux `log dmp`.
- **Taille** : tout fichier > 5 Mio refusé.
- **Binaires non listés** : un fichier détecté binaire (`git ls-files --eol` → `-text` / `i/-text`) hors liste blanche explicite (`scripts/binary-allowlist.txt`, validée par secdevops : icônes `.svg`/`.png`, polices `.ttf`/`.otf`/`.woff2` sous `src/app/qml/assets/` avec leur licence) est refusé.
- Le **fichier de verrou FFmpeg** doit vivre hors de `third_party/` (SEC-17), ex. `deps/ffmpeg.lock.json`.

## 3. `identity` — e-mail des auteurs et committers

```bash
expected='81617903+rolkapone@users.noreply.github.com'
git log --all --format='%H %ae %ce' | awk -v e="$expected" '$2!=e || $3!=e {print; bad=1} END{exit bad}'
```
- Porte sur **tout l'historique** (`--all`), pas seulement la PR : un commit fautif ancien est aussi un problème (A6).
- Le nom attendu est `Carol MBONZIMA` (avertissement seulement s'il diffère).
- **Trailers des messages** (`Co-Authored-By`, `Signed-off-by`, `Reviewed-by`…) : extraire les adresses entre `<…>` ; admises : la noreply du PO et l'adresse noreply d'Anthropic des trailers `Co-Authored-By` (présente dans 939e704) **sous réserve de décision du PO** (`licences.md` §7, point 7). Écrire l'exception dans le script sous forme d'expression régulière échappée (`noreply@anthropic\.com`), pour qu'elle ne soit pas elle-même détectée comme une adresse en clair.
- Règle stricte : on n'écrit **jamais** l'adresse professionnelle du PO dans le dépôt, même dans une liste noire. La détection repose uniquement sur la **liste blanche** (toute autre adresse = échec).
- Merges faits sur l'interface web de GitHub : le committer serait « GitHub » avec l'adresse noreply générique de GitHub ; interdit par défaut (les merges se font en local, `--no-ff`, par devops).
- Recommandation au PO (SEC-05) : sur GitHub, *Settings > Emails* : « Keep my email addresses private » et « Block command line pushes that expose my email ».

Correction à apporter à `scripts/precommit-check.ps1` (constat du 2026-10-21) : `$mailAllowed` accepte **toute** adresse `@users.noreply.github.com` ; restreindre à la noreply exacte du PO (et au trailer Anthropic si le PO l'accepte).

## 4. `secrets` — gitleaks

- Outil : **gitleaks** (MIT), binaire Linux de la release GitHub, version ≥ 8.25 (syntaxe `[[rules.allowlists]]`), épinglé + SHA-256 vérifiée. Ne pas utiliser `gitleaks-action` (clé de licence exigée pour les comptes d'organisation, et inutile).
- Commande : `gitleaks git --config .gitleaks.toml --redact --no-banner --exit-code 1 --log-opts="--all"` (historique complet, valeurs masquées dans la sortie).
- Configuration : `.gitleaks.toml` à la racine (fourni par secdevops, **non testé localement** : gitleaks n'est pas installé sur la machine du PO) : règles par défaut + `enh-email-address` (toute adresse sauf la noreply du PO) + `enh-windows-user-path` (`C:\Users\<nom>` et équivalents) + `enh-build-path`.
- Local : `winget install -e --id Gitleaks.Gitleaks` puis, dans le hook, `gitleaks git --staged --config .gitleaks.toml --redact` si l'outil est présent (sinon les motifs de `precommit-check.ps1` restent le filet).

## 5. `zero-network-src` et `zero-network-bin`

Spécification complète : `zero-reseau.md` §3 à §5 et §10.
- `zero-network-src` : `pwsh scripts/check-no-network.ps1 -SourcesOnly` (C1). Peut tourner sur ubuntu avec PowerShell 7.
- `zero-network-bin` : après le job `core`, téléverser `build/ci/bin` en artefact (rétention 1 jour), puis `scripts/check-no-network.ps1 -Path build/ci/bin -Report zero-network.md` (C2/C3/C4) ; joindre le rapport en artefact.
- **Le squelette actuel de `check-no-network.ps1` doit être adapté** au format de `scripts/network-allowlist.json` (schéma v1 : `forbiddenFiles`, `forbiddenImports`, `warnImports`, `exceptions[]` au niveau des **fonctions** avec date d'expiration, `stringScan`) et utiliser `dumpbin /imports` (fonctions + imports différés) au lieu de `/dependents` seul.
- Les exceptions `scope: dev-only` (E3, E3b : avformat BtbN) font **échouer** `package.ps1` (livrable) mais pas la CI de développement.

## 6. `deps` — dépendances et licences

1. **Manifeste** (`vcpkg.json`, `vcpkg-configuration.json`) : `builtin-baseline` = 40 hex (échec tant que `TODO-VCPKG-BASELINE-SHA`) ; aucune `overrides` sans commentaire de justification ; features interdites (liste de `zero-reseau.md` §3, dont `ffmpeg[gpl|nonfree|x264|x265|fdk-aac|openssl]`, `qtbase[network]`) ; triplet maison présent.
2. **Registre des licences** : chaque dépendance du manifeste (ports et features) et chaque binaire listé dans les verrous (`deps/*.lock.json`) doit avoir une ligne dans `docs/security/licences.md` avec un verdict `OK` ; sinon échec (script `scripts/licences.ps1 -Check`).
3. **Vulnérabilités** : osv-scanner (Apache-2.0) ne prend pas en charge, à ma connaissance, l'écosystème vcpkg **[H]** à vérifier par devops ; en attendant :
   - `cve-watch` hebdomadaire (`schedule`) : script qui lit les versions des ports clés (FFmpeg, Qt, libpng, libjpeg-turbo, FreeType, HarfBuzz, zlib, PCRE2, spdlog, nlohmann-json) depuis la baseline vcpkg (`vcpkg x-package-info --x-json` ou `ports/<port>/vcpkg.json` du commit épinglé) et interroge la base NVD (API 2.0, sans clé, débit limité) par CPE ; rapport en artefact ; CVSS ≥ 7 non listé dans `docs/security/accepted-risks.md` → job rouge (non bloquant pour les PR, **bloquant avant un tag de release**) ;
   - FFmpeg BtbN (spike) : version lue dans le verrou, même comparaison.
   - Si osv-scanner gère un jour vcpkg ou un SBOM SPDX produit par vcpkg (`share/<port>/vcpkg.spdx.json`), l'ajouter : `osv-scanner scan --format markdown`.

## 7. `workflow-lint`

- Échec si une ligne `uses:` n'est pas suivie de `@[0-9a-f]{40}` (ou si `TODO-SHA` subsiste), si `permissions` manque au niveau du workflow ou dépasse `contents: read`, si `pull_request_target`, `self-hosted` ou `secrets.` apparaissent.
- Optionnel : **zizmor** (MIT, `pip install zizmor==<version>`), `zizmor --offline .github/workflows` (injections d'expressions, `persist-credentials`, épinglage).
- Dependabot pour les actions (mises à jour des SHA) : possible plus tard, avec revue secdevops de chaque PR.

## 8. `sast` — analyse statique C++

- **MSVC** : preset `ci` avec `/W4 /WX` (ADR-022) ; ajouter un preset `analyze` (`/analyze /analyze:external-` + règles `NativeRecommendedRules.ruleset`) exécuté dans `sast` ; avertissements C6xxx de dépassement de tampon et de déréférencement nul = échec.
- **cppcheck** (GPL-3.0, outil) : version épinglée, `cppcheck --project=build/ci/compile_commands.json --enable=warning,performance,portability --inline-suppr --error-exitcode=1 --suppress=*:*/vcpkg_installed/*`.
- **semgrep** (LGPL-2.1, CLI) : uniquement des **règles locales** (`.semgrep/enhance.yml`, à écrire par secdevops dans ENH-003), `semgrep scan --config .semgrep --metrics=off --error`. Règles prévues : fonctions C dangereuses (`strcpy`, `sprintf`, `gets`, `wcscpy`, `_tcscpy`), `new`/`delete` nus hors RAII, `system(` / `_wsystem(` / `CreateProcess` (aucun processus enfant prévu au MVP), `LoadLibraryW(` / `LoadLibraryA(` (exiger `LoadLibraryExW` avec drapeaux `LOAD_LIBRARY_SEARCH_*`), `SetDllDirectory` avec un chemin non vide, `av_dump_format`, `avio_open`, `avformat_open_input` avec une URL non vide, `NVSDK_NGX_UpdateFeature`, `setEngineHostCodeAllowed(true)`, `IPluginRegistry::loadLibrary`, `ShellExecute` sur une chaîne venue du QML.
- `clang-tidy` (déjà prévu, job `tidy`) complète.
- Installation locale (aucun de ces outils n'est installé aujourd'hui) : `winget install -e --id Cppcheck.Cppcheck` ; semgrep : `pip install semgrep==<version>` (prise en charge Windows à vérifier, sinon WSL ou CI seulement) ; osv-scanner : binaire de la release GitHub avec SHA-256.

## 9. Ce que devops doit écrire (récapitulatif)

| Élément | Action |
|---|---|
| `.github/workflows/security.yml` | Nouveau workflow sans `paths-ignore` : jobs `identity`, `forbidden-files`, `secrets`, `zero-network-src`, `deps`, `workflow-lint` ; `cve-watch` en `schedule` hebdomadaire + `workflow_dispatch` |
| `.github/workflows/ci.yml` | Retirer l'emplacement `security` (déplacé) ; ajouter `zero-network-bin` et `sast` après `core` ; remplacer les `TODO-SHA` |
| `scripts/check-no-network.ps1` | Adapter au schéma v1 de `network-allowlist.json` ; modes `-SourcesOnly`, `-Path`, `-Report` ; imports au niveau des fonctions ; scan de chaînes sans table de certificats |
| `scripts/licences.ps1 -Check` | Comparer manifeste + verrous au registre `licences.md` |
| `scripts/precommit-check.ps1` | Restreindre `$mailAllowed` ; ajouter les chemins et extensions du §2 ; appeler gitleaks `--staged` s'il est installé |
| `scripts/binary-allowlist.txt`, `deps/ffmpeg.lock.json` | Créer (liste vide au départ ; verrou déplacé hors `third_party/`) |
| `docs/security/accepted-risks.md` | Créé par secdevops quand le premier risque CVE est accepté |

Fichiers de configuration fournis par secdevops : `.gitleaks.toml`, `scripts/network-allowlist.json`. Ils ne sont pas commités par secdevops : devops les ajoute explicitement dans le commit de la story ENH-003.
