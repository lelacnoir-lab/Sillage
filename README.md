# Sillage

![Sillage](docs/sillage-card.jpg)

Groovebox à huit pistes pour le M-VAVE FM-1, basée sur **Skiff et Felucca**. 448 presets classés, séquenceur 64 pas, mode SONG, mixeur live, égaliseur quatre bandes par piste et traitement master.

**[▶ Try in your browser / Essayer dans le navigateur](https://lelacnoir-lab.github.io/Sillage/demo.html)** · **[Installer Sillage 2.2.1](https://lelacnoir-lab.github.io/Sillage/installer.html)** · **[Ouvrir l’éditeur web](https://lelacnoir-lab.github.io/Sillage/editor.html)** · **[Téléchargements](https://github.com/lelacnoir-lab/Sillage/releases)**

## Version 2.2.1

Cette version conserve le traitement audio de la base 2.2 validée sur FM-1 et ajoute l’appui long sur FX pour verrouiller les effets live. Le filtre agit sur le signal direct, indépendamment des retours de réverbération et de delay. Les réverbes d’origine sont conservées ; les corrections plus lourdes expérimentées ensuite ne sont pas reprises.

Le firmware publié est exactement le fichier validé sur matériel, SHA-256 `8bba5cbb0e42a0a50f5d4819b482f0132a66fbbc06c04298cbca56c66a476e36`, identifiant USB `FM-1_946221`. Son menu de version peut afficher **Sillage 2.2.1 FX test** : aucun nouveau binaire n’a été recompilé pour changer ce libellé.

## Éditeur web

Dans Chrome ou Edge sur ordinateur, brancher le FM-1 en USB, ouvrir l’éditeur et cliquer sur **Connecter**. Autoriser le MIDI / SysEx. L’éditeur n’installe aucun firmware automatiquement.

- Réglages agrandis et regroupés par modules, selon les paramètres disponibles sur le FM-1.
- Presets d’usine et bibliothèque personnelle par familles, recherche et filtre par moteur. Les presets d’usine sont lus sur l’appareil ; les familles personnelles sont conservées dans les exports JSON.
- Comparaison temporaire **A/B** : prendre A, modifier B, puis basculer. Volume, panoramique, mutes, groove, séquence et bus globaux restent en place. Pas de sauvegarde automatique.
- Image Sillage du Hub, présentation simplifiée et bouton de langue japonais retiré.

Ces améliorations de l’éditeur n’ajoutent aucun traitement audio au firmware.

## English

An eight-track FM-1 groovebox based on Skiff and Felucca. Version 2.2.1 retains the validated 2.2 audio/reverb processing, adds long-press FX latching and isolates the live filter from reverb/delay returns. The web editor adds larger grouped controls, family/engine search and temporary A/B sound comparison, including FM6 patches and Drum-X kit instruments. It does not automatically save to the FM-1 or install firmware.

## Guides et sources

- [Prise en main](Prise-en-main.md)
- [Manuel de référence](Manuel-de-reference.md)
- [Mode d’emploi de l’éditeur](docs/editor-guide.txt)
- [Notes de version 2.2.1](RELEASE_NOTES_2.2.1.md)
- [Licence](LICENSE) et [provenance des composants et samples](LICENSING.md)

Dérivé de [Skiff, par illuminus5](https://github.com/illuminus5/skiff), lui-même basé sur Felucca par Leo Kuroshita (Hügelton Instruments). Les auteurs et licences des polices, sons et composants restent indiqués dans les sources et notices.

## Recompiler

Sous Linux x86-64, installer le compilateur JieLi indiqué dans BUILDING.md, définir JIELI_TOOLCHAIN puis lancer `bash recompiler-fm1.sh`. Le résultat est `build/felucca.fwsc`. Le dossier `generated/` contient les en-têtes de cette version. Les sources des modules WebAssembly intégrés à l’éditeur sont documentées dans [web/embedded-module-sources](web/embedded-module-sources/README.txt).
