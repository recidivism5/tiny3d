#include "tiny3d.h"

GLuint tid;
int16_t *doodoo;
int doodooFrames;
int curFrame = 0;

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

void update(double time, double deltaTime, int width, int height, int nAudioFrames, int16_t *audioSamples){
	static int counter = 0;
	static bool high = false;
	for (int i = 0; i < nAudioFrames; i++){
		if (counter == 300){
			counter = 0;
			high = !high;
		}
		audioSamples[i*2] = (SHRT_MAX-1) * (high ? 1 : -1);
		audioSamples[i*2+1] = audioSamples[i*2];
		counter++;
	}

	glViewport(0,0,width,height);

	glClearColor(1,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
}

int main(int argc, char **argv){
    open_window(640,480);
}