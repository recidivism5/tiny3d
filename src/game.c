#include <bmf.h>
#include <platform.h>

///////////////////////////////////////// types:

typedef struct {
	vec3 position, angle;
} Camera;

///////////////////////////////////////// globals:

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
uint32_t screen[SCREEN_WIDTH*SCREEN_HEIGHT];
float depthBuffer[SCREEN_WIDTH*SCREEN_HEIGHT];

mat4 project, modelView[32], modelViewProject;
int mvIndex = 0;

vec4 verts[3];
int vcount = 0;

///////////////////////////////////////// code:

void perspective(float fovy, float aspect, float nearZ, float farZ){
	mat4_persp_rh_no(project,fovy,aspect,nearZ,farZ);
}

void mv_apply(){
	mat4_mul(modelView[mvIndex],modelViewProject,modelView[mvIndex]);
	mat4_mul(project,modelView[mvIndex],modelViewProject);
}

void load_identity(){
	mat4_identity(modelView[mvIndex]);
}

void translate(float x, float y, float z){
	mat4_translate(modelViewProject,(vec3){x,y,z});
	mv_apply();
}

void rotate(float x, float y, float z, float angle){
	mat4_rotate(modelViewProject,(vec3){x,y,z},angle);
	mv_apply();
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

void clip_textured(ClipVertex *vin, int cin, ClipVertex *vout, int *cout, int axis, float coef){
	*cout = 0;
	ClipVertex *prev = vin+cin-1;
	bool prevInside = prev->position[axis]*coef <= prev->position[3];
	for (int i = 0; i < cin; i++){
		ClipVertex *cur = vin+i;
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
			vec3_lerp(
				(float *)prev->normal,
				(float *)cur->normal,
				lerpAmt,
				(float *)vout->normal
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

bool clip_axis_textured(ClipVertex *v, int *c, int axis){
	static ClipVertex b[6];
	int bc = 0;
	clip_textured(v,*c,b,&bc,axis,1.0f);
	if (!bc){
		return false;
	}
	*c = 0;
	clip_textured(b,bc,v,c,axis,-1.0f);
	return (*c) > 0;
}

int orient2d(ivec2 a, ivec2 b, ivec2 c){
	return (b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0]);
}

void draw_bmf(BMF *b){
	ClipVertex cv[6];
	ivec2 scrpos[6];
	int cvc;
	Object *o = b->objects;
	for (Trigroup *t = o->trigroups; t < o->trigroups+o->trigroupCount; t++){
		Image *texture = b->textures + t->textureIndex;
		for (VertexIndex *i = t->vertexIndices; i < t->vertexIndices+t->vertexIndexCount; i+=3){
			cvc = 3;
			for (int j = 0; j < 3; j++){
				Vertex *v = o->vertices+i[j].posnorm;
				vec2 *uv = o->uvs+i[j].uv;
				mat4_mul_vec3_pos(modelViewProject,v->position,cv[j].position);
				mat4_mul_vec3_dir(modelView[mvIndex],v->normal,cv[j].normal);
				vec2_copy(uv[j],cv[j].uv);
			}
			if (
				clip_axis_textured(cv,&cvc,0) &&
				clip_axis_textured(cv,&cvc,1) &&
				clip_axis_textured(cv,&cvc,2)
			){
				for (int j = 0; j < cvc; j++){
					for (int k = 0; k < 3; k++){
						cv[j].position[k] /= cv[j].position[3];
					}
					cv[j].position[3] = 1.0f;

					scrpos[j][0] = (int)roundf((1.0f + cv[j].position[0]) * 0.5f * ((float)SCREEN_WIDTH-1));
					scrpos[j][1] = (int)roundf((1.0f + cv[j].position[1]) * 0.5f * ((float)SCREEN_HEIGHT-1));
				}
				for (int j = 1; j < cvc-1; j++){
					float nz = 
						(cv[j].position[0]-cv[0].position[0])*(cv[j+1].position[1]-cv[0].position[1]) - 
						(cv[j].position[1]-cv[0].position[1])*(cv[j+1].position[0]-cv[0].position[0]);
					if (nz < 0.0f){
						goto L0; //back face cull
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
								if (depth < depthBuffer[p[1]*SCREEN_WIDTH+p[0]]){
									depthBuffer[p[1]*SCREEN_WIDTH+p[0]] = depth;
									vec2 uv;
									for (int k = 0; k < 2; k++){
										float whole;
										uv[k] = modff(fabsf(
											cv[0].uv[k] * bary[0] +
											cv[j].uv[k] * bary[1] +
											cv[j+1].uv[k] * bary[2]
										),&whole);
									}
									uv[0] *= texture->width-1;
									uv[1] *= texture->height-1;
									uv[0] = roundf(uv[0]);
									uv[1] = roundf(uv[1]);
									screen[p[1]*SCREEN_WIDTH+p[0]] = texture->pixels[((int)uv[1])*texture->width + (int)uv[0]];
								}
							}
						}
					}
				}
			}
			L0:;
		}
	}
}

void keydown(int key){
	printf("keydown: %c\n",key);
}

void keyup(int key){
	printf("keyup: %c\n",key);
}

int main(int argc, char **argv){
	for (int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; i++){
		screen[i] = 0xffff0000;
	}
	draw_line(0,0,100,100,0xffffffff);
	open_window(SCREEN_WIDTH,SCREEN_HEIGHT,screen);
}