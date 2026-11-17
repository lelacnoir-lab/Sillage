# Sillage 2.2 bêta

![Sillage 2.2 bêta — triangle blanc sur fond bleu nuit](docs/images/sillage-banniere.png)

Groovebox à huit pistes pour le M-VAVE FM-1. 448 presets classés, séquenceur, mode SONG, scènes et mode PERFORM, mixeur live et éditeur USB.

**[Télécharger le firmware Sillage 2.2 bêta (.fwsc)](https://github.com/lelacnoir-lab/Sillage/raw/refs/heads/main/downloads/Sillage-2.2-beta.fwsc)**

Cette bêta reprend le fichier installable d'origine pour le M-VAVE FM-1. Les corrections d'affichage apportées au code depuis ce build seront intégrées lors d'une prochaine compilation.

- [Prise en main](Prise-en-main.md)
- [Manuel de référence](Manuel-de-reference.md)
- [Publication GitHub Pages](MISE-EN-LIGNE-GITHUB.md)

Firmware dérivé de Skiff par illuminus5 et de Felucca par Leo Kuroshita (Hügelton Instruments). Voir [licence](LICENSE), [provenance et licences des composants](LICENSING.md) et [projet Skiff](https://github.com/illuminus5/skiff).

## Recompiler

Sous Linux x86-64, installez le compilateur JieLi indiqué dans BUILDING.md, définissez JIELI_TOOLCHAIN puis lancez `bash recompiler-fm1.sh`. Le résultat est dans `build/felucca.fwsc`. Le répertoire `generated/` contient les en-têtes de ce paquet.
