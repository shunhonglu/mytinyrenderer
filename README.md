## 1. Transformation

### 1.1 Model Transformation

you can rotate your model around y-axis now, and you can also scale or translate your model. but remenber that the model file in obj format is in the cube $[-1, 1]^{3}$.

```cpp
void set_model_mat(const float &angle, const Vec3f& scale, const Vec3f& translate);
```

**you can add rotation around any axis!**

### 1.2 View Transformation

We can set the position of the camera, the direction of the camera, and the direction of the camera up. But remember that the direction of the camera and the direction of the camera up must be orthogonal. It's not convenient that you can imporve it.

```cpp
void set_view_mat(Vec3f& camera_position, Vec3f& camera_dir, Vec3f& up);
```

### 1.3 Projection Transformation

```cpp
void set_projection_matrix(const float& eye_fov, const float& aspect_ratio, const float& zNear, const float& zFar);
```cpp
