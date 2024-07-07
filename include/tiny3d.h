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
#include <limits.h>

#include <debugbreak.h>
#include <whereami.h>
#include <tinymath.h>

#if __APPLE__
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
	#if _WIN32
		#define WIN32_LEAN_AND_MEAN
		#define NOMINMAX
		#define UNICODE
		#define COBJMACROS
		#include <windows.h>//must include before gl.h
	#endif
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif

#define TINY3D_SAMPLE_RATE 44100
#define TINY3D_AUDIO_BUFSZ 8192

#define KEY_MOUSE_LEFT -1
#define KEY_MOUSE_RIGHT -2
typedef enum {
    US_SCANCODE_ESC = 1,
    US_SCANCODE_1 = 2,
    US_SCANCODE_2 = 3,
    US_SCANCODE_3 = 4,
    US_SCANCODE_4 = 5,
    US_SCANCODE_5 = 6,
    US_SCANCODE_6 = 7,
    US_SCANCODE_7 = 8,
    US_SCANCODE_8 = 9,
    US_SCANCODE_9 = 10,
    US_SCANCODE_0 = 11,
    US_SCANCODE_MINUS = 12,
    US_SCANCODE_EQUAL = 13,
    US_SCANCODE_BACKSPACE = 14,
    US_SCANCODE_TAB = 15,
    US_SCANCODE_Q = 16,
    US_SCANCODE_W = 17,
    US_SCANCODE_E = 18,
    US_SCANCODE_R = 19,
    US_SCANCODE_T = 20,
    US_SCANCODE_Y = 21,
    US_SCANCODE_U = 22,
    US_SCANCODE_I = 23,
    US_SCANCODE_O = 24,
    US_SCANCODE_P = 25,
    US_SCANCODE_LEFTBRACKET = 26,
    US_SCANCODE_RIGHTBRACKET = 27,
    US_SCANCODE_ENTER = 28,
    US_SCANCODE_CTRL = 29,
    US_SCANCODE_A = 30,
    US_SCANCODE_S = 31,
    US_SCANCODE_D = 32,
    US_SCANCODE_F = 33,
    US_SCANCODE_G = 34,
    US_SCANCODE_H = 35,
    US_SCANCODE_J = 36,
    US_SCANCODE_K = 37,
    US_SCANCODE_L = 38,
    US_SCANCODE_SEMICOLON = 39,
    US_SCANCODE_APOSTROPHE = 40,
    US_SCANCODE_GRAVE = 41,
    US_SCANCODE_LSHIFT = 42,
    US_SCANCODE_BACKSLASH = 43,
    US_SCANCODE_Z = 44,
    US_SCANCODE_X = 45,
    US_SCANCODE_C = 46,
    US_SCANCODE_V = 47,
    US_SCANCODE_B = 48,
    US_SCANCODE_N = 49,
    US_SCANCODE_M = 50,
    US_SCANCODE_COMMA = 51,
    US_SCANCODE_PERIOD = 52,
    US_SCANCODE_SLASH = 53,
    US_SCANCODE_RSHIFT = 54,
    US_SCANCODE_PRTSC = 55,
    US_SCANCODE_ALT = 56,
    US_SCANCODE_SPACE = 57,
    US_SCANCODE_CAPS = 58,
    US_SCANCODE_F1 = 59,
    US_SCANCODE_F2 = 60,
    US_SCANCODE_F3 = 61,
    US_SCANCODE_F4 = 62,
    US_SCANCODE_F5 = 63,
    US_SCANCODE_F6 = 64,
    US_SCANCODE_F7 = 65,
    US_SCANCODE_F8 = 66,
    US_SCANCODE_F9 = 67,
    US_SCANCODE_F10 = 68,
    US_SCANCODE_NUM = 69,
    US_SCANCODE_SCROLL = 70,
    US_SCANCODE_HOME = 71,
    US_SCANCODE_UP = 72,
    US_SCANCODE_PGUP = 73,
    US_SCANCODE_MINUS_KEYPAD = 74,
    US_SCANCODE_LEFT = 75,
    US_SCANCODE_CENTER = 76,
    US_SCANCODE_RIGHT = 77,
    US_SCANCODE_PLUS_KEYPAD = 78,
    US_SCANCODE_END = 79,
    US_SCANCODE_DOWN = 80,
    US_SCANCODE_PGDN = 81,
    US_SCANCODE_INS = 82,
    US_SCANCODE_DEL = 83
} us_scancode_e;

typedef struct {
	uint8_t r,g,b,a;
} color_t;

typedef struct {
	int width, height;
	color_t *pixels;
} image_t;

typedef struct {
	int width, height;
	color_t *pixels;
	float *depth;
} framebuffer_t;

//globals, use but don't modify:
extern int window_width, window_height; //updated automatically whenever the window size is changed

//define these:
extern void init(void); //called once after window+opengl context initialization, but before the first call to update
extern void update(double time, double deltaTime, int nAudioFrames, int16_t *audioSamples);
extern void keydown(int scancode);
extern void keyup(int scancode);
extern void mousemove(int x, int y);

//utility functions:
void open_window(int min_width, int min_height, char *name); //call this to start
bool is_fullscreen();
void toggle_fullscreen();
bool is_mouse_locked();
void toggle_mouse_lock();
void error_box(char *msg);
void fatal_error(char *format, ...);
char *format_string(char *format, ...);
char *local_path_to_absolute(char *format, ...);
char *local_path_to_absolute_vararg(char *format, va_list args);
FILE *fopen_relative(char *format, ...);
unsigned char *load_file(int *size, char *format, ...);
char *load_file_as_cstring(char *format, ...);
unsigned char *load_image(bool flip_vertically, int *width, int *height, char *format, ...);
int16_t *load_audio(int *nFrames, char *format, ...);
wchar_t *get_keyboard_layout_name();
void get_key_text(int scancode, wchar_t *buf, int bufcount);
float get_dpi_scale();
void get_font_name(char *path, char *out, int outCount);

//text:
void register_font(char *path);
void text_set_font(char *font_family, int size);
void text_set_target_image(unsigned char *pixels, int width, int height);
void text_set_color(float r, float g, float b);
void text_get_bounds(char *str, int *width, int *height);
void text_draw(int x, int y, char *str);

//2d:
void t2d_set_source_image(image_t *img);
void t2d_set_destination_image(image_t *img);
void t2d_blit(int srcx, int srcy, int width, int height, int dstx, int dsty, bool xflip);
void t2d_line(int x0, int y0, int x1, int y1, color_t color);

//3d:
void t3d_set_framebuffer(framebuffer_t *f);
void t3d_set_texture(image_t *t);
void t3d_clear(color_t color);
void t3d_perspective(float fovy, float aspect, float nearZ, float farZ);
void t3d_ortho(float left, float right, float bottom, float top, float nearZ, float farZ);
void t3d_load_identity();
void t3d_translate(float x, float y, float z);
void t3d_rotate(float x, float y, float z, float angle);
void t3d_position(float x, float y, float z);
void t3d_texcoord(float u, float v);

//software rendering assistance:
//draws a framebuffer with integer scaling as high as will fit, centered in the window
//rgb is background color
void draw_framebuffer(image_t *framebuffer, float r, float g, float b);
void get_framebuffer_pos(int x, int y, int *outx, int *outy);

#define COUNT(arr) (sizeof(arr)/sizeof(*arr))
#define LERP(a,b,t) ((a) + (t)*((b)-(a)))
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