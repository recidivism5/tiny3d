#include <tiny3d.h>

char *assertPath;

static char fmtStr[4096];

void fatal_error(char *format, ...){
	va_list args;
	va_start(args,format);

	vsnprintf(fmtStr,COUNT(fmtStr),format,args);
	fprintf(stderr,"%s\n",fmtStr);
	error_box(fmtStr);

	va_end(args);
	
	exit(1);
}

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