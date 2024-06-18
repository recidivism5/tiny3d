#include "tiny3d.h"

double accumulated_time = 0.0;
double interpolant;
#define TICK_RATE 20.0
#define SEC_PER_TICK (1.0 / TICK_RATE)

float mouse_sensitivity = 0.1f;

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
color_t screen_pixels[SCREEN_WIDTH*SCREEN_HEIGHT];
float screen_depth[SCREEN_WIDTH*SCREEN_HEIGHT];
framebuffer_t screen = {
	.width = SCREEN_WIDTH,
	.height = SCREEN_HEIGHT,
	.pixels = screen_pixels,
	.depth = screen_depth
};

image_t sponge;

void keydown(int scancode){
	switch (scancode){
		case KEY_MOUSE_LEFT: printf("click\n"); break;
		case 1: exit(0); break;
		case 33: toggle_fullscreen(); break;
		case 'C': toggle_mouse_lock(); break;
	}
}

void keyup(int scancode){
	switch (scancode){
	}
}

void mousemove(int x, int y){
	
}

void tick(){
	
}

int16_t *doodoo;
int doodooFrames;
int curFrame = 0;

void update(double time, double deltaTime, int nAudioFrames, int16_t *audioSamples){
	accumulated_time += deltaTime;
	while (accumulated_time >= SEC_PER_TICK){
		accumulated_time -= SEC_PER_TICK;
		tick();
	}
	interpolant = accumulated_time / SEC_PER_TICK;

	for (int i = 0; i < nAudioFrames; i++){
		if (curFrame >= doodooFrames){
			curFrame = 0;
		}
		audioSamples[i*2] = doodoo[curFrame*2];
		audioSamples[i*2+1] = doodoo[curFrame*2+1];
		curFrame++;
	}

	t3d_clear((color_t){255,0,0,255});
	t3d_perspective(0.5f*M_PI,(float)screen.width/screen.height,0.01f,100.0f);
	t3d_load_identity();
	t3d_translate(0,0,-1.25);
	static double r = 0.0;
	r += deltaTime; if (r > 2.0*M_PI) r -= 2.0*M_PI;
	t3d_rotate(0,1,0,0.5f*M_PI*sin(r));
	t3d_translate(-0.5f,-0.5f,0.0f);
	t3d_position(0,0,0); t3d_texcoord(0,0);
	t3d_position(1,0,0); t3d_texcoord(1,0);
	t3d_position(1,1,0); t3d_texcoord(1,1);
	t3d_position(1,1,0); t3d_texcoord(1,1);
	t3d_position(0,1,0); t3d_texcoord(0,1);
	t3d_position(0,0,0); t3d_texcoord(0,0);

	//text_set_target_image(screen.pixels,screen.width,screen.height);
	//text_set_color(1,1,1);
	//text_draw(0,screen.width,0,screen.height,"jojfoil hat\nswagcoin");

	draw_framebuffer((image_t *)&screen);
}

void init(void){
	toggle_fullscreen();
}

int main(int argc, char **argv){
	t3d_set_framebuffer(&screen);
	sponge.pixels = load_image(true,&sponge.width,&sponge.height,"res/screenshot.png");
	t3d_set_texture(&sponge);
	doodoo = load_audio(&doodooFrames,"res/frequency.mp3");
    open_window(SCREEN_WIDTH,SCREEN_HEIGHT,"gerald's conundrum");
}