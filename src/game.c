#include "tiny3d.h"
#include <math.h>
#include <limits.h>

int16_t *doodoo;
int doodooFrames;
int curFrame = 0;

typedef struct {
	int16_t *frames;
	int nFrames;
	int curFrame;
} AudioSource;
AudioSource audioSources[16];
int nAudioSources = 0;

void keydown(int key){
	printf("keydown: %d\n",key);
	if (key == 'F'){
		exit(0);
	} else if (key == 'C'){
		lock_mouse(!is_mouse_locked());
	}
}

void keyup(int key){
	printf("keyup: %d\n",key);
}

void mousemove(int x, int y){
	printf("mousemove: %d %d\n",x,y);
}

void update(double time, double deltaTime, int nFrames, signed short *frames){
	for (int i = 0; i < nFrames; i++){
		if (curFrame >= doodooFrames){
			curFrame = 0;
		}
		frames[i*2] = doodoo[curFrame*2];
		frames[i*2+1] = doodoo[curFrame*2+1];
		curFrame++;
	}
	clear_screen(0xff00ff00);
}

int main(int argc, char **argv){
	puts(local_path_to_absolute("joj.png"));
	doodoo = load_audio(&doodooFrames, "just.mp3");
    open_window(3);
}