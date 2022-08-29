#include <PathTrace/scene/propagation.h>

#include <cmath>
#include <algorithm>
#include <random>

constexpr float pi = static_cast<float>(M_PI);

namespace impl {

    std::tuple<vec3<float>, float> importanceSampleCosine(float r1, float r2, float e) {
        float fac = std::sqrt(1.0F - std::pow(r2, 2.0F / (e + 1)));

        float cos_theta = std::pow(r2, 1.0F / (e + 1));

        vec3<float> vec = {fac * std::cos(2.0F * pi * r1), fac * std::sin(2.0F * pi * r1), cos_theta};

        float p = (e + 1) * std::pow(cos_theta, e) / (2.0F * pi);

        return std::make_tuple(vec, p);
    }

    // Transform vector from tangent space coordinates to world coordinates
    vec3<float> localToGlobal(vec3<float> vec, vec3<float> n) {
        assert(n.getLengthSquared() > 0.0F);
        // Generate vector linearly independent of n
        vec3<float> d;
        if(std::abs(n[0]) > 0.0F) {
            if(std::abs(n[1]) > 0.0F) {
                d = vec3<float>{0.0F, -n[0], n[1]};
            }
            else {
                d = vec3<float>{0.0F, -n[0], n[2]};
            }
        }
        else {
            if(std::abs(n[1]) > 0.0F) {
                d = vec3<float>{-n[1], n[2], 0.0F};
            }
            else {
                d = vec3<float>{1.0F, 0.0F, 0.0F};
            }
        }

        d = d.normalize();
        assertNormalized(d);

        // Generate basis b1, b2, n using n, d
        auto b1 = cross(d, n).normalize();
        auto b2 = cross(b1, n).normalize();

        assert(std::abs(dot(b1, n)) < 1E-4F);
        assert(std::abs(dot(b2, n)) < 1E-4F);
        assert(std::abs(dot(b1, b2)) < 1E-4F);

        // Change basis of vec from the generated basis to the standard basis
        vec3<float> vx = {b1[0], b2[0], n[0]};
        vec3<float> vy = {b1[1], b2[1], n[1]};
        vec3<float> vz = {b1[2], b2[2], n[2]};

        return {dot(vx, vec), dot(vy, vec), dot(vz, vec)};
    }

    std::tuple<float, float> getFresnelReflectance(float ray_dot, float ri_leaving, float ri_entering) {
        auto sin_theta_i = std::sqrt(std::max(1.0F - ray_dot * ray_dot, 0.0F));
        auto sin_theta_t = ri_leaving / ri_entering * sin_theta_i;

        // Total reflection
        if(sin_theta_t >= 1.0F) {
            return std::make_tuple(1.0F, 0.0F);
        }

        // TODO: Extract as Snell
        auto cos_theta_t = std::sqrt(std::max(1.0F - sin_theta_t * sin_theta_t, 0.0F));

        auto r_parallel = ((ri_entering * ray_dot) - (ri_leaving * cos_theta_t)) / ((ri_entering * ray_dot) + (ri_leaving * cos_theta_t));
        auto r_perpendicular = ((ri_leaving * ray_dot) - (ri_entering * cos_theta_t)) / ((ri_leaving * ray_dot) + (ri_entering * cos_theta_t));

        // Unpolarized reflectance
        auto reflectance = (r_parallel * r_parallel + r_perpendicular * r_perpendicular) / 2.0F;

        return std::make_tuple(reflectance, cos_theta_t);
    }

}

LambertianBRDF::LambertianBRDF() noexcept = default;

std::tuple<Ray, float, float> LambertianBRDF::propagateRay(Ray /*ray*/, vec3<float> pos, vec3<float> normal, float epsilon, RandomEngine &re,
                                                           const Material * /*material*/) const noexcept {
    using namespace impl;

    assertNormalized(normal);

    std::uniform_real_distribution<float> dist(0, 1);

    auto [local_dir, p] = importanceSampleCosine(dist(re), dist(re), 1.0F);
    assertNormalized(local_dir);

    vec3<float> dir = localToGlobal(local_dir, normal);
    assertNormalized(dir);
    Ray out_ray = {pos + dir * epsilon, dir};

    return std::make_tuple(out_ray, 1.0F, p);
}

std::tuple<Spectrum, float, float> LambertianBRDF::getSpectrum(Ray /*from_camera*/, Ray to_light, vec3<float> pos, vec3<float> normal, Spectrum light_spectrum,
                                                               const Material *material, bool /*synthetic*/) const noexcept {
    assertNormalized(normal);
    assertNormalized(to_light.dir);
    float shade_factor = std::max(dot(normal, to_light.dir), 0.0F) / pi;

    Spectrum spectrum_multiplier = {material->getDiffuseColor(pos)};

    return std::make_tuple(spectrum_multiplier * light_spectrum, shade_factor, 1.0F);
}

GlassBDF::GlassBDF() noexcept = default;

std::tuple<Ray, float, float> GlassBDF::propagateRay(Ray ray, vec3<float> pos, vec3<float> normal, float epsilon, RandomEngine &re,
                                                     const Material *material) const noexcept {
    using namespace impl;

    assertNormalized(normal);
    assertNormalized(ray.dir);

    float ray_dot = -dot(ray.dir, normal);

    float refractive_index = material->getRefractiveIndex(pos);
    float ri_leaving = ray_dot >= 0 ? 1.0F : refractive_index;
    float ri_entering = ray_dot >= 0 ? refractive_index : 1.0F;

    auto [rat, cosThetaT] = getFresnelReflectance(std::abs(ray_dot), ri_leaving, ri_entering);

    assert(rat >= 0.0F && rat <= 1.0F);
    std::bernoulli_distribution d(rat);

    if(d(re)) {
        // Reflect

        vec3<float> dir = reflect(ray.dir, normal * (ray_dot < 0.0F ? -1.0F : 1.0F));
        assertNormalized(dir);
        Ray out_ray = {pos + dir * epsilon, dir};

        return std::make_tuple(out_ray, rat, rat);
    }
    else {
        // Refract

        auto ri_ratio = ri_leaving / ri_entering;

        auto out_dir = ray.dir * ri_ratio + normal * (ri_ratio * std::abs(ray_dot) - cosThetaT) * (ray_dot < 0.0F ? -1.0F : 1.0F);
        out_dir = out_dir.normalize();

        auto ri_fac = (ri_entering * ri_entering) / (ri_leaving * ri_leaving);

        Ray out_ray = {pos + out_dir * epsilon, out_dir};
        return std::make_tuple(out_ray, ri_fac * (1.0F - rat), 1.0F - rat);
    }
}

std::tuple<Spectrum, float, float> GlassBDF::getSpectrum(Ray from_camera, Ray to_light, vec3<float> pos, vec3<float> /*normal*/, Spectrum light_spectrum,
                                                         const Material *material, bool synthetic) const noexcept {
    auto out_spectrum = light_spectrum;

    if(dot(from_camera.dir, to_light.dir) <= 0.0F) {
        out_spectrum = out_spectrum * Spectrum{material->getSpecularColor(pos)};
    }
    else {
        out_spectrum = out_spectrum * Spectrum{material->getDiffuseColor(pos)};
    }

    auto p = synthetic ? 0.0F : 1.0F;

    return std::make_tuple(out_spectrum, 1.0F, p);
}

MirrorBRDF::MirrorBRDF(bool one_way) noexcept : one_way(one_way) {}

std::tuple<Ray, float, float> MirrorBRDF::propagateRay(Ray ray, vec3<float> pos, vec3<float> normal, float epsilon, RandomEngine & /*re*/,
                                                       const Material * /*material*/) const noexcept {
    assertNormalized(ray.dir);

    // Transmit through the back face of one-way mirrors
    auto unaligned = dot(ray.dir, normal) > 0.0F;
    if(this->one_way && unaligned) {
        auto out_dir = ray.dir;

        Ray out_ray = {pos + out_dir * epsilon, out_dir};
        return std::make_tuple(out_ray, 1.0F, 1.0F);
    }

    // Reflect otherwise
    auto normal_dir = normal;
    if(!this->one_way && unaligned) {
        normal_dir = normal_dir * -1.0F;
    }

    vec3<float> dir = reflect(ray.dir, normal_dir);
    assertNormalized(dir);
    Ray out_ray = {pos + dir * epsilon, dir};

    return std::make_tuple(out_ray, 1.0F, 1.0F);
}

std::tuple<Spectrum, float, float> MirrorBRDF::getSpectrum(Ray from_camera, Ray to_light, vec3<float> pos, vec3<float> /*normal*/, Spectrum light_spectrum,
                                                           const Material *material, bool synthetic) const noexcept {
    auto out_spectrum = light_spectrum;

    if(!this->one_way || (dot(from_camera.dir, to_light.dir) <= 0.0F)) {
        out_spectrum = out_spectrum * material->getSpecularColor(pos);
    }

    auto p = synthetic ? 0.0F : 1.0F;

    return std::make_tuple(out_spectrum, 1.0F, p);
}
