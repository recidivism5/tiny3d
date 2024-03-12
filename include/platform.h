#include <stdint.h>

extern void keydown(int key);
extern void keyup(int key);
extern void update(double time, double deltaTime);

void open_window(int width, int height, int fbWidth, int fbHeight, uint32_t *framebuffer);