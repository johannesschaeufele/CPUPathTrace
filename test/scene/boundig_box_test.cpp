#include <PathTrace/scene/bounding_box.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

TEST(AABBTest, IntersectionTest) { // NOLINT
    const auto sphere = Sphere(vec3<float>(0.0F, 0.0F, 0.0F), 1.0F);

    const AABB aabb = AABB(sphere.getBoundingVolume(), std::make_unique<Sphere>(sphere));

    const float angled_dist = std::sqrt(2.0F) / 2.0F;

    for(int dim = 0; dim < 3; dim++) {
        const float x = dim == 0 ? 1.0F : 0.0F;
        const float y = dim == 1 ? 1.0F : 0.0F;
        const float z = dim == 2 ? 1.0F : 0.0F;

        for(int orientation = 0; orientation < 1; orientation++) {
            const float factor = orientation == 0 ? -1.0F : 1.0F;

            Ray ray_hit = Ray{vec3<float>(x, y, z) * factor * 5.0F, vec3<float>(x, y, z) * factor * (-1.0F)};
            EXPECT_THAT(aabb.getIntersection(ray_hit), testing::FloatEq(4.0F)) << "dim=" << dim << ", orientation=" << orientation;

            for(int dim2 = 0; dim2 < 3; dim2++) {
                if(dim == dim2) {
                    continue;
                }

                const float x2 = dim2 == 0 ? 1.0F : 0.0F;
                const float y2 = dim2 == 1 ? 1.0F : 0.0F;
                const float z2 = dim2 == 2 ? 1.0F : 0.0F;

                Ray ray_hit_angled = Ray{vec3<float>(x, y, z) * factor * 1.5F, (vec3<float>(x + x2, y + y2, z + z2) * factor * (-1.0F)).normalize()};
                EXPECT_THAT(aabb.getIntersection(ray_hit_angled), testing::FloatEq(angled_dist)) << "dim=" << dim << ", orientation=" << orientation;
            }

            Ray ray_inside = Ray{vec3<float>(x, y, z) * factor * 0.5F, vec3<float>(x, y, z) * factor * (-1.0F)};
            EXPECT_THAT(aabb.getIntersection(ray_inside), testing::FloatEq(0.0F)) << "dim=" << dim << ", orientation=" << orientation;

            Ray ray_miss1 = Ray{vec3<float>(x, y, z) * factor * 5.0F, vec3<float>(x, y, z) * factor};
            EXPECT_THAT(aabb.getIntersection(ray_miss1), testing::Lt(0.0F)) << "dim=" << dim << ", orientation=" << orientation;

            Ray ray_miss2 = Ray{vec3<float>(7.0F * x - 2.0F, 7.0F * y - 2.0F, 7.0F * z - 2.0F) * factor, vec3<float>(x, y, z) * factor * (-1.0F)};
            EXPECT_THAT(aabb.getIntersection(ray_miss2), testing::Lt(0.0F)) << "dim=" << dim << ", orientation=" << orientation;
        }
    }
}
