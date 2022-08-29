#ifndef PATHTRACE_BOUNDING_BOX_H
#define PATHTRACE_BOUNDING_BOX_H

#include <PathTrace/base.h>
#include <PathTrace/scene/object.h>

#include <memory>

/**
 * POD struct Representing the geometry of a 3D AABB (axis-aligned bounding box)
 */
struct AABBArea {
    //! 3D vector containing the lower bound coordinates in all dimensions
    vec3<float> low;
    //! 3D vector containing the upper bound coordinates in all dimensions
    vec3<float> high;
};

/**
 * Represents a node of a bounding volume hierarchy of AABBs
 * Instances are either a leaf that contains an object or an inner node
 *  that has exactly two child nodes
 */
class AABB {
  public:
    AABBArea area;

    std::unique_ptr<AABB> left;
    std::unique_ptr<AABB> right;
    std::unique_ptr<Object> child;

    bool leaf;

    AABB();
    AABB(AABB &&other) noexcept;
    AABB &operator=(AABB &&other) noexcept;

    /**
     * Constructs an inner node given two child nodes
     * The newly constructed node will contain the smallest AABB area
     *  which contains the area of both children
     *
     * Here, left and right refer to the internal structure of the tree
     *  and are decoupled from the three-dimensional geometry of the AABBs
     *
     * @param left Left child node
     * @param right Right child node
     */
    AABB(AABB &&left, AABB &&right);

    /**
     * Constructs a leaf node, given an object and its bounding box
     *
     * @param area The bounding box of the object
     * @param child The object
     */
    AABB(AABBArea area, std::unique_ptr<Object> &&child) noexcept;

    /**
     * Intersect a ray with this node's bounding box and return the smallest distance
     *  along the ray that leads to a point inside the bounding box
     *
     * @param ray The ray to intersect with the bounding box
     * @return The smallest distance from the ray origin in the direction of the ray
     *  to a point in the bounding box, or a value less than 0 if there is no intersection
     */
    float getIntersection(const Ray &ray) const noexcept;
};

#endif /* PATHTRACE_BOUNDING_BOX_H */
