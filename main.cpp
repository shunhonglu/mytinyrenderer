#include <iostream>
#include <string>
#include <vector>

#include "geometry.h"
#include "model.h"
#include "our_gl.h"
#include "tgaimage.h"

Model* model = NULL;
const int width = 1600;
const int height = 1600;
TGAImage image(width, height, TGAImage::RGB);
TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
vector<vector<float>> sample_list;
vector<vector<TGAColor>> sample_list_color;

Vec3f light_pos(5, 0, 5);
Vec3f camera_position(1, 0.5, 1);
Vec3f camera_direction(-2, -1, -2);
Vec3f up(-1, 4, -1);
float near = 0.1f;
float far = 10.f;

void init();

struct BumpShader : public IShader {
    BumpShader(float am = 5.f, float kd_ = 2.f, float ks_ = 0.8f, const mat<4, 4, float>& u_M = View * ModelMat,
               const mat<4, 4, float>& u_MIT = (View * ModelMat).invert_transpose())
        : ambient(am),
          kd(kd_),
          ks(ks_),
          varying_uv(mat<2, 3, float>()),
          varying_normal(mat<3, 3, float>()),
          varying_tri(mat<4, 3, float>()),
          uniform_M(u_M),
          uniform_MIT(u_MIT),
          l_pos(proj<3>(u_M * toVec4f(light_pos, 1.f))) {}

    float ambient, kd, ks;
    mat<2, 3, float> varying_uv;
    mat<3, 3, float> varying_normal;
    mat<4, 3, float> varying_tri;
    mat<4, 4, float> uniform_M;    //  View*ModelView
    mat<4, 4, float> uniform_MIT;  // (View*ModelView).invert_transpose()
    Vec3f l_pos;

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));          // read the uv from .obj file
        varying_normal.set_col(nthvert, model->normal(iface, nthvert));  // read the normal from .obj file
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));         // read the vertex from .obj file

        varying_tri.set_col(nthvert, uniform_M * gl_Vertex);  // compute the coordinates of a vertex in space
        gl_Vertex = transformation(gl_Vertex);                // transform it to screen coordinates
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor& color) {
        float sum = bar[0] + bar[1] + bar[2];
        Vec2f uv = varying_uv * bar / sum;  // interpolate uv for the current pixel
        Vec3f normal = (varying_normal * bar / sum).normalize();
        Vec4f view_pos = varying_tri * bar / sum;

        //
        mat<3, 3, float> A;
        A[0] = proj<3>(varying_tri.col(1)) - proj<3>(varying_tri.col(0));
        A[1] = proj<3>(varying_tri.col(2)) - proj<3>(varying_tri.col(0));
        A[2] = normal;

        mat<3, 3, float> AI = A.invert();

        Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);

        mat<3, 3, float> B;
        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, normal);

        Vec3f n = (B * model->normal(uv)).normalize();
        //

        Vec3f l = (l_pos - Vec3f(view_pos[0], view_pos[1], view_pos[2])).normalize();
        Vec3f eye_dir = Vec3f(-view_pos[0], -view_pos[1], -view_pos[2]).normalize();
        Vec3f h = (l + eye_dir).normalize();  // half vector!

        float diffuse = std::max(0.f, n * l);
        float specular = pow(std::max(h * n, 0.0f), model->specular(uv));

        color = model->diffuseBilinear(uv);
        TGAColor c = color;
        for (int i = 0; i < 3; i++) color[i] = std::min<float>(ambient + c[i] * (kd * diffuse + ks * specular), 255);
        return false;  // no, we do not discard this pixel
    }
};

int main(int argc, char** argv) {
    std::string output_image = "phong_output.tga";
    std::string zbuffer_image = "phong_zbuffer.tga";

    if (argc == 2) {
        output_image = argv[1] + std::string("_output.tga");
        zbuffer_image = argv[1] + std::string("_zbuffer.tga");
    }

    init();

    {
        IShader* s;
        // **Shader** initialization needs **ModelMat**, **View**
        set_model_mat(0.f, Vec3f(1.f, 1.f, 1.f), Vec3f(0.f, 0.f, 0.f));
        set_view_mat(camera_position, camera_direction, up);
        set_projection_matrix(90.f, 1.f, near, far);
        set_viewport_mat(0, 0, width, height);
        s = new BumpShader();

        for (int i = 0; i < model->nfaces(); i++) {
            Vec4f screen_coords[3];
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = s->vertex(i, j);
            }
            triangle(screen_coords, *s, image, zbuffer, sample_list, sample_list_color, near, far);
        }
        delete s;
    }

    image.flip_vertically();  // to place the origin in the bottom left corner
                              // of the image
    zbuffer.flip_vertically();
    image.write_tga_file(output_image.c_str());
    zbuffer.write_tga_file(zbuffer_image.c_str());

    delete model;
    return 0;
}

void init() {
    model = new Model("../obj/diablo3_pose/diablo3_pose.obj");
    sample_list = vector<vector<float>>(width * height, vector<float>(4, far));
    sample_list_color = vector<vector<TGAColor>>(width * height, vector<TGAColor>(4));

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            zbuffer.set(i, j, TGAColor(255));
        }
    }
}
