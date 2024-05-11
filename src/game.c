#include <tiny3d.h>
#include <fast_obj.h>

void set_material_diffuse(float r, float g, float b){
	glMaterialfv(GL_FRONT, GL_DIFFUSE, (GLfloat[]){r,g,b,1.0});
}

double accTime = 0.0;
#define TICK_RATE 20.0
#define SEC_PER_TICK (1.0 / TICK_RATE)

typedef struct {
	vec2 dim;
	vec3 prevPos,curPos,vel;
	bool onGround;
} Entity;

typedef struct {
	vec3 min,max;
} MMBB;

void get_entity_mmbb(Entity *e, MMBB *m){
	vec3 dim = {e->dim[0],e->dim[1],e->dim[0]};
	for (int i = 0; i < 3; i++){
		m->min[i] = e->curPos[i]-0.5f*dim[i];
		m->max[i] = e->curPos[i]+0.5f*dim[i];
	}
}

void get_expanded_mmbb(MMBB *src, MMBB *dst, vec2 v){
	*dst = *src;
	for (int i = 0; i < 3; i++){
		if (v[i] > 0){
			dst->max[i] += v[i];
		} else {
			dst->min[i] += v[i];
		}
	}
}

void get_mmbb_center(MMBB *m, vec2 c){
	for (int i = 0; i < 3; i++){
		c[i] = m->min[i] + 0.5f*(m->max[i]-m->min[i]);
	}
}

void draw_mmbb(MMBB *m){
	glBegin(GL_LINES);
	glVertex3f(m->min[0],m->min[1],m->min[2]);
	glVertex3f(m->max[0],m->min[1],m->min[2]);
	glVertex3f(m->min[0],m->min[1],m->max[2]);
	glVertex3f(m->max[0],m->min[1],m->max[2]);

	glVertex3f(m->min[0],m->min[1],m->min[2]);
	glVertex3f(m->min[0],m->min[1],m->max[2]);
	glVertex3f(m->max[0],m->min[1],m->max[2]);
	glVertex3f(m->max[0],m->min[1],m->min[2]);

	glVertex3f(m->min[0],m->max[1],m->min[2]);
	glVertex3f(m->max[0],m->max[1],m->min[2]);
	glVertex3f(m->min[0],m->max[1],m->max[2]);
	glVertex3f(m->max[0],m->max[1],m->max[2]);

	glVertex3f(m->min[0],m->max[1],m->min[2]);
	glVertex3f(m->min[0],m->max[1],m->max[2]);
	glVertex3f(m->max[0],m->max[1],m->max[2]);
	glVertex3f(m->max[0],m->max[1],m->min[2]);

	glVertex3f(m->min[0],m->min[1],m->min[2]);
	glVertex3f(m->min[0],m->max[1],m->min[2]);

	glVertex3f(m->max[0],m->min[1],m->min[2]);
	glVertex3f(m->max[0],m->max[1],m->min[2]);

	glVertex3f(m->min[0],m->min[1],m->max[2]);
	glVertex3f(m->min[0],m->max[1],m->max[2]);

	glVertex3f(m->max[0],m->min[1],m->max[2]);
	glVertex3f(m->max[0],m->max[1],m->max[2]);
	glEnd();
}

void get_entity_interp_pos(Entity *e, vec2 pos){
	vec2_lerp(e->prevPos,e->curPos,(float)(accTime/SEC_PER_TICK),pos);
}

Entity player = {
	.dim = {2.0f,1.2f},
	.curPos = {4,1,0}
};

fastObjMesh *mesh;

void update_entity(Entity *e){

}

struct {
	bool
	left,
	right,
	backward,
	forward,
	jump;
} keys;

void keydown(int key){
	if (key == 'F'){
		exit(0);
	} else if (key == 'C'){
		lock_mouse(!is_mouse_locked());
	}
	switch (key){
		case 'A': keys.left = true; break;
		case 'D': keys.right = true; break;
		case 'S': keys.backward = true; break;
		case 'W': keys.forward = true; break;
		case ' ': if (player.onGround) player.vel[1] = 0.5f; break;
	}
}

void keyup(int key){
	switch (key){
		case 'A': keys.left = false; break;
		case 'D': keys.right = false; break;
		case 'S': keys.backward = false; break;
		case 'W': keys.forward = false; break;
	}
}

void mousemove(int x, int y){
}

int clip(float *vin, int cin, float *vout, int axis, float val, bool less){
	int cout = 0;
	float *prev = vin+(cin-1)*3;
	bool prevInside = ((prev[axis] - val) >= 0.0f) ^ less;
	for (int i = 0; i < cin; i++){
		float *cur = vin+i*3;
		bool curInside = ((cur[axis] - val) >= 0.0f) ^ less;
		if (prevInside ^ curInside){
			float *l, *g;
			if (cur[axis] > prev[axis]){
				l = prev;
				g = cur;
			} else {
				l = cur;
				g = prev;
			}
			vec3 d;
			vec3_sub(g,l,d);
			vec3_scale(d,(val-l[axis]) / (g[axis]-l[axis]),d);
			vec3_add(l,d,vout);
			vout+=3;
			cout++;
		}
		if (curInside){
			vec3_copy(cur,vout);
			vout+=3;
			cout++;
		}
		prev = cur;
		prevInside = curInside;
	}
	return cout;
}

int clip_triangle_to_mmbb(float *vin, float *vout, MMBB *m){
	int cout;
	static float b[6*3];
	memcpy(vout,vin,3*3*sizeof(float));
	cout = clip(vout,3,b,0,m->min[0],false);
	cout = clip(b,cout,vout,0,m->max[0],true);
	cout = clip(vout,cout,b,1,m->min[1],false);
	cout = clip(b,cout,vout,1,m->max[1],true);
	cout = clip(vout,cout,b,2,m->min[2],false);
	return clip(b,cout,vout,2,m->max[2],true);
}

void update(double time, double deltaTime, int width, int height, int nAudioFrames, int16_t *audioSamples){
	static bool init = false;
	if (!init){
		init = true;
	}

	accTime += deltaTime;
	while (accTime >= 1.0/20.0){
		accTime -= 1.0/20.0;
		
		//TICK:
		if ((keys.left || keys.right) && !(keys.left && keys.right)){
			player.vel[0] = LERP(player.vel[0],keys.left ? -0.2f : 0.2f,0.2f);
		} else {
			player.vel[0] = LERP(player.vel[0],0,0.2f);
		}
		update_entity(&player);
	}

	glViewport(0,0,width,height);

	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*glMatrixMode(GL_PROJECTION);
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
	set_material_diffuse(1,1,0);
	glBegin(GL_TRIANGLES);
	for (unsigned int v = 0; v < mesh->index_count; v++){
		fastObjIndex *i = mesh->indices+v;
		glNormal3fv(mesh->normals+3*i->n);
		glVertex3fv(mesh->positions+3*i->p);
	}
	glEnd();
	set_material_diffuse(1,0,0);
	player.curPos[0] = 5*sin(time);
	MMBB m;
	get_entity_mmbb(&player,&m);
	draw_mmbb(&m);
	for (unsigned int i = 0; i < mesh->index_count; i+=3){
		vec3 vout[6];
		for (int j = 0; j < 3; j++){
			vec3_copy(mesh->positions+mesh->indices[i+j].p*3,vout+j);
		}
		int cout = clip_triangle_to_mmbb(vout,vout,&m);
		if (cout){
			set_material_diffuse(0,1,0);
			glPointSize(5);
			glBegin(GL_POINTS);
			for (int j = 0; j < cout; j++){
				glVertex3fv(vout+j);
			}
			glEnd();
		}
	}
	glDisable(GL_LIGHTING);*/

	uint32_t *pix = malloc(width*height*sizeof(*pix));
	ASSERT(pix);
	for (int i = 0; i < width*height; i++){
		pix[i] = 0x000000ff;
	}
	
	text_set_target_image(pix,width,height);
	text_draw(0,width,0,height,"dabchin");
	GLuint tex;
	glGenTextures(1,&tex);
	glBindTexture(GL_TEXTURE_2D,tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,pix);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2d(0,0); glVertex2d(-1,-1);
	glTexCoord2d(1,0); glVertex2d(1,-1);
	glTexCoord2d(1,1); glVertex2d(1,1);
	glTexCoord2d(0,1); glVertex2d(-1,1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	free(pix);
	glDeleteTextures(1,&tex);
}

int main(int argc, char **argv){
	mesh = fast_obj_read(local_path_to_absolute("mesh/test.obj"));
	ASSERT(mesh);
	text_set_font("Nunito-Regular.ttf");
    open_window(640,480);
}