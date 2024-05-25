#define TB_IMPL
#define TB_LIB_OPTS
//#define TB_OPT_ATTR_W 64
#define TB_OPT_TRUECOLOR
#define S3L_USE_WIDER_TYPES 1
#define S3L_MAX_PIXELS 1
#define S3L_PIXEL_FUNCTION render
#include "termbox2.h"
#include "small3dlib.h"
#include "alligatorModel.h"

S3L_Unit normals[ALLIGATOR_VERTEX_COUNT * 3];

S3L_Scene scene;

uint32_t previousTriangle = 1000;
S3L_Vec4 n0, n1, n2, toLight;

void render(S3L_PixelInfo *p) {
    if (p->triangleID != previousTriangle) {
        S3L_getIndexedTriangleValues(
            p->triangleIndex, 
            scene.models[p->modelIndex].triangles, 
            normals, 
            3, 
            &n0, &n1, &n2
        );
    }

    S3L_Vec4 normal;
    normal.x = S3L_interpolateBarycentric(n0.x, n1.x, n2.x, p->barycentric);
    normal.y = S3L_interpolateBarycentric(n0.y, n1.y, n2.y, p->barycentric);
    normal.z = S3L_interpolateBarycentric(n0.z, n1.z, n2.z, p->barycentric);
    S3L_vec3Normalize(&normal);

    S3L_Unit shading = (S3L_vec3Dot(normal, toLight) + S3L_F) / 2;
    shading = S3L_interpolate(shading, 0, p->depth, 32 * S3L_F);

    unsigned int r, g, b;

    r = S3L_clamp(S3L_interpolateByUnitFrom0(200, shading), 0, 255);
    g = S3L_clamp(S3L_interpolateByUnitFrom0(255, shading), 0, 255);
    b = S3L_clamp(S3L_interpolateByUnitFrom0(150, shading), 0, 255);

    r = (r << 16) & 0xFF0000;
    g = (g << 8) & 0x00FF00;
    b = b & 0x0000FF;
    
    tb_printf(p->x, p->y, r | g | b, 0x121212, "▚");
}

int main(int argc, char **argv) {
    struct tb_event ev;
    int y = 0;

    tb_init();
    tb_set_output_mode(TB_OUTPUT_TRUECOLOR);
    int width = tb_width(), height = tb_height();
    S3L_resolutionX = width;
    S3L_resolutionY = height;


    S3L_vec4Set(&toLight, 10, -10, -10, 0);
    S3L_vec3Normalize(&toLight);

    alligatorModelInit();
    S3L_computeModelNormals(alligatorModel, normals, 0);
    
    S3L_sceneInit(&alligatorModel, 1, &scene);

    scene.camera.transform.translation.z = -12 * S3L_F;
    scene.camera.transform.translation.x = 12 * S3L_F;
    scene.camera.transform.translation.y = 8 * S3L_F;

    S3L_lookAt(scene.models[0].transform.translation, &(scene.camera.transform));

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            tb_set_cell(i, j, '\0', TB_DEFAULT, TB_HI_BLACK);
        }
    }
    
    S3L_newFrame();
    S3L_drawScene(scene);
    tb_present();

    tb_poll_event(&ev);
    tb_shutdown();

    return 0;
}
