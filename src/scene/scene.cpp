#include <PathTrace/scene/scene.h>

#include <array>
#include <cmath>
#include <algorithm>
#include <limits>
#include <utility>
#include <cassert>
#include <random>

namespace impl {
    AABB constructBVH(std::vector<AABB> &&bounding_boxes) {
        if(bounding_boxes.empty()) {
            return {};
        }

        if(bounding_boxes.size() == 1) {
            return std::move(bounding_boxes[0]);
        }

        constexpr int dim_count = 3;

        // Determine median lower location (cutoff) in each dimension
        std::array<float, dim_count> medians;
        for(int dim = 0; dim < dim_count; dim++) {
            std::vector<float> min_coords;
            min_coords.reserve(bounding_boxes.size());

            for(const AABB &aabb : bounding_boxes) {
                min_coords.push_back(aabb.area.low[dim]);
            }
            auto median_location = min_coords.begin() + (static_cast<int>(min_coords.size()) / 2 - 1);
            std::nth_element(min_coords.begin(), median_location, min_coords.end());

            medians[dim] = *median_location;
        }

        // Calculate surface area for both bounding boxes using median cutoff in each dimension
        constexpr float inf = std::numeric_limits<float>::infinity();
        std::array<float, dim_count> surface_areas;
        for(int dim = 0; dim < dim_count; dim++) {
            std::array<vec3<float>, 2> combined_low;
            std::array<vec3<float>, 2> combined_high;

            std::fill(combined_low.begin(), combined_low.end(), vec3<float>{inf, inf, inf});
            std::fill(combined_high.begin(), combined_high.end(), vec3<float>{-inf, -inf, -inf});

            for(const AABB &aabb : bounding_boxes) {
                int index = aabb.area.low[dim] <= medians[dim] ? 0 : 1;

                combined_low[index] = min(combined_low[index], aabb.area.low);
                combined_high[index] = max(combined_high[index], aabb.area.high);
            }

            float surface_area = static_cast<float>(0);
            for(int index = 0; index < 2; index++) {
                auto d = combined_high[index] - combined_low[index];
                surface_area += 2 * (d[0] * d[1] + d[1] * d[2] + d[0] * d[2]);
            }

            surface_areas[dim] = surface_area;
        }

        // Choose the cutoff in the dimension that minimizes surface area
        int min_index = 0;
        float min_surface = surface_areas[0];
        for(int dim = 1; dim < dim_count; dim++) {
            if(surface_areas[dim] < min_surface) {
                min_surface = surface_areas[dim];
                min_index = dim;
            }
        }

        // Partition AABBs using chosen cutoff
        std::vector<AABB> left_children;
        left_children.reserve(bounding_boxes.size() / 2);
        std::vector<AABB> right_children;
        right_children.reserve((bounding_boxes.size() + 1) / 2);

        for(AABB &aabb : bounding_boxes) {
            if(aabb.area.low[min_index] <= medians[min_index]) {
                left_children.emplace_back(std::move(aabb));
            }
            else {
                right_children.emplace_back(std::move(aabb));
            }
        }

        // Ensure that left and right child count are roughly equal to prevent degeneracy
        while(left_children.size() > 1 && (left_children.size() > 2 * right_children.size())) {
            auto last_it = left_children.begin() + (static_cast<int>(left_children.size()) - 1);
            right_children.emplace_back(std::move(*last_it));
            left_children.erase(last_it);
        }

        AABB left_child = constructBVH(std::move(left_children));
        AABB right_child = constructBVH(std::move(right_children));

        AABB combined(std::move(left_child), std::move(right_child));

        return combined;
    }

    std::tuple<float, const Object *> getChildIntersection(const AABB &aabb, const Ray &ray, float t_max) {
        if(aabb.leaf) {
            auto child_t = aabb.child->getIntersection(ray);

            return std::make_tuple(child_t, aabb.child.get());
        }

        constexpr auto zero = static_cast<float>(0);

        auto left_t = aabb.left->getIntersection(ray);
        auto right_t = aabb.right->getIntersection(ray);
        assert(!std::isnan(left_t));
        assert(!std::isnan(right_t));

        auto close_t = std::min(left_t, right_t);
        auto far_t = std::max(left_t, right_t);
        const AABB &close = left_t < right_t ? *aabb.left : *aabb.right;
        const AABB &far = left_t < right_t ? *aabb.right : *aabb.left;

        std::tuple<float, const Object *> close_intersection = std::make_tuple(static_cast<float>(-1), nullptr);
        if(close_t >= zero && close_t < t_max) {
            close_intersection = getChildIntersection(close, ray, t_max);
        }

        auto close_intersection_t = std::get<0>(close_intersection);
        if(close_intersection_t >= zero) {
            if(close_intersection_t < far_t) {
                return close_intersection;
            }

            t_max = std::min(t_max, close_intersection_t);
        }

        if(far_t >= zero && far_t < t_max) {
            auto far_intersection = getChildIntersection(far, ray, t_max);

            auto far_intersection_t = std::get<0>(far_intersection);
            if(far_intersection_t < zero || (close_intersection_t >= zero && close_intersection_t < far_intersection_t)) {
                return close_intersection;
            }
            else {
                return far_intersection;
            }
        }

        return close_intersection;
    }
}

Scene::Scene(std::vector<std::unique_ptr<Object>> &&objects, std::vector<std::unique_ptr<LightSource>> &&light_sources) {
    this->light_sources = std::move(light_sources);

    std::vector<AABB> aabbs;
    aabbs.reserve(objects.size());
    for(std::unique_ptr<Object> &object : objects) {
        aabbs.emplace_back(object->getBoundingVolume(), std::move(object));
    }

    this->bounding_box = impl::constructBVH(std::move(aabbs));

    // Initialize object light sources
    this->registerEmissiveObjects(this->bounding_box);

    int emissive_object_count = static_cast<int>(this->object_light_source_probabilities.size());

    auto cumulative_probability = 0.0F;
    for(int i = 0; i < emissive_object_count; i++) {
        float probability = this->object_light_source_probabilities[i];

        this->object_light_source_probabilities[i] += cumulative_probability;
        cumulative_probability += probability;
    }

    // Normalize probabilities
    for(int i = 0; i < emissive_object_count; i++) {
        this->object_light_source_probabilities[i] /= cumulative_probability;
    }
}

void Scene::registerEmissiveObjects(const AABB &aabb) {
    if(aabb.leaf) {
        const Object *object = aabb.child.get();
        const auto *material = object->getMaterialHandler()->probeMaterial();

        auto emission = material->probeEmission();
        auto emission_color = emission.getColor();

        auto emissive_power = (emission_color[0] + emission_color[1] + emission_color[2]) * emission_color[3];
        if(emissive_power <= 0.0F) {
            return;
        }

        auto object_probability = emissive_power * object->getSurfaceArea();
        if(object_probability <= 0.0F) {
            return;
        }

        object_light_sources.push_back(object);
        object_light_source_probabilities.push_back(object_probability);
    }
    else {
        registerEmissiveObjects(*aabb.left);
        registerEmissiveObjects(*aabb.right);
    }
}

std::tuple<float, const Object *> Scene::getIntersection(const Ray &ray) const noexcept {
    auto t = this->bounding_box.getIntersection(ray);
    assert(!std::isnan(t));

    if(t >= static_cast<float>(0)) {
        return impl::getChildIntersection(this->bounding_box, ray, std::numeric_limits<float>::max());
    }
    else {
        return std::make_tuple(t, nullptr);
    }
}

std::vector<std::tuple<vec3<float>, Spectrum, float>> Scene::sampleLights(vec3<float> pos, vec3<float> /*n*/, RandomEngine &re) const noexcept {
    std::uniform_real_distribution<float> dist(0, 1);

    int emissive_object_count = static_cast<int>(this->object_light_sources.size());
    int object_sample_count = std::min(2 + static_cast<int>(std::log10(emissive_object_count + 1)), emissive_object_count);

    std::vector<std::tuple<vec3<float>, Spectrum, float>> lights;
    lights.reserve(this->light_sources.size() + object_sample_count);

    for(const std::unique_ptr<LightSource> &light : this->light_sources) {
        auto [target, pd] = light->importanceSample(pos);

        Ray ray{pos, (target - pos).normalize()};
        lights.emplace_back(target, light->getSpectrum(ray), pd);
    }

    for(int i = 0; i < object_sample_count; i++) {
        auto r = dist(re);

        auto it = std::lower_bound(this->object_light_source_probabilities.begin(), this->object_light_source_probabilities.end(), r);
        assert(it != this->object_light_source_probabilities.end());

        int object_index = static_cast<int>(it - this->object_light_source_probabilities.begin());
        assert(object_index >= 0);
        assert(object_index < this->object_light_sources.size());

        assert(this->object_light_source_probabilities[object_index] >= r);
        assert(object_index == 0 || this->object_light_source_probabilities[object_index - 1] < r);

        auto selection_p = this->object_light_source_probabilities[object_index];
        if(object_index > 0) {
            selection_p -= this->object_light_source_probabilities[object_index - 1];
        }
        selection_p *= float(object_sample_count);

        const auto *object = this->object_light_sources[object_index];

        auto [surface_pos, surface_p, surface_cull] = object->sampleSurface(re);
        auto surface_n = object->getSurfaceNormal(surface_pos);

        auto to_light = (surface_pos - pos);
        auto dir = to_light.normalize();

        auto abs_dot = std::abs(dot(-dir, surface_n));

        if(!(abs_dot > 0.0F)) {
            continue;
        }
        if(!(to_light.getLengthSquared() > 0.0F)) {
            continue;
        }
        if(surface_cull) {
            if(!(dot(dir, surface_n) < 0.0F)) {
                continue;
            }
        }

        // Conversion factor between ray direction pdf and surface point pdf
        auto conversion_factor = to_light.getLengthSquared() / abs_dot;

        const auto *material = object->getMaterialHandler()->getMaterial(surface_pos);

        Ray ray{pos, dir};
        lights.emplace_back(surface_pos, material->getEmission(ray, surface_pos), selection_p * surface_p * conversion_factor);
    }

    return lights;
}
