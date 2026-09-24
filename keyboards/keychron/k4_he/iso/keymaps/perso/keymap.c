/* Copyright 2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ===========================================================================
 * Keymap perso — Keychron K4 HE ISO
 * ===========================================================================
 *
 * Ce que ce fichier ajoute par rapport a la keymap Keychron d'origine :
 *
 *   1. La touche en haut a droite (PROF_CYC) fait defiler les 3 profils HE
 *      1 -> 2 -> 3 -> 1.
 *   2. A chaque changement de profil (ma touche, Fn+P+Z/X/C, ou le Launcher)
 *      et au demarrage, l'eclairage passe en couleur unie propre au profil,
 *      en gardant la luminosite actuelle. Si l'eclairage est eteint, il le
 *      reste.
 *   3. Verr. Maj et Verr. Num s'allument d'une couleur differente, elle aussi
 *      propre au profil.
 *   4. Au demarrage, le clavier revient toujours sur le profil 1 (typing).
 *
 * Tout ce qui se regle se trouve dans le bloc CONFIGURATION juste en dessous.
 * =========================================================================== */

#include QMK_KEYBOARD_H
#include "keychron_common.h"

/* profile_select() / profile_get_current_index() : gestion des profils HE.
 * Voir keyboards/keychron/common/analog_matrix/profile.h */
#include "profile.h"

/* os_indicator_config_t : les reglages d'indicateurs envoyes par le Launcher
 * (couleur, et cases "desactiver Verr. Maj / Verr. Num"). */
#include "keychron_rgb_type.h"

/* Pour lire et reecrire la configuration RGB stockee dans l'EEPROM. */
#include "eeprom.h"
#include "nvm_eeprom_eeconfig_internal.h"

/* ===========================================================================
 * CONFIGURATION — c'est ici que tu ajustes les couleurs
 * ===========================================================================
 *
 * QMK travaille en HSV, pas en #RRGGBB. Chaque composante est un octet 0-255 :
 *
 *   h (hue / teinte) ....... la couleur elle-meme, sur une roue qui boucle
 *                            (0 et 255 sont tous deux rouges). Valeurs de
 *                            reference, tirees de quantum/color.h :
 *
 *        0   Rouge          85   Vert           170  Bleu
 *        21  Orange        106   Vert printemps 191  Violet
 *        43  Jaune         128   Cyan           213  Magenta
 *        64  Vert-jaune    140   Bleu clair     234  Rose
 *
 *                            Depuis une couleur CSS : teinte = degres x 255/360
 *
 *   s (saturation) ......... 255 = couleur franche et pure.
 *                            Baisser = delave vers le blanc (180 = pastel,
 *                            100 = tres pale, 0 = blanc pur).
 *
 *   v (value / luminosite) . PAS utilise pour la couleur du clavier : on
 *                            conserve volontairement la luminosite que tu as
 *                            reglee au clavier (Fn+molette de luminosite).
 *                            Utilise seulement pour les touches de
 *                            verrouillage, voir LOCK_BRIGHTNESS plus bas.
 */

typedef struct {
    uint8_t h; // teinte
    uint8_t s; // saturation
} hue_sat_t;

typedef struct {
    hue_sat_t keyboard; // couleur unie de tout le clavier
    hue_sat_t lock;     // couleur de Verr. Maj / Verr. Num quand ils sont actifs
} profile_colors_t;

/* Les 3 profils HE, dans l'ordre du Launcher.
 *
 * Profil 1 (typing)  : clavier bleu clair, verrouillages rouges
 * Profil 2 (gaming)  : clavier rouge,      verrouillages bleu clair
 * Profil 3 (manette) : clavier violet,     verrouillages verts
 */
// clang-format off
static const profile_colors_t profile_colors[PROFILE_COUNT] = {
    //                    clavier       Verr. Maj / Verr. Num
    /* 1 - typing  */ { { 160, 255 },   { 0, 255 } }, // bleu           / rouge
    /* 2 - gaming  */ { { 140, 255 },   { 0, 255 } }, // bleu clair     / rouge
    /* 3 - manette */ { { 110, 255 },   { 0, 255 } }, // bleu turquoise / rouge
};
// clang-format on

/* Luminosite des touches Verr. Maj / Verr. Num quand elles sont actives.
 * Volontairement fixe et elevee pour qu'elles ressortent nettement, meme si le
 * reste du clavier est en faible luminosite, et pour qu'elles restent visibles
 * quand l'eclairage general est eteint. Baisse cette valeur si c'est trop vif. */
#define LOCK_BRIGHTNESS 255

/* Profil applique a chaque demarrage du clavier. */
#define PROFILE_ON_BOOT PROFILE_TYPING

/* Index des 3 profils, pour rendre le code lisible. */
enum profile_index {
    PROFILE_TYPING  = 0,
    PROFILE_GAMING  = 1,
    PROFILE_GAMEPAD = 2,
};

/* ===========================================================================
 * FIN DE LA CONFIGURATION
 * =========================================================================== */

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
};

#define FN_MAC MO(MAC_FN)
#define FN_WIN MO(WIN_FN)

/* Mon keycode perso. QK_USER_0 vaut 0x7E40 : c'est la plage que QMK reserve aux
 * keycodes definis dans une keymap. Aucun risque de collision avec les keycodes
 * Keychron (BT_HST1, PROF1...), qui vivent dans la plage QK_KB_* 0x7E00-0x7E1F. */
enum custom_keycodes {
    PROF_CYC = QK_USER_0, // defile les profils HE : 1 -> 2 -> 3 -> 1
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [MAC_BASE] = LAYOUT_iso_101(
        KC_ESC,   KC_BRID,  KC_BRIU,  KC_MCTRL, KC_LNPAD, UG_VALD,  UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  KC_DEL,   KC_HOME,  KC_END,   KC_PGUP,  KC_PGDN,  PROF_CYC,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,                      KC_P7,    KC_P8,    KC_P9,    KC_PPLS,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_NUHS,  KC_ENT,             KC_P4,    KC_P5,    KC_P6,
        KC_LSFT,  KC_NUBS,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,            KC_RSFT,  KC_UP,    KC_P1,    KC_P2,    KC_P3,    KC_PENT,
        KC_LCTL,  KC_LOPTN, KC_LCMMD,                               KC_SPC,                                 KC_RCMMD, FN_MAC,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT,  KC_P0,    KC_PDOT          ),

    [MAC_FN] = LAYOUT_iso_101(
        _______,  KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   _______,  _______,  _______,  _______,  _______,  UG_TOGG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,  _______,
        UG_TOGG,  UG_NEXT,  UG_VALU,  UG_HUEU,  UG_SATU,  UG_SPDU,  _______,  _______,  _______,  _______,  _______,  _______,  _______,                      _______,  _______,  _______,  _______,
        _______,  UG_PREV,  UG_VALD,  UG_HUED,  UG_SATD,  UG_SPDD,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  BAT_LVL,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______          ),

    [WIN_BASE] = LAYOUT_iso_101(
        KC_ESC,   KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   KC_DEL,   KC_HOME,  KC_END,   KC_PGUP,  KC_PGDN,  PROF_CYC,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,                      KC_P7,    KC_P8,    KC_P9,    KC_PPLS,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_NUHS,  KC_ENT,             KC_P4,    KC_P5,    KC_P6,
        KC_LSFT,  KC_NUBS,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,            KC_RSFT,  KC_UP,    KC_P1,    KC_P2,    KC_P3,    KC_PENT,
        KC_LCTL,  KC_LWIN,  KC_LALT,                                KC_SPC,                                 KC_RALT,  FN_WIN,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT,  KC_P0,    KC_PDOT          ),

    [WIN_FN] = LAYOUT_iso_101(
        _______,  KC_BRID,  KC_BRIU,  KC_TASK,  KC_FILE,  UG_VALD,  UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  _______,  _______,  _______,  _______,  _______,  UG_TOGG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,    P2P4G,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,  _______,
        UG_TOGG,  UG_NEXT,  UG_VALU,  UG_HUEU,  UG_SATU,  UG_SPDU,  _______,  _______,  _______,  _______,  _______,  _______,  _______,                      _______,  _______,  _______,  _______,
        _______,  UG_PREV,  UG_VALD,  UG_HUED,  UG_SATD,  UG_SPDD,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  BAT_LVL,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______          )
};
// clang-format on

/* ===========================================================================
 * Couleurs du profil courant
 * =========================================================================== */

/* Renvoie les couleurs du profil actuellement actif.
 * Le "if" est une ceinture de securite : si jamais l'index lu etait hors de
 * l'intervalle 0-2, on lirait de la memoire au hasard dans le tableau. */
static const profile_colors_t *current_colors(void) {
    uint8_t index = profile_get_current_index();
    if (index >= PROFILE_COUNT) index = PROFILE_TYPING;
    return &profile_colors[index];
}

/* ===========================================================================
 * Appliquer la couleur unie du profil
 * ===========================================================================
 *
 * Deux choses a faire, et il faut les deux :
 *
 *   1. Mettre a jour la configuration RGB "a chaud" (en RAM), celle que la
 *      matrice relit a chaque image.
 *
 *      Attention, piege : on ecrit les champs directement au lieu d'appeler
 *      rgb_matrix_mode_noeeprom() / rgb_matrix_sethsv_noeeprom(), parce que ces
 *      deux fonctions commencent par "if (!rgb_matrix_config.enable) return;"
 *      (cf. quantum/rgb_matrix/rgb_matrix.c). Eclairage eteint, elles ne
 *      feraient rien et la copie en RAM resterait sur l'ancienne couleur. Or
 *      rgb_matrix_enable(), declenchee par Fn+haut-droite pour rallumer,
 *      reecrit l'EEPROM a partir de cette copie RAM : la couleur du profil
 *      serait perdue au rallumage.
 *
 *   2. L'ecrire aussi dans l'EEPROM (la memoire qui survit a l'extinction).
 *      Pas pour la persistance en elle-meme, mais parce que le combo
 *      Fn+P+Z/X/C de Keychron, apres son clignotement rouge de ~3,5 s,
 *      RECHARGE toute la configuration RGB depuis l'EEPROM. Sans l'etape 2,
 *      ce rechargement effacerait la couleur qu'on vient d'appliquer.
 *
 * Et une precaution importante pour l'etape 2 : on lit le bloc EEPROM
 * existant, on ne modifie QUE mode / teinte / saturation, puis on le reecrit.
 * On ne touche donc jamais :
 *
 *   - le champ "enable" (eclairage allume ou eteint). C'est essentiel : ce meme
 *     combo Fn+P+Z/X/C allume temporairement l'eclairage pour son clignotement.
 *     Si on ecrivait la structure complete a cet instant, on graverait
 *     "eclairage allume" dans l'EEPROM et un eclairage eteint se rallumerait
 *     tout seul au changement de profil.
 *   - le champ "hsv.v" (luminosite), qu'on veut conserver tel quel.
 */
static void apply_profile_lighting(void) {
    const hue_sat_t color = current_colors()->keyboard;

    // 1. Configuration en RAM. On ne touche ni hsv.v (luminosite) ni enable
    //    (allume/eteint) : ils gardent exactement leur valeur actuelle.
    rgb_matrix_config.mode  = RGB_MATRIX_SOLID_COLOR;
    rgb_matrix_config.hsv.h = color.h;
    rgb_matrix_config.hsv.s = color.s;

    //    Relance le rendu pour que le changement soit visible immediatement.
    //    Inutile si l'eclairage est eteint : il n'y a rien a afficher.
    if (rgb_matrix_is_enabled()) {
        rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
    }

    // 2. Lecture / modification ciblee / reecriture du bloc EEPROM.
    rgb_config_t stored;
    eeprom_read_block(&stored, EECONFIG_RGB_MATRIX, sizeof(stored));
    stored.mode  = RGB_MATRIX_SOLID_COLOR;
    stored.hsv.h = color.h;
    stored.hsv.s = color.s;
    eeprom_update_block(&stored, EECONFIG_RGB_MATRIX, sizeof(stored));
}

/* ===========================================================================
 * Detecter les changements de profil
 * ===========================================================================
 *
 * Il n'existe aucun crochet appele par Keychron quand le profil change. On
 * surveille donc la valeur nous-memes.
 *
 * housekeeping_task_user() est appelee par la boucle principale du firmware
 * (pas a chaque touche) : quelques milliers de fois par seconde. Le cout ici
 * est de lire un octet et de le comparer, soit ~24 ns sur le Cortex-M4 a
 * 84 MHz — a comparer aux centaines de microsecondes que la meme boucle passe
 * deja a lire les 101 capteurs magnetiques et a pousser la matrice RGB.
 *
 * Interet : ca couvre les trois facons de changer de profil d'un seul coup —
 * ma touche, le combo Fn+P+Z/X/C, et le Launcher.
 */
static uint8_t last_profile = PROFILE_ON_BOOT;

void housekeeping_task_user(void) {
    uint8_t current = profile_get_current_index();

    if (current != last_profile) {
        last_profile = current;
        apply_profile_lighting();
    }
}

/* ===========================================================================
 * Au demarrage du clavier
 * ===========================================================================
 *
 * A ce stade, matrice HE et matrice RGB sont toutes deux deja initialisees
 * (ordre dans quantum/keyboard.c : matrix_init() puis rgb_matrix_init() puis,
 * en dernier, ce crochet). On peut donc changer de profil et d'eclairage sans
 * risque.
 *
 * Le "false" de profile_select dit : pas de clignotement rouge de confirmation.
 */
void keyboard_post_init_user(void) {
    profile_select(PROFILE_ON_BOOT, false);
    last_profile = PROFILE_ON_BOOT;
    apply_profile_lighting();
}

/* ===========================================================================
 * Ma touche de defilement des profils
 * ===========================================================================
 *
 * process_record_user() est appelee a chaque appui ET a chaque relachement de
 * touche, avant tout le reste. Renvoyer false = "j'ai traite cette touche,
 * arretez tout" ; renvoyer true = "continuez normalement".
 */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode == PROF_CYC) {
        if (record->event.pressed) { // on ignore le relachement
            uint8_t next = profile_get_current_index() + 1;
            if (next >= PROFILE_COUNT) next = 0; // 3 -> retour a 1

            // false = pas de clignotement rouge : la nouvelle couleur unie du
            // clavier suffit a identifier le profil. housekeeping_task_user()
            // detectera le changement et appliquera la couleur.
            profile_select(next, false);
        }
        return false;
    }

    return true; // toutes les autres touches : comportement d'origine
}

/* ===========================================================================
 * Verr. Maj et Verr. Num colores
 * ===========================================================================
 *
 * Keychron allume ces deux touches dans os_state_indicate(), avec la couleur
 * choisie dans le Launcher. Cette fonction n'est pas surchargeable (definition
 * "forte" dans common/rgb/keychron_rgb.c, que la consigne interdit de
 * modifier). On repeint donc par-dessus, via deux crochets qui s'executent
 * APRES elle :
 *
 *   - rgb_matrix_indicators_advanced_user() quand l'eclairage est allume
 *     (appele apres rgb_matrix_indicators(), cf. quantum/rgb_matrix.c)
 *   - rgb_matrix_none_indicators_user()     quand l'eclairage est eteint
 *     (les touches de verrouillage restent alors visibles seules sur un
 *     clavier noir — c'est le comportement Keychron d'origine, conserve)
 *
 * On respecte au passage les cases "desactiver l'indicateur Verr. Maj / Num"
 * du Launcher, via os_ind_cfg, pour ne rien casser de son comportement.
 */
extern os_indicator_config_t os_ind_cfg;

static void paint_lock_keys(void) {
    const hue_sat_t lock = current_colors()->lock;

    // Le driver LED veut du RGB, on convertit depuis notre HSV.
    rgb_t rgb = hsv_to_rgb((hsv_t){.h = lock.h, .s = lock.s, .v = LOCK_BRIGHTNESS});

    // Etat reel des verrouillages, tel que l'OS le rapporte.
    led_t leds = host_keyboard_led_state();

    // CAPS_LOCK_INDEX (55) et NUM_LOCK_INDEX (33) viennent de iso/config.h.
    if (leds.caps_lock && !os_ind_cfg.disable.caps_lock) {
        rgb_matrix_set_color(CAPS_LOCK_INDEX, rgb.r, rgb.g, rgb.b);
    }
    if (leds.num_lock && !os_ind_cfg.disable.num_lock) {
        rgb_matrix_set_color(NUM_LOCK_INDEX, rgb.r, rgb.g, rgb.b);
    }
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    paint_lock_keys();
    return true; // true = laisse le reste de la chaine d'indicateurs tourner
}

void rgb_matrix_none_indicators_user(void) {
    paint_lock_keys();
}
