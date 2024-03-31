#include "tiny3d.h"
#include <math.h>

void keydown(int key){
	printf("keydown: %c\n",key);
}

void keyup(int key){
	printf("keyup: %c\n",key);
}

void update(double time, double deltaTime, int width, int height, int nSamples, float *samples){
	printf("time: %lf\n",deltaTime);
	static double audioTime = 0;
	for (int i = 0; i < nSamples; i++){
		samples[i] = 0.85*sin(audioTime);
		audioTime += 0.1;
	}
	glViewport(0,0,width,height);
	glClearColor(0,1,0.5+0.5*sin(time),1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main(int argc, char **argv){
    open_window(640,480);
}