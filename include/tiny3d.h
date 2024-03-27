#pragma once

#include <debugbreak.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
extern uint32_t screen[SCREEN_WIDTH*SCREEN_HEIGHT];

//define these:
extern void keydown(int key);
extern void keyup(int key);
extern void update(double time, double deltaTime);

//utility functions:
void error_box(char *msg);
void fatal_error(char *format, ...);
char *format_string(char *format, ...);
char *local_path_to_absolute(char *format, ...);
FILE *fopen_relative(char *format, ...);
unsigned char *load_file(int *size, char *format, ...);
char *load_file_as_cstring(char *format, ...);
uint32_t *load_image(bool flip_vertically, int *width, int *height, char *format, ...);
//scale = 0: fullscreen
void open_window(int scale);

//drawing functions:
void clear_screen(uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);

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

// (‑●‑●)> dual licensed under the WTFPL v2 and MIT licenses
//   without any warranty.
//   by Gregory Pakosz (@gpakosz)
// https://github.com/gpakosz/whereami

#ifndef WHEREAMI_H
#define WHEREAMI_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WAI_FUNCSPEC
  #define WAI_FUNCSPEC
#endif
#ifndef WAI_PREFIX
#define WAI_PREFIX(function) wai_##function
#endif

/**
 * Returns the path to the current executable.
 *
 * Usage:
 *  - first call `int length = wai_getExecutablePath(NULL, 0, NULL);` to
 *    retrieve the length of the path
 *  - allocate the destination buffer with `path = (char*)malloc(length + 1);`
 *  - call `wai_getExecutablePath(path, length, NULL)` again to retrieve the
 *    path
 *  - add a terminal NUL character with `path[length] = '\0';`
 *
 * @param out destination buffer, optional
 * @param capacity destination buffer capacity
 * @param dirname_length optional recipient for the length of the dirname part
 *   of the path.
 *
 * @return the length of the executable path on success (without a terminal NUL
 * character), otherwise `-1`
 */
WAI_FUNCSPEC
int WAI_PREFIX(getExecutablePath)(char* out, int capacity, int* dirname_length);

/**
 * Returns the path to the current module
 *
 * Usage:
 *  - first call `int length = wai_getModulePath(NULL, 0, NULL);` to retrieve
 *    the length  of the path
 *  - allocate the destination buffer with `path = (char*)malloc(length + 1);`
 *  - call `wai_getModulePath(path, length, NULL)` again to retrieve the path
 *  - add a terminal NUL character with `path[length] = '\0';`
 *
 * @param out destination buffer, optional
 * @param capacity destination buffer capacity
 * @param dirname_length optional recipient for the length of the dirname part
 *   of the path.
 *
 * @return the length of the module path on success (without a terminal NUL
 * character), otherwise `-1`
 */
WAI_FUNCSPEC
int WAI_PREFIX(getModulePath)(char* out, int capacity, int* dirname_length);

#ifdef __cplusplus
}
#endif

#endif // #ifndef WHEREAMI_H
