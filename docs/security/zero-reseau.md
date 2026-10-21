# Enhance — Preuve « zéro réseau » (ENH-028, ADR-027)

Statut : v1.0 du 2026-10-21 (secdevops). Spécification à implémenter par devops dans `scripts/check-no-network.ps1` et le job CI `zero-network` ; liste blanche dans `scripts/network-allowlist.json` (fichier de sécurité : toute modification passe par secdevops).

## 1. Définition

**Enhance n'ouvre aucune connexion réseau, ne résout aucun nom, n'envoie ni ne reçoit aucun octet sur le réseau**, à aucun moment (démarrage, traitement, fermeture), et fonctionne à l'identique sans réseau.

Exceptions explicitement admises (et seulement celles-ci) :
- **E1 — fichiers sur un partage réseau choisi par l'utilisateur** (lecteur mappé ou chemin UNC) : le trafic SMB est émis par le redirecteur de Windows (processus `System`), à la demande explicite de l'utilisateur, jamais en tâche de fond (SEC-10).
- **E2 — import de `ws2_32.dll` par `Qt6Core.dll` limité à `WSAAsyncSelect`, `WSAStartup`, `WSACleanup`** (constaté le 2026-10-21 sur Qt 6.11.2 vcpkg sans Network : ordinaux #101, #115, #116, rien d'autre) : boucle d'événements (`QSocketNotifier`) et initialisation de Winsock, aucune socket créée sans `QtNetwork` ; revalidation à chaque version de Qt, échéance 2027-04-21 (SEC-22). Avertissements `mpr` / `netapi32` de `Qt6Core` acceptés (chemins réseau choisis par l'utilisateur).
- **E3, E3c, E3d, E3e (temporaires, `dev-only`)** — imports réseau (`ws2_32`, `dnsapi`, bibliothèques TLS du système) des DLL `avformat`, `avcodec` et `avfilter` du build BtbN 8.1.3 (libsrt, librist, libssh, libzmq compilés dedans) ; neutralisés à l'exécution (AVIO maison, `io_open` restrictif, `protocol_whitelist=file`, aucun filtre `zmq`/`azmq`) ; refusés en mode `-Package` ; échéance 2027-01-21 ; **SEC-06 (FFmpeg vcpkg minimal sans réseau) reste obligatoire avant ENH-030**.

Le comportement du pilote NVIDIA et de ses services hors du processus d'Enhance est hors de notre contrôle (risque résiduel R-2 du threat model), mais **toute activité réseau déclenchée par l'usage d'une API NVIDIA depuis Enhance** (ex. NGX Update Module, SEC-01) entre dans le périmètre et doit être absente ou explicitement acceptée par le PO.

## 2. Couches de contrôle

| # | Contrôle | Où | Bloquant | Ce qu'il attrape | Limite |
|---|---|---|---|---|---|
| C1 | Recherche d'API réseau dans les **sources** | CI (chaque PR) + pré-commit | Oui | Code d'Enhance qui appelle ou lie du réseau | Ne voit pas les tiers |
| C2 | **Fichiers interdits** dans le dossier déployé | CI (build `ci`) + `package.ps1` | Oui | `Qt6Network.dll`, bibliothèques HTTP | Seulement les noms connus |
| C3 | **Imports PE** (`dumpbin /imports`), au niveau des **fonctions** | CI + `package.ps1` | Oui | Liaison statique à Winsock, WinHTTP, WinINet, URLMon… | Ne voit pas `LoadLibrary` + `GetProcAddress` |
| C4 | **Chaînes** dans les binaires (URL, noms d'hôtes, chemins personnels) | `package.ps1` (+ CI sur l'exe) | Oui pour nos binaires, avertissement pour les tiers | URL codées en dur, chemins de build | Faux positifs (certificats Authenticode, docs embarquées) |
| C5 | **Modules chargés à l'exécution** | Test GPU local (`test-gpu.ps1`) | Avertissement → enquête | DLL réseau chargées dynamiquement (par nous ou un SDK) | Une DLL chargée n'est pas une connexion |
| C6 | **Test dynamique pare-feu + audit WFP** | Local, manuel guidé (PO / QA), avant chaque merge `[GPU]` qui touche aux SDK et avant chaque release | Oui | Toute tentative de connexion d'`Enhance.exe` | Ne voit pas les services NVIDIA hors processus |
| C7 | **Test hors ligne** (carte réseau désactivée) + capture globale | Local, avant chaque release et dans ENH-001 (SEC-01) | Oui | Dépendance fonctionnelle au réseau ; trafic NVIDIA déclenché par NGX | Attribution parfois indirecte |

## 3. C1 — Sources (`src/`, `tests/` exclus, `CMakeLists.txt`, `cmake/`, `vcpkg.json`, QML)

Échec si une de ces expressions apparaît (insensible à la casse ; une exception exige un commentaire `// enh-network-allow: SEC-xx` validé par secdevops — aucune attendue) :

- En-têtes / bibliothèques : `winsock`, `winsock2.h`, `ws2tcpip.h`, `mswsock.h`, `winhttp.h`, `wininet.h`, `urlmon.h`, `iphlpapi.h`, `windns.h`, `websocket.h`, `http.h` (HTTP Server API), `#pragma comment(lib, "ws2_32` / `winhttp` / `wininet` / `urlmon` ; dans CMake : `ws2_32`, `winhttp`, `wininet`, `urlmon`, `Qt6::Network`, `Qt::Network`, `Network` dans `find_package(Qt6 COMPONENTS ...)`, `CURL::`, `cpprest`.
- Fonctions : `WSAStartup`, `socket(`, `connect(`, `getaddrinfo`, `gethostbyname`, `InternetOpen`, `InternetOpenUrl`, `HttpSendRequest`, `WinHttpOpen`, `URLDownloadToFile`, `URLOpenStream`, `BITS` (`IBackgroundCopyManager`), `NVSDK_NGX_UpdateFeature`, `avformat_network_init`, `avio_open(` / `avio_open2(` (interdits : on n'ouvre que par `AVIOContext` maison).
- Qt / QML : `QNetwork`, `QTcpSocket`, `QUdpSocket`, `QSslSocket`, `QWebSocket`, `QDnsLookup`, `QHostInfo`, `XMLHttpRequest`, `Qt.openUrlExternally`, `source:\s*"https?:`, `import QtNetwork`, `import QtWebEngine`, `import QtWebView`.
- Chaînes : `"https?://` dans `src/` (hors commentaires de licence : liste d'exceptions par fichier dans `network-allowlist.json` → `sourceUrlExceptions`).
- `vcpkg.json` : ports ou features interdits `qtbase[network]` (ou feature `network`), `qtnetworkauth`, `qtwebengine`, `qtwebview`, `qtwebsockets`, `qthttpserver`, `curl`, `cpprestsdk`, `openssl`, `ffmpeg[gpl]`, `ffmpeg[nonfree]`, `ffmpeg[openssl]`, `ffmpeg[x264]`, `ffmpeg[x265]`, `ffmpeg[fdk-aac]`.
- Exigences positives (le contrôle échoue si elles **manquent** une fois le pipeline réel présent) : chaîne `protocol_whitelist` posée à `file` et remplacement de `io_open` dans `src/pipeline/gpu/ffmpeg/`.

## 4. C2 et C3 — Dossier déployé : fichiers et imports

Entrée : un dossier (sortie du build `ci`, ou dossier portable avant zippage). Pour **chaque** `.exe` et `.dll` (récursif, plugins Qt compris) :

1. **Fichiers interdits** (échec immédiat) : `Qt6Network*.dll`, `Qt6WebEngine*`, `Qt6WebSockets*`, `Qt6WebView*`, `Qt6NetworkAuth*`, `libcurl*.dll`, `cpprest*.dll`, `libssl*.dll`, `libcrypto*.dll`, `winhttp.dll`, `wininet.dll`, `urlmon.dll` (copies locales), plugins `tls\*`, `networkinformation\*`, `bearer\*`.
2. **Imports** : `dumpbin /nologo /imports <fichier>` (dumpbin de VS localisé par `vswhere`, chemin absolu). Construire la liste (DLL importée → fonctions importées), **y compris les imports différés** (section « Section contains the following delay load imports »).
3. **DLL réseau interdites** à l'import (échec sauf exception de la liste blanche, comparée **au niveau des fonctions**) : `ws2_32.dll`, `wsock32.dll`, `mswsock.dll`, `winhttp.dll`, `wininet.dll`, `urlmon.dll`, `websocket.dll`, `httpapi.dll`, `dnsapi.dll`, `Qt6Network.dll`, `webio.dll`, `winhttpcom.dll`.
4. **DLL à signaler** (avertissement dans le rapport, pas d'échec ; justification à consigner) : `iphlpapi.dll`, `netapi32.dll` (Qt6Core : noms de partages, informations d'utilisateur), `mpr.dll` (Qt6Core : lecteurs réseau mappés), `secur32.dll`, `sspicli.dll`, `crypt32.dll`, `cryptnet.dll`, `wldp.dll`, `bcrypt.dll`, `ncrypt.dll`, `rpcrt4.dll`.
5. **Règle de liste blanche** (`scripts/network-allowlist.json`, schéma ci-dessous) : une exception désigne un module (motif de nom), une DLL importée et **la liste exacte des fonctions autorisées**. Échec si une fonction importée n'est pas dans la liste (ex. `Qt6Core.dll` qui importerait `connect` ou `getaddrinfo`). Chaque exception porte : justification, propriétaire, date de revue, **date d'expiration** ou condition (« avant ENH-030 »). Une exception expirée = échec.
6. Les DLL NVIDIA livrées (`nvngx_vsr.dll`) passent par les mêmes règles (constaté le 2026-10-21 sur le SDK 1.1.0 : imports `ADVAPI32`, `KERNEL32`, `USER32`, `nvcuda` uniquement).

État constaté le 2026-10-21 (analyse des imports par `objdump -p`, à refaire avec `dumpbin` dans le script) : `nvngx_vsr.dll`, `nvngx_truehdr.dll`, `NvOFFRUC.dll`, `nvinfer_11.dll`, `nvinfer_plugin_11.dll`, `nvonnxparser_11.dll`, `nvinfer_lean_11.dll`, `nvinfer_dispatch_11.dll` : **aucun import réseau statique**. En revanche `nvngx_vsr.dll` charge dynamiquement des modules (`LoadLibraryExW`) et référence `cryptnet.dll` / `wldp.dll` (vérification de signature) : voir C5–C7.

### Schéma de `scripts/network-allowlist.json`

```json
{
  "schemaVersion": 1,
  "forbiddenFiles": ["Qt6Network*.dll", "..."],
  "forbiddenImports": ["ws2_32.dll", "..."],
  "warnImports": ["iphlpapi.dll", "..."],
  "exceptions": [
    {
      "id": "E2",
      "module": "Qt6Core.dll",
      "import": "ws2_32.dll",
      "allowedFunctions": ["WSAAsyncSelect", "WSAStartup", "WSACleanup"],
      "justification": "...",
      "owner": "secdevops",
      "reviewed": "2026-10-21",
      "expires": null
    }
  ],
  "stringScan": { "...": "..." },
  "sourceUrlExceptions": []
}
```

Le fichier de référence est `scripts/network-allowlist.json` (exceptions constatées sur les vrais binaires le 2026-10-21 ; l'extrait ci-dessus est indicatif).

## 5. C4 — Chaînes dans les binaires

- Outil : lecture du fichier en ASCII **et** UTF-16LE (équivalent de `strings -a -n 6` et `strings -el`), en **excluant la table de certificats Authenticode** (répertoire de sécurité du PE) : sinon chaque binaire signé remonte des URL `crl3.digicert.com` / `ocsp.digicert.com` (constaté sur les DLL NVIDIA).
- **Nos binaires** (`Enhance.exe`, plugins QML d'Enhance) : échec sur `https?://` (sauf liste `stringScan.ownBinaryUrlExceptions`, vide au départ, ex. URL de licence affichée dans « À propos »), sur `[A-Za-z]:\\Users\\`, `\\Users\\`, sur l'adresse de build `F:\P153` ou `C:\vb` (SEC-16), et sur toute adresse e-mail autre que la noreply du PO.
- **Tiers** (Qt, FFmpeg, NVIDIA) : rapport seulement (les URL de documentation sont normales) ; signaler les noms d'hôtes qui ne sont ni de la documentation ni des certificats.

## 6. C5 — Modules chargés à l'exécution (test GPU local)

À la fin d'un traitement complet réel (VSR + interpolation + NVENC) dans `tests/gpu/`, le test énumère les modules du processus (`EnumProcessModulesEx` + `GetModuleFileNameW`) et écrit dans le rapport :
- les modules **réseau** présents (`ws2_32`, `mswsock`, `winhttp`, `wininet`, `urlmon`, `dnsapi`, `cryptnet`, `webio`) et, si possible, quel module les a chargés (pile capturée par un point d'arrêt conditionnel en session de diagnostic) ;
- le **chemin réel** de `nvcuda.dll`, `nvcuvid.dll`, `nvEncodeAPI64.dll`, `nvofapi64.dll`, `nvngx_vsr.dll`, du cœur NGX et des DLL TensorRT : les DLL pilote doivent venir de `System32` / `DriverStore`, les DLL de SDK du dossier de l'application (SEC-01, SEC-08) ; une DLL NGX chargée depuis un autre dossier (ex. dossier de mise à jour NGX) = **échec**.
Un module réseau chargé n'est pas une preuve de connexion : il déclenche une enquête et le test C6.

## 7. C6 — Test dynamique pare-feu et audit WFP (local, droits administrateur, fait par le PO ou QA)

Script fourni par devops : `scripts/test-no-network-dynamic.ps1` (doit s'exécuter en administrateur, refuse sinon ; **restaure toujours** l'état initial dans un `finally`).

1. Activer l'audit de la plateforme de filtrage : `auditpol /set /subcategory:"Filtering Platform Connection" /success:enable /failure:enable` (mémoriser l'état précédent avec `auditpol /get` pour le restaurer).
2. Créer une règle sortante bloquante ciblée : `New-NetFirewallRule -DisplayName "Enhance-ZeroNet-Test" -Direction Outbound -Program "<dossier>\Enhance.exe" -Action Block -Profile Any` (et la même en entrée).
3. Noter l'heure, lancer `Enhance.exe` sur une file de 3 clips **locaux** (dont un avec sous-titres, un nécessitant l'interpolation), premier lancement à cache TensorRT vide (construction du moteur), puis un second lancement ; ouvrir « À propos », changer de thème, quitter.
4. Pendant l'exécution, échantillonner toutes les 250 ms `Get-NetTCPConnection -OwningProcess <pid>` et `Get-NetUDPEndpoint -OwningProcess <pid>`.
5. Lire le journal Sécurité : événements **5156** (connexion autorisée), **5157** (connexion bloquée), **5158**/**5159** (liaison de port) dont le champ « Application Name » est `Enhance.exe`, depuis l'heure notée.
6. **Critères** : 0 événement 5156/5157/5158/5159 pour Enhance, 0 point de terminaison TCP/UDP, traitement réussi et sorties identiques (taille ± 0, même empreinte de flux vidéo) à celles d'une exécution sans règle.
7. Nettoyage : suppression de la règle, restauration d'`auditpol`.

## 8. C7 — Test hors ligne et capture globale (local, avant release ; obligatoire dans ENH-001 pour SEC-01)

- **M1 — hors ligne fonctionnel** : désactiver toutes les cartes réseau (mode avion), effacer le cache TensorRT, lancer une file complète : tout doit fonctionner, aucun message d'erreur, aucun délai anormal (un délai de 15–30 s au premier appel NGX ou à la vérification de signature trahirait une tentative réseau qui expire).
- **M2 — capture globale pendant l'initialisation NVIDIA** : réseau **actif**, capture `pktmon start --capture --comp nics --pkt-size 0` (outil Windows intégré) pendant 60 s **au repos** (ligne de base), puis pendant 60 s couvrant le **premier** `NVSDK_NGX_CUDA_Init` + création de la fonction VSR + construction TensorRT ; convertir (`pktmon etl2pcap`) et comparer les destinations (DNS demandés, hôtes contactés) entre les deux fenêtres. Toute destination NVIDIA (`*.nvidia.com`, `*.nvidiagrid.net`, CDN associés) ou d'autorité de certification (`*.digicert.com`) **apparue dans la fenêtre NGX** est rapportée à secdevops avec le processus émetteur (`Get-NetTCPConnection` / ETW `Microsoft-Windows-TCPIP`) → décision SEC-01 / SEC-07.
- **M3 — origine de la DLL VSR** : après M2, vérifier par C5 que `nvngx_vsr.dll` chargée est bien celle du dossier de l'application, et qu'aucun fichier n'est apparu dans les dossiers NGX du système (`%ProgramData%\NVIDIA\NGX\` **[H]** emplacement à confirmer) pendant le test.

## 9. Ce qui fait échouer une story

Toute violation de C1, C2, C3 (hors exception valide), C4 sur nos binaires, ou de C6/C7 = **BLOCK** secdevops. C5 seul = enquête, pas de blocage. Le rapport de chaque exécution est joint au rapport de story (`docs/security/sprint-N.md`).

## 10. Ce que devops doit écrire

| Fichier | Contenu |
|---|---|
| `scripts/check-no-network.ps1 -Path <dossier> [-Sources <racine>] [-Report <fichier>]` | C1 (si `-Sources`), C2, C3 (imports + imports différés, au niveau des fonctions), C4 ; lit `scripts/network-allowlist.json` ; code de sortie 0 = OK, 1 = violation, 2 = outil manquant (`dumpbin` introuvable) ; rapport Markdown sans chemin personnel |
| `scripts/test-no-network-dynamic.ps1` | C6 (administrateur, restauration garantie) |
| `tests/gpu/loaded_modules_test.cpp` (avec le gpu-engineer) | C5 |
| Job CI `zero-network` | C1 sur le dépôt + C2/C3/C4 sur le build `ci` (sans Qt ni GPU) ; voir `ci-securite.md` |
| `package.ps1` | Appel de `check-no-network.ps1` sur le dossier portable avant zippage (échec = pas de ZIP) |
