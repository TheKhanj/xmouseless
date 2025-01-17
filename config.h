#ifndef CONFIG_H
#define CONFIG_H

#include <X11/X.h>
#include <X11/Xutil.h>
#include <stdbool.h>

#include "xmouseless.h"

static Mode mode = NormalMode;

static const ModeModifier mode_modifiers[ModesLength] = {
		/* keysym         sticky */
		{.keysym = XK_semicolon, .sticky = false},
		{.keysym = XK_s, .sticky = true},
};

/**
 * the rate at which the mouse moves in Hz
 * does not change its speed
 */
static const unsigned int move_rate = 50;

/**
 * the default speed of the mouse pointer
 * in pixels per second
 */
static const unsigned int default_speed = 1500;

/* changes the speed of the mouse pointer */
static SpeedBinding speed_bindings[] = {
		/* key             speed */
		{XK_Alt_L, 3000},
		{XK_Control_L, 300},
};

/**
 * moves the mouse pointer
 * you can also add any other direction (e.g. diagonals)
 */
static MoveBinding move_bindings[] = {
		/* key         x      y */
		{XK_h, -1, 0},
		{XK_l, 1, 0},
		{XK_k, 0, -1},
		{XK_j, 0, 1},
};

/**
 * 1: left
 * 2: middle
 * 3: right
 */
static ClickBinding click_bindings[] = {
		/* key         button */
		{XK_f, 1},
		{XK_i, 2},
		{XK_d, 3},
};

/**
 * scrolls up, down, left and right
 * a higher value scrolls faster
 */
static ScrollBinding scroll_bindings[] = {
		{XK_n, 0, 25},
		{XK_p, 0, -25},
};

/* executes shell commands */
static ShellBinding shell_bindings[] = {

};

/* exits on key release which allows click and exit with one key */
static KeySym exit_keys[] = {XK_Escape, XK_q};

#endif // CONFIG_H
