#ifndef PATHTRACE_LIGHT_H
#define PATHTRACE_LIGHT_H

#include <PathTrace/base.h>
#include <PathTrace/util/color.h>

#include <utility>

/**
 * Represents a spectrum of light
 */
class Spectrum {
  private:
    Color<float> color;

  public:
    Spectrum(Color<float> color = {0.0F, 0.0F, 0.0F, 0.0F}) noexcept;

    Color<float> getColor() const noexcept;

    Spectrum operator+(Spectrum other) const noexcept;
    Spectrum operator*(Spectrum other) const noexcept;
    Spectrum operator*(float factor) const noexcept;
    Spectrum operator/(float divisor) const noexcept;
};

/**
 * Light sources in a scene emit a spectrum of light and can be sampled
 */
class LightSource {
  public:
    virtual ~LightSource() = default;

    /**
     * Importance samples a position on the surface of the light source
     *  from a given position on the surface of an object
     *
     * @param pos Surface position on the object
     * @return Tuple of the sampled position and the corresponding probability density
     */
    virtual std::tuple<vec3<float>, float> importanceSample(vec3<float> pos) const noexcept = 0;

    /**
     * Gets the emitted spectrum for a specified ray pointing towards the light source
     *
     * @param ray A ray from an arbitrary surface pointing towards the light source
     * @return The emitted spectrum
     */
    virtual Spectrum getSpectrum(Ray ray) const noexcept = 0;
};

/**
 * Point lights are single points emitting a constant spectrum in all directions
 */
class PointLightSource final : public LightSource {
  private:
    vec3<float> pos;
    Spectrum spectrum;

  public:
    virtual ~PointLightSource() = default;
    PointLightSource(vec3<float> pos, Spectrum spectrum) noexcept;

    std::tuple<vec3<float>, float> importanceSample(vec3<float> pos) const noexcept override;
    Spectrum getSpectrum(Ray ray) const noexcept override;
};

#endif /* PATHTRACE_LIGHT_H */
