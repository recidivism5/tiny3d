#include <base.h>
#include <stb_image.h>

typedef struct {
	uint8_t r,g,b,a;
} Color;

typedef struct {
	int width, height, pitch;
	Color *pixels;
} Image;

void load_image(Image *t, char *name){
	int channels;
	t->pixels = (Color *)stbi_load(local_path_to_absolute("res/textures/%s",name),&t->width,&t->height,&channels,4);
	t->pitch = t->width*sizeof(*t->pixels);
}

#define PIXEL_AT(img,x,y) (((Color *)((char *)img->pixels+(y)*img->pitch))[(x)])

void blit8(Image *src, Image *dst, int sx, int sy, int dx, int dy, int rotation, bool xflip){
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
				for (int y = 0; y < h; y++){
					for (int x = 0; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+7-x,sy*8+y);
					}
				}
				break;
			case 1:
				for (int y = 0; y < h; y++){
					for (int x = 0; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+y,sy*8+x);
					}
				}
				break;
			case 2:
				for (int y = 0; y < h; y++){
					for (int x = 0; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+x,sy*8+7-y);
					}
				}
				break;
			case 3:
				for (int y = 0; y < h; y++){
					for (int x = 0; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+7-y,sy*8+7-x);
					}
				}
				break;
		}
	} else {
		switch (rotation){
			case 0:
				for (int y = 0; y < h; y++){
					for (int x = 0; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+x,sy*8+y);
					}
				}
				break;
			case 1:
				for (int y = 0; y < h; y++){
					for (int x = 0; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+7-y,sy*8+x);
					}
				}
				break;
			case 2:
				for (int y = 0; y < h; y++){
					for (int x = 0; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+7-x,sy*8+7-y);
					}
				}
				break;
			case 3:
				for (int y = 0; y < h; y++){
					for (int x = 0; x < w; x++){
						PIXEL_AT(dst,dx+x,dy+y) = PIXEL_AT(src,sx*8+y,sy*8+7-x);
					}
				}
				break;
		}
	}
}

//globals:
Image screen = {
	.width = 256,
	.height = 192
};

Image atlas;

//code:
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

		SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);

		SDL_RenderClear(renderer);

		SDL_LockTexture(screenTexture,0,&screen.pixels,&screen.pitch);
		for (int i = 0; i < 4; i++){
			blit8(&atlas,&screen,0,2,9*i,0,i,false);
		}
		for (int i = 0; i < 4; i++){
			blit8(&atlas,&screen,0,2,9*i,8,i,true);
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