#ifndef PATHTRACE_OBJECT_H
#define PATHTRACE_OBJECT_H

#include <PathTrace/base.h>
#include <PathTrace/scene/propagation.h>

#include <memory>

/**
 * The virtual MaterialHandler class provides the non-geometric properties
 *  of an object, consisting of a Material and a BSDF, which determine
 *  how rays behave after intersecting the object's surface
 */
class MaterialHandler {
  public:
    virtual ~MaterialHandler() = default;

    /**
     * Probes for the typical material of the object, mainly to probe the emissiveness
     *  of the material
     * Should return an emissive material for objects that are at all emissive
     * See also the related Material::probeEmission
     *
     * @return Material with properties typical for the object
     */
    virtual const Material *probeMaterial() const noexcept;

    virtual const Material *getMaterial(vec3<float> pos) const noexcept = 0;
    virtual const BSDF *getBSDF(vec3<float> pos) const noexcept = 0;
};

/**
 * Class representing constant non-geometric properties, independent of the volume or surface position
 */
class ConstantMaterialHandler final : public MaterialHandler {
  private:
    std::shared_ptr<Material> material;
    std::shared_ptr<BSDF> bsdf;

  public:
    virtual ~ConstantMaterialHandler() = default;
    ConstantMaterialHandler(std::shared_ptr<Material> material, std::shared_ptr<BSDF> bsdf);

    const Material *probeMaterial() const noexcept override;
    const Material *getMaterial(vec3<float> pos) const noexcept override;
    const BSDF *getBSDF(vec3<float> pos) const noexcept override;
};

struct AABBArea;

/**
 * Objects represent 3D geometry with surfaces that can be intersected
 */
class Object {
  private:
    std::shared_ptr<MaterialHandler> material_handler;

  public:
    virtual ~Object() = default;
    Object();
    Object(std::shared_ptr<MaterialHandler> material_handler) noexcept;

    /**
     * Computes the distance along the ray of the first point
     *  along the ray intersecting this object
     *
     * @param ray The ray to intersect with this object
     * @return Distance along the ray to the first point intersecting the object,
     *  or a negative value if there is no intersection
     */
    virtual float getIntersection(const Ray &ray) const noexcept = 0;

    /**
     * Computes the surface normal of the object at a given point
     *
     * @param pos Point on the surface of the object
     * @return Normal vector in object coordinates of length 1
     */
    virtual vec3<float> getSurfaceNormal(vec3<float> pos) const noexcept = 0;
    const MaterialHandler *getMaterialHandler() const noexcept;
    void setMaterialHandler(std::shared_ptr<MaterialHandler> material_handler);

    /**
     * Computes a bounding volume fully containing the object
     *
     * @return Bounding volume that fully contains the object
     */
    virtual AABBArea getBoundingVolume() const noexcept = 0;

    /**
     * Computes the outside surface or "front face" area of the object, that is the area of the
     *  surface of the object, where the any ray pointing towards that surface
     *  lies in the same hemisphere as the normal vector for the surface
     *
     * @return Outside surface area of the object
     */
    virtual float getSurfaceArea() const noexcept;

    /**
     * Samples a point on the surface of the object
     *
     * @param re RandomEngine used to generate random bits used during sampling
     * @return Tuple of surface position, the corresponding probability density,
     *  and whether backface culling should be performed
     */
    virtual std::tuple<vec3<float>, float, bool> sampleSurface(RandomEngine &re) const noexcept;
};

/**
 * A null object with no surface area or volume
 */
class NullObject final : public Object {
  public:
    virtual ~NullObject() = default;
    NullObject() = default;

    float getIntersection(const Ray &ray) const noexcept override;
    vec3<float> getSurfaceNormal(vec3<float> pos) const noexcept override;
    AABBArea getBoundingVolume() const noexcept override;
    float getSurfaceArea() const noexcept override;
};

/**
 * A three-dimensional sphere
 */
class Sphere final : public Object {
  private:
    vec3<float> origin;
    float radius;
    float radius2;

  public:
    virtual ~Sphere() = default;
    Sphere(vec3<float> origin, float radius);

    float getIntersection(const Ray &ray) const noexcept override;

    vec3<float> getSurfaceNormal(vec3<float> pos) const noexcept override;
    AABBArea getBoundingVolume() const noexcept override;
    float getSurfaceArea() const noexcept override;
    std::tuple<vec3<float>, float, bool> sampleSurface(RandomEngine &re) const noexcept override;
};

/**
 * A three-dimensional triangle with interpolation via barycentric coordinates
 */
class Triangle final : public Object {
  public:
    vec3<float> a;
    vec3<float> b;
    vec3<float> c;
    vec3<float> normal_a;
    vec3<float> normal_b;
    vec3<float> normal_c;

  private:
    bool cull_backface;

  public:
    virtual ~Triangle() noexcept = default;
    Triangle(vec3<float> a, vec3<float> b, vec3<float> c, bool cull_backface = false);

    float getIntersection(const Ray &ray) const noexcept override;

    vec3<float> getSurfaceNormal(vec3<float> pos) const noexcept override;
    AABBArea getBoundingVolume() const noexcept override;
    float getSurfaceArea() const noexcept override;
    std::tuple<vec3<float>, float, bool> sampleSurface(RandomEngine &re) const noexcept override;
};

#endif /* PATHTRACE_OBJECT_H */
