# Enhance — Décisions du PO

Une ligne par décision, numérotation conservée. Texte repris de l'ancien `docs/backlog.md` (n° 1 à 25) ; n° 26 à 28 ajoutées le 2026-10-29.

Décisions n° 1 à 15 : **2026-10-21** (validation de la vision `vision.md`, de ce backlog v0.2, de la stack ADR-001 Qt 6 / Qt Quick et de la direction visuelle). Décisions n° 16 à 19 : **2026-10-23** ; n° 20 à 25 : **2026-10-24**. Numérotation continue, sans doublon ; les renvois « décision n° N » des autres documents restent valables.

1. **Cadence** : on n'interpole jamais au-delà de 60 i/s ; une source >= 50 i/s n'est pas interpolée ; 24 i/s -> vrai 60 i/s interpolé (ENH-012). **Précision** : toute source strictement sous 50 i/s (y compris 31-49 i/s) est interpolée vers 60 i/s.
2. **Conteneur MVP** : MP4 ; bascule automatique en MKV, avec message, si une piste de sous-titres est incompatible MP4 (ENH-013, ENH-024).
3. **Clips de référence** : clips libres (Blender, Xiph) + 1 ou 2 extraits des archives du PO, jamais commités (ENH-005, ENH-029).
4. **Publication** : gratuite, code privé pour l'instant ; FFmpeg LGPL en liaison dynamique (ENH-006, ENH-030).
5. **Si la 5070 ne tient pas 60 i/s** : qualité d'abord, vitesse mesurée et affichée ; interpolation dégradée seulement dans « Rapide » (ENH-002, ENH-012, ENH-018, ENH-020). *Le spike montre que la 5070 tient largement 60 i/s en Équilibré (103-112 i/s) ; la clause reste valable pour « Qualité max » (36,5 i/s en HEVC).*
6. **Quitter pendant un traitement** : seule exception à « pas de confirmation ». La croix de la fenêtre réduit l'app dans la barre des tâches sans arrêter le traitement ; Ctrl+Q ouvre une petite feuille « Quitter et abandonner le traitement en cours ? » (ENH-021).
7. **Toolchain** : Visual Studio 2026 (MSVC v145) remplace VS 2022 ; CUDA Toolkit 13.4 accepte ce compilateur hôte **sans** `-allow-unsupported-compiler` (confirmé par le spike).
8. **Pause** : la VRAM reste réservée pendant une pause pour une reprise instantanée (MVP) ; libérer la carte pendant une pause viendra avec la reprise après plantage (ENH-021, ENH-038).
9. **Sous-titres** : SRT/WebVTT convertis en `mov_text` pour rester en MP4 ; bascule MKV seulement pour ASS/PGS/VobSub, pièces jointes ou audio non MP4 (ENH-013).
10. **Préparation des modèles d'interpolation** (construction du moteur TensorRT) : au premier traitement, avec un message affiché à l'utilisateur, pas à l'installation ; mis en cache ensuite (ENH-044). *Mesuré : ~85 s à vide, jusqu'à 2,5 min si le PC est chargé.*
11. **Direction visuelle validée** : maquettes `docs/design/ui/maquettes/`, accent **Iris**, fond **Mica + verre**, police **Segoe UI Variable** (système, non livrée), icônes **Lucide (ISC)** (`design-system.md`). La ligne « Jeu d'icônes » du registre des licences est à passer de « À CONFIRMER » à « OK » par secdevops (tâche du Sprint 1).
12. **Outil local Python** : **Python 3.12 + PyTorch (roues CPU suffisantes)** approuvé **comme outil local** pour exporter RIFE en ONNX depuis les poids **MIT** de Practical-RIFE. **Export maison obligatoire** ; l'ONNX de vs-mlrt (dépôt GPL-3.0) est **écarté de la livraison** (reste local, comparaison). Le PO a relu la licence MIT des poids et **accepte le risque résiduel Vimeo-90K** (SEC-03, `licences-composants.md` §7.2). Rien de Python n'est livré (ENH-043).
13. **Comparaison des modèles d'interpolation** : RIFE 4.25 comparé à FILM, IFRNet et EMA-VFI (qualité, vitesse 720p TensorRT, licence des poids) pour départager « Qualité max » et disposer d'un repli licence (ENH-045).
14. **Seuils de qualité en part de la marge restante** (remplace « +5 VMAF ») : écart VMAF vs Lanczos >= **50 %** de (100 - VMAF Lanczos) en **Qualité max**, >= **35 %** en **Équilibré**, >= **20 %** en **Rapide** (voir « Seuils QA de référence » ci-dessous). Raison : les sources libres sont déjà douces, Lanczos y atteint 92-96 de VMAF (`docs/qa/resultats/2026-10-21_reference-lanczos.md`), +5 y est arithmétiquement impossible.
15. **Git** : aucune mention de Claude dans les commits ni les PR ; aucun push sans l'accord du PO.
16. **Test UNC d'ENH-008 reporté à ENH-015** (décision du 2026-10-23) : `Win32File.PartageUnc` (`ENH_TEST_UNC_DIR`), non exécuté faute de partage accessible, est reporté à ENH-015 ; le point UNC du critère AVIO d'ENH-008 est « non vérifié — reporté à ENH-015 ».
17. **Barre de titre et accentuation Windows** (2026-10-23) : la barre de titre suit la couleur d'accentuation Windows choisie par l'utilisateur (`ColorPrevalence`).
18. **Opacité du verre sur fond vif** (2026-10-23) : réglée à ENH-016 sur du vrai contenu ; piste : teinte allégée dans le biseau.
19. **Aucune vraie frappe clavier / clic sur le poste du PO** (2026-10-23) : aucun agent n'envoie de vraies frappes clavier ni de clics sur le poste du PO (incident QA d'ENH-004) ; les tests clavier sont automatisés ou faits par le PO.
20. **Qualité de la super-résolution et seuil de débit** (2026-10-24) : ENH-040 validée sur le périmètre du pont, critère de qualité VSR déplacé dans ENH-009 ; **ENH-053 créée** : spike « niveaux VSR 1 à 4 sur jeu B compressé + alternative Real-ESRGAN via TensorRT », dont ENH-009 dépend ; **seuil de 300 i/s en q4 8 bits ramené à un avertissement** (10 bits >= 300 maintenu ; le goulot de la chaîne est NVENC, ~105 i/s) (voir « Mouvements v0.5 »).
21. **Traçabilité des commits et équipe** (2026-10-24) : chaque commit porte une ligne `Agent: <rôle>` (rôle de l'agent auteur du changement) ; le README comporte une section « Équipe ». Cette ligne n'est pas une mention de Claude (décision n° 15 inchangée).
22. **Poids RIFE** (2026-10-24) : l'équipe télécharge elle-même `flownet.pkl` (RIFE 4.25, Practical-RIFE, MIT) au sprint 3, SHA-256 consigné, README + `LICENSE` archivés, jamais commités (ENH-043).
23. **WER** (2026-10-24) : Enhance exclu des rapports d'erreurs Windows, aucun vidage mémoire ni envoi ; en cas de plantage, une ligne dans le journal local avec seulement un code d'erreur, sans chemin ni contenu (ENH-025).
24. **Seuils SSIM / PSNR validés** (2026-10-24) : PSNR +3,0 / +1,9 / +1,0 dB ; (1 - SSIM) x 0,5 / 0,65 / 0,8 vs Lanczos (Qualité max / Équilibré / Rapide) ; VMAF-NEG : part - 10 pts ; réajustables après le spike ENH-053.
25. **Notification de fin de file** (2026-10-24) : au MVP, clignotement de l'icône dans la barre des tâches + progression sur l'icône + résumé dans l'app + son discret désactivable ; toast Windows quand l'installeur existera (ENH-019, ENH-033).
26. **Budget de sprint** (2026-10-29) : le budget d'un sprint = 100 % du quota hebdomadaire de l'abonnement.
27. **Électricité** (2026-10-29) : coût de l'électricité compté pour 0 EUR.
28. **Relevé de départ** (2026-10-29) : relevé `/usage` de départ = 82 % du quota hebdomadaire consommé le 2026-10-29.
