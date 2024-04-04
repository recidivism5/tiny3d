#include <tiny3d.h>

char *assertPath;

void fatal_error(char *format, ...){
	va_list args;
	va_start(args,format);

	static char msg[4096];
	vsnprintf(msg,COUNT(msg),format,args);
	fprintf(stderr,"%s\n",msg);
	error_box(msg);

	va_end(args);
	
	exit(1);
}

static char fmtStr[4096];
char *format_string(char *format, ...){
	va_list args;
	va_start(args,format);

	int len = vsnprintf(fmtStr,COUNT(fmtStr),format,args);
	ASSERT(len > 0 && len < COUNT(fmtStr));

	va_end(args);

	return fmtStr;
}

static char root[4096];
static char *rootEnd = 0;
char *local_path_to_absolute_vararg(char *format, va_list args){
	int len;
	if (!rootEnd){
		len = wai_getExecutablePath(0,0,0);
		ASSERT(len < COUNT(root));
		ASSERT(0 < wai_getExecutablePath(root,len,0));
		root[len] = 0;

		rootEnd = strrchr(root,'/');
		if (!rootEnd){
			rootEnd = strrchr(root,'\\');
			ASSERT(rootEnd);
		}
		rootEnd++;
		rootEnd[0] = 0;
	}

	size_t remaining = root+COUNT(root)-rootEnd;
	len = vsnprintf(rootEnd,remaining,format,args);
	ASSERT(len > 0 && len < remaining);

	return root;
}

char *local_path_to_absolute(char *format, ...){
	va_list args;
	va_start(args,format);

	char *path = local_path_to_absolute_vararg(format,args);

	va_end(args);

	return path;
}

FILE *fopen_relative(char *format, ...){
	va_list args;
	va_start(args,format);

	assertPath = local_path_to_absolute_vararg(format,args);
	FILE *f = fopen(assertPath,"rb");
	ASSERT_FILE(f);

	va_end(args);

	return f;
}

unsigned char *load_file(int *size, char *format, ...){
	va_list args;
	va_start(args,format);

	assertPath = local_path_to_absolute_vararg(format,args);
	FILE *f = fopen(assertPath,"rb");
	ASSERT_FILE(f);
	fseek(f,0,SEEK_END);
	long len = ftell(f);
	ASSERT_FILE(len > 0);
	fseek(f,0,SEEK_SET);
	unsigned char *buf = malloc(len);
	ASSERT_FILE(buf);
	fread(buf,1,len,f);
	fclose(f);

	va_end(args);

	*size = len;

	return buf;
}

char *load_file_as_cstring(char *format, ...){
	va_list args;
	va_start(args,format);

	assertPath = local_path_to_absolute_vararg(format,args);
	FILE *f = fopen(assertPath,"rb");
	ASSERT_FILE(f);
	fseek(f,0,SEEK_END);
	long len = ftell(f);
	ASSERT_FILE(len > 0);
	fseek(f,0,SEEK_SET);
	char *str = malloc(len+1);
	ASSERT_FILE(str);
	fread(str,1,len,f);
	str[len] = 0;
	fclose(f);

	va_end(args);

	return str;
}

#if USE_GL
#else
	uint32_t screen[SCREEN_WIDTH*SCREEN_HEIGHT];
	
	void clear_screen(uint32_t color){
		for (int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; i++){
			screen[i] = color;
		}
	}

	void draw_line(int x0, int y0, int x1, int y1, uint32_t color){
		int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
		int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
		int err = (dx>dy ? dx : -dy)/2, e2;
		while (1){
			screen[y0*SCREEN_WIDTH+x0] = color;
			if (x0==x1 && y0==y1) break;
			e2 = err;
			if (e2 >-dx) { err -= dy; x0 += sx; }
			if (e2 < dy) { err += dx; y0 += sy; }
		}
	}
#endif