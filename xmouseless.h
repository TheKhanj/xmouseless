#ifndef XMOUSELESS_H
#define XMOUSELESS_H

#include <X11/X.h>

#define LENGTH(X) (sizeof X / sizeof X[0])

typedef struct {
	KeySym keysym;
	float x;
	float y;
} MoveBinding;

typedef struct {
	KeySym keysym;
	unsigned int button;
} ClickBinding;

typedef struct {
	KeySym keysym;
	float x;
	float y;
} ScrollBinding;

typedef struct {
	KeySym keysym;
	unsigned int speed;
} SpeedBinding;

typedef struct {
	KeySym keysym;
	char *command;
} ShellBinding;

#endif // XMOUSELESS_H
