#include "tiny3d.h"

double accumulated_time = 0.0;
double interpolant;
#define TICK_RATE 20.0
#define SEC_PER_TICK (1.0 / TICK_RATE)

float mouse_sensitivity = 0.1f;

#define SCREEN_WIDTH 512
#define SCREEN_HEIGHT 512
color_t screen_pixels[SCREEN_WIDTH*SCREEN_HEIGHT];
float screen_depth[SCREEN_WIDTH*SCREEN_HEIGHT];
framebuffer_t screen = {
	.width = SCREEN_WIDTH,
	.height = SCREEN_HEIGHT,
	.pixels = screen_pixels,
	.depth = screen_depth
};
image_t paddle, ball;
float pv = 0.0f;
vec2 pp, pc = {SCREEN_WIDTH-84,20},
bp, bc, bv;

char *text[] = {
	"      ",
	"Hi.",
	"My name is Ian Bryant.",
	"I am the creator of tiny3d.",
	"Recently gone through a",
	"period of great brainfog.",
	"About to come back and",
	"really swag on the competition",
	"such as GLFW and SDL.",
	"Press F to toggle fullscreen.",
	"Get comfortable.",
	"You're in for a long story.",
	"But first,",
	"Raven67854 wants a game.",
	"...",
	"Ok.",
	"   ",
	"Press space to move.",
	"Keep the ball on screen.",
	"..."
	"Or else.",
	"          ",
	"I never should've fucked with that gluten shit.",
	"Nvm lied about the long story.",
	"Too tired to write that shit.",
	"Been f'ing with my diet too much.",
	"It feels like it's all I think about",
	"these days...",
	"Trying to figure things out.",
	"I've gone down a lot of rabbit holes.",
	"Someone should take the source code",
	"and add a second ball at 5O points.",
	"Make it green.",
	"Funny enough...",
	"this typewriter font lacks characters",
	"for zero and one. So I have to iterate",
	"through the score string to replace",
	"them with uppercase O and lowercase l.",
	"Just like a real typewriter I guess.",
	"Shoutout to Marc.",
	"sorry I didn't detect",
	"your keyboard yet.",
	"I forgot what F is on AZERTY.",
};

char scratch[256];
int si = 16, ti = 0;
int score;

bool playing = false;
bool dead = false;
bool ball_dropped = false;
bool move;
int count = 0;
bool advance = true;

void reset(){
	bc[0] = SCREEN_WIDTH/2-11;
	bc[1] = (SCREEN_HEIGHT/5)*3;
	memcpy(bp,bc,sizeof(bc));
	bv[0] = 0;
	bv[1] = 0;
	ball_dropped = false;
	advance = true;
	score = 0;
}

void keydown(int scancode){
	switch (scancode){
		case US_SCANCODE_ESC: exit(0); break;
		case US_SCANCODE_F: toggle_fullscreen(); toggle_mouse_lock(); break;
		case US_SCANCODE_R: if (dead){dead = false; playing = false; si = 16; ti = 0; count = 0; reset();} break;
		case US_SCANCODE_SPACE: move = true; break;
	}
}

void keyup(int scancode){
	switch (scancode){
		case US_SCANCODE_SPACE: move = false; break;
	}
}

void mousemove(int x, int y){
	int fx,fy;
	get_framebuffer_pos(x,y,&fx,&fy);
}

void tick(){
	if (advance){
		count++;
		if (count == 0){
			ti = 0;
			si++;
			if (si == 17){
				playing = true;
				reset();
			}
			if (si == 20){
				ball_dropped = true;
			}
			if (si >= COUNT(text)){
				si = 0;
				advance = false;
			}
		}
		if (count == 1){
			count = 0;
			ti++;
			if (ti > strlen(text[si])){
				ti--;
				count = -10;
			}
		}
	}

	if (playing){
		float mv = 30.0f;
		float ls = 0.2f;
		if (move){
			pv = LERP(pv,-mv,ls);
		} else {
			pv = LERP(pv,mv,ls);
		}
		vec2_copy(pc,pp);
		pc[0] = pp[0] + pv;
		if (pc[0] <= 0){
			pc[0] = 0;
			pv = 0;
		} else if (pc[0] >= (SCREEN_WIDTH-84)){
			pc[0] = SCREEN_WIDTH-84;
			pv = 0;
		}
		if (ball_dropped){
			bv[1] -= 0.8f;
			vec2_copy(bc,bp);
			vec2_add(bc,bv,bc);
			float t = (pp[1]+paddle.height - bp[1]) / bv[1];
			if (t > 0.0f && t < 1.0f){
				float ipx = LERP(pp[0],pc[0],t);
				float ibx = LERP(bp[0],bc[0],t);
				float iby = LERP(bp[1],bc[1],t);
				if (ibx > (ipx-ball.width) && ibx < (ipx+paddle.width)){
					score++;
					bv[1] = 18.4f;
					bv[0] = 0.06f*((ibx+0.5f*ball.width) - (ipx + 0.5f*paddle.width));
					bc[0] = ibx;
					bc[1] = iby;
				}
			}
			if (bc[0] < -ball.width || bc[0] > SCREEN_WIDTH || bc[1] < -ball.height){
				dead = true;
			}
		}
	}
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
		static int counter = 0;
		if (counter >= (1+dead)){
			counter = 0;
			curFrame++;
		}
		counter++;
	}

	t3d_clear((color_t){0,0,0,255});

	if (playing){
		t2d_set_destination_image(&screen);
		t2d_set_source_image(&paddle);
		t2d_blit(0,0,paddle.width,paddle.height,(int)roundf(LERP(pp[0],pc[0],interpolant)),(int)roundf(LERP(pp[1],pc[1],interpolant)),false);
		t2d_set_source_image(&ball);
		t2d_blit(0,0,ball.width,ball.height,(int)roundf(LERP(bp[0],bc[0],interpolant)),(int)roundf(LERP(bp[1],bc[1],interpolant)),false);
		{
			int w,h;
			sprintf(scratch,"%i",score);
			int len = strlen(scratch);
			for (int i = 0; i < len; i++){
				if (scratch[i] == '0') scratch[i] = 'O';
				else if (scratch[i] == '1') scratch[i] = 'l';
			}
			text_get_bounds(scratch,&w,&h);
			text_set_color(0,1,0);
			text_draw(SCREEN_WIDTH/2-w/2,5*SCREEN_HEIGHT/6+h/2,scratch);
		}
	}

	text_set_target_image(screen.pixels,screen.width,screen.height);
	
	
	if (!dead){
		text_set_color(1,1,0);
		memcpy(scratch,text[si],ti);
		scratch[ti] = 0;
	} else {
		text_set_color(1,0,0);
		sprintf(scratch,"Mission failed. Press R to retry.");
	}
	{
		int w,h;
		text_get_bounds(scratch,&w,&h);
		text_draw(SCREEN_WIDTH/2-w/2,SCREEN_HEIGHT/2+h/2,scratch);
	}

	draw_framebuffer((image_t *)&screen,0,0.3f,0);
}

void init(void){
	//toggle_fullscreen();
}

int main(int argc, char **argv){
	t3d_set_framebuffer(&screen);
	paddle.pixels = load_image(true,&paddle.width,&paddle.height,"res/paddle.png");
	ball.pixels = load_image(true,&ball.width,&ball.height,"res/ball.png");
	doodoo = load_audio(&doodooFrames,"res/fastBeat9.mp3");
	register_font(local_path_to_absolute("res/typewriter.ttf"));
	char fname[256];
	get_font_name(local_path_to_absolute("res/typewriter.ttf"),fname,sizeof(fname));
	text_set_font(fname,12);
    open_window(SCREEN_WIDTH,SCREEN_HEIGHT,"Gluten Ball");
}