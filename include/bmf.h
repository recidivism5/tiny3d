#pragma once

#include <base.h>
#include <linmath.h>
#include <image.h>

typedef struct {
    vec3 position, normal;
} Vertex;

typedef struct {
    vec4 position;
    vec3 normal;
    vec2 uv;
} ClipVertex;

typedef struct {
    int posnorm, uv;
} VertexIndex;

typedef struct {
    int textureIndex;
    int vertexIndexCount;
    VertexIndex *vertexIndices;
} Trigroup;

typedef struct {
    char *name;
    vec3 position;
    int vertexCount;
    Vertex *vertices;
    int uvCount;
    vec2 *uvs;
    int trigroupCount;
    Trigroup *trigroups;
} Object;

typedef struct {
    int textureCount;
    Image *textures;
    int objectCount;
    Object *objects;
} BMF;

void load_bmf(BMF *b, char *path);