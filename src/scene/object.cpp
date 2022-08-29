#include <PathTrace/scene/object.h>
#include <PathTrace/scene/bounding_box.h>

#include <cassert>
#include <cmath>
#include <algorithm>
#include <utility>

const std::shared_ptr<Material> default_material = std::make_shared<ConstantMaterial>(Color<float>(1.0F, 1.0F, 1.0F, 1.0F));
const std::shared_ptr<BSDF> default_bsdf = std::make_shared<LambertianBRDF>();
const std::shared_ptr<MaterialHandler> default_material_handler = std::make_shared<ConstantMaterialHandler>(default_material, default_bsdf);

const Material *MaterialHandler::probeMaterial() const noexcept {
    return default_material.get();
}

ConstantMaterialHandler::ConstantMaterialHandler(std::shared_ptr<Material> material, std::shared_ptr<BSDF> bsdf) :
  material(std::move(material)), bsdf(std::move(bsdf)) {}

const Material *ConstantMaterialHandler::getMaterial(vec3<float> /*pos*/) const noexcept {
    return this->material.get();
}

const BSDF *ConstantMaterialHandler::getBSDF(vec3<float> /*pos*/) const noexcept {
    return this->bsdf.get();
}

const Material *ConstantMaterialHandler::probeMaterial() const noexcept {
    return this->material.get();
}

Object::Object() : material_handler(default_material_handler) {}

Object::Object(std::shared_ptr<MaterialHandler> material_handler) noexcept : material_handler(std::move(material_handler)) {}

const MaterialHandler *Object::getMaterialHandler() const noexcept {
    return this->material_handler.get();
}

void Object::setMaterialHandler(std::shared_ptr<MaterialHandler> material_handler) {
    this->material_handler = std::move(material_handler);
}

float Object::getSurfaceArea() const noexcept {
    return 0.0F;
}

std::tuple<vec3<float>, float, bool> Object::sampleSurface(RandomEngine & /*re*/) const noexcept {
    return std::make_tuple(vec3<float>{}, 0.0F, false);
}

float NullObject::getIntersection(const Ray & /*ray*/) const noexcept {
    return -1.0F;
}

vec3<float> NullObject::getSurfaceNormal(vec3<float> /*pos*/) const noexcept {
    return {0.0F, 1.0F, 0.0F};
}

AABBArea NullObject::getBoundingVolume() const noexcept {
    return {};
}

float NullObject::getSurfaceArea() const noexcept {
    return 0.0F;
}

Sphere::Sphere(vec3<float> origin, float radius) : origin(origin), radius(radius), radius2(radius * radius) {
    assert(radius >= 0.0F);
}

float Sphere::getIntersection(const Ray &ray) const noexcept {
    auto co = ray.origin - this->origin;
    auto d = dot(ray.dir, co);
    auto discriminant = d * d - co.getLengthSquared() + this->radius2;

    if(discriminant >= 0) {
        float t = -(d + std::sqrt(discriminant));

        return t;
    }

    return -static_cast<float>(1);
}

vec3<float> Sphere::getSurfaceNormal(vec3<float> pos) const noexcept {
    return (pos - this->origin).normalize();
}

AABBArea Sphere::getBoundingVolume() const noexcept {
    vec3<float> d = {this->radius, this->radius, this->radius};
    return {this->origin - d, this->origin + d};
}

float Sphere::getSurfaceArea() const noexcept {
    constexpr float pi = static_cast<float>(M_PI);

    return 4.0F * pi * this->radius2;
}

std::tuple<vec3<float>, float, bool> Sphere::sampleSurface(RandomEngine &re) const noexcept {
    constexpr float pi = static_cast<float>(M_PI);

    std::uniform_real_distribution<float> dist(0, 1);

    auto theta = 2.0F * pi * dist(re);
    auto phi = std::acos(1.0F - 2.0F * dist(re));
    auto x = std::sin(phi) * std::cos(theta);
    auto y = std::sin(phi) * std::sin(theta);
    auto z = std::cos(phi);

    auto pos = this->origin + vec3<float>{x, y, z} * this->radius;
    auto p = 1.0F / (4.0F * pi * this->radius2);

    return std::make_tuple(pos, p, false);
}

Triangle::Triangle(vec3<float> a, vec3<float> b, vec3<float> c, bool cull_backface) : a(a), b(b), c(c), cull_backface(cull_backface) {
    auto face_normal = cross(b - a, c - a).normalize();

    this->normal_a = face_normal;
    this->normal_b = face_normal;
    this->normal_c = face_normal;
}

vec3<float> Triangle::getSurfaceNormal(vec3<float> pos) const noexcept {
    auto ab = this->b - this->a;
    auto ac = this->c - this->a;
    auto ap = pos - this->a;

    float d00 = dot(ab, ab);
    float d01 = dot(ab, ac);
    float d11 = dot(ac, ac);
    float d20 = dot(ap, ab);
    float d21 = dot(ap, ac);

    float inv_d = 1.0F / (d00 * d11 - d01 * d01);

    auto v = (d11 * d20 - d01 * d21) * inv_d;
    auto w = (d00 * d21 - d01 * d20) * inv_d;
    auto u = 1.0F - v - w;

    return (this->normal_a * u + this->normal_b * v + this->normal_c * w).normalize();
}

float Triangle::getIntersection(const Ray &ray) const noexcept {
    constexpr float epsilon = 1E-6F;

    auto ab = this->b - this->a;
    auto ac = this->c - this->a;
    auto pvec = cross(ray.dir, ac);
    auto det = dot(ab, pvec);

    if(this->cull_backface) {
        if(det <= epsilon) {
            return -static_cast<float>(1);
        }
    }
    else {
        if(std::abs(det) <= epsilon) {
            return -static_cast<float>(1);
        }
    }

    float inv_det = 1.0F / det;

    auto tvec = ray.origin - this->a;
    auto u = dot(tvec, pvec) * inv_det;
    if(u < 0 || u > 1) {
        return -static_cast<float>(1);
    }

    auto qvec = cross(tvec, ab);
    auto v = dot(ray.dir, qvec) * inv_det;
    if(v < 0 || u + v > 1) {
        return -static_cast<float>(1);
    }

    auto t = dot(ac, qvec) * inv_det;

    return t;
}

AABBArea Triangle::getBoundingVolume() const noexcept {
    return {min(min(this->a, this->b), this->c), max(max(this->a, this->b), this->c)};
}

float Triangle::getSurfaceArea() const noexcept {
    return cross(this->b - this->a, this->c - this->a).getLength() / 2.0F;
}

std::tuple<vec3<float>, float, bool> Triangle::sampleSurface(RandomEngine &re) const noexcept {
    std::uniform_real_distribution<float> dist(0, 1);

    auto r1 = dist(re);
    auto r2 = dist(re);

    auto rr1 = std::sqrt(r1);

    vec3<float> pos = this->a * (1.0F - rr1) + this->b * (rr1 * (1.0F - r2)) + this->c * (rr1 * r2);

    // TODO: Precompute
    auto area = cross(this->b - this->a, this->c - this->a).getLength() / 2.0F;
    auto p = 1.0F / area;

    return std::make_tuple(pos, p, this->cull_backface);
}
