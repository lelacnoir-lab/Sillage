# Sillage 2.2 bêta

![Sillage 2.2 bêta — triangle ambre sur fond bleu nuit](docs/images/sillage-banniere.png)

Groovebox à huit pistes pour le M-VAVE FM-1. 448 presets classés, séquenceur, mode SONG, scènes et mode PERFORM, mixeur live et éditeur USB.

- [Prise en main](Prise-en-main.md)
- [Manuel de référence](Manuel-de-reference.md)
- [Publication GitHub Pages](MISE-EN-LIGNE-GITHUB.md)

Firmware dérivé de Skiff par illuminus5 et de Felucca par Leo Kuroshita (Hügelton Instruments). Voir [licence](LICENSE), [provenance et licences des composants](LICENSING.md) et [projet Skiff](https://github.com/illuminus5/skiff).

## Recompiler

Sous Linux x86-64, installez le compilateur JieLi indiqué dans BUILDING.md, définissez JIELI_TOOLCHAIN puis lancez `bash recompiler-fm1.sh`. Le résultat est dans `build/felucca.fwsc`. Le répertoire `generated/` contient les en-têtes de ce paquet.
