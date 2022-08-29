#ifndef PATHTRACE_SCENE_H
#define PATHTRACE_SCENE_H

#include <PathTrace/base.h>
#include <PathTrace/scene/object.h>
#include <PathTrace/scene/bounding_box.h>
#include <PathTrace/scene/light.h>

#include <utility>

/**
 * The Scene class represents and owns the geometrical description of a scene as well as light sources
 * Allows ray-object intersection and sampling of light sources including emissive geometry
 */
class Scene {
  private:
    std::vector<std::unique_ptr<LightSource>> light_sources;
    std::vector<const Object *> object_light_sources;
    std::vector<float> object_light_source_probabilities;
    AABB bounding_box;

  protected:
    void registerEmissiveObjects(const AABB &aabb);

  public:
    /**
     * Constructs a scene containing the given (potentially emissive) objects and light sources
     *
     * @param objects Objects making up the scene
     * @param light_sources Light sources in the scene (excluding emissive objects)
     */
    Scene(std::vector<std::unique_ptr<Object>> &&objects, std::vector<std::unique_ptr<LightSource>> &&light_sources);

    /**
     * Intersects a ray with the scene
     *
     * @param ray The ray to intersect the scene with
     * @return Tuple of the distance along the ray of the first intersection, or a negative value if there is no intersection,
     *  and a non-owning raw pointer to the first object hit, or nullptr if there is no intersection
     */
    std::tuple<float, const Object *> getIntersection(const Ray &ray) const noexcept;

    /**
     * Samples all light sources and emissive objects in the scene from a given position
     * The same light source may be sampled multiple times, and only a subset of all light sources in the scene may be sampled
     * The returned probability densities are already adjusted for the number of light sources sampled
     *
     * @param pos Position to sample the lights from
     * @param n Surface normal at the position to sample the lights from
     * @param re RandomEngine to generate random bits for sampling
     * @return A vector of tuples of the sampled position of a light source, along with the Spectrum it emits towards the given point,
     *  and the corresponding probability density
     */
    std::vector<std::tuple<vec3<float>, Spectrum, float>> sampleLights(vec3<float> pos, vec3<float> n, RandomEngine &re) const noexcept;
};

#endif /* PATHTRACE_SCENE_H */
