#include <string>
#include <vector>
#include <iostream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model *model     = NULL;
IShader* s;
const int width  = 800;
const int height = 800;
float near = 0.5f;
float far = 2.5f;
vector<vector<float>> sample_list;
vector<vector<TGAColor>> sample_list_color;

// case 1
// unit vector!
Vec3f light_dir(0,0,1);
Vec3f camera_position(0, 0, 1.5);
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

struct GouraudShader : public IShader {
    GouraudShader(const mat<4,4,float>& u_M = View*ModelMat, const mat<4,4,float>& u_MIT = (View*ModelMat).invert_transpose()) :
        varying_intensity(Vec3f()), varying_uv(mat<2,3,float>()), uniform_M(u_M), uniform_MIT(u_MIT) {}

    Vec3f varying_intensity; // written by vertex shader, read by fragment shader
    mat<2,3,float> varying_uv;        // same as above
    mat<4,4,float> uniform_M;   //  View*ModelMat
    mat<4,4,float> uniform_MIT; // (View*ModelMat).invert_transpose();

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert)); // read the uv from .obj file

        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        gl_Vertex = transformation(gl_Vertex);     // transform it to screen coordinates

        Vec4f normal = uniform_MIT * toVec4f(model->normal(iface, nthvert), 0.f);
        varying_intensity[nthvert] = std::max(0.f, proj<3>(normal).normalize()*light_dir.normalize()); // get diffuse lighting intensity
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        float sum = bar[0] + bar[1] + bar[2];
        float intensity = varying_intensity*bar/sum;   // interpolate intensity for the current pixel
        Vec2f uv = varying_uv*bar/sum;                 // interpolate uv for the current pixel
        if(intensity < 1e-2) {
            return true;
        }
        color = model->diffuse(uv)*intensity;      // well duh
        return false;                              // no, we do not discard this pixel
    }
};

struct BumpShader : public IShader {
    BumpShader(const mat<4,4,float>& u_M = View*ModelMat, const mat<4,4,float>& u_MIT = (View*ModelMat).invert_transpose()) :
        varying_uv(mat<2,3,float>()), uniform_M(u_M), uniform_MIT(u_MIT) {}

    mat<2,3,float> varying_uv;  // same as above
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        gl_Vertex = transformation(gl_Vertex);     // transform it to screen coordinates
        return gl_Vertex;
   }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        float sum = bar[0] + bar[1] + bar[2];
        Vec2f uv = varying_uv*bar/sum;                 // interpolate uv for the current pixel

        Vec4f normal = uniform_MIT * toVec4f(model->normal(uv), 0.f);
        Vec3f n = proj<3>(normal).normalize();
        Vec3f l = light_dir.normalize();
        float intensity = std::max(0.f, n*l);
        color = model->diffuse(uv)*intensity;      // well duh
        return false;                              // no, we do not discard this pixel
    }
};

struct PhongShader : public IShader {
    PhongShader(float am = 5.f, float kd_ = 1.f, float ks_ = 0.6f, const mat<4,4,float>& u_M = View*ModelMat, const mat<4,4,float>& u_MIT = (View*ModelMat).invert_transpose()) :
        ambient(am), kd(kd_), ks(ks_), varying_uv(mat<2,3,float>()), uniform_M(u_M), uniform_MIT(u_MIT) {}

    float ambient, kd, ks;
    mat<2,3,float> varying_uv;  // same as above
    mat<4,4,float> uniform_M;   //  Projection*ModelView
    mat<4,4,float> uniform_MIT; // (Projection*ModelView).invert_transpose()

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        gl_Vertex = transformation(gl_Vertex);     // transform it to screen coordinates
        return gl_Vertex;
   }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        float sum = bar[0] + bar[1] + bar[2];
        Vec2f uv = varying_uv*bar/sum;                 // interpolate uv for the current pixel

        Vec4f normal = uniform_MIT * toVec4f(model->normal(uv), 0.f);
        Vec3f n = proj<3>(normal).normalize();
        Vec3f l = light_dir.normalize();
        float diffuse = std::max(0.f, n*l);

        Vec3f r = (n*(n*l*2.f) - l).normalize();
        float specular = pow(std::max(r.z, 0.0f), model->specular(uv));
        color = model->diffuse(uv); 
        TGAColor c = color;
        for (int i=0; i<3; i++) color[i] = std::min<float>(ambient + c[i]*(kd*diffuse + ks*specular), 255);
        return false;                              // no, we do not discard this pixel
    }
};

int main(int argc, char** argv) {
    std::string output_image = "gouraud_output.tga";
    std::string zbuffer_image = "gouraud_zbuffer.tga";

    // **Shader** initialization needs **ModelMat**, **View**
    set_model_mat(0.f, Vec3f(1.f, 1.f, 1.f), Vec3f(0.f, 0.f, 0.f));
    set_view_mat(camera_position, camera_direction, up);
    set_projection_matrix(90.f, 1.f, near, far);
    set_viewport_mat(0, 0, width, height);

    if(argc>=2) {
        if(std::string(argv[1]) == "bump") {
            s = new BumpShader();
            output_image = "bump_output.tga";
            zbuffer_image = "bump_zbuffer.tga";
        }
        else if(std::string(argv[1]) == "phong") {
            s = new PhongShader();
            output_image = "phong_output.tga";
            zbuffer_image = "phong_zbuffer.tga";
        }
        else {
            s = new GouraudShader();
        }
            
        if(argc == 3) {
            model = new Model(argv[2]);
        }
        else {
            model = new Model("obj/african_head.obj");
        }
    }
    else {
        s = new GouraudShader();
        model = new Model("obj/african_head.obj");
    }

    TGAImage image  (width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    // sample_list = vector<vector<float>>(width*height, vector<float>(4, far));
    // sample_list_color = vector<vector<TGAColor>>(width*height, vector<TGAColor>(4));

    for(int i=0; i<width; ++i) {
        for(int j=0; j<height; ++j)
            zbuffer.set(i, j, TGAColor(255));
    }

    for (int i=0; i<model->nfaces(); i++) {
        Vec4f screen_coords[3];
        for (int j=0; j<3; j++) {
            screen_coords[j] = s->vertex(i, j);
        }
        // TODO: sample_list 需要恰当初始化！
        triangle(screen_coords, *s, image, zbuffer, sample_list, sample_list_color, near, far);
    }

    image.  flip_vertically(); // to place the origin in the bottom left corner of the image
    zbuffer.flip_vertically();
    image.  write_tga_file(output_image.c_str());
    zbuffer.write_tga_file(zbuffer_image.c_str());

    delete model;
    delete s;
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