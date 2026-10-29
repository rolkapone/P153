### ENH-008b — Matrice de formats étendue  [GPU] [Must | 3 pts]
En tant qu'utilisateur, je veux que toutes les variantes courantes de fichiers vidéo s'ouvrent afin de ne jamais convertir au préalable.
Critères d'acceptation :
- [ ] Étant donné le **reste de la matrice valide** (MP4 x VP9 / AV1 / MPEG-2, MKV x H.264 / HEVC / VP9 / MPEG-2, MOV x HEVC, MPEG-TS x H.264 / HEVC / AV1 [à vérifier], WebM x AV1, AVI x MPEG-2), quand on décode, alors nombre d'images = `ffprobe -count_frames` et chemin attendu (NVDEC ou repli journalisé).
- [ ] Variantes 8 / 10 bits (dont H.264 10 bits : repli attendu, à vérifier sur Blackwell), **dimensions impaires**, **fréquence variable (VFR)**, **rotation** (métadonnée), changement de résolution en cours de flux (bascule logicielle au vol journalisée), entrelacé : aucun plantage, comportement documenté.
- [ ] Aucune régression sur ENH-008 : seuils inchangés (>= 1 000 i/s sur `BBB-FAST.mp4`, VRAM <= 600 Mio, dérive <= 50 Mio sur 10 fichiers) ; **pas de baisse > 5 %** par rapport à la mesure de `main`.
- [ ] SEC-13 : dimensions impaires, 1x1 et 8192x4320 sans débordement.
- [ ] **Seuils QA** (`docs/qa/sprint-1-seuils.md` §3-§5) : mêmes règles de mesure que ENH-008 ; une combinaison dont l'encodeur de génération est absent du FFmpeg de mesure est notée « non générée » et le critère « non vérifié » (jamais PASS par défaut).
- [ ] **Petite tâche (suite d'ENH-008)** : l'échec du préchargement de `nvcuvid.dll` est **journalisé** (niveau avertissement, code d'erreur Windows, sans chemin personnel) ; test unitaire avec un chargeur factice (sans GPU).
Notes : dépend d'ENH-008 ; **doit passer avant ENH-048** (qui rejoue la matrice) ; au plus tard Sprint 3. Corpus généré par `tests/gpu/corpus/make-corpus.ps1`.
