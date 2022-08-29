#ifndef PATHTRACE_PROPAGATION_H
#define PATHTRACE_PROPAGATION_H

#include <PathTrace/base.h>
#include <PathTrace/scene/light.h>
#include <PathTrace/scene/material.h>

#include <memory>

/**
 * BSDFs (bidirectional scattering distribution functions) allow sampling
 *  outgoing rays of light from an object given rays of incoming light
 *
 * This includes reflection, transmission, and subsurface scattering
 */
class BSDF {
  public:
    virtual ~BSDF() = default;

    /**
     * Samples the BSDF to obtain an outgoing ray from an incoming ray (or vice versa), where the incoming ray
     *  intersects an object at the given position, where the object has this BSDF
     *
     * @param ray The incoming ray that intersects the object at the given position
     * @param pos Point on the surface at the object that the ray intersects
     * @param normal Surface normal of the object at the intersected position
     * @param epsilon Epsilon value used to offset the outgoing ray origin away from the object surface
     * @param re RandomEngine to generate random bits
     * @param material Material of the object at the intersected surface point
     * @return Tuple of the outgoing ray, a factor to apply to the contribution of radiance transported along the rays,
     *  and corresponding probability density
     */
    virtual std::tuple<Ray, float, float> propagateRay(Ray ray, vec3<float> pos, vec3<float> normal, float epsilon, RandomEngine &re,
                                                       const Material *material) const noexcept = 0;

    /**
     * Determines the incoming spectrum from an outgoing spectrum, where an incoming ray
     *  intersects an object at the given position, where the object has this BSDF
     *
     * @param from_camera Incoming ray
     * @param to_light Outgoing ray
     * @param pos Point on the surface at the object that the ray intersects
     * @param normal Surface normal of the object at the intersected position
     * @param light_spectrum Outgoing spectrum
     * @param material Material of the object at the intersected surface point
     * @param synthetic False if the incoming and outgoing rays were generated used propagateRay, True otherwise
     * @return Tuple of the incoming spectrum, a shading factor, and the probability density of the pair of the incoming and outgoing ray occurring
     */
    virtual std::tuple<Spectrum, float, float> getSpectrum(Ray from_camera, Ray to_light, vec3<float> pos, vec3<float> normal, Spectrum light_spectrum,
                                                           const Material *material, bool synthetic = false) const noexcept = 0;
};

/**
 * The lambertian BRDF diffusely reflects light equally in all directions
 * It follows the cosine law
 */
class LambertianBRDF : public BSDF {
  public:
    virtual ~LambertianBRDF() = default;
    LambertianBRDF() noexcept;

    std::tuple<Ray, float, float> propagateRay(Ray ray, vec3<float> pos, vec3<float> normal, float epsilon, RandomEngine &re,
                                               const Material *material) const noexcept override;
    std::tuple<Spectrum, float, float> getSpectrum(Ray from_camera, Ray to_light, vec3<float> pos, vec3<float> normal, Spectrum light_spectrum,
                                                   const Material *material, bool synthetic = false) const noexcept override;
};

/**
 * The glass BDF specularly reflects and refractively transmits light according to the fresnel equations
 */
class GlassBDF : public BSDF {
  public:
    virtual ~GlassBDF() = default;
    GlassBDF() noexcept;

    std::tuple<Ray, float, float> propagateRay(Ray ray, vec3<float> pos, vec3<float> normal, float epsilon, RandomEngine &re,
                                               const Material *material) const noexcept override;
    std::tuple<Spectrum, float, float> getSpectrum(Ray from_camera, Ray to_light, vec3<float> pos, vec3<float> normal, Spectrum light_spectrum,
                                                   const Material *material, bool synthetic = false) const noexcept override;
};

/**
 * The mirror BRDF perfectly reflects all light that hits the surface
 */
class MirrorBRDF : public BSDF {
  private:
    bool one_way;

  public:
    virtual ~MirrorBRDF() = default;

    /**
     * Constructs a mirror BRDF
     *
     * @param one_way True if light should pass through the back face of mirror surface, False if they should also instead be reflective
     */
    MirrorBRDF(bool one_way = false) noexcept;

    std::tuple<Ray, float, float> propagateRay(Ray ray, vec3<float> pos, vec3<float> normal, float epsilon, RandomEngine &re,
                                               const Material *material) const noexcept override;
    std::tuple<Spectrum, float, float> getSpectrum(Ray from_camera, Ray to_light, vec3<float> pos, vec3<float> normal, Spectrum light_spectrum,
                                                   const Material *material, bool synthetic = false) const noexcept override;
};

/**
 * A class that allows the additive combination of several BSDFs
 *
 * @tparam Ts BSDF types
 */
template<class... Ts>
class CombinedBSDF : public BSDF {
  private:
    std::tuple<Ts...> components;
    std::array<float, sizeof...(Ts)> weights;
    std::array<float, sizeof...(Ts)> probabilities;

  public:
    virtual ~CombinedBSDF() = default;

    /**
     * Constructs a CombinedBSDF from several individual BSDFs along with corresponding weights
     *
     * @param components BSDFs to combine
     * @param weights Weights of the individual BSDFs
     */
    CombinedBSDF(Ts... components, std::array<float, sizeof...(Ts)> weights) noexcept;

    std::tuple<Ray, float, float> propagateRay(Ray ray, vec3<float> pos, vec3<float> normal, float epsilon, RandomEngine &re,
                                               const Material *material) const noexcept override;
    std::tuple<Spectrum, float, float> getSpectrum(Ray from_camera, Ray to_light, vec3<float> pos, vec3<float> normal, Spectrum light_spectrum,
                                                   const Material *material, bool synthetic = false) const noexcept override;
};

#endif /* PATHTRACE_MATERIAL_H */
