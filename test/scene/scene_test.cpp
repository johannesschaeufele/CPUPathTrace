#include <PathTrace/scene/scene.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

TEST(SceneTest, IntersectionTest) { // NOLINT
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<LightSource>> light_sources;

    auto sphere1 = Sphere(vec3<float>(-1.0F, -1.0F, -1.0F), 1.0F);
    auto sphere2 = Sphere(vec3<float>(1.0F, 1.0F, 1.0F), 1.0F);

    objects.push_back(std::make_unique<Sphere>(sphere1));
    objects.push_back(std::make_unique<Sphere>(sphere2));

    Scene scene = Scene(std::move(objects), std::move(light_sources));

    {
        Ray ray = Ray{vec3<float>(-0.5F, -0.5F, -5.0F), vec3<float>(0.0F, 0.0F, 1.0F)};
        auto [t, intersected] = scene.getIntersection(ray);
        EXPECT_THAT(t, testing::Ge(0.0F));
        EXPECT_THAT(intersected, testing::NotNull());
        if(intersected) {
            EXPECT_THAT(intersected->getBoundingVolume().low, testing::Eq(sphere1.getBoundingVolume().low));
            EXPECT_THAT(intersected->getBoundingVolume().high, testing::Eq(sphere1.getBoundingVolume().high));
        }
    }

    {
        Ray ray = Ray{vec3<float>(0.5F, 0.5F, -5.0F), vec3<float>(0.0F, 0.0F, 1.0F)};
        auto [t, intersected] = scene.getIntersection(ray);
        EXPECT_THAT(t, testing::Ge(0.0F));
        EXPECT_THAT(intersected, testing::NotNull());
        if(intersected) {
            EXPECT_THAT(intersected->getBoundingVolume().low, testing::Eq(sphere2.getBoundingVolume().low));
            EXPECT_THAT(intersected->getBoundingVolume().high, testing::Eq(sphere2.getBoundingVolume().high));
        }
    }

    {
        Ray ray = Ray{vec3<float>(0.0F, 0.0F, 0.0F), vec3<float>(0.0F, 0.0F, 1.0F)};
        auto t = std::get<0>(scene.getIntersection(ray));
        EXPECT_THAT(t, testing::Lt(0.0F));
    }
}
