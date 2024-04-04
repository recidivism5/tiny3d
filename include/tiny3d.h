#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include <debugbreak.h>
#include <whereami.h>
#include <tinymath.h>

#define TINY3D_SAMPLE_RATE 44100
#define TINY3D_AUDIO_BUFSZ 8192

#define USE_GL 0
#if USE_GL
	#if __APPLE__
		#include <OpenGL/OpenGL.h>
		#include <OpenGL/gl.h>
	#else
		#include <GL/gl.h>
	#endif
	void open_window(int width, int height); //width = 0: fullscreen
	//define this:
	extern void update(double time, double deltaTime, int width, int height, int nSamples, signed short *samples);
#else
	#define SCREEN_WIDTH 256
	#define SCREEN_HEIGHT 192
	extern uint32_t screen[SCREEN_WIDTH*SCREEN_HEIGHT];
	void open_window(int scale); //scale = 0: fullscreen
	//define this:
	extern void update(double time, double deltaTime, int nSamples, signed short *samples);

	//drawing functions:
	void clear_screen(uint32_t color);
	void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
#endif

//define these:
#define KEY_MOUSE_LEFT 128
#define KEY_MOUSE_RIGHT 129
extern void keydown(int key);
extern void keyup(int key);
extern void mousemove(int x, int y);

bool is_mouse_locked(void);
void lock_mouse(bool locked);

//utility functions:
void error_box(char *msg);
void fatal_error(char *format, ...);
char *format_string(char *format, ...);
char *local_path_to_absolute(char *format, ...);
char *local_path_to_absolute_vararg(char *format, va_list args);
FILE *fopen_relative(char *format, ...);
unsigned char *load_file(int *size, char *format, ...);
char *load_file_as_cstring(char *format, ...);
uint32_t *load_image(bool flip_vertically, int *width, int *height, char *format, ...);

#define COUNT(arr) (sizeof(arr)/sizeof(*arr))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLAMP(a,min,max) ((a) < (min) ? (min) : (a) > (max) ? (max) : (a))
#define SWAP(temp,a,b) (temp)=(a); (a)=(b); (b)=(temp)
#define COMPARE(a,b) (((a) > (b)) - ((a) < (b)))
#define RGBA(r,g,b,a) ((r) | ((g)<<8) | ((b)<<16) | ((a)<<24))

#define FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#ifndef NDEBUG
#define ASSERT(cnd)\
	do {\
		if (!(cnd)){\
			fprintf(stderr,"Assert failed:\nFile: %s\nLine: %d\nCondition: %s\n",FILENAME,__LINE__,#cnd);\
			debug_break();\
			exit(1);\
		}\
	} while (0)
#else
#define ASSERT(cnd)\
	do {\
		if (!(cnd)){\
			fatal_error("Assert failed:\nFile: %s\nLine: %d\nCondition: %s",FILENAME,__LINE__,#cnd);\
			exit(1);\
		}\
	} while (0)
#endif

extern char *assertPath;
#ifndef NDEBUG
#define ASSERT_FILE(cnd)\
	do {\
		if (!(cnd)){\
			fprintf(stderr,"Assert failed:\nFile: %s\nLine: %d\nCondition: %s\nFailed to load file: %s\n",FILENAME,__LINE__,#cnd,assertPath);\
			debug_break();\
			exit(1);\
		}\
	} while (0)
#else
#define ASSERT_FILE(cnd)\
	do {\
		if (!(cnd)){\
			fatal_error("Assert failed:\nFile: %s\nLine: %d\nCondition: %s\nFailed to load file: %s",FILENAME,__LINE__,#cnd,assertPath);\
			exit(1);\
		}\
	} while (0)
#endif