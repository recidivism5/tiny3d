#include <tiny3d.h>

int window_width, window_height;

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
    uint16_t uMajorVersion;
    uint16_t uMinorVersion;
    uint16_t uNumOfTables;
    uint16_t uSearchRange;
    uint16_t uEntrySelector;
    uint16_t uRangeShift;
}TT_OFFSET_TABLE;

//Tables in TTF file and there placement and name (tag)
typedef struct _tagTT_TABLE_DIRECTORY{
    char szTag[4]; //table name
    uint32_t uCheckSum; //Check sum
    uint32_t uOffset; //Offset from beginning of file
    uint32_t uLength; //length of the table in bytes
}TT_TABLE_DIRECTORY;

//Header of names table
typedef struct _tagTT_NAME_TABLE_HEADER{
    uint16_t uFSelector; //format selector.
    uint16_t uNRCount; //Name Records count
    uint16_t uStorageOffset; //Offset for strings storage, 
                           //from start of the table
}TT_NAME_TABLE_HEADER;

//Record in names table
typedef struct _tagTT_NAME_RECORD{
    uint16_t uPlatformID;
    uint16_t uEncodingID;
    uint16_t uLanguageID;
    uint16_t uNameID;
    uint16_t uStringLength;
    uint16_t uStringOffset; //from start of storage area
}TT_NAME_RECORD;

#ifndef _WIN32
	#define MAKEWORD(a, b)      ((uint16_t)(((uint8_t)(((uintptr_t)(a)) & 0xff)) | ((uint16_t)((uint8_t)(((uintptr_t)(b)) & 0xff))) << 8))
	#define MAKELONG(a, b)      ((uint32_t)(((uint16_t)(((uintptr_t)(a)) & 0xffff)) | ((uint32_t)((uint16_t)(((uintptr_t)(b)) & 0xffff))) << 16))
	#define LOWORD(l)           ((uint16_t)(((uintptr_t)(l)) & 0xffff))
	#define HIWORD(l)           ((uint16_t)((((uintptr_t)(l)) >> 16) & 0xffff))
	#define LOBYTE(w)           ((uint8_t)(((uintptr_t)(w)) & 0xff))
	#define HIBYTE(w)           ((uint8_t)((((uintptr_t)(w)) >> 8) & 0xff))
#endif
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
		if (!memcmp(tblDir.szTag,"name",4)){
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

static image_t *src_img, *dst_img;

void t2d_set_source_image(image_t *img){
	src_img = img;
}

void t2d_set_destination_image(image_t *img){
	dst_img = img;
}

void t2d_blit(int srcx, int srcy, int width, int height, int dstx, int dsty, bool xflip){
	if (dstx < -width || dsty < -height){
		return;
	}
	int xo = MAX(-dstx,0);
	int yo = MAX(-dsty,0);
	int w = MIN(dst_img->width-dstx,width);
	if (w <= 0){
		return;
	}
	int h = MIN(dst_img->height-dsty,height);
	if (h <= 0){
		return;
	}
	for (int y = yo; y < h; y++){
		for (int x = xo; x < w; x++){
			color_t *c = src_img->pixels + y*src_img->width + (xflip ? width-1-x : x);
			if (c->a) dst_img->pixels[(dsty+y)*dst_img->width+dstx+x] = *c;
		}
	}
}

void t2d_line(int x0, int y0, int x1, int y1, color_t color){
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = (dx>dy ? dx : -dy)/2, e2;
	while (x0 >= 0 && x0 < dst_img->width && y0 >= 0 && y0 < dst_img->height){
		dst_img->pixels[y0*dst_img->width+x0] = color;
		if (x0==x1 && y0==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

static mat4 project, model_view[32], model_view_project;
static int model_view_index = 0;

static vec3 positions[3];
static int position_count = 0;

static vec2 uvs[3];
static int uv_count = 0;

static framebuffer_t *framebuffer;
static image_t *texture;

///////////////////////////////////////// code:

static void mv_apply(){
	mat4_mul(model_view[model_view_index],model_view_project,model_view[model_view_index]);
	mat4_mul(project,model_view[model_view_index],model_view_project);
}

typedef struct {
	vec4 position;
	vec2 uv;
} vertex_t;

static void clip(vertex_t *vin, int cin, vertex_t *vout, int *cout, int axis, float coef){
	*cout = 0;
	vertex_t *prev = vin+cin-1;
	bool prevInside = prev->position[axis]*coef <= prev->position[3];
	for (int i = 0; i < cin; i++){
		vertex_t *cur = vin+i;
		bool curInside = cur->position[axis]*coef <= cur->position[3];
		
		if (prevInside ^ curInside){
			float p = prev->position[3] - prev->position[axis]*coef;
			float lerpAmt = p / (p - (cur->position[3] - cur->position[axis]*coef));
			vec4_lerp(
				(float *)prev->position,
				(float *)cur->position,
				lerpAmt,
				(float *)vout->position
			);
			vec2_lerp(
				(float *)prev->uv,
				(float *)cur->uv,
				lerpAmt,
				(float *)vout->uv
			);
			vout++;
			(*cout)++;
		}

		if (curInside){
			*vout = *cur;
			vout++;
			(*cout)++;
		}

		prev = cur;
		prevInside = curInside;
	}
}

static bool clip_axis(vertex_t *v, int *c, int axis){
	static vertex_t b[6];
	int bc = 0;
	clip(v,*c,b,&bc,axis,1.0f);
	if (!bc){
		return false;
	}
	*c = 0;
	clip(b,bc,v,c,axis,-1.0f);
	return (*c) > 0;
}

static int orient2d(ivec2 a, ivec2 b, ivec2 c){
	return (b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0]);
}

static void try_draw_triangle(){
	if (position_count == 3 && uv_count == 3){
		position_count = 0;
		uv_count = 0;
		static vertex_t cv[6];
		static ivec2 scrpos[6];
		int cvc = 3;
//TODO: draw triangle without clipping if all vertices are within view frustum: https://youtu.be/ai9SwUQF74c?si=PPILudXZ-LF8nhsr&t=548
		for (int i = 0; i < 3; i++){
			mat4_mul_vec3_pos(model_view_project,(float *)(positions+i),cv[i].position);
			vec2_copy(uvs[i],cv[i].uv);
		}
		if (
			clip_axis(cv,&cvc,0) &&
			clip_axis(cv,&cvc,1) &&
			clip_axis(cv,&cvc,2)
		){
			for (int j = 0; j < cvc; j++){
				for (int k = 0; k < 3; k++){
					cv[j].position[k] /= cv[j].position[3];
				}
				cv[j].position[3] = 1.0f / cv[j].position[3];
				cv[j].uv[0] *= cv[j].position[3];
				cv[j].uv[1] *= cv[j].position[3];
				scrpos[j][0] = (int)roundf((1.0f + cv[j].position[0]) * 0.5f * ((float)framebuffer->width-1));
				scrpos[j][1] = (int)roundf((1.0f + cv[j].position[1]) * 0.5f * ((float)framebuffer->height-1));
			}
			for (int j = 1; j < cvc-1; j++){
				float nz = 
					(cv[j].position[0]-cv[0].position[0])*(cv[j+1].position[1]-cv[0].position[1]) - 
					(cv[j].position[1]-cv[0].position[1])*(cv[j+1].position[0]-cv[0].position[0]);
				if (nz < 0.0f){
					return; //back face cull
				}
				ivec2 smin, smax;
				for (int k = 0; k < 2; k++){
					smin[k] = MIN(MIN(scrpos[0][k],scrpos[j][k]),scrpos[j+1][k]);
					smax[k] = MAX(MAX(scrpos[0][k],scrpos[j][k]),scrpos[j+1][k]);
				}
				ivec2 p;
				for (p[1] = smin[1]; p[1] <= smax[1]; p[1]++){
					for (p[0] = smin[0]; p[0] <= smax[0]; p[0]++){
						vec3 bary = {
							(float)orient2d(scrpos[j],scrpos[j+1],p),
							(float)orient2d(scrpos[j+1],scrpos[0],p),
							(float)orient2d(scrpos[0],scrpos[j],p)
						};
						if (bary[0] >= 0 && bary[1] >= 0 && bary[2] >= 0){
							float sum = bary[0]+bary[1]+bary[2];
							for (int k = 0; k < 3; k++){
								bary[k] /= sum;
							}
							float depth = 
								cv[0].position[2] * bary[0] +
								cv[j].position[2] * bary[1] +
								cv[j+1].position[2] * bary[2];
							if (depth < framebuffer->depth[p[1]*framebuffer->width+p[0]]){
								float wt = 1.0f / (bary[0] * cv[0].position[3] + bary[1] * cv[j].position[3] + bary[2] * cv[j+1].position[3]);
								vec2 uv;
								for (int k = 0; k < 2; k++){
									uv[k] = (cv[0].uv[k] * bary[0] +
											cv[j].uv[k] * bary[1] +
											cv[j+1].uv[k] * bary[2]) * wt;
								}
								uv[0] *= texture->width;
								uv[1] *= texture->height;
								uv[0] = MIN(uv[0],texture->width-1);
								uv[1] = MIN(uv[1],texture->height-1);
								color_t *c = texture->pixels + ((int)uv[1])*texture->width + (int)uv[0];
								if (c->a){
									framebuffer->pixels[p[1]*framebuffer->width+p[0]] = *c;
									framebuffer->depth[p[1]*framebuffer->width+p[0]] = depth;
								}
							}
						}
					}
				}
			}
		}
	}
}

//public 3d:

void t3d_set_framebuffer(framebuffer_t *f){
	framebuffer = f;
}

void t3d_set_texture(image_t *t){
	texture = t;
}

void t3d_clear(color_t color){
	for (int i = 0; i < framebuffer->width*framebuffer->height; i++){
		framebuffer->pixels[i] = color;
		framebuffer->depth[i] = 1.0f;
	}
}

void t3d_perspective(float fovy, float aspect, float nearZ, float farZ){
	mat4_persp_rh_no(project,fovy,aspect,nearZ,farZ);
}

void t3d_ortho(float left, float right, float bottom, float top, float nearZ, float farZ){
	mat4_ortho_rh_no(project,left,right,bottom,top,nearZ,farZ);
}

void t3d_load_identity(){
	mat4_identity(model_view[model_view_index]);
}

void t3d_translate(float x, float y, float z){
	mat4_translate(model_view_project,(vec3){x,y,z});
	mv_apply();
}

void t3d_rotate(float x, float y, float z, float angle){
	mat4_rotate(model_view_project,(vec3){x,y,z},angle);
	mv_apply();
}

void t3d_position(float x, float y, float z){
	positions[position_count][0] = x;
	positions[position_count][1] = y;
	positions[position_count][2] = z;
	position_count++;
	try_draw_triangle();
}

void t3d_texcoord(float u, float v){
	uvs[uv_count][0] = u;
	uvs[uv_count][1] = v;
	uv_count++;
	try_draw_triangle();
}

// software rendering assistance:
void draw_framebuffer(image_t *framebuffer){
    glViewport(0,0,window_width,window_height);
    glClearColor(0.0f,0.0f,1.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    static GLuint texture = 0;
    if (!texture){
        glGenTextures(1,&texture);
        glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer->width, framebuffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer->pixels);
    int scale = 1;
    while (framebuffer->width*scale <= window_width && framebuffer->height*scale <= window_height){
        scale++;
    }
    scale--;
    int scaledWidth = scale * framebuffer->width;
    int scaledHeight = scale * framebuffer->height;
    int x = window_width/2-scaledWidth/2;
    int y = window_height/2-scaledHeight/2;
    glViewport(x,y,scaledWidth,scaledHeight);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(-1,-1);
    glTexCoord2f(1,0); glVertex2f(1,-1);
    glTexCoord2f(1,1); glVertex2f(1,1);
    glTexCoord2f(0,1); glVertex2f(-1,1);
    glEnd();
}