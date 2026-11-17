# Publier Sillage 2.2 beta sur GitHub

1. Créer un dépôt public `sillage-fm1`. Copier **le contenu de GitHub/** à sa racine, y compris les fichiers cachés, puis pousser sur `main`.
2. Dans Settings → Pages, choisir **Deploy from a branch**, puis `main` et `/docs`. L’accueil sera sur `https://TON-COMPTE.github.io/sillage-fm1/` et l’éditeur sur `…/editor.html`.
3. Créer une préversion **Sillage 2.2 beta** avec le tag `v2.2.0-beta.1` sur le commit publié. Joindre `Sillage-2.2-beta.fwsc`, le ZIP à partager et `SHA256SUMS.txt`. Les sources exactes sont déjà dans le dépôt au tag.
4. Pour The Firmware Hub, ajouter les URL réelles de l’éditeur et de la démo au `fmhub.json`. Ce paquet garde l’identité bêta `FM-1_700`. Avant de le soumettre au Hub, choisir une identité propre à Sillage, reconstruire le firmware et son installeur, puis valider le paquet exact sur matériel, y compris le retour vers un autre firmware.

Le nom Sillage 2.2 beta désigne cette distribution. L’identifiant USB et la chaîne de version interne héritée sont conservés dans cette compilation bêta. Pour identifier précisément le paquet lors d’un test, utilisez son nom et le SHA-256 fourni.

[Soumission au Firmware Hub](https://thefirmwarehub.com/submit/)
