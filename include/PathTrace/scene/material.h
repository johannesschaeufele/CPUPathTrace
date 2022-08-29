#ifndef PATHTRACE_MATERIAL_H
#define PATHTRACE_MATERIAL_H

#include <PathTrace/base.h>
#include <PathTrace/scene/light.h>
#include <PathTrace/util/vector.h>
#include <PathTrace/util/color.h>

/**
 * The virtual Material class encodes the volume or surface properties of objects, such as colors
 */
class Material {
  public:
    virtual ~Material() = default;

    /**
     * Returns the diffuse color of an object at a given position
     *
     * @param pos The surface position
     * @return Color that applies to diffuse reflection at the given surface position of the object
     */
    virtual Color<float> getDiffuseColor(vec3<float> pos) const noexcept = 0;

    /**
     * Returns the specular color of an object at a given position
     *
     * @param pos The surface position
     * @return Color that applies to specular reflection at the given surface position of the object
     */
    virtual Color<float> getSpecularColor(vec3<float> pos) const noexcept;

    /**
     * Returns the refractive index of the object at a given surface position
     *
     * @param pos The surface position
     * @return The refractive index at the given surface position of the object
     */
    virtual float getRefractiveIndex(vec3<float> pos) const noexcept;

    /**
     * Returns the emission of an object for a ray that hits the object at the given surface position
     *
     * @param ray Ray pointing towards the given surface position
     * @param pos Surface position that the ray will hit directly
     * @return Emission the specified ray collects from this object
     */
    virtual Spectrum getEmission(Ray ray, vec3<float> pos) const noexcept;

    /**
     * Heuristically probes the emission of the entire object
     * For non-emissive objects, this should return a completely dark spectrum.
     * For emissive objects, this must not return a completely dark spectrum,
     *  but should instead return an emission based on the average and/or maximum emission.
     *
     * @return Heuristic emission of this object
     */
    virtual Spectrum probeEmission() const noexcept;
};

/**
 * Class representing constant materials with constant properties, independent of the volume or surface position
 */
class ConstantMaterial final : public Material {
  private:
    Color<float> diffuse_color;
    float refractive_index;
    Spectrum emission;

  public:
    virtual ~ConstantMaterial() = default;
    ConstantMaterial(Color<float> diffuse_color = Color<float>(1.0F, 1.0F, 1.0F, 1.0F), float refractive_index = 1.0F, Spectrum emission = {}) noexcept;

    Color<float> getDiffuseColor(vec3<float> pos) const noexcept override;
    float getRefractiveIndex(vec3<float> pos) const noexcept override;
    Spectrum getEmission(Ray ray, vec3<float> pos) const noexcept override;
    Spectrum probeEmission() const noexcept override;
};

#endif // PATHTRACE_MATERIAL_H
