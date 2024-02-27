#include <base.h>
#include <stb_image.h>

typedef union {
	struct {
		uint8_t r,g,b,a;
	};
	uint32_t w;
} Color;

//globals:
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
Color screen[SCREEN_WIDTH*SCREEN_HEIGHT];

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

		screen[0].r = 255;
		screen[0].a = 0;

		SDL_UpdateTexture(screenTexture,0,screen,SCREEN_WIDTH*sizeof(*screen));

		SDL_RenderCopy(renderer,screenTexture,0,0);

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}