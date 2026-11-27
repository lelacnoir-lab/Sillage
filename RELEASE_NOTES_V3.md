# Sillage V3 — Release notes / Notes de version

## English

Sillage by T-TAURI is an eight-track groovebox for M-VAVE FM-1, based on Skiff and Felucca. It retains 64 steps per track and 448 factory presets organized by instrument family.

### New in V3

- **EVOLVE:** progressively add scale-aware notes into empty steps without replacing the original phrase. K1 sets the amount, K3 selects the octave of future additions (-1, 0, +1), K4 sets their length (5–100% of one step). Changes are committed at the next loop start during playback. OCT- resets the selected track; OCT+ resets all. Drum and mapped-key engines are excluded. The overlay is temporary and is not saved in projects.
- **Sound Design:** choose an engine in EDIT → PRESET INIT, then press EDIT. Dedicated blue pages identify the creation workflow. A star marks a modified sound. SAVE SOUND ends the workflow and OCT+ opens user-slot selection and naming. Leaving EDIT restores the normal section colour.
- **Live transpose:** hold ENV to open the panel. K1/ALGORITHM selects the track, K2 includes/excludes it, K3 selects NOW, 1/8, 1/4, 1/2 or BAR. Play the keyboard to request a change; F4 is zero. A queued key flashes until applied. Drum engines are excluded. Leaving the panel retains the transposition; it is not stored in projects.
- **Quantized FX HOLD:** long-press FX latches the live page. K2 replaces CRUSH with timing for both start and stop: NOW, 1/8, 1/4, 1/2, 1 BAR, 2 BAR. The live filter acts on the dry signal independently of delay/reverb returns. Quantized release restores the dry first beat rather than delaying it by a full grid.
- **Direct FILL:** the last E key (E5) in FX HOLD replaces VINYL. Hold it to call the existing Fill, release to stop; it never latches. Prepare conditional steps in SEQ → FEEL → CHANCE above 100%. Conditions still apply to an entire step, including simultaneous drum hits; no per-drum-lane Fill condition was added.
- **EVOLVE display fix:** removed the full-screen blank during knob redraws, without allocating another framebuffer.

### Existing features retained

Eight freely assignable tracks; up to 64 steps per track; family-ordered sound browsing; editable DRUM/DRUM-X; bar copy/paste/duplicate/clear/delete; automation and parameter locks; per-track SONG chains; groove and decay variation; mutes, solos and Performance Loop; track four-band EQ; Juno Chorus I/II; shared reverb/delay/chorus buses; ping-pong delay; Final Master with stereo width; web editor with grouped controls, library and temporary A/B comparison. Existing MIDI CC and MIDI Learn remain; the editor imports DX7 single-voice/32-voice SysEx files for choosing and saving individual FM6 sounds.

### Scope and compatibility

- Target: **M-VAVE FM-1**. No V3 hardware validation is claimed for SMK-37 or other models.
- Public name: **Sillage V3**. The unchanged binary reports **Sillage V3 beta 1**, USB identity **FM-1_946301**. This is intentional; it has not been rebuilt just to rename it.
- Original 2.2-based reverb processing is retained. No new reverb algorithm, Bluetooth support, recorded pitch bend, per-instrument Fill or 128-step extension is included.
- User feedback reports working local functions. The retained test report separates simulated/native build checks from physical audio validation; the maintainer's final hardware session remains to be recorded before publication.
- The web demo runs the firmware logic but cannot certify hardware CPU load, USB transfer or physical stability. Its browser storage is separate from earlier demos.
- Back up projects, presets and user samples before updating. EVOLVE and live transpose are temporary. Use SAVE → USER for sounds and SAVE → PROJECT for the piece. Previous releases remain available.

### Build evidence

Native compilation passed. Firmware SHA-256: `1b3e0fcaa41fcad3d459ccfc977c20df3f492a0e941e7407c7f2ebe40878b9ed`.
App image 580276 bytes; main RAM 93820/94208 bytes (388 bytes free); pool 351644/360448 bytes before mandatory reserve; RAM code 31408/32512 bytes. This is a tight fit, not a guarantee of spare real-time CPU.
Recorded regressions cover Sound Design saving, EVOLVE, live transpose, quantized FX return and Fill, and the redraw correction. With new features inactive, 441882 stereo audio frames matched the baseline exactly.

## Français

### Nouveautés et accès

- **SEQ → EVOLVE**, après FEEL : K1 quantité, K3 octave des prochains ajouts (-1/0/+1), K4 durée. OCT- RESET TRACK, OCT+ RESET ALL. Ajouts dans la gamme sur les pas libres, originaux préservés ; application au prochain début de boucle. Fonction temporaire, non enregistrée dans les projets.
- **EDIT → PRESET INIT** : choisir le moteur avec K1 puis EDIT. Pages bleues dédiées, étoile « son modifié », dernière page SAVE SOUND avec OCT+ pour l'emplacement utilisateur et le nom.
- **ENV maintenu → TRANSPOSE** : choisir les pistes et la grille, transposer au clavier. F4 remet à zéro. La note en attente clignote ; batteries exclues. La transposition reste en quittant la page mais n'est pas sauvegardée.
- **FX maintenu → FX HOLD** : K2 règle le déclenchement et l'arrêt quantifiés jusqu'à deux mesures. Le filtre live reste indépendant des retours d'effets. Le retour quantifié retrouve le premier temps sans l'effet live.
- **E5 FILL dans FX HOLD** : maintien momentané, réutilisant les conditions déjà disponibles dans FEEL → CHANCE. La condition porte sur tout le pas, pas sur un instrument de batterie isolé.
- Correction du clignotement EVOLVE pendant la manipulation des knobs.

Les huit pistes, 64 pas par piste, 448 presets, édition des mesures, SONG, groove, mixage et éditeur web sont conservés. Les réverbes restent celles de la base 2.2. Le nom public est Sillage V3 ; le menu interne affiche encore V3 beta 1, identité FM-1_946301. Le firmware n'a pas été recompilé pour modifier ce texte. Sauvegarder ses données avant installation. Voir le manuel complet et le guide rapide anglais.
