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
extern void update(double time, double deltaTime, int nAudioFrames, int16_t *audioSamples);
extern void keydown(int scancode);
extern void keyup(int scancode);
extern void mousemove(int x, int y);

//utility functions:
void open_window(int width, int height); //call this to start
void toggle_fullscreen();
bool is_mouse_locked(void);
void lock_mouse(bool locked);
void error_box(char *msg);
void fatal_error(char *format, ...);
char *format_string(char *format, ...);
char *local_path_to_absolute(char *format, ...);
char *local_path_to_absolute_vararg(char *format, va_list args);
FILE *fopen_relative(char *format, ...);
unsigned char *load_file(int *size, char *format, ...);
char *load_file_as_cstring(char *format, ...);
uint32_t *load_image(bool flip_vertically, int *width, int *height, char *format, ...);
int16_t *load_audio(int *nFrames, char *format, ...);
wchar_t *get_keyboard_layout_name();
void get_key_text(int scancode, wchar_t *buf, int bufcount);
float get_dpi_scale();

//text:
void get_font_name(char *path, char *out, int outCount);
void text_set_target_image(uint32_t *pixels, int width, int height);
void text_set_font(char *ttfPathFormat, ...);
void text_set_font_height(int height);
void text_set_color(float r, float g, float b);
void text_draw(int left, int right, int bottom, int top, char *str);

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
void draw_framebuffer(image_t *framebuffer);

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