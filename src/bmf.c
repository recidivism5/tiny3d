#include <bmf.h>
#include <stb_image.h>

void load_bmf(BMF *b, char *path){
    assertPath = local_path_to_absolute(path);
    FILE *f = fopen(assertPath,"rb");
    ASSERT_FILE(f);
    ASSERT_FILE(1 == fread(&b->textureCount,sizeof(b->textureCount),1,f));
    ASSERT_FILE(b->textureCount > 0 && b->textureCount < 256);
    b->textures = malloc(b->textureCount * sizeof(*b->textures));
    ASSERT_FILE(b->textures);
    for (Image *t = b->textures; t < b->textures+b->textureCount; t++){
        int size, comp;
        ASSERT_FILE(1 == fread(&size,sizeof(size),1,f));
        t->pixels = (Color *)stbi_load_from_file(f,&t->width,&t->height,&comp,4); //UB: unknown if stb is fault tolerant. I don't think it is.
        ASSERT_FILE(t->pixels);
    }
    ASSERT_FILE(1 == fread(&b->objectCount,sizeof(b->objectCount),1,f));
    ASSERT_FILE(b->objectCount > 0 && b->objectCount < 256);
    b->objects = malloc(b->objectCount * sizeof(*b->objects));
    ASSERT_FILE(b->objects);
    for (Object *o = b->objects; o < b->objects+b->objectCount; o++){
        int len;
        ASSERT_FILE(1 == fread(&len,sizeof(len),1,f));
        ASSERT_FILE(len > 0 && len < 256);
        o->name = malloc(len+1);
        ASSERT_FILE(o->name);
        ASSERT_FILE(1 == fread(o->name,len,1,f));
        o->name[len] = 0;
        ASSERT_FILE(1 == fread(o->position,sizeof(o->position),1,f));
        ASSERT_FILE(1 == fread(&o->vertexCount,sizeof(o->vertexCount),1,f));
        ASSERT_FILE(o->vertexCount > 0 && o->vertexCount < 65536);
        o->vertices = malloc(o->vertexCount * sizeof(*o->vertices));
        ASSERT_FILE(o->vertices);
        ASSERT_FILE(1 == fread(o->vertices,o->vertexCount*sizeof(*o->vertices),1,f)); //UB
        ASSERT_FILE(1 == fread(&o->uvCount,sizeof(o->uvCount),1,f));
        ASSERT_FILE(o->uvCount > 0 && o->uvCount < 65536);
        o->uvs = malloc(o->uvCount * sizeof(*o->uvs));
        ASSERT_FILE(o->uvs);
        ASSERT_FILE(1 == fread(o->uvs,o->uvCount * sizeof(*o->uvs),1,f)); //UB
        ASSERT_FILE(1 == fread(&o->trigroupCount,sizeof(o->trigroupCount),1,f));
        ASSERT_FILE(o->trigroupCount > 0 && o->trigroupCount <= b->textureCount);
        o->trigroups = malloc(o->trigroupCount * sizeof(*o->trigroups));
        ASSERT_FILE(o->trigroups);
        for (Trigroup *t = o->trigroups; t < o->trigroups+o->trigroupCount; t++){
            ASSERT_FILE(1 == fread(&t->textureIndex,sizeof(t->textureIndex),1,f));
            ASSERT_FILE(t->textureIndex >= 0 && t->textureIndex < b->textureCount);
            ASSERT_FILE(1 == fread(&t->vertexIndexCount,sizeof(t->vertexIndexCount),1,f));
            ASSERT_FILE(t->vertexIndexCount > 0 && t->vertexIndexCount < 65536 && (t->vertexIndexCount % 3) == 0);
            t->vertexIndices = malloc(t->vertexIndexCount * sizeof(*t->vertexIndices));
            ASSERT_FILE(t->vertexIndices);
            ASSERT_FILE(1 == fread(t->vertexIndices,t->vertexIndexCount*sizeof(*t->vertexIndices),1,f)); //UB
        }
    }
}