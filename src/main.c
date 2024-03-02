#include <bmf.h>

///////////////////////////////////////// types:

typedef struct {
	vec3 position, angle;
} Camera;

///////////////////////////////////////// globals:

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
Color screen[SCREEN_WIDTH*SCREEN_HEIGHT];
float depthBuffer[SCREEN_WIDTH*SCREEN_HEIGHT];

mat4 projection, modelView[32], tempMat;
int mvIndex = 0;

Color currentColor;
vec4 verts[3];
int vcount = 0;

///////////////////////////////////////// code:

void perspective(float fovy, float aspect, float nearZ, float farZ){
	mat4_persp_rh_no(projection,fovy,aspect,nearZ,farZ);
}

void mv_apply(){
	mat4_mul(modelView[mvIndex],tempMat,modelView[mvIndex]);
}

void load_identity(){
	mat4_identity(modelView[mvIndex]);
}

void translate(float x, float y, float z){
	mat4_translate(tempMat,(vec3){x,y,z});
	mv_apply();
}

void rotate(float x, float y, float z, float angle){
	mat4_rotate(tempMat,(vec3){x,y,z},angle);
	mv_apply();
}

void line(int x0, int y0, int x1, int y1, Color color){
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

void clip(vec4 *vin, int cin, vec4 *vout, int *cout, int axis, float coef){
	cout[0] = 0;
	vec4 *prev = vin+cin-1;
	bool prevInside = prev[0][axis]*coef <= prev[0][3];
	for (int i = 0; i < cin; i++){
		vec4 *cur = vin+i;
		bool curInside = cur[0][axis]*coef <= cur[0][3];

		if (prevInside ^ curInside){
			float p = prev[0][3] - prev[0][axis]*coef;
			vec4_lerp(
				(float *)prev,
				(float *)cur,
				p / (p - (cur[0][3] - cur[0][axis]*coef)),
				(float *)vout
			);
			vout++;
			cout[0]++;
		}

		if (curInside){
			vec4_copy((float *)cur,(float *)vout);
			vout++;
			cout[0]++;
		}

		prev = cur;
		prevInside = curInside;
	}
}

bool clip_axis(vec4 *v, int *c, int axis){
	static vec4 b[6];
	int bc = 0;
	clip(v,*c,b,&bc,axis,1.0f);
	if (!bc){
		return false;
	}
	*c = 0;
	clip(b,bc,v,c,axis,-1.0f);
	return (*c) > 0;
}

void vertex(float x, float y, float z){
	ASSERT(vcount < 3);
	verts[vcount][0] = x;
	verts[vcount][1] = y;
	verts[vcount][2] = z;
	verts[vcount][3] = 1.0f;
	vcount++;
	if (vcount == 3){
		vcount = 0;
		static vec4 cv[6];
		int cvc = 3;
		mat4_mul(projection,modelView[mvIndex],tempMat);
		for (int i = 0; i < 3; i++){
			mat4_mul_vec4(tempMat,verts[i],cv[i]);
		}
		if (
			clip_axis(cv,&cvc,0) &&
			clip_axis(cv,&cvc,1) &&
			clip_axis(cv,&cvc,2)
		){
			for (int i = 0; i < cvc; i++){
				float x = cv[i][0]/cv[i][3];
				float y = cv[i][1]/cv[i][3];

				int sx = (int)roundf((1.0f + x) * 0.5f * ((float)SCREEN_WIDTH-1));
				int sy = (int)roundf((1.0f + -y) * 0.5f * ((float)SCREEN_HEIGHT-1));
				screen[sy*SCREEN_WIDTH+sx].w = 0xffffffff;
			}
		}
	}
}

void draw_triangle(Vertex *v, vec2 *uvs){
	static vec4 cv[6];
	int cvc = 3;
	mat4_mul(projection,modelView[mvIndex],tempMat);
	for (int i = 0; i < 3; i++){
		mat4_mul_vec4(tempMat,v[i].position,cv[i]);
	}
	if (
		clip_axis(cv,&cvc,0) &&
		clip_axis(cv,&cvc,1) &&
		clip_axis(cv,&cvc,2)
	){
		for (int i = 0; i < cvc; i++){
			float x = cv[i][0]/cv[i][3];
			float y = cv[i][1]/cv[i][3];

			int sx = (int)roundf((1.0f + x) * 0.5f * ((float)SCREEN_WIDTH-1));
			int sy = (int)roundf((1.0f + -y) * 0.5f * ((float)SCREEN_HEIGHT-1));
			screen[sy*SCREEN_WIDTH+sx].w = 0xffffffff;
		}
	}
}

void draw_bmf(BMF *b){
	Object *o = b->objects;
	for (Trigroup *t = o->trigroups; t < o->trigroups+o->trigroupCount; t++){
		for (VertexIndex *i = t->vertexIndices; i < t->vertexIndices+t->vertexIndexCount; i+=3){
			o->vertices[i[0].posnorm];
			o->uvs[i[0].uv];
		}
	}
}

SDL_Window *window;
SDL_Renderer *renderer;
void cleanup(void){
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
    SDL_Quit();
}

int SDL_main(int argc, char* argv[]){
	set_error_callback(cleanup);

	SDL_ASSERT(!SDL_Init(SDL_INIT_EVERYTHING));

#if defined linux && SDL_VERSION_ATLEAST(2, 0, 8) //linux is bad I guess
    SDL_ASSERT(SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0"));
#endif

    window = SDL_CreateWindow(
		"tiny3d",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640, 480,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
	);
	SDL_ASSERT(window);
	renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
	);
	SDL_ASSERT(renderer);
	SDL_SetWindowMinimumSize(window,SCREEN_WIDTH,SCREEN_HEIGHT);
	SDL_ASSERT(!SDL_RenderSetLogicalSize(renderer,SCREEN_WIDTH,SCREEN_HEIGHT));
	SDL_ASSERT(!SDL_RenderSetIntegerScale(renderer,true));
	SDL_Texture *screenTexture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH,
		SCREEN_HEIGHT
	);

	uint64_t ms0 = SDL_GetTicks64();

	BMF banana;
	load_bmf(&banana,"res/models/banana.bmf");

	bool quit = false;

	while(!quit){
		SDL_Event e;
		while (SDL_PollEvent(&e)){
			switch (e.type){
				case SDL_QUIT:{
					quit = true;
					break;
				}
				case SDL_KEYDOWN:{
					switch (e.key.keysym.sym){
						case SDLK_ESCAPE:{
							quit = true;
							break;
						}
					}
					break;
				}
			}
		}

		float seconds = (float)((double)(SDL_GetTicks64() - ms0) / 1000.0);

		int mx,my;
		SDL_GetMouseState(&mx,&my);
		float lx,ly;
		SDL_RenderWindowToLogical(renderer,mx,my,&lx,&ly);
		mx = (int)lx;
		my = (int)ly;

		memset(screen,0,sizeof(screen));
		perspective(0.5f*(float)M_PI,(float)SCREEN_WIDTH/SCREEN_HEIGHT,0.01f,1000.0f);
		load_identity();
		translate(2*sinf(seconds),0,0);
		vertex(0,0,-2);
		vertex(1.5f,0.5f,-2);
		vertex(1,1,-2);

		SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);

		SDL_RenderClear(renderer);

		SDL_UpdateTexture(screenTexture,0,screen,SCREEN_WIDTH*sizeof(*screen));

		SDL_RenderCopy(renderer,screenTexture,0,0);

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}