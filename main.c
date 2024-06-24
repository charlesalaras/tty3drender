#define TB_IMPL
#define TB_LIB_OPTS
//#define TB_OPT_ATTR_W 64
#define CLEAR_COLOR 0xEEEEEE
#define TB_OPT_TRUECOLOR
#define S3L_USE_WIDER_TYPES 1
#define S3L_MAX_PIXELS 1
#define S3L_PIXEL_FUNCTION render
#include "termbox2.h"
#include "small3dlib.h"
#include "alligatorModel.h"
#include <math.h>

S3L_Unit normals[ALLIGATOR_VERTEX_COUNT * 3];

S3L_Scene scene;

uint32_t previousTriangle = 1000;
S3L_Vec4 n0, n1, n2, toLight;

// https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color/37624009#37624009
// reverses the rgb gamma
#define inverseGamma(t) (((t) <= 0.0404482362771076) ? ((t)/12.92) : pow(((t) + 0.055)/1.055, 2.4))

//CIE L*a*b* f function (used to convert XYZ to L*a*b*)  http://en.wikipedia.org/wiki/Lab_color_space
#define LABF(t) ((t >= 8.85645167903563082e-3) ? powf(t,0.333333333333333) : (841.0/108.0)*(t) + (4.0/29.0))


float
rgbToCIEL(float r, float g, float b)
{
   float y;
   r = r / 255.0;
   g = g / 255.0;
   b = b / 255.0;

   r=inverseGamma(r);
   g=inverseGamma(g);
   b=inverseGamma(b);

   //Observer = 2°, Illuminant = D65 
   y = 0.2125862307855955516*r + 0.7151703037034108499*g + 0.07220049864333622685*b;

   // At this point we've done RGBtoXYZ now do XYZ to Lab

   // y /= WHITEPOINT_Y; The white point for y in D65 is 1.0

    y = LABF(y);

   /* This is the "normal conversion which produces values scaled to 100
    Lab.L = 116.0*y - 16.0;
   */
   return(1.16*y - 0.16); // return values for 0.0 >=L <=1.0
}

const char* lighting[] = { 
    ".",
    "▁",
    "░",
    "-",
    "+",
    "▒",
    "*",
    "=",
    "/",
    "▚",
    "▙",
    "%",
    "#",
    "@",
    "▓",
    "█",
};

const char* calcLighting(unsigned int r, unsigned int g, unsigned int b) {
    unsigned int index = (15 * rgbToCIEL(r, g, b));
    if (index > 15 || index < 0) {
        tb_shutdown();
        printf("ERROR!: Bad index found with index %d",  index);
        exit(1);
    }
    return lighting[index];
}

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

    const char* lightingChar = calcLighting(r, g, b);

    r = (r << 16) & 0xFF0000;
    g = (g << 8) & 0x00FF00;
    b = b & 0x0000FF;
    
    tb_printf(p->x, p->y, r | g | b, CLEAR_COLOR, lightingChar);
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

    int active = 1;
    
    while (active) {

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                tb_set_cell(i, j, '\0', TB_DEFAULT, CLEAR_COLOR);
            }
        }
        S3L_newFrame();
        S3L_drawScene(scene);
        tb_present();

        S3L_Vec4 camF, camR;
        S3L_rotationToDirections(scene.camera.transform.rotation, 20, &camF, &camR, 0);

        tb_poll_event(&ev);

        if (ev.key == 0) {
            switch(ev.ch) {
                case 'q':
                    active = 0;
                    break;
                case 'w': // +x
                    scene.camera.transform.rotation.x += 1;
                    break;
                case 'a': // +y
                    scene.camera.transform.rotation.y += 1;
                    break;
                case 's': // -x
                    scene.camera.transform.rotation.x -= 1;
                    break;
                case 'd': // -y
                    scene.camera.transform.rotation.y -= 1;
                    break;
                default: break;
            }
        } else {
            switch(ev.key) {
                case 65517: // UP
                    S3L_vec3Add(&scene.camera.transform.translation, camF);
                    break;
                case 65516: // DOWN
                    S3L_vec3Sub(&scene.camera.transform.translation, camF);
                    break;
                case 65515: // LEFT
                    S3L_vec3Sub(&scene.camera.transform.translation, camR);
                    break;
                case 65514: // RIGHT
                    S3L_vec3Add(&scene.camera.transform.translation, camR);
                    break;
                case 27:
                    active = 0;
                    break;
                default: break;
            }
        }
    }
    tb_shutdown();

    return 0;
}
