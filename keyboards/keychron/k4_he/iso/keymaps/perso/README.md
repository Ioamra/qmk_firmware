# Keymap perso — Keychron K4 HE ISO

Tout tient dans `keyboards/keychron/k4_he/iso/keymaps/perso/keymap.c`.
Aucun fichier Keychron n'est modifié.

- La touche en haut à droite fait défiler les profils HE : 1 → 2 → 3 → 1.
  Elle s'affiche `CUSTOM(64)` dans le Launcher — c'est le keycode `PROF_CYC`.
- À chaque changement de profil (la touche, Fn+P+Z/X/C, ou le Launcher) et au
  démarrage : couleur unie du profil, luminosité conservée. Éteint reste éteint.
- Le clavier démarre toujours sur le profil 1 (typing).

| Profil | Clavier | Verr. Maj / Num |
|---|---|---|
| 1 · typing | bleu clair | rouge |
| 2 · gaming | rouge | bleu clair |
| 3 · manette | violet | vert |

## Prérequis — une fois par machine

Installer **[QMK MSYS](https://msys.qmk.fm/)** : un terminal livré avec le
compilateur ARM, Python et le CLI `qmk`. Toutes les commandes de ce README s'y
lancent.

Puis, dans ce terminal :

```bash
git clone -b k4he-perso https://github.com/Ioamra/qmk_firmware.git
cd qmk_firmware
git submodule update --init --depth 1 lib/chibios lib/chibios-contrib lib/printf lib/lufa
qmk config user.qmk_home="$(pwd -W)"
```

La dernière ligne enregistre le chemin du dépôt dans
`%LOCALAPPDATA%\QMK.EXE\qmk.exe\qmk.exe.ini`. C'est pour ça que les commandes de
compilation et de flash n'indiquent aucun chemin — et c'est aussi pourquoi elles
fonctionnent depuis n'importe quel répertoire courant.

## Changer une couleur

Dans `keyboards/keychron/k4_he/iso/keymaps/perso/keymap.c`, ligne 96 :

```c
static const profile_colors_t profile_colors[PROFILE_COUNT] = {
    //                    clavier            Verr. Maj / Verr. Num
    /* 1 - typing  */ { { 140, 255 },        {   0, 255 } },
    /* 2 - gaming  */ { {   0, 255 },        { 140, 255 } },
    /* 3 - manette */ { { 191, 255 },        {  85, 255 } },
};
```

Chaque paire est `{ teinte, saturation }`, en **HSV**, composantes sur 0-255.
Une table de teintes de référence est en commentaire au-dessus, dans le fichier.

Deux autres réglages dans le même fichier :

- ligne 108 — `LOCK_BRIGHTNESS 255` : luminosité des touches de verrouillage.
- ligne 111 — `PROFILE_ON_BOOT PROFILE_TYPING` : profil au démarrage
  (`PROFILE_TYPING`, `PROFILE_GAMING` ou `PROFILE_GAMEPAD`).

La luminosité générale du clavier n'est volontairement pas réglable ici : le
firmware conserve celle réglée au clavier.

## Flasher

**1. Mettre en bootloader.** Débrancher l'USB → interrupteur sur `Cable` →
maintenir **Esc** → rebrancher → attendre 3 s → relâcher.
Le clavier devient inerte, rétroéclairage éteint : c'est le bon signe.

**2. Dans le terminal QMK MSYS** (la commande recompile puis flashe) :

```bash
qmk flash -kb keychron/k4_he/iso -km perso
```

`-kb keychron/k4_he/iso` désigne le clavier, `-km perso` le dossier
`keymaps/perso`. Le clavier physique n'a pas à être désigné : `dfu-util` prend
le seul périphérique en bootloader — donc un seul clavier à la fois.

Succès = `File downloaded successfully`. Le clavier redémarre seul.
Le message `Device's firmware is corrupt` au début est normal.

Si la commande répond `No DFU capable USB device available` alors que le clavier
est bien inerte, c'est qu'il manque le pilote WinUSB : l'installeur de
[QMK Toolbox](https://github.com/qmk/qmk_toolbox/releases) le pose.

Pour compiler sans flasher : `qmk compile -kb keychron/k4_he/iso -km perso`.

## Ne pas mettre à jour le firmware depuis le Launcher

Ça écraserait ce firmware par celui de Keychron. Le bandeau « mise à jour
disponible » est informatif : le Launcher compare un numéro de version et ne
sait pas que ce firmware est un build maison. Si c'est fait quand même, il
suffit de reflasher.
