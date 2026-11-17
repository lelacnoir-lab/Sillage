# Sillage — Prise en main

Sillage est une groovebox à huit pistes pour le M‑VAVE FM‑1, dérivée de Skiff et de Felucca. Chaque piste peut accueillir un synthé ou un kit de batterie. Les 448 presets usine sont classés par famille. Le séquenceur, qui va jusqu’à 64 pas par piste, sert à construire le morceau et PERFORM permet de jouer les patterns, les scènes et les mutes en direct.

Ce guide décrit Sillage 2.2 beta. La mesure de charge audio et le transfert de samples utilisateur restent à vérifier sur le FM‑1 physique.

## Partie 1 — Prise en main rapide

1. **Choisir une piste et un son.** Sur HOME, tourne ALGORITHM pour sélectionner T1 à T8. Tourne PRESETS pour parcourir les sons, regroupés par famille. Pour accéder directement à une famille, maintiens EDIT et utilise les touches indiquées.
2. **Faire une boucle.** Ouvre SEQ → NOTES. Sur une piste mélodique, sélectionne un pas et joue une touche pour ajouter ou retirer la note correspondante. Sur une batterie, les touches blanches sélectionnent les pas de la lane. Lance la lecture avec PLAY et utilise REC pour enregistrer en jouant.
3. **Ajuster le son.** EDIT donne accès aux paramètres du moteur. ENV règle les enveloppes. FX applique les effets, et GLO → MIXER rassemble niveau, panoramique et envois de la piste.
4. **Jouer en live.** Maintiens ARP jusqu’à l’ouverture de PERFORM, puis relâche. En KEYS TRACK, les blanches lancent les patterns de la piste sélectionnée. En KEYS SCENE, elles rappellent les scènes du morceau.
5. **Sauvegarder.** Pour garder le morceau, va dans SAVE → PROJECT. Pour réutiliser seulement le son, va dans SAVE → USER.

## Copier une mesure dans un pattern

Arrête la lecture, sélectionne la piste et ouvre **SEQ → BAR**. Avec **K1**, choisis la mesure 1. Avec **K2**, choisis **COPY**, puis appuie sur **OCT+**. Passe à **PASTE** avec **K2**, choisis la mesure 3 avec **K3**, puis appuie deux fois sur **OCT+**. La mesure 3 doit déjà exister. Son contenu est remplacé.

Sur la même page, **DUPLICATE** insère une copie juste après la mesure sélectionnée si le pattern a assez de place. **CLEAR** vide la mesure en conservant sa place. Pour les détails, consulte [le manuel de référence](Manuel-de-reference.md#seq-rec-et-song).

## Donner plus de niveau et de largeur au mix

Ouvre **GLO → FINAL MASTER**, active **MASTER FX** avec **K4**, puis règle **LOUD** (K1) pour le gain et **WIDTH** (K3) pour la largeur stéréo. **CEILING** (K2) règle le plafond des crêtes.
