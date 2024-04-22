#include <tiny3d.h>
#include <fast_obj.h>

fastObjMesh *mesh;

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
	glViewport(0,0,width,height);

	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0,(double)width/height,0.01,1000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(0,-4,-10);
	glRotated(20*time,0,1,0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	GLfloat lightpos[] = {.5, 1., 1., 0.};
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	glColor3f(1,0,0);
	glBegin(GL_TRIANGLES);
	for (int v = 0; v < mesh->index_count; v++){
		fastObjIndex *i = mesh->indices+v;
		glNormal3fv(mesh->normals+3*i->n);
		glVertex3fv(mesh->positions+3*i->p);
	}
	glEnd();
	glDisable(GL_LIGHTING);
}

int main(int argc, char **argv){
	mesh = fast_obj_read(local_path_to_absolute("mesh/test.obj"));
    open_window(640,480);
}