# Publier Sillage V3 — procédure après validation

## Préparation locale

1. Relire README, RELEASE_NOTES_V3.md, Manuel-de-reference.md et le guide PDF anglais.
2. Ouvrir docs/index.html, docs/installer.html et docs/demo.html en local. Aucune installation ne démarre sans clic, autorisation MIDI et confirmation.
3. Vérifier `python3 tools/package_v3_release.py`. Le script vérifie le binaire et les sources épinglées ; il ne recompile pas.
4. Consigner le retour final du mainteneur sur FM-1 : EVOLVE sans clignotement, notes/CPU en charge, transposition, premier kick au retour FX, FILL, sauvegarde et rechargement des sons/projets. Ne pas présenter les tests web comme une validation matérielle.

## GitHub — uniquement après accord explicite

5. Examiner le diff et envoyer les sources V3, y compris les nouveaux fichiers non suivis, les en-têtes generated/, les notices et les pages docs/. Les .fwsc sont ignorés par défaut : ajouter explicitement downloads/Sillage.fwsc avec `git add -f` si nécessaire. Ne pas envoyer sdk/, build/, dist/, données privées ou dossiers de travail.
6. Les anciens workflows sont désormais manuels et ne se déclenchent plus sur push. Aucun ancien workflow 2.2 ne doit être lancé pour V3.
7. Le workflow `Build Sillage FM-1 candidate` compile et vérifie un candidat V3 sans publier ; il nécessite une source SDK accessible au runner. Les dépendances externes restent les mêmes que la compilation locale.
8. Le workflow `Publish Sillage V3` se déclenche sur main lorsque son propre fichier change, ou manuellement avec la confirmation `SILLAGE V3`. Ce déclenchement a été activé pour la publication V3 explicitement autorisée par le mainteneur. Il vérifie les fichiers, prépare le ZIP et publie v3.0.0 sans remplacer une release existante.
9. Après exécution, examiner la release et consigner les retours matériels complémentaires si disponibles. Ne pas modifier les anciennes releases. L'identifiant interne reste FM-1_946301 et l'écran ABOUT conserve V3 beta 1.

## Pages et Firmware Hub

Les fichiers docs/ sont préparés pour remplacer les pages sur la même adresse GitHub Pages. Le moment de leur mise en ligne dépend de la configuration Pages du dépôt : un push sur la branche servie peut suffire. Examiner cette configuration avant l'envoi ; pour une revue distante sans toucher aux pages publiques, utiliser une branche de préparation.

Les champs image/editor/play et les URL existantes de fmhub.json sont conservés ; seul le texte descriptif V3 est actualisé. Une fois la release et Pages publiées, vérifier leurs liens et leur version depuis un navigateur. Ne pas promettre un délai automatique de synchronisation ni l'absence de contrôle du Firmware Hub : son traitement reste externe au dépôt.

Rien n'est publié par la seule préparation locale de ces fichiers.
