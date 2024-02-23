#include <base.h>
#include <stb_image.h>

typedef struct {
	uint8_t r,g,b,a;
} Color;

typedef struct {
	int width, height, pitch;
	Color *pixels;
} Image;

//globals:
Image screen = {
	.width = 256,
	.height = 192
};

Image atlas;

//code:

void load_image(Image *t, char *name){
	int channels;
	t->pixels = (Color *)stbi_load(local_path_to_absolute("res/textures/%s",name),&t->width,&t->height,&channels,4);
	t->pitch = t->width*sizeof(*t->pixels);
}

#define PIXEL_AT(img,x,y) (((Color *)((char *)(img)->pixels+(y)*(img)->pitch))[(x)])

void blit8(Image *src, Image *dst, int sx, int sy, int dx, int dy, int rotation, bool xflip){
	if (dx < -7 || dy < -7){
		return;
	}
	int xo = MAX(-dx,0);
	int yo = MAX(-dy,0);
	int w = MIN(dst->width-dx,8);
	if (w <= 0){
		return;
	}
	int h = MIN(dst->height-dy,8);
	if (h <= 0){
		return;
	}
	if (xflip){
		switch (rotation){
			case 0:
				for (int y = yo; y < h; y++){
					for (int x = xo; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+7-x,sy*8+y);
					}
				}
				break;
			case 1:
				for (int y = yo; y < h; y++){
					for (int x = xo; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+y,sy*8+x);
					}
				}
				break;
			case 2:
				for (int y = yo; y < h; y++){
					for (int x = xo; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+x,sy*8+7-y);
					}
				}
				break;
			case 3:
				for (int y = yo; y < h; y++){
					for (int x = xo; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+7-y,sy*8+7-x);
					}
				}
				break;
		}
	} else {
		switch (rotation){
			case 0:
				for (int y = yo; y < h; y++){
					for (int x = xo; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+x,sy*8+y);
					}
				}
				break;
			case 1:
				for (int y = yo; y < h; y++){
					for (int x = xo; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+7-y,sy*8+x);
					}
				}
				break;
			case 2:
				for (int y = yo; y < h; y++){
					for (int x = xo; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+7-x,sy*8+7-y);
					}
				}
				break;
			case 3:
				for (int y = yo; y < h; y++){
					for (int x = xo; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+y,sy*8+7-x);
					}
				}
				break;
		}
	}
}

int SDL_main(int argc, char* argv[]){
	SDL_ASSERT(!SDL_Init(SDL_INIT_EVERYTHING));

#if defined linux && SDL_VERSION_ATLEAST(2, 0, 8) //linux is bad I guess
    SDL_ASSERT(SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0"));
#endif

    SDL_Window *window = SDL_CreateWindow(
		"tiny3d",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640, 480,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
	);
	SDL_ASSERT(window);
	SDL_Renderer *renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
	);
	SDL_ASSERT(renderer);
	SDL_SetWindowMinimumSize(window,screen.width,screen.height);
	SDL_ASSERT(!SDL_RenderSetLogicalSize(renderer,screen.width,screen.height));
	SDL_ASSERT(!SDL_RenderSetIntegerScale(renderer,true));
	SDL_Texture *screenTexture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		screen.width,
		screen.height
	);

	load_image(&atlas,"atlas.png");

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

		int mx,my;
		SDL_GetMouseState(&mx,&my);
		float lx,ly;
		SDL_RenderWindowToLogical(renderer,mx,my,&lx,&ly);
		mx = (int)lx;
		my = (int)ly;

		SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);

		SDL_RenderClear(renderer);

		SDL_LockTexture(screenTexture,0,&screen.pixels,&screen.pitch);
		for (int y = 0; y < screen.height; y++){
			for (int x = 0; x < screen.width; x++){
				PIXEL_AT(&screen,x,y) = (Color){0,0,0,255};
			}
		}
		for (int i = 0; i < 4; i++){
			blit8(&atlas,&screen,0,2,mx+9*i,my,i,false);
		}
		for (int i = 0; i < 4; i++){
			blit8(&atlas,&screen,0,2,mx+9*i,my+8,i,true);
		}
		SDL_UnlockTexture(screenTexture);

		SDL_RenderCopy(renderer,screenTexture,0,0);

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}