#include <PathTrace/scene/material.h>

float Material::getRefractiveIndex(vec3<float> /*pos*/) const noexcept {
    return 1.0F;
}

Spectrum Material::getEmission(Ray /*ray*/, vec3<float> /*pos*/) const noexcept {
    return {};
}

Spectrum Material::probeEmission() const noexcept {
    return {};
}

Color<float> Material::getSpecularColor(vec3<float> /*pos*/) const noexcept {
    return Color<float>{1.0F, 1.0F, 1.0F, 1.0F};
}

ConstantMaterial::ConstantMaterial(Color<float> diffuse_color, float refractive_index, Spectrum emission) noexcept :
  diffuse_color(diffuse_color), refractive_index(refractive_index), emission(emission) {}

Color<float> ConstantMaterial::getDiffuseColor(vec3<float> /*pos*/) const noexcept {
    return this->diffuse_color;
}

float ConstantMaterial::getRefractiveIndex(vec3<float> /*pos*/) const noexcept {
    return this->refractive_index;
}

Spectrum ConstantMaterial::getEmission(Ray /*ray*/, vec3<float> /*pos*/) const noexcept {
    return this->emission;
}

Spectrum ConstantMaterial::probeEmission() const noexcept {
    return this->emission;
}
