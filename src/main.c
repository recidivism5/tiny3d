#include <base.h>

typedef struct {
	uint8_t r,g,b,a;
} Color;

#define SCREEN_WIDTH    256
#define SCREEN_HEIGHT   192
Color *screen;
int pitch;

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

		SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);

		SDL_RenderClear(renderer);

		SDL_LockTexture(screenTexture,0,&screen,&pitch);
		SDL_UnlockTexture(screenTexture);

		SDL_RenderCopy(renderer,screenTexture,0,0);

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}