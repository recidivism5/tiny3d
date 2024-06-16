#include <stdio.h>
#include <stdlib.h>
#include "tinydir.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COUNT(arr) (sizeof(arr)/sizeof(*arr))
#define FOR(var,count) for(i32 var = 0; var < (count); var++)
typedef struct {
	u32 size;
	u8 *bytes;
}Bytes;
i32 main(i32 argc, u8 **argv){
	if (argc < 2){
		printf("Usage: iconwriter <folder>\nProduces icon.ico\n");
		return 1;
	}
	tinydir_dir dir;
	tinydir_open_sorted(&dir, argv[1]);
	i32 numImages = 0;
	for (int i = 0; i < dir.n_files; i++){
		tinydir_file file;
		tinydir_readfile_n(&dir,&file,i);
		if (!file.is_dir){
			numImages++;
		}
	}
	FILE *out = fopen("icon.ico","wb");
	u16 header[3] = {0,1,numImages};
	fwrite(header,sizeof(header),1,out);
	i32 offset = 6+16*numImages;
	Bytes *pngs = malloc(numImages*sizeof(Bytes));
	i32 imageIndex = 0;
	for (i32 i = 0; i < dir.n_files; i++){
		tinydir_file file;
		tinydir_readfile_n(&dir,&file,i);
		if (!file.is_dir){
			Bytes *p = pngs+imageIndex;
			FILE *in = fopen(file.path,"rb");
			fseek(in,0,SEEK_END);
			p->size = ftell(in);
			fseek(in,0,SEEK_SET);
			p->bytes = malloc(p->size);
			fread(p->bytes,p->size,1,in);
			fclose(in);
			u8 winfo[4]={p->bytes[16+3],p->bytes[16+4+3],0,0};
			fwrite(winfo,sizeof(winfo),1,out);
			u16 pinfo[2]={0,32};
			fwrite(pinfo,sizeof(pinfo),1,out);
			u32 binfo[2]={p->size,offset};
			fwrite(binfo,sizeof(binfo),1,out);
			offset += p->size;
			imageIndex++;
		}
	}
	for (i32 i = 0; i < numImages; i++){
		fwrite(pngs[i].bytes,pngs[i].size,1,out);
		free(pngs[i].bytes);
	}
	free(pngs);
	fclose(out);
	tinydir_close(&dir);
}