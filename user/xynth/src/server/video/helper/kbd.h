/***************************************************************************
    begin                : Sun Feb 20 2005
    copyright            : (C) 2005 - 2007 by Alper Akcan
    email                : distchx@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 ***************************************************************************/

#define KEYCODE_KEYS	128
#define KEYCODE_PLAIN	0
#define KEYCODE_SHIFT	1
#define KEYCODE_ALTGR	2
#define KEYCODE_FLAGS	3

#define KG_CAPSSHIFT	8
#define NR_KEYS			255

#define NUM_VGAKEYMAPS	(1 << KG_CAPSSHIFT)
static unsigned short s_video_helper_keybd_keymap[NUM_VGAKEYMAPS][NR_KEYS];

static S_KEYCODE_CODE s_video_helper_keybd_keycode_[KEYCODE_KEYS][KEYCODE_FLAGS] = {
/*  keycode,     plain name,            shift name,      	altgr name */
/*   0 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*   1 */ 	{S_KEYCODE_ESCAPE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*   2 */ 	{S_KEYCODE_ONE,		S_KEYCODE_EXCLAM,	S_KEYCODE_NOCODE},
/*   3 */ 	{S_KEYCODE_TWO,		S_KEYCODE_AT,		S_KEYCODE_AT},
/*   4 */ 	{S_KEYCODE_THREE,	S_KEYCODE_NUMBERSIGN,	S_KEYCODE_NOCODE},
/*   5 */ 	{S_KEYCODE_FOUR,	S_KEYCODE_DOLLAR,	S_KEYCODE_DOLLAR},
/*   6 */	{S_KEYCODE_FIVE,	S_KEYCODE_PERCENT,	S_KEYCODE_CURRENCY},
/*   7 */ 	{S_KEYCODE_SIX,		S_KEYCODE_ASCIICIRCUM,	S_KEYCODE_NOCODE},
/*   8 */ 	{S_KEYCODE_SEVEN,	S_KEYCODE_AMPERSAND,	S_KEYCODE_BRACELEFT},
/*   9 */ 	{S_KEYCODE_EIGHT,	S_KEYCODE_ASTERISK,	S_KEYCODE_BRACKETLEFT},
/*  10 */ 	{S_KEYCODE_NINE,	S_KEYCODE_PARENLEFT,	S_KEYCODE_BRACKETRIGHT},
/*  11 */ 	{S_KEYCODE_ZERO,	S_KEYCODE_PARENRIGHT,	S_KEYCODE_BRACERIGHT},
/*  12 */ 	{S_KEYCODE_MINUS,	S_KEYCODE_UNDERSCORE,	S_KEYCODE_BACKSLASH},
/*  13 */ 	{S_KEYCODE_EQUAL,	S_KEYCODE_PLUS,		S_KEYCODE_NOCODE},
/*  14 */ 	{S_KEYCODE_DELETE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  15 */ 	{S_KEYCODE_TAB,		S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  16 */ 	{S_KEYCODE_q,		S_KEYCODE_Q,		S_KEYCODE_NOCODE},
/*  17 */ 	{S_KEYCODE_w,		S_KEYCODE_W,		S_KEYCODE_NOCODE},
/*  18 */ 	{S_KEYCODE_e,		S_KEYCODE_E,		S_KEYCODE_NOCODE},
/*  19 */ 	{S_KEYCODE_r,		S_KEYCODE_R,		S_KEYCODE_NOCODE},
/*  20 */ 	{S_KEYCODE_t,		S_KEYCODE_T,		S_KEYCODE_NOCODE},
/*  21 */ 	{S_KEYCODE_y,		S_KEYCODE_Y,		S_KEYCODE_NOCODE},
/*  22 */ 	{S_KEYCODE_u,		S_KEYCODE_U,		S_KEYCODE_NOCODE},
/*  23 */ 	{S_KEYCODE_i,		S_KEYCODE_I,		S_KEYCODE_NOCODE},
/*  24 */ 	{S_KEYCODE_o,		S_KEYCODE_O,		S_KEYCODE_NOCODE},
/*  25 */ 	{S_KEYCODE_p,		S_KEYCODE_P,		S_KEYCODE_NOCODE},
/*  26 */	{S_KEYCODE_BRACKETLEFT,	S_KEYCODE_BRACELEFT,	S_KEYCODE_NOCODE},
/*  27 */ 	{S_KEYCODE_BRACKETRIGHT,S_KEYCODE_BRACERIGHT,	S_KEYCODE_ASCIITILDE},
/*  28 */ 	{S_KEYCODE_RETURN,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  29 */ 	{S_KEYCODE_LEFTCONTROL,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  30 */ 	{S_KEYCODE_a,		S_KEYCODE_A,		S_KEYCODE_NOCODE},
/*  31 */	{S_KEYCODE_s,		S_KEYCODE_S,		S_KEYCODE_NOCODE},
/*  32 */ 	{S_KEYCODE_d,		S_KEYCODE_D,		S_KEYCODE_NOCODE},
/*  33 */	{S_KEYCODE_f,		S_KEYCODE_F,		S_KEYCODE_NOCODE},
/*  34 */ 	{S_KEYCODE_g,		S_KEYCODE_G,		S_KEYCODE_NOCODE},
/*  35 */ 	{S_KEYCODE_h,		S_KEYCODE_H,		S_KEYCODE_NOCODE},
/*  36 */ 	{S_KEYCODE_j,		S_KEYCODE_J,		S_KEYCODE_NOCODE},
/*  37 */ 	{S_KEYCODE_k,		S_KEYCODE_K,		S_KEYCODE_NOCODE},
/*  38 */ 	{S_KEYCODE_l,		S_KEYCODE_L,		S_KEYCODE_NOCODE},
/*  39 */ 	{S_KEYCODE_SEMICOLON,	S_KEYCODE_COLON,	S_KEYCODE_NOCODE},
/*  40 */ 	{S_KEYCODE_APOSTROPHE,	S_KEYCODE_QUOTEDBL,	S_KEYCODE_NOCODE},
/*  41 */ 	{S_KEYCODE_GRAVE,	S_KEYCODE_ASCIITILDE,	S_KEYCODE_NOCODE},
/*  42 */ 	{S_KEYCODE_LEFTSHIFT,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  43 */ 	{S_KEYCODE_BACKSLASH,	S_KEYCODE_BAR,		S_KEYCODE_NOCODE},
/*  44 */ 	{S_KEYCODE_z,		S_KEYCODE_Z,		S_KEYCODE_NOCODE},
/*  45 */	{S_KEYCODE_x,		S_KEYCODE_X,		S_KEYCODE_NOCODE},
/*  46 */ 	{S_KEYCODE_c,		S_KEYCODE_C,		S_KEYCODE_CENT},
/*  47 */ 	{S_KEYCODE_v,		S_KEYCODE_V,		S_KEYCODE_NOCODE},
/*  48 */ 	{S_KEYCODE_b,		S_KEYCODE_B,		S_KEYCODE_NOCODE},
/*  49 */ 	{S_KEYCODE_n,		S_KEYCODE_N,		S_KEYCODE_NOCODE},
/*  50 */ 	{S_KEYCODE_m,		S_KEYCODE_M,		S_KEYCODE_NOCODE},
/*  51 */ 	{S_KEYCODE_COMMA,	S_KEYCODE_LESS,		S_KEYCODE_NOCODE},
/*  52 */ 	{S_KEYCODE_PERIOD,	S_KEYCODE_GREATER,	S_KEYCODE_NOCODE},
/*  53 */ 	{S_KEYCODE_SLASH,	S_KEYCODE_QUESTION,	S_KEYCODE_NOCODE},
/*  54 */ 	{S_KEYCODE_RIGHTSHIFT,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  55 */ 	{S_KEYCODE_KP_MULTIPLY,	S_KEYCODE_NOCODE,	S_KEYCODE_HEX_C},
/*  56 */ 	{S_KEYCODE_ALT,		S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  57 */ 	{S_KEYCODE_SPACE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  58 */ 	{S_KEYCODE_CAPS_LOCK,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  59 */ 	{S_KEYCODE_F1,		S_KEYCODE_F13,		S_KEYCODE_NOCODE},
/*  60 */ 	{S_KEYCODE_F2,		S_KEYCODE_F14,		S_KEYCODE_NOCODE},
/*  61 */ 	{S_KEYCODE_F3,		S_KEYCODE_F15,		S_KEYCODE_NOCODE},
/*  62 */ 	{S_KEYCODE_F4,		S_KEYCODE_F16,		S_KEYCODE_NOCODE},
/*  63 */ 	{S_KEYCODE_F5,		S_KEYCODE_F17,		S_KEYCODE_NOCODE},
/*  64 */ 	{S_KEYCODE_F6,		S_KEYCODE_F18,		S_KEYCODE_NOCODE},
/*  65 */ 	{S_KEYCODE_F7,		S_KEYCODE_F19,		S_KEYCODE_NOCODE},
/*  66 */ 	{S_KEYCODE_F8,		S_KEYCODE_F20,		S_KEYCODE_NOCODE},
/*  67 */ 	{S_KEYCODE_F9,		S_KEYCODE_F21,		S_KEYCODE_NOCODE},
/*  68 */ 	{S_KEYCODE_F10,		S_KEYCODE_F22,		S_KEYCODE_NOCODE},
/*  69 */ 	{S_KEYCODE_NUM_LOCK,	S_KEYCODE_NOCODE,	S_KEYCODE_HEX_A},
/*  70 */ 	{S_KEYCODE_SCROLL_LOCK,	S_KEYCODE_SHOW_MEMORY,	S_KEYCODE_SHOW_REGISTERS},
/*  71 */ 	{S_KEYCODE_KP_7,	S_KEYCODE_HOME,		S_KEYCODE_HEX_7},
/*  72 */ 	{S_KEYCODE_KP_8,	S_KEYCODE_UP,		S_KEYCODE_HEX_8},
/*  73 */ 	{S_KEYCODE_KP_9,	S_KEYCODE_PAGEUP,	S_KEYCODE_HEX_9},
/*  74 */ 	{S_KEYCODE_KP_SUBTRACT,	S_KEYCODE_NOCODE,	S_KEYCODE_HEX_D},
/*  75 */ 	{S_KEYCODE_KP_4,	S_KEYCODE_LEFT,		S_KEYCODE_HEX_4},
/*  76 */	{S_KEYCODE_KP_5,	S_KEYCODE_NOCODE,	S_KEYCODE_HEX_5},
/*  77 */	{S_KEYCODE_KP_6,	S_KEYCODE_RIGHT,	S_KEYCODE_HEX_6},
/*  78 */	{S_KEYCODE_KP_ADD,	S_KEYCODE_NOCODE,	S_KEYCODE_HEX_E},
/*  79 */ 	{S_KEYCODE_KP_1,	S_KEYCODE_END,		S_KEYCODE_HEX_1},
/*  80 */ 	{S_KEYCODE_KP_2,	S_KEYCODE_DOWN,		S_KEYCODE_HEX_2},
/*  81 */ 	{S_KEYCODE_KP_3,	S_KEYCODE_PAGEDOWN,	S_KEYCODE_HEX_3},
/*  82 */ 	{S_KEYCODE_KP_0,	S_KEYCODE_INSERT,	S_KEYCODE_HEX_0},
/*  83 */ 	{S_KEYCODE_KP_PERIOD,	S_KEYCODE_REMOVE,	S_KEYCODE_NOCODE},
/*  84 */	{S_KEYCODE_LAST_CONSOLE,S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  85 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  86 */	{S_KEYCODE_LESS,	S_KEYCODE_GREATER,	S_KEYCODE_BAR},
/*  87 */ 	{S_KEYCODE_F11,		S_KEYCODE_F23,		S_KEYCODE_NOCODE},
/*  88 */ 	{S_KEYCODE_F12,		S_KEYCODE_F24,		S_KEYCODE_NOCODE},
/*  89 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  90 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  91 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  92 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  93 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  94 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  95 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  96 */ 	{S_KEYCODE_KP_ENTER,	S_KEYCODE_NOCODE,	S_KEYCODE_HEX_F},
/*  97 */ 	{S_KEYCODE_RIGHTCONTROL,S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/*  98 */ 	{S_KEYCODE_KP_DIVIDE,	S_KEYCODE_NOCODE,	S_KEYCODE_HEX_B},
/*  99 */ 	{S_KEYCODE_VOIDSYMBOL,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 100 */ 	{S_KEYCODE_ALTGR,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 101 */ 	{S_KEYCODE_BREAK,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 102 */ 	{S_KEYCODE_HOME,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 103 */ 	{S_KEYCODE_UP,		S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 104 */	{S_KEYCODE_PAGEUP,	S_KEYCODE_SCROLL_BACKWARD,	S_KEYCODE_NOCODE},
/* 105 */	{S_KEYCODE_LEFT,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 106 */	{S_KEYCODE_RIGHT,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 107 */	{S_KEYCODE_END,		S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 108 */	{S_KEYCODE_DOWN,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 109 */	{S_KEYCODE_PAGEDOWN,	S_KEYCODE_SCROLL_FORWARD,	S_KEYCODE_NOCODE},
/* 110 */	{S_KEYCODE_INSERT,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 111 */	{S_KEYCODE_REMOVE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 112 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 113 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 114 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 115 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 116 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 117 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 118 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 119 */ 	{S_KEYCODE_PAUSE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 120 */ 	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 121 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 122 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 123 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 124 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 125 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 126 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
/* 127 */	{S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE,	S_KEYCODE_NOCODE},
};
