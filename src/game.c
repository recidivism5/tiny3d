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
	for (int i = 0; i < nAudioFrames; i++){
		if (curFrame >= doodooFrames){
			curFrame = 0;
		}
		audioSamples[i*2] = doodoo[curFrame*2];
		audioSamples[i*2+1] = doodoo[curFrame*2+1];
		curFrame++;
	}

	glViewport(0,0,width,height);

	glClearColor(1,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);

	if (!tid){
		int w,h;
		uint32_t *p = load_image(false,&w,&h,"test.jpg");
		glGenTextures(1,&tid);
		glBindTexture(GL_TEXTURE_2D,tid);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,p);
	}

	glBindTexture(GL_TEXTURE_2D,tid);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex2f(-1,-1);
	glTexCoord2f(1,0); glVertex2f(1,-1);
	glTexCoord2f(1,1); glVertex2f(1,1);
	glTexCoord2f(0,1); glVertex2f(-1,1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

int main(int argc, char **argv){
	doodoo = load_audio(&doodooFrames, "ohshit.mp3");
    open_window(640,480);
}