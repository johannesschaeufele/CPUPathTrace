#include <PathTrace/scene/light.h>

Spectrum::Spectrum(Color<float> color) noexcept : color(color) {}

Color<float> Spectrum::getColor() const noexcept {
    return this->color;
}

Spectrum Spectrum::operator+(Spectrum other) const noexcept {
    Color<float> combined_color = this->color + other.color;

    return {combined_color};
}

Spectrum Spectrum::operator*(Spectrum other) const noexcept {
    Color<float> combined_color = this->color * other.color;

    return {combined_color};
}

Spectrum Spectrum::operator*(float factor) const noexcept {
    Color<float> multiplied_color = this->color * factor;

    return {multiplied_color};
}

Spectrum Spectrum::operator/(float divisor) const noexcept {
    Color<float> divided_color = this->color / divisor;

    return {divided_color};
}

PointLightSource::PointLightSource(vec3<float> pos, Spectrum spectrum) noexcept : pos(pos), spectrum(spectrum) {}

std::tuple<vec3<float>, float> PointLightSource::importanceSample(vec3<float> /*pos*/) const noexcept {
    return std::make_tuple(this->pos, 1.0F);
}

Spectrum PointLightSource::getSpectrum(Ray /*ray*/) const noexcept {
    return this->spectrum;
}
