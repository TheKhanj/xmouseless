/* xmouseless */
#include <X11/X.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XTest.h>

#define LENGTH(X) (sizeof X / sizeof X[0])

#include "config.h"
#include "xmouseless.h"

Display *dpy;
Window root;
Cursor scroll_cursor;
pthread_t movethread;

static unsigned int speed;
static Mode last_mode;

struct {
	float x;
	float y;
	float speed_x;
	float speed_y;
} mouse_info;

struct {
	float x;
	float y;
	float speed_x;
	float speed_y;
} scroll_info;

struct {
	float x;
	float y;
} scroll_mode_mouse_info;

void get_pointer();
void move_relative(float x, float y);
void click(unsigned int button, Bool is_press);
void click_full(unsigned int button);
void scroll(float x, float y);
void handle_key(KeyCode keycode, Bool is_press);
void init_x();
void close_x(int exit_code);

void get_pointer() {
	int x, y;
	int di;
	unsigned int dui;
	Window dummy;
	XQueryPointer(dpy, root, &dummy, &dummy, &x, &y, &di, &di, &dui);
	mouse_info.x = x;
	mouse_info.y = y;
}

void move_relative(float x, float y) {
	mouse_info.x += x;
	mouse_info.y += y;
	XWarpPointer(dpy, None, root, 0, 0, 0, 0, (int)mouse_info.x,
							 (int)mouse_info.y);
	XFlush(dpy);
}

void click(unsigned int button, Bool is_press) {
	XTestFakeButtonEvent(dpy, button, is_press, CurrentTime);
	XFlush(dpy);
}

void click_full(unsigned int button) {
	XTestFakeButtonEvent(dpy, button, 1, CurrentTime);
	XTestFakeButtonEvent(dpy, button, 0, CurrentTime);
	XFlush(dpy);
}

bool in_range(float x) { return -0.5 < x && x < 0.5; }

bool is_scrolling_speed_epsilon() {
	return in_range(scroll_info.speed_x / move_rate * 4) &&
				 in_range(scroll_info.speed_y / move_rate * 4);
}

void scroll(float x, float y) {
	Window focused;
	int revert_to;
	XGetInputFocus(dpy, &focused, &revert_to);
	XDefineCursor(dpy, focused, scroll_cursor);

	scroll_info.x += x;
	scroll_info.y += y;
	while (scroll_info.y <= -0.51) {
		scroll_info.y += 1;
		click_full(4);
	}
	while (scroll_info.y >= 0.51) {
		scroll_info.y -= 1;
		click_full(5);
	}
	while (scroll_info.x <= -0.51) {
		scroll_info.x += 1;
		click_full(6);
	}
	while (scroll_info.x >= 0.51) {
		scroll_info.x -= 1;
		click_full(7);
	}

	if (is_scrolling_speed_epsilon()) {
		XUndefineCursor(dpy, focused);
	}
}

void init_x() {
	int i;
	int screen;

	/* initialize support for concurrent threads */
	XInitThreads();

	dpy = XOpenDisplay((char *)0);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	scroll_cursor = XCreateFontCursor(dpy, XC_fleur);

	/* turn auto key repeat off */
	XAutoRepeatOff(dpy);

	/* grab keys until success */
	for (i = 0; i < 100; i++) {
		if (XGrabKeyboard(dpy, root, False, GrabModeAsync, GrabModeAsync,
											CurrentTime) == GrabSuccess) {
			return;
		}
		usleep(10000);
	}

	printf("grab keyboard failed\n");
	close_x(EXIT_FAILURE);
}

void close_x(int exit_status) {
	/* turn auto repeat on again */
	XAutoRepeatOn(dpy);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	XCloseDisplay(dpy);
	exit(exit_status);
}

void *move_forever(void *val) {
	/* this function is executed in a seperate thread */
	while (1) {
		/* move mouse? */
		if (mouse_info.speed_x != 0 || mouse_info.speed_y != 0) {
			move_relative((float)mouse_info.speed_x * speed / move_rate,
										(float)mouse_info.speed_y * speed / move_rate);
		}
		/* scroll? */
		if (!is_scrolling_speed_epsilon()) {
			scroll((float)scroll_info.speed_x / move_rate,
						 (float)scroll_info.speed_y / move_rate);
		}
		usleep(1000000 / move_rate);
	}
}

void handle_normal_mode_key(KeySym keysym, Bool is_press) {
	unsigned int i;

	/* move bindings */
	for (i = 0; i < LENGTH(move_bindings); i++) {
		if (move_bindings[i].keysym == keysym) {
			int sign = is_press ? 1 : -1;
			mouse_info.speed_x += sign * move_bindings[i].x;
			mouse_info.speed_y += sign * move_bindings[i].y;
		}
	}

	/* click bindings */
	for (i = 0; i < LENGTH(click_bindings); i++) {
		if (click_bindings[i].keysym == keysym) {
			click(click_bindings[i].button, is_press);
			printf("click: %i %i\n", click_bindings[i].button, is_press);
		}
	}

	/* speed bindings */
	for (i = 0; i < LENGTH(speed_bindings); i++) {
		if (speed_bindings[i].keysym == keysym) {
			speed = is_press ? speed_bindings[i].speed : default_speed;
			printf("speed: %i\n", speed);
		}
	}

	/* scroll bindings */
	for (i = 0; i < LENGTH(scroll_bindings); i++) {
		if (scroll_bindings[i].keysym == keysym) {
			int sign = is_press ? 1 : -1;
			scroll_info.speed_x += sign * scroll_bindings[i].x;
			scroll_info.speed_y += sign * scroll_bindings[i].y;
		}
	}

	/* shell and exit bindings only on key release */
	if (!is_press) {
		/* shell bindings */
		for (i = 0; i < LENGTH(shell_bindings); i++) {
			if (shell_bindings[i].keysym == keysym) {
				printf("executing: %s\n", shell_bindings[i].command);
				if (fork() == 0) {
					system(shell_bindings[i].command);
					exit(EXIT_SUCCESS);
				}
			}
		}

		/* exit */
		for (i = 0; i < LENGTH(exit_keys); i++) {
			if (exit_keys[i] == keysym) {
				close_x(EXIT_SUCCESS);
			}
		}
	}
}

void handle_scroll_mode_key(KeySym keysym, Bool is_press) {
	unsigned int i;

	for (i = 0; i < LENGTH(move_bindings); i++) {
		if (move_bindings[i].keysym == keysym) {
			if (is_press) {
				scroll_mode_mouse_info.x += move_bindings[i].x * move_rate / 8;
				scroll_mode_mouse_info.y += move_bindings[i].y * move_rate / 8;
				printf("scroll mode speed x: %f, y: %f\n", scroll_mode_mouse_info.x,
							 scroll_mode_mouse_info.y);
			}
		}
	}

	scroll_info.speed_x = scroll_mode_mouse_info.x;
	scroll_info.speed_y = scroll_mode_mouse_info.y;
}

void switch_mode_if_necessary(KeySym keysym, Bool is_press) {
	for (int new_mode = 0; new_mode < LENGTH(mode_modifiers); new_mode++) {
		const ModeModifier *modif = mode_modifiers + new_mode;
		if (modif->keysym != keysym)
			continue;
		if (modif->sticky) {
			if (is_press) {
				last_mode = mode;
				mode = new_mode;
			} else
				mode = last_mode;

		} else
			mode = new_mode;
	}

	if (keysym == mode_modifiers[ScrollMode].keysym && !is_press) {
		scroll_info.speed_x = 0;
		scroll_info.speed_y = 0;
		scroll_mode_mouse_info.x = 0;
		scroll_mode_mouse_info.y = 0;
		Window focused;
		int revert_to;
		XGetInputFocus(dpy, &focused, &revert_to);
		XUndefineCursor(dpy, focused);
	}
}

void handle_key(KeyCode keycode, Bool is_press) {
	KeySym keysym;
	keysym = XkbKeycodeToKeysym(dpy, keycode, 0, 0);
	switch_mode_if_necessary(keysym, is_press);

	if (mode == NormalMode) {
		handle_normal_mode_key(keysym, is_press);
	} else if (mode == ScrollMode) {
		handle_scroll_mode_key(keysym, is_press);
	}
}

int main() {
	char keys_return[32];
	int rc;
	int i, j;

	init_x();

	get_pointer();
	mouse_info.speed_x = 0;
	mouse_info.speed_y = 0;
	speed = default_speed;
	last_mode = mode;

	scroll_info.x = 0;
	scroll_info.y = 0;
	scroll_info.speed_x = 0;
	scroll_info.speed_y = 0;

	scroll_mode_mouse_info.x = 0;
	scroll_mode_mouse_info.y = 0;

	/* start the thread for mouse movement and scrolling */
	rc = pthread_create(&movethread, NULL, &move_forever, NULL);
	if (rc != 0) {
		printf("Unable to start thread.\n");
		return EXIT_FAILURE;
	}
	/* get the initial state of all keys */
	XQueryKeymap(dpy, keys_return);
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 8; j++) {
			if (keys_return[i] & (1 << j)) {
				handle_key(8 * i + j, 1);
			}
		}
	}

	/* handle key presses and releases */
	while (1) {
		XEvent event;
		XNextEvent(dpy, &event);

		switch (event.type) {
		case KeyPress:
		case KeyRelease:
			get_pointer();
			handle_key(event.xkey.keycode, event.xkey.type == KeyPress);
			break;
		}
	}
}
