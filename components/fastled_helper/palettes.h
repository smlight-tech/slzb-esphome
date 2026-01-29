#pragma once

#ifdef PALETTES

#define FASTLED_INTERNAL  // remove annoying pragma messages

#include <FastLED.h>

namespace esphome {
namespace fastled_helper {

#undef DEFINE_GRADIENT_PALETTE

#define DEFINE_GRADIENT_PALETTE(X) \
  FL_ALIGN_PROGMEM \
  static const ::TProgmemRGBGradientPalette_byte X[] FL_PROGMEM =

DEFINE_GRADIENT_PALETTE(Fire_gp){0, 0, 0, 0, 128, 255, 0, 0, 224, 255, 255, 0, 255, 255, 255, 255};

// Gradient palette "Sunset_Real_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Sunset_Real.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE(Sunset_Real_gp){0,  120, 0,   0,   22, 179, 22,  0,  51, 255, 104, 0, 85, 167,
                                        22, 18,  135, 100, 0,  103, 198, 16, 0,  130, 255, 0, 0,  160};

DEFINE_GRADIENT_PALETTE(dkbluered_gp){
    0,   1,   0,   4,   8,   1,   0,   13,  17,  1,   0,   29,  25,  1,   0,   52,  33,  1,   0,   83,  42,
    1,   0,   123, 51,  1,   0,   174, 59,  1,   0,   235, 68,  1,   2,   255, 76,  4,   17,  255, 84,  16,
    45,  255, 93,  37,  82,  255, 102, 69,  127, 255, 110, 120, 168, 255, 119, 182, 217, 255, 127, 255, 255,
    255, 135, 255, 217, 184, 144, 255, 168, 123, 153, 255, 127, 73,  161, 255, 82,  40,  170, 255, 45,  18,
    178, 255, 17,  5,   186, 255, 2,   1,   195, 234, 0,   1,   204, 171, 0,   1,   212, 120, 0,   1,   221,
    79,  0,   1,   229, 48,  0,   1,   237, 26,  0,   1,   246, 12,  0,   1,   255, 4,   0,   1};

DEFINE_GRADIENT_PALETTE(Optimus_Prime_gp){0,  5,   16,  18,  25,  5,   16,  18,  51,  7,   25,  39, 76, 8,   38,
                                          71, 102, 64,  99,  106, 127, 194, 189, 151, 153, 182, 63, 42, 178, 167,
                                          6,  2,   204, 100, 3,   1,   229, 53,  1,   1,   255, 53, 1,  1};

DEFINE_GRADIENT_PALETTE(warmGrad_gp){0,   252, 252, 172, 25, 239, 255, 61,  53, 247, 45,  17, 76, 197, 82,  19, 96, 239,
                                     255, 61,  124, 83,  4,  1,   153, 247, 45, 17,  214, 23, 15, 17,  255, 1,  1,  1};

DEFINE_GRADIENT_PALETTE(coldGrad_gp){0,   66, 186, 192, 43,  1, 22, 71, 79,  2, 104, 142, 117, 66, 186, 192,
                                     147, 2,  104, 142, 186, 1, 22, 71, 224, 2, 104, 142, 255, 4,  27,  28};

DEFINE_GRADIENT_PALETTE(hotGrad_gp){0, 157, 21,  2,   35,  229, 244, 16,  73, 255, 44,  7,   107, 142,
                                    7, 1,   153, 229, 244, 16,  206, 142, 7,  1,   255, 135, 36,  0};

DEFINE_GRADIENT_PALETTE(pinkGrad_gp){0,   249, 32, 145, 28,  208, 1,  7,   43,  249, 1,   19,  56,  126, 152, 10,
                                     73,  234, 23, 84,  89,  224, 45, 119, 107, 232, 127, 158, 127, 244, 13,  89,
                                     150, 188, 6,  52,  175, 177, 70, 14,  221, 194, 1,   8,   255, 112, 0,   1};

DEFINE_GRADIENT_PALETTE(comfy_gp){0, 255, 255, 45, 43, 208, 93, 1, 137, 224, 1, 242, 181, 159, 1, 29, 255, 63, 4, 68};

DEFINE_GRADIENT_PALETTE(cyperpunk_gp){0,   3,   6,   72, 38,  12,  50, 188, 109, 217, 35, 1,
                                      135, 242, 175, 12, 178, 161, 32, 87,  255, 24,  6,  108};

DEFINE_GRADIENT_PALETTE(girl_gp){0,   103, 1,  10,  33,  109, 1, 12, 76,  159, 5, 48, 119, 175, 55, 103,
                                 127, 175, 55, 103, 178, 159, 5, 48, 221, 109, 1, 12, 255, 103, 1,  10};

DEFINE_GRADIENT_PALETTE(xmas_gp){0,   0,   12, 0, 40,  0,   55, 0, 66,  1,  117, 2, 77,  1,   84, 1,
                                 81,  0,   55, 0, 119, 0,   12, 0, 153, 42, 0,   0, 181, 121, 0,  0,
                                 204, 255, 12, 8, 224, 121, 0,  0, 244, 42, 0,   0, 255, 42,  0,  0};

DEFINE_GRADIENT_PALETTE(acid_gp){0,   0, 12,  0,   61,  153, 239, 112, 127, 0, 12,  0, 165, 106,
                                 239, 2, 196, 167, 229, 71,  229, 106, 239, 2, 255, 0, 12,  0};

DEFINE_GRADIENT_PALETTE(blueSmoke_gp){0,  0,   0,   0, 12, 1,  1,   3,  53, 8,  1,   22, 80, 4, 6,   89, 119, 2,
                                      25, 216, 145, 7, 10, 99, 186, 15, 2,  31, 233, 2,  1,  5, 255, 0,  0,   0};

DEFINE_GRADIENT_PALETTE(gummy_gp){
    0, 8, 47,  5,  31, 77, 122, 6, 63, 249, 237, 7,  95, 232, 51,  1,   127, 215,
    0, 1, 159, 47, 1,  3,  191, 1, 7,  16,  223, 52, 22, 6,   255, 239, 45,  1,
};

DEFINE_GRADIENT_PALETTE(leo_gp){
    0, 0,  0,   0, 16, 0,   0,   0, 32, 0,   0,   0, 18, 0,   0,   0, 64, 16,  8,   0, 80, 80,  40,
    0, 96, 16,  8, 0,  112, 0,   0, 0,  128, 0,   0, 0,  144, 0,   0, 0,  160, 0,   0, 0,  176, 0,
    0, 0,  192, 0, 0,  0,   208, 0, 0,  0,   224, 0, 0,  0,   240, 0, 0,  0,   255, 0, 0,  0,
};

DEFINE_GRADIENT_PALETTE(aurora_gp){
    0,   17,  177, 13,   // Greenish
    64,  121, 242, 5,    // Greenish
    128, 25,  173, 121,  // Turquoise
    192, 250, 77,  127,  // Pink
    255, 171, 101, 221   // Purple
};

DEFINE_GRADIENT_PALETTE(redwhite_gp){
    0,   255, 0,   0,   25, 255, 255, 255, 51,  255, 0,   0,   76, 255, 255, 255, 102, 255, 0,   0,   127, 255,
    255, 255, 153, 255, 0,  0,   178, 255, 255, 255, 204, 255, 0,  0,   229, 255, 255, 255, 255, 255, 0,   0,
};

// Gradient palette "ib_jul01_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ing/xmas/tn/ib_jul01.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE(ib_jul01_gp){0, 194, 1, 1, 94, 1, 29, 18, 132, 57, 131, 28, 255, 113, 1, 1};

// Gradient palette "es_vintage_57_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/vintage/tn/es_vintage_57.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_vintage_57_gp){0, 2, 1, 1, 53, 18, 1, 0, 104, 69, 29, 1, 153, 167, 135, 10, 255, 46, 56, 4};

// Gradient palette "es_vintage_01_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/vintage/tn/es_vintage_01.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 32 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_vintage_01_gp){0,   4,  1, 1, 51,  16, 0, 1, 76,  97, 104, 3, 101, 255, 131, 19,
                                          127, 67, 9, 4, 153, 16, 0, 1, 229, 4,  1,   1, 255, 4,   1,   1};

// Gradient palette "es_rivendell_15_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/rivendell/tn/es_rivendell_15.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_rivendell_15_gp){0,  1,  14,  5,   101, 16, 36,  14,  165, 56,
                                            68, 30, 242, 150, 156, 99, 255, 150, 156, 99};

// Gradient palette "rgi_15_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ds/rgi/tn/rgi_15.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 36 bytes of program space.

DEFINE_GRADIENT_PALETTE(rgi_15_gp){0, 4,  1,   31, 31, 55, 1,   16,  63, 197, 3,   7,  95, 59, 2,   17, 127, 6,
                                   2, 34, 159, 39, 6,  33, 191, 112, 13, 32,  223, 56, 9,  35, 255, 22, 6,   38};

// Gradient palette "retro2_16_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ma/retro2/tn/retro2_16.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 8 bytes of program space.

DEFINE_GRADIENT_PALETTE(retro2_16_gp){0, 188, 135, 1, 255, 46, 7, 1};

// Gradient palette "Analogous_1_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/red/tn/Analogous_1.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE(Analogous_1_gp){0, 3, 0, 255, 63, 23, 0, 255, 127, 67, 0, 255, 191, 142, 0, 45, 255, 255, 0, 0};

// Gradient palette "es_pinksplash_08_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/pink_splash/tn/es_pinksplash_08.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_pinksplash_08_gp){0,   126, 11,  255, 127, 197, 1,   22,  175, 210,
                                             157, 172, 221, 157, 3,   112, 255, 157, 3,   112};

// Gradient palette "es_pinksplash_07_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/pink_splash/tn/es_pinksplash_07.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_pinksplash_07_gp){0,  229, 1,   1,   61, 242, 4,   63,  101, 255, 12,  255, 127, 249,
                                             81, 252, 153, 255, 11, 235, 193, 244, 5,   68,  255, 232, 1,   5};

// Gradient palette "Coral_reef_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/other/tn/Coral_reef.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE(Coral_reef_gp){0,  40, 199, 197, 50,  10, 152, 155, 96,  1, 111, 120,
                                       96, 43, 127, 162, 139, 10, 73,  111, 255, 1, 34,  71};

// Gradient palette "es_ocean_breeze_068_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/ocean_breeze/tn/es_ocean_breeze_068.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_ocean_breeze_068_gp){0,   100, 156, 153, 51,  1, 99, 137, 101, 1, 68, 84,
                                                104, 35,  142, 168, 178, 0, 63, 117, 255, 1, 10, 10};

// Gradient palette "es_ocean_breeze_036_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/ocean_breeze/tn/es_ocean_breeze_036.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_ocean_breeze_036_gp){0, 1, 6, 7, 89, 1, 99, 111, 153, 144, 209, 255, 255, 0, 73, 82};

// Gradient palette "departure_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/mjf/tn/departure.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 88 bytes of program space.

DEFINE_GRADIENT_PALETTE(departure_gp){0,   8,   3,   0,   42,  23,  7,   0,   63,  75,  38,  6,   84,  169, 99,  38,
                                      106, 213, 169, 119, 116, 255, 255, 255, 138, 135, 255, 138, 148, 22,  255, 24,
                                      170, 0,   255, 0,   191, 0,   136, 0,   212, 0,   55,  0,   255, 0,   55,  0};

// Gradient palette "es_landscape_64_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/landscape/tn/es_landscape_64.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 36 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_landscape_64_gp){0,   0,   0,   0,   37,  2,   25,  1,   76,  15,  115, 5,
                                            127, 79,  213, 1,   128, 126, 211, 47,  130, 188, 209, 247,
                                            153, 144, 182, 205, 204, 59,  117, 250, 255, 1,   37,  192};

// Gradient palette "es_landscape_33_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/landscape/tn/es_landscape_33.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_landscape_33_gp){0,  1,   5,   0, 19, 32, 23,  1,  38,  161, 55, 1,
                                            63, 229, 144, 1, 66, 39, 142, 74, 255, 1,   4,  1};

// Gradient palette "rainbowsherbet_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ma/icecream/tn/rainbowsherbet.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE(rainbowsherbet_gp){0,  255, 33,  4,   43,  255, 68,  25, 86,  255, 7,   25, 127, 255,
                                           82, 103, 170, 255, 255, 242, 209, 42, 255, 22,  255, 87, 255, 65};

// Gradient palette "gr65_hult_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/hult/tn/gr65_hult.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE(gr65_hult_gp){0,   247, 176, 247, 48,  255, 136, 255, 89,  220, 29,  226,
                                      160, 7,   82,  178, 216, 1,   124, 109, 255, 1,   124, 109};

// Gradient palette "gr64_hult_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/hult/tn/gr64_hult.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 32 bytes of program space.

DEFINE_GRADIENT_PALETTE(gr64_hult_gp){0,   1,  124, 109, 66,  1, 93, 79, 104, 52, 65, 1,  130, 115, 127, 1,
                                      150, 52, 65,  1,   201, 1, 86, 72, 239, 0,  55, 45, 255, 0,   55,  45};

// Gradient palette "GMT_drywet_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/gmt/tn/GMT_drywet.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE(GMT_drywet_gp){0,   47,  30,  2, 42, 213, 147, 24, 84, 103, 219, 52, 127, 3,
                                       219, 207, 170, 1, 48, 214, 212, 1,  1,  111, 255, 1,  7,   33};

// Gradient palette "ib15_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ing/general/tn/ib15.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE(ib15_gp){0,   113, 91, 147, 72,  157, 88, 78, 89,  208, 85, 33,
                                 107, 255, 29, 11,  141, 137, 31, 39, 255, 59,  33, 89};

// Gradient palette "Fuschia_7_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ds/fuschia/tn/Fuschia-7.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE(Fuschia_7_gp){0, 43, 3,   153, 63, 100, 4,   103, 127, 188,
                                      5, 66, 191, 161, 11, 115, 255, 135, 20,  182};

// Gradient palette "es_emerald_dragon_08_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/emerald_dragon/tn/es_emerald_dragon_08.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_emerald_dragon_08_gp){0, 97, 255, 1, 101, 47, 133, 1, 178, 13, 43, 1, 255, 2, 10, 1};

// Gradient palette "lava_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/neota/elem/tn/lava.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 52 bytes of program space.

DEFINE_GRADIENT_PALETTE(lava_gp){0,   0,   0,   0,   46,  18,  0,   0,   96,  113, 0,   0,   108,
                                 142, 3,   1,   119, 175, 17,  1,   146, 213, 44,  2,   174, 255,
                                 82,  4,   188, 255, 115, 4,   202, 255, 156, 4,   218, 255, 203,
                                 4,   234, 255, 255, 4,   244, 255, 255, 71,  255, 255, 255, 255};

// Gradient palette "Colorfull_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Colorfull.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 44 bytes of program space.

DEFINE_GRADIENT_PALETTE(Colorfull_gp){0,   10,  85,  5,   25,  29,  109, 18,  60,  59,  138, 42, 93,  83,  99,
                                      52,  106, 110, 66,  64,  109, 123, 49,  65,  113, 139, 35, 66,  116, 192,
                                      117, 98,  124, 255, 255, 137, 168, 100, 180, 155, 255, 22, 121, 174};

// Gradient palette "Magenta_Evening_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Magenta_Evening.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE(Magenta_Evening_gp){0, 71, 27, 39,  31, 130, 11,  51,  63, 213, 2,   64, 70, 232,
                                            1, 66, 76, 252, 1,  69,  108, 123, 2,  51,  255, 46, 9,  35};

// Gradient palette "Pink_Purple_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Pink_Purple.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 44 bytes of program space.

DEFINE_GRADIENT_PALETTE(Pink_Purple_gp){0,   19,  2,   39,  25,  26,  4,   45,  51,  33,  6,   52,  76,  68,  62,
                                        125, 102, 118, 187, 240, 109, 163, 215, 247, 114, 217, 244, 255, 122, 159,
                                        149, 221, 149, 113, 78,  188, 183, 128, 57,  155, 255, 146, 40,  123};

// Gradient palette "es_autumn_19_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/autumn/tn/es_autumn_19.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 52 bytes of program space.

DEFINE_GRADIENT_PALETTE(es_autumn_19_gp){0,   26,  1,   1,   51,  67,  4,   1,   84,  118, 14,  1,   104,
                                         137, 152, 52,  112, 113, 65,  1,   122, 133, 149, 59,  124, 137,
                                         152, 52,  135, 113, 65,  1,   142, 139, 154, 46,  163, 113, 13,
                                         1,   204, 55,  3,   1,   249, 17,  1,   1,   255, 17,  1,   1};

// Gradient palette "BlacK_Blue_Magenta_White_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/BlacK_Blue_Magenta_White.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE(BlacK_Blue_Magenta_White_gp){0, 0,   0,   0,   42, 0,   0,   45,  84, 0,   0,   255, 127, 42,
                                                     0, 255, 170, 255, 0,  255, 212, 255, 55, 255, 255, 255, 255, 255};

// Gradient palette "BlacK_Magenta_Red_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/BlacK_Magenta_Red.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE(BlacK_Magenta_Red_gp){0, 0,   0,   0,   63, 42, 0,   45,  127, 255,
                                              0, 255, 191, 255, 0,  45, 255, 255, 0,   0};

// Gradient palette "BlacK_Red_Magenta_Yellow_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/BlacK_Red_Magenta_Yellow.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE(BlacK_Red_Magenta_Yellow_gp){0, 0,  0,   0,   42, 42,  0,   0,   84, 255, 0,   0,   127, 255,
                                                     0, 45, 170, 255, 0,  255, 212, 255, 55, 45,  255, 255, 255, 0};

// Gradient palette "Blue_Cyan_Yellow_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/Blue_Cyan_Yellow.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE(Blue_Cyan_Yellow_gp){0,   0,   0,   255, 63,  0,  55,  255, 127, 0,
                                             255, 255, 191, 42,  255, 45, 255, 255, 255, 0};

// Gradient palette "bhw1_28_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw1/tn/bhw1_28.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 32 bytes of program space.

DEFINE_GRADIENT_PALETTE(bhw1_28_gp){0,   75, 1, 221, 30,  252, 73, 255, 48,  169, 0,  242, 119, 0, 149, 242,
                                    170, 43, 0, 242, 206, 252, 73, 255, 232, 78,  12, 214, 255, 0, 149, 242};

static CRGBPalette16 paletteArr[] = {
    PartyColors_p,
    RainbowColors_p,
    RainbowStripeColors_p,
    Sunset_Real_gp,
    dkbluered_gp,
    warmGrad_gp,
    coldGrad_gp,
    hotGrad_gp,
    pinkGrad_gp,
    cyperpunk_gp,
    redwhite_gp,
    Fire_gp,
    Optimus_Prime_gp,
    comfy_gp,
    girl_gp,
    xmas_gp,
    acid_gp,
    blueSmoke_gp,
    gummy_gp,
    leo_gp,
    aurora_gp,
    ib_jul01_gp,
    es_vintage_57_gp,
    es_vintage_01_gp,
    es_rivendell_15_gp,
    rgi_15_gp,
    retro2_16_gp,
    Analogous_1_gp,
    es_pinksplash_08_gp,
    es_pinksplash_07_gp,
    Coral_reef_gp,
    es_ocean_breeze_068_gp,
    es_ocean_breeze_036_gp,
    departure_gp,
    es_landscape_64_gp,
    es_landscape_33_gp,
    rainbowsherbet_gp,
    gr64_hult_gp,
    gr65_hult_gp,
    GMT_drywet_gp,
    ib15_gp,
    Fuschia_7_gp,
    es_emerald_dragon_08_gp,
    lava_gp,
    Colorfull_gp,
    Magenta_Evening_gp,
    Pink_Purple_gp,
    es_autumn_19_gp,
    BlacK_Blue_Magenta_White_gp,
    BlacK_Magenta_Red_gp,
    BlacK_Red_Magenta_Yellow_gp,
    Blue_Cyan_Yellow_gp,
    bhw1_28_gp,
};

}  // namespace fastled_helper
}  // namespace esphome

#endif
