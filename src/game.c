#include "tiny3d.h"
#include <math.h>

void draw_line(int x0, int y0, int x1, int y1, uint32_t color){
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = (dx>dy ? dx : -dy)/2, e2;
	while (1){
		screen[y0*SCREEN_WIDTH+x0] = color;
		if (x0==x1 && y0==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void keydown(int key){
	printf("keydown: %c\n",key);
}

void keyup(int key){
	printf("keyup: %c\n",key);
}

void update(double time, double deltaTime){
	for (int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; i++){
		screen[i] = 0xffff0000;
	}
	draw_line(0,0,100,100+50*sin(time),0x00ffffff);
}

int main(int argc, char **argv){
    open_window(3);
}