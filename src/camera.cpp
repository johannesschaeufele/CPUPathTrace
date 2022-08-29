#include <PathTrace/camera.h>

#include <cmath>
#include <random>
#include <algorithm>

std::tuple<float, float> CircularApertureSampler::sampleAperture(RandomEngine &re) const noexcept {
    constexpr float pi = static_cast<float>(M_PI);

    std::uniform_real_distribution<float> dist(0, 1);

    auto r = std::sqrt(dist(re));
    auto theta = 2 * pi * dist(re);

    auto x = r * std::cos(theta);
    auto y = r * std::sin(theta);

    return std::make_tuple(x, y);
}

HexagonalApertureSampler::HexagonalApertureSampler(float horizontal_ratio) noexcept {
    this->horizontal_ratio = std::min(std::max(horizontal_ratio, 0.0F), 1.0F);
}

std::tuple<float, float> HexagonalApertureSampler::sampleAperture(RandomEngine &re) const noexcept {
    std::uniform_real_distribution<float> dist(0, 1);
    std::bernoulli_distribution dist_flip;

    float x; // NOLINT(cppcoreguidelines-init-variables)
    float y; // NOLINT(cppcoreguidelines-init-variables)
    bool in_polygon; // NOLINT(cppcoreguidelines-init-variables)
    do {
        x = dist(re);
        y = dist(re);

        auto relative_x = x - horizontal_ratio;

        in_polygon = (relative_x <= 0.0F) || (relative_x / (1.0F - horizontal_ratio)) >= y;
    } while(!in_polygon);

    if(dist_flip(re)) {
        x = -x;
    }
    if(dist_flip(re)) {
        y = -y;
    }

    return std::make_tuple(x, y);
}

Camera::Camera(vec3<float> origin, vec3<float> look_at, vec3<float> up, float focal_length, float height, float aspect_ratio) noexcept :
  Camera(origin, look_at, up, focal_length, height, aspect_ratio, 0.0F, 0.0F, nullptr, 0.0F) {}

Camera::Camera(vec3<float> origin, vec3<float> look_at, vec3<float> up, float focal_length, float height, float aspect_ratio, float aperture_width,
               float aperture_height, std::unique_ptr<ApertureSampler> &&aperture_sampler, float focal_plane_dist) noexcept {
    // Pinhole camera parameters
    this->origin = origin;

    auto forward_dir = (look_at - origin).normalize();
    this->forward = forward_dir * focal_length;

    auto up_dir = up.normalize();
    float height_half = height / 2.0F;
    this->up = up_dir * height_half;
    auto right_dir = cross(this->forward, this->up).normalize();
    float width_half = height_half * aspect_ratio;
    this->right = right_dir * width_half;

    // Aperture
    this->aperture_width_half = aperture_width / 2.0F;
    this->aperture_height_half = aperture_height / 2.0F;
    this->aperture_sampler = std::move(aperture_sampler);

    // Thin lens model, focal plane
    this->focal_plane_dist = focal_plane_dist;
}

Ray Camera::shootRay(float x, float y, float pixel_width, float pixel_height, RandomEngine &re) const noexcept {
    std::uniform_real_distribution<float> dist_x(-pixel_width / 2.0F, pixel_width / 2.0F);
    std::uniform_real_distribution<float> dist_y(-pixel_height / 2.0F, pixel_height / 2.0F);

    auto offset_x = dist_x(re);
    auto offset_y = dist_y(re);

    auto sensor_x = x + offset_x;
    auto sensor_y = y + offset_y;

    auto sensor_pos = this->origin - this->forward - this->up * sensor_y - this->right * sensor_x;

    // Sample aperture
    auto aperture_offset_x = 0.0F;
    auto aperture_offset_y = 0.0F;
    if(aperture_sampler) {
        auto [sample_x, sample_y] = aperture_sampler->sampleAperture(re);

        aperture_offset_x = sample_x * this->aperture_width_half;
        aperture_offset_y = sample_y * this->aperture_height_half;
    }
    auto ray_origin = this->origin + this->up * aperture_offset_x + this->right * aperture_offset_y;

    vec3<float> ray_dir;
    if(this->focal_plane_dist > 0.0F) {
        // Apply thin lens
        auto base_dir = (this->origin - sensor_pos).normalize();
        auto ray_target = this->origin + base_dir * (this->focal_plane_dist / dot(this->forward, base_dir));
        ray_dir = (ray_target - ray_origin).normalize();
    }
    else {
        ray_dir = (ray_origin - sensor_pos).normalize();
    }

    return {ray_origin, ray_dir};
}
