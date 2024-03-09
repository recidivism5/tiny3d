#include <image.h>

void load_image(Image *i, char *path){
    int channels;
    assertPath = local_path_to_absolute(path);
    //stbi_set_flip_vertically_on_load(1);
    //i->pixels = (Color *)stbi_load(assertPath,&i->width,&i->height,&channels,4);
    ASSERT_FILE(i->pixels);
}