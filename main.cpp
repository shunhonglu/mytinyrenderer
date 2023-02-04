#include <vector>
#include <iostream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model *model     = NULL;
const int width  = 800;
const int height = 800;

// case 1
Vec3f light_dir(0,0,1);
Vec3f camera_position(0, 0, 2);
Vec3f camera_direction(0,0,-1);
Vec3f up(0,1,0);

// case 2
// Vec3f light_dir(0,0,1);
// Vec3f camera_position(2, 0, 0);
// Vec3f camera_direction(-1,0,0);
// Vec3f up(0,1,0);

// case 3
// Vec3f light_dir(0,0,1);
// Vec3f camera_position(2, 2, 2);
// Vec3f camera_direction(-1,-1,-1);
// Vec3f up(-1,2,-1);
// float near = 1.f;
// float far = 5.f;

float near = 1.f;
float far = 3.f;

struct GouraudShader : public IShader {
    Vec3f varying_intensity; // written by vertex shader, read by fragment shader

    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        gl_Vertex = transformation(gl_Vertex);     // transform it to screen coordinates
        varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert)*light_dir); // get diffuse lighting intensity
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        float intensity = varying_intensity*bar;   // interpolate intensity for the current pixel
        color = TGAColor(255, 255, 255)*intensity; // well duh
        return false;                              // no, we do not discard this pixel
    }
};

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }

    set_model_mat(0.f, Vec3f(1.f, 1.f, 1.f), Vec3f(0.f, 0.f, 0.f));
    set_view_mat(camera_position, camera_direction, up);
    set_projection_matrix(90.f, 1.f, near, far);
    set_viewport_mat(0, 0, width, height);

    TGAImage image  (width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    for(int i=0; i<width; ++i) {
        for(int j=0; j<height; ++j)
            zbuffer.set(i, j, TGAColor(255));
    }

    GouraudShader shader;
    for (int i=0; i<model->nfaces(); i++) {
        Vec4f screen_coords[3];
        for (int j=0; j<3; j++) {
            screen_coords[j] = shader.vertex(i, j);
        }
        triangle(screen_coords, shader, image, zbuffer, near, far);
    }

    image.  flip_vertically(); // to place the origin in the bottom left corner of the image
    zbuffer.flip_vertically();
    image.  write_tga_file("output.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    delete model;
    return 0;
}

// #include "tgaimage.h"
// #include "geometry.h"

// int main() {
//     TGAImage image  (100, 100, TGAImage::RGB);
//     const TGAColor green  = TGAColor(0, 255, 0, 255);

//     for(int i = 0; i<image.get_width(); ++i) {
//         for(int j = 0; j<image.get_height();++j){
//             image.set(i, j, green);
//         }
//     }

//     image.  flip_vertically(); // to place the origin in the bottom left corner of the image
//     image.  write_tga_file("twocube.tga");
//     return 0;
// }