#include "tiny3d.h"
#include <math.h>

void keydown(int key){
	printf("keydown: %c\n",key);
}

void keyup(int key){
	printf("keyup: %c\n",key);
}

void update(double time, double deltaTime){
	clear_screen(0xffff0000);
	draw_line(0,0,100,100+50*sin(time),0x00ffffff);
}

int main(int argc, char **argv){
	ASSERT(0);
    open_window(3);
}