#include <PathTrace/scene/bounding_box.h>

#include <cassert>
#include <memory>
#include <limits>

namespace impl {
    AABBArea combineAreas(AABBArea a, AABBArea b) {
        return {min(a.low, b.low), max(a.high, b.high)};
    }
}

AABB::AABB() : child(std::make_unique<NullObject>()), leaf(true) {}

AABB::AABB(AABB &&other) noexcept :
  area(other.area), left(std::move(other.left)), right(std::move(other.right)), child(std::move(other.child)), leaf(other.leaf) {}

AABB::AABB(AABB &&left, AABB &&right) {
    this->area = impl::combineAreas(left.area, right.area);
    this->left = std::make_unique<AABB>(std::move(left));
    this->right = std::make_unique<AABB>(std::move(right));

    this->leaf = false;
}

AABB::AABB(AABBArea area, std::unique_ptr<Object> &&child) noexcept : area(area), child(std::move(child)), leaf(true) {}

AABB &AABB::operator=(AABB &&other) noexcept {
    this->area = other.area;
    this->left = std::move(other.left);
    this->right = std::move(other.right);
    this->child = std::move(other.child);
    this->leaf = other.leaf;

    return *this;
}

float AABB::getIntersection(const Ray &ray) const noexcept {
    assertNormalized(ray.dir);

    auto zero = static_cast<float>(0);

    float ix = std::abs(ray.dir[0]) > zero ? static_cast<float>(1) / ray.dir[0] : std::numeric_limits<float>::max();
    float iy = std::abs(ray.dir[1]) > zero ? static_cast<float>(1) / ray.dir[1] : std::numeric_limits<float>::max();
    float iz = std::abs(ray.dir[2]) > zero ? static_cast<float>(1) / ray.dir[2] : std::numeric_limits<float>::max();

    auto ld = this->area.low - ray.origin;
    auto hd = this->area.high - ray.origin;

    float t1 = ld[0] * ix;
    float t2 = hd[0] * ix;
    float t3 = ld[1] * iy;
    float t4 = hd[1] * iy;
    float t5 = ld[2] * iz;
    float t6 = hd[2] * iz;

    float t_min = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float t_max = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));
    assert(!std::isnan(t_min));
    assert(!std::isnan(t_max));

    float t = t_min;

    if(t_max < zero || t_min > t_max) {
        return -static_cast<float>(1);
    }

    if(t_min < zero && t_min <= t_max && t_max >= zero) {
        t = zero;
    }

    return t;
}
