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

bool is_little_endian(){
    int i = 1;
    return *((char *)&i);
}

//This is TTF file header
typedef struct _tagTT_OFFSET_TABLE{
    USHORT uMajorVersion;
    USHORT uMinorVersion;
    USHORT uNumOfTables;
    USHORT uSearchRange;
    USHORT uEntrySelector;
    USHORT uRangeShift;
}TT_OFFSET_TABLE;

//Tables in TTF file and there placement and name (tag)
typedef struct _tagTT_TABLE_DIRECTORY{
    char szTag[4]; //table name
    ULONG uCheckSum; //Check sum
    ULONG uOffset; //Offset from beginning of file
    ULONG uLength; //length of the table in bytes
}TT_TABLE_DIRECTORY;

//Header of names table
typedef struct _tagTT_NAME_TABLE_HEADER{
    USHORT uFSelector; //format selector.
    USHORT uNRCount; //Name Records count
    USHORT uStorageOffset; //Offset for strings storage, 
                           //from start of the table
}TT_NAME_TABLE_HEADER;

//Record in names table
typedef struct _tagTT_NAME_RECORD{
    USHORT uPlatformID;
    USHORT uEncodingID;
    USHORT uLanguageID;
    USHORT uNameID;
    USHORT uStringLength;
    USHORT uStringOffset; //from start of storage area
}TT_NAME_RECORD;

#define SWAPSHORT(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPSHORT(HIWORD(x)), SWAPSHORT(LOWORD(x)))
#define ENSURE_SHORT(x) (is_little_endian() ? SWAPSHORT(x) : x)
#define ENSURE_LONG(x) (is_little_endian() ? SWAPLONG(x) : x)

void get_font_name(char *path, char *out, int outCount){
	FILE *f = fopen(path,"rb");
	ASSERT(f);
	TT_OFFSET_TABLE ttOffsetTable;
	ASSERT(1 == fread(&ttOffsetTable,sizeof(ttOffsetTable),1,f));
	ttOffsetTable.uMajorVersion = ENSURE_SHORT(ttOffsetTable.uMajorVersion);
    ttOffsetTable.uMinorVersion = ENSURE_SHORT(ttOffsetTable.uMinorVersion);
	ASSERT(ttOffsetTable.uMajorVersion == 1 || ttOffsetTable.uMinorVersion == 0);
	ttOffsetTable.uNumOfTables = ENSURE_SHORT(ttOffsetTable.uNumOfTables);
	ASSERT(ttOffsetTable.uNumOfTables > 0 && ttOffsetTable.uNumOfTables < 64);
	TT_TABLE_DIRECTORY tblDir;
	for (int i = 0; i < ttOffsetTable.uNumOfTables; i++){
		ASSERT(1 == fread(&tblDir,sizeof(tblDir),1,f));
		if (!strnicmp(tblDir.szTag,"name",4)){
			goto L1;
		}
	}
	ASSERT(0 && "get_font_name: name table not found.");
	L1:;
	tblDir.uLength = ENSURE_LONG(tblDir.uLength);
	tblDir.uOffset = ENSURE_LONG(tblDir.uOffset);
	ASSERT(!fseek(f,tblDir.uOffset,SEEK_SET));
	TT_NAME_TABLE_HEADER ttNTHeader;
    ASSERT(1 == fread(&ttNTHeader,sizeof(ttNTHeader),1,f));
	ttNTHeader.uFSelector = ENSURE_SHORT(ttNTHeader.uFSelector);
	ASSERT(ttNTHeader.uFSelector == 0);
    ttNTHeader.uNRCount = ENSURE_SHORT(ttNTHeader.uNRCount);
    ttNTHeader.uStorageOffset = ENSURE_SHORT(ttNTHeader.uStorageOffset);
    TT_NAME_RECORD ttRecord;
    for(int i = 0; i < ttNTHeader.uNRCount; i++){
		ASSERT(1 == fread(&ttRecord,sizeof(ttRecord),1,f));
		ttRecord.uNameID = ENSURE_SHORT(ttRecord.uNameID);
		//1 says that this is font name. 0 for example determines copyright info
		if(ttRecord.uNameID == 1){
			ttRecord.uStringLength = ENSURE_SHORT(ttRecord.uStringLength);
			if (ttRecord.uStringLength){
				ASSERT(ttRecord.uStringLength > 0 && ttRecord.uStringLength < outCount);
				ttRecord.uStringOffset = ENSURE_SHORT(ttRecord.uStringOffset);
				ASSERT(!fseek(f,tblDir.uOffset + ttNTHeader.uStorageOffset + ttRecord.uStringOffset, SEEK_SET));
				fread(out,ttRecord.uStringLength,1,f);
				out[ttRecord.uStringLength] = 0;
				fclose(f);
				return;
			}
		}
	}
	ASSERT(0 && "get_font_name: name not found.");
}