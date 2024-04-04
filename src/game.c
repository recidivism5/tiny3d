#include "tiny3d.h"
#include <math.h>
#include <limits.h>

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

void update(double time, double deltaTime, int nSamples, signed short *samples){
	static double audioTime = 0;
	for (int i = 0; i < nSamples; i++){
		samples[i*2] = SHRT_MAX * 0.025*sin(audioTime);
		samples[i*2+1] = samples[i*2];
		audioTime += 0.1;
	}
	clear_screen(0xff00ff00);
}

int main(int argc, char **argv){
	puts(local_path_to_absolute("joj.png"));
    open_window(3);
}