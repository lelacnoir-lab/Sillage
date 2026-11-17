# Sillage 2.2 beta — Manuel de référence


### Commandes de façade

| Commande | Utilisation |
| --- | --- |
| HOME | Affiche les huit pistes. Rappuie sur HOME pour passer à la vue spectre. |
| ALGORITHM | Sélectionne la piste. Les pages de piste et les potards suivent cette sélection. |
| SELECT | Parcourt les pages de la section ouverte. En PERFORM, règle le moment de lancement. |
| PRESETS | Parcourt les sons sur HOME. Sa fonction dépend de la page, par exemple le pas dans NOTES ou TRACK / SCENE dans PERFORM. |
| KNOB 1 à 4 | Modifient les quatre paramètres ou actions affichés à l’écran. |
| OCT− / OCT+ | Changent l’octave en jeu. Sur les pages d’action, OCT+ valide l’action affichée et OCT− permet de revenir. |
| PLAY / REC | Démarrent ou arrêtent la lecture et pilotent l’enregistrement. |

### HOME et choix des sons

HOME montre le son et la séquence de chaque piste. La piste sélectionnée affiche sa famille de son. Les pistes de batterie indiquent DRUM ou DRUM‑X. La vue spectre trace les fréquences et laisse brièvement une marque aux crêtes. À gauche du triangle, **V** indique les voix occupées et le budget disponible. À droite, **CPU** représente le temps consacré au calcul audio et son pic sur le matériel. L’aperçu Web affiche **CPU --**.

Les familles couvrent les basses, leads, nappes, claviers, plucks, batteries et effets. Parmi les moteurs du sélecteur, **ANALOG** sert à la synthèse analogique virtuelle, **TRIO** aux timbres à plusieurs oscillateurs, **FM6** à la synthèse FM, **PHASE** au travail de la phase, **VOICE** aux formants et **GRAIN** aux textures granulaires. **SAMPLE** joue des sons échantillonnés, **SLICE** joue leurs découpes, et **DRUM / DRUM‑X** servent aux kits. Les pages EDIT s’adaptent au moteur chargé.

### EDIT, ENV, LFO et SEL

- **EDIT 1 / EDIT 2** donnent accès aux réglages propres au moteur. Selon le son, tu peux aussi trouver des pages d’opérateurs FM, de lanes de batterie ou de découpes de samples.
- **VOICE** règle le mode de jeu, le glide et la priorité. **VOICE 2** propose ALLOC (rotation ou réutilisation des voix), DETUNE (désaccord en unison) et SPREAD (largeur stéréo).
- **PRESET INIT** se trouve à la fin d’EDIT. Choisis le moteur sur K1, puis appuie sur EDIT pour ouvrir les pages du son initialisé. Enregistre-le ensuite dans SAVE → USER.
- **ENV 1** règle l’évolution du niveau de la note. Attack fixe le temps de montée, Decay la descente vers le niveau Sustain, et Release l’extinction après la fin de la note.
- **ENV 2** fournit une seconde courbe indépendante. Pour ouvrir le filtre rapidement sur une nappe à attaque lente, va dans **LFO → MOD**, choisis un routage avec K1, **ENV 2** comme source, **CUTOFF** comme destination, puis règle la quantité.
- **LFO / LFO 2** règlent la vitesse, la forme, la phase, le fondu et les options de synchronisation ou de déclenchement. **MOD** relie une source de modulation à un paramètre.
- **SEL** ouvre SCALE, CHORD et FOLLOW. SCALE définit la gamme et sa transposition, CHORD configure les accords et le strum, et FOLLOW règle le suivi entre pistes. La quantification de hauteur dans SCALE concerne les notes jouées. La quantisation de REC concerne leur placement dans le temps.

### SEQ, REC et SONG

| Page | Utilisation |
| --- | --- |
| **NOTES** | Piste mélodique (hauteur, position, durée, vélocité) ou grille de batterie (lane, frappe, accent, vélocité). PRESETS déplace la sélection dans la grille mélodique. |
| **BAR** | Édite une mesure de la piste sélectionnée. Voir les commandes ci-dessous. |
| **FEEL** | Règle le comportement des pas, dont la probabilité et les répétitions ou flams selon la grille et le type de piste. |
| **PATTERN** | K1 longueur, K2 division, K3 swing, K4 gate. PRESETS règle le sens de lecture. |
| **GROOVE** | K1 Humanize, K2 Accent, K3 Timing, K4 Velocity. Les réglages sont propres à la piste. |
| **GROOVE 2** | K1 ajoute des variations de decay. |
| **AUTOMATION** | Consulte et édite les changements de paramètres enregistrés. Pour un verrou de paramètre, tiens un pas et tourne un potard compatible. |
| **SONG** | Compose une liste d’entrées par piste. K1 sélectionne l’entrée, K2 choisit son contenu, K3 règle les répétitions et K4 règle le mode SONG. OCT+ valide la sélection affichée. ALGORITHM accède aux huit pistes. |

Double-clique sur **REC** pour ouvrir les options d’enregistrement. La première page rassemble le métronome, le précompte et le niveau du clic. Avec SELECT, passe à **QUANTIZE**. **K4** sélectionne T1 à T8, **K1** choisit la grille et **K2** active ou désactive la quantisation. **TRACK** reprend la division de la piste. **D** signifie pointé et **T** triolet. La grille va de la blanche au trente-deuxième, avec des variantes pointées et triolets jusqu’au seizième.

**Éditer une mesure.** À l’arrêt, ouvre SEQ → BAR. K1 choisit la mesure, K2 l’action et K3 la destination pour PASTE. L’écran indique la piste et les pas concernés. OCT+ applique l’action. Pour PASTE, CLEAR et DELETE, un deuxième OCT+ confirme et OCT− annule. DUPLICATE insère une copie, CLEAR vide la mesure et DELETE la retire. Les verrous et l’automation suivent l’édition d’une piste ou d’un kit entier. Sur DRUM et DrumX, K4 choisit KIT ou une lane. Sur une lane, l’édition porte sur les frappes et les accents. Pour modifier un pattern partagé, crée d’abord une copie avec COPY+ dans SONG.

**Effacer une piste.** Sélectionne la piste avec ALGORITHM, maintiens SEQ et utilise **CLEAR** dans SEQ TOOLS.

**Effacer le pattern sur les huit pistes.** Arrête la lecture, place-toi sur l’emplacement voulu, puis maintiens **SEQ + G5** environ une seconde. L’écran indique la lettre concernée. Relâche avant la fin de l’appui pour annuler.

### FX et mixeur GLO

| Page FX | Réglages |
| --- | --- |
| **FX MIX** | K1 distorsion, K2 chorus, K3 envoi delay, K4 envoi réverb. |
| **REVERB 2** | Tonalité et coupe-bas de l’envoi réverb de la piste. |
| **DRIVE** | Quantité, type et tonalité de la distorsion. |
| **INSERT FX** | K1 choisit l’effet de piste. Pour **Juno Chorus**, K2 sélectionne I ou II et K3 règle le dosage. |
| **SLICER** | Active le découpage rythmique et règle le motif, la vitesse et la profondeur. Si une piste est hachée, vérifie cette page. |
| **DUCK** | Choisis la piste de déclenchement, la profondeur et le relâchement. Exemple classique avec la basse comme piste traitée et le kick comme source. |
| **EQ GAIN / EQ FREQ** | K1 à K4 correspondent aux graves, bas‑médiums, hauts‑médiums et aigus. La première page règle les gains et la seconde les fréquences. |
| **DELAY / REVERB** | K1 dose l’envoi de la piste. K2 à K4 agissent sur le bus partagé. Pour le delay, il s’agit du temps, du feedback et de la couleur. Pour la réverb, du type, de la taille et du decay. |

Dans **GLO → MIXER**, les quatre potards règlent **LEVEL, PAN, REV SEND et DLY SEND** de la piste sélectionnée. Dans les pages **BUS GLOBAL**, les réglages concernent l’effet partagé. La réverb propose **Room, Spring, Hall et Stadium**. Le pré‑delay se trouve sur la deuxième page de réverb globale. Le delay global a son propre dosage MIX. Le chorus global est également réglable depuis GLO.

En dernière page de GLO, **FINAL MASTER** traite le mix stéréo. K1 (LOUD) augmente le gain, K2 (CEILING) fixe le plafond des crêtes internes, K3 (WIDTH) élargit la stéréo et K4 (MASTER FX) active ou désactive le traitement. La jauge montre la réduction de gain. HOME affiche MASTER ON ou MASTER OFF. Le traitement est désactivé par défaut.

### MIDI

Maintiens HOME puis relâche pour ouvrir le menu. Choisis MIDI avec ALGORITHM et fais défiler les lignes avec PRESETS. Les huit entrées **T1 CHANNEL** à **T8 CHANNEL** permettent de régler les canaux. Par défaut, chaque piste utilise le canal du même numéro. AUTO CHANNEL peut réserver un canal pour jouer la piste sélectionnée.

### Mixeur live et PERFORM

**GLO maintenu** ouvre le mixeur live. Les huit paires vont de gauche à droite. Tiens une **blanche** pour isoler sa piste. Appuie brièvement sur la **noire à sa droite** pour couper ou réactiver cette même piste. En tenant la noire, tu la coupes temporairement. **E5 = ALL** réactive les pistes mutées et annule les solos. **F5 = FILL** active le fill pendant l’appui. **G5 = TAP** sert à taper le tempo. Les quatre potards contrôlent les volumes des quatre pistes affichées. Tourne ALGORITHM pour déplacer cette fenêtre de T1–T4 jusqu’à T5–T8.

**ARP maintenu** ouvre et verrouille PERFORM. **ARP + HOME** est un deuxième raccourci. Appuie brièvement sur HOME ou ARP pour fermer la vue. Un appui bref sur ARP depuis la vue normale ouvre les réglages d’arpégiateur de la piste. Les deux pages règlent notamment le mode, la division, les octaves, le gate, le swing, la probabilité et le maintien.

- **KEYS TRACK.** Tourne ALGORITHM pour sélectionner la piste. Les touches blanches correspondent aux emplacements **A à P**. La lettre mise en évidence indique le pattern actif. Le nom affiché est celui du son chargé.
- **KEYS SCENE.** Tourne PRESETS pour choisir SCENE. Maintiens SAVE et appuie sur une touche blanche pour mémoriser la combinaison d’entrées et de mutes. Appuie ensuite sur cette blanche pour la rappeler.
- **LAUNCH.** SELECT choisit **LOOP** (fin de boucle de la piste), **BAR** (prochaine mesure), **BEAT** (prochain temps) ou **NOW** (immédiat).
- **Potards.** K1 agit sur le timbre selon le moteur, K2 sur la réverb, K3 sur le delay, K4 sur le volume. Les valeurs restent en place quand tu relâches les commandes. **OCT+** rétablit sur la piste sélectionnée les valeurs qu’elle avait avant tes ajustements dans PERFORM.
- **Touches noires.** Elles commandent les mutes des huit pistes. Un appui bref programme la bascule au point LAUNCH. L’indication **BLACK: MUTE** rappelle leur rôle.

### Batterie, DrumX et samples

**DRUM** donne accès aux lanes du kit et à leurs paramètres. **DRUM‑X** permet de sculpter séparément les sons du kit. Dans **EDIT → SOUND**, sélectionne la lane en jouant sa touche et parcours ses pages. Dans **EDIT → LANE MIX**, sélectionne la lane à mixer. **K4 → FX** active ou coupe les effets de cet instrument. Quand FX est coupé, le pictogramme est barré.

Pour importer un kick, ouvre l’éditeur Sillage dans Chrome ou Edge, relie le FM‑1 en USB et autorise MIDI/SysEx. Dans **Samples**, sélectionne **USR1**, importe le fichier et lance son transfert. Le parcours jusqu’à USR1 a été testé en simulation. Le transfert USB et la lecture du sample après redémarrage restent à valider sur appareil.

### SAVE et nommage

- **SAVE → USER.** Arrête la lecture, choisis un emplacement avec **K1**, tourne **K4** pour sélectionner **SAVE**, puis valide avec **OCT+**. Donne un nom et valide avec **OCT+**. Un emplacement déjà occupé demande une confirmation. K2 donne accès au chargement et K3 à l’effacement ou au renommage selon l’action choisie.
- **SAVE → PROJECT.** Arrête la lecture, choisis l’emplacement avec **K1**, sélectionne **SAVE** avec **K4** puis valide avec **OCT+**. Dans l’écran de nommage, **OCT+** confirme et **OCT−** annule. Pour rouvrir un morceau, utilise l’action **LOAD** avec **K3**, puis valide.
- **SAVE → PATTERNS** donne accès à la réserve de patterns utilisée par SONG. Le mode PERFORM sert à rappeler leurs emplacements ou des scènes pendant le jeu.

## Glossaire

| Terme | Signification |
| --- | --- |
| **Piste** | Un son ou un kit, sa séquence et ses réglages. |
| **Preset** | Un son mémorisé avec ses paramètres. |
| **Pattern** | Une séquence de notes ou de frappes. A à P repèrent les emplacements de la piste. |
| **Scène** | Une combinaison d’entrées et de mutes rappelable en live. |
| **Voix / Unison** | Une voix est une unité de synthèse active. L’unison superpose plusieurs voix pour une note. |
| **Send / Bus / Insert** | Le send dose l’envoi d’une piste, le bus traite les envois partagés et l’insert agit directement sur une piste. |
| **Quantize / Humanize** | Quantize recale les notes sur la grille. Humanize introduit des variations de jeu. |
| **Flam / Ratchet / Probabilité** | Frappes rapprochées, répétitions dans un pas et chance qu’un événement se déclenche. |
| **Parameter lock** | Valeur d’un paramètre imposée sur un pas précis. |
| **Mute / Solo / Duck** | Couper une piste, l’isoler, ou baisser son niveau au déclenchement d’une autre piste. |
| **USR** | Emplacement de samples utilisateur. |
