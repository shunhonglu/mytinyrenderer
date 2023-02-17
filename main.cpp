#include <iostream>
#include <memory>
#include <numeric>
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
vector<vector<float>> sample_list_shadow;
vector<vector<TGAColor>> sample_list_color;

Vec3f light_pos(0, 100, 100);
Vec3f camera_position(60, 60, 60);
Vec3f camera_direction(-1.5, 0, -1);
Vec3f up(0, 1, 0);
float near = 0.2f;
float far = 300.f;

void init();

struct DepthShader : public IShader {
    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));  // read the vertex from .obj file
        gl_Vertex = transformation(gl_Vertex);                    // transform it to screen coordinates
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f, TGAColor&) {
        return false;  // no, we do not discard this pixel
    }
};

struct ShadowShader : public IShader {
    ShadowShader(const mat<4, 4, float>& u_Mshadow = mat<4, 4, float>(), float am = 5.f, float kd_ = 2.f,
                 float ks_ = 0.8f, const mat<4, 4, float>& u_M = View * ModelMat,
                 const mat<4, 4, float>& u_MIT = (View * ModelMat).invert_transpose())
        : ambient(am),
          kd(kd_),
          ks(ks_),
          varying_uv(mat<2, 3, float>()),
          varying_normal(mat<3, 3, float>()),
          varying_tri(mat<4, 3, float>()),
          uniform_M(u_M),
          uniform_MIT(u_MIT),
          uniform_Mshadow(u_Mshadow),
          l_pos(proj<3>(u_M * toVec4f(light_pos, 1.f))) {}

    float ambient, kd, ks;
    mat<2, 3, float> varying_uv;
    mat<3, 3, float> varying_normal;
    mat<4, 3, float> varying_tri;
    mat<4, 4, float> uniform_M;    //  View*ModelMat
    mat<4, 4, float> uniform_MIT;  // (View*ModelMat).invert_transpose()
    mat<4, 4, float> uniform_Mshadow;
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
        Vec4f view_pos = varying_tri * bar / sum;
        Vec4f sb_p = uniform_Mshadow * (view_pos);  // corresponding point in the shadow buffer
        for (int i = 0; i < 3; ++i) {
            sb_p[i] /= sb_p[3];
        }
        int idx = int(sb_p[0]) + int(sb_p[1]) * width;  // index in the shadowbuffer array
        float sum_depth = std::accumulate(sample_list_shadow[idx].begin(), sample_list_shadow[idx].end(), 0.f) / 4.f;
        float shadow = 0.3f + 0.7 * (std::abs(sb_p[3] - sum_depth) < 0.1f);

        Vec2f uv = varying_uv * bar / sum;
        Vec3f normal = (varying_normal * bar / sum).normalize();

        Vec3f l = (l_pos - Vec3f(view_pos[0], view_pos[1], view_pos[2])).normalize();
        Vec3f eye_dir = Vec3f(-view_pos[0], -view_pos[1], -view_pos[2]).normalize();
        Vec3f h = (l + eye_dir).normalize();  // half vector!

        float diffuse = std::max(0.f, normal * l);
        float specular = pow(std::max(h * normal, 0.0f), model->specular(uv));

        color = model->diffuseBilinear(uv);
        TGAColor c = color;
        for (int i = 0; i < 3; i++)
            color[i] = std::min<float>(ambient + c[i] * shadow * (kd * diffuse + ks * specular), 255);
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
        // **Shader** initialization needs **ModelMat**, **View**
        set_model_mat(0.f, Vec3f(1.f, 1.f, 1.f), Vec3f(0.f, 0.f, 0.f));
        Vec3f light_dir = Vec3f(-light_pos[0], -light_pos[1], -light_pos[2]);
        // TODO:
        Vec3f light_up = Vec3f(-light_pos[0] * light_pos[1], (std::pow(light_pos[0], 2) + std::pow(light_pos[2], 2)),
                               -light_pos[2] * light_pos[1]);
        set_view_mat(light_pos, light_dir, light_up);
        set_projection_matrix(90.f, 1.f, near, far);
        set_viewport_mat(0, 0, width, height);
        std::shared_ptr<IShader> s(new DepthShader);

        for (int i = 0; i < model->nfaces(); i++) {
            Vec4f screen_coords[3];
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = s->vertex(i, j);
            }
            triangle(screen_coords, *s, image, zbuffer, sample_list_shadow, sample_list_color, near, far);
        }
    }

    Matrix M = Viewport * Projection * View * ModelMat;
    image.clear();
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            zbuffer.set(i, j, TGAColor(255));
        }
    }

    {
        // **Shader** initialization needs **ModelMat**, **View**
        set_model_mat(0.f, Vec3f(1.f, 1.f, 1.f), Vec3f(0.f, 0.f, 0.f));
        set_view_mat(camera_position, camera_direction, up);
        set_projection_matrix(90.f, 1.f, near, far);
        set_viewport_mat(0, 0, width, height);
        std::shared_ptr<IShader> s(new ShadowShader(M * (View * ModelMat).invert()));

        for (int i = 0; i < model->nfaces(); i++) {
            Vec4f screen_coords[3];
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = s->vertex(i, j);
            }
            triangle(screen_coords, *s, image, zbuffer, sample_list, sample_list_color, near, far);
        }
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
    model = new Model("../obj/fox/fox.obj");
    sample_list = vector<vector<float>>(width * height, vector<float>(4, far));
    sample_list_shadow = vector<vector<float>>(width * height, vector<float>(4, far));
    sample_list_color = vector<vector<TGAColor>>(width * height, vector<TGAColor>(4));

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            zbuffer.set(i, j, TGAColor(255));
        }
    }
}
