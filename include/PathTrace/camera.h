#ifndef PATHTRACE_CAMERA_H
#define PATHTRACE_CAMERA_H

#include <PathTrace/base.h>

#include <memory>

/**
 * Abstract class representing an aperture shape that can be sampled uniformly
 */
class ApertureSampler {
  public:
    virtual ~ApertureSampler() = default;

    /**
     * Uniformly samples the aperture shape and returns a point in the range
     *  [-1, 1] x [-1, 1] that is within the aperture shape
     *
     * @param re Random engine to generate random bits used for sampling
     * @return A tuple of the sampled x and y position
     */
    virtual std::tuple<float, float> sampleAperture(RandomEngine &re) const noexcept = 0;
};

/**
 * Class representing a sampleable aperture with circular shape
 */
class CircularApertureSampler final : public ApertureSampler {
  public:
    virtual ~CircularApertureSampler() = default;
    std::tuple<float, float> sampleAperture(RandomEngine &re) const noexcept override;
};

/**
 * Class representing a sampleable aperture with hexagonal shape
 */
class HexagonalApertureSampler final : public ApertureSampler {
  private:
    float horizontal_ratio;

  public:
    virtual ~HexagonalApertureSampler() = default;

    /**
     * Constructs a hexagonal aperture sample for a hexagon with the specified horizontal ratio
     * The hexagon will have two equal length flat segments at the top and bottom,
     *  and two equally sized and angled segments each to the left and right.
     * The horizontal ratio is the proportion of the length of the flat segments to the top and bottom
     *  in relation to the total width.
     *
     * @param horizontal_ratio Value in range [0, 1] representing
     *  the ratio of the horizontal part of the hexagon
     */
    HexagonalApertureSampler(float horizontal_ratio) noexcept;

    std::tuple<float, float> sampleAperture(RandomEngine &re) const noexcept override;
};

/**
 * The camera class contains information on how a scene is viewed
 * This implementation represents a 3D perspective camera
 *  with configurable aperture and an optional thin lens
 *
 * It is used to generate the rays originating from the scene and eventually
 * falling onto the (virtual) image sensors
 */
class Camera {
  private:
    vec3<float> origin;
    vec3<float> forward;
    vec3<float> up;
    vec3<float> right;

    float aperture_width_half;
    float aperture_height_half;
    std::unique_ptr<ApertureSampler> aperture_sampler;

    float focal_plane_dist;

  public:
    /**
     * Constructs a pinhole camera given a look-at point and other camera parameters
     *
     * @param origin Origin of the camera. This point lies on the middle of the virtual image sensor
     * @param look_at Point that the camera will be directly pointed at, this will be used to determine
     *  the "forward" direction or optical axis of the camera, should be distinct from the origin
     * @param up "Up" direction of the camera, typically perpendicular to the optical axis, should be non-zero
     * @param focal_length Focal length: distance between the image sensor and aperture, should be greater than 0
     * @param height Length of the virtual image sensor in the "up" direction, should be non-zero
     * @param aspect_ratio Aspect ratio of the image sensor expressed as width / height, should be non-zero
     */
    Camera(vec3<float> origin, vec3<float> look_at, vec3<float> up, float focal_length, float height, float aspect_ratio) noexcept;

    /**
     * Constructs a camera with an aperture and a thin lens given a look-at point and other camera parameters
     * @param origin Origin of the camera. This point lies on the middle of the virtual image sensor
     * @param look_at Point that the camera will be directly pointed at, this will be used to determine
     *  the "forward" direction or optical axis of the camera, should be distinct from the origin
     * @param up "Up" direction of the camera, typically perpendicular to the optical axis, should be non-zero
     * @param focal_length Focal length: distance between the image sensor and aperture, should be greater than 0
     * @param height Length of the virtual image sensor in the "up" direction, should be non-zero
     * @param aspect_ratio Aspect ratio of the image sensor expressed as width / height, should be non-zero
     * @param aperture_width Relative extent of the aperture in the "right" direction
     * @param aperture_height Relative extent of the aperture in the "up" direction
     * @param aperture_sampler ApertureSampler defining the shape of the aperture
     * @param focal_plane_dist Distance of the focal plane for the thin lens, setting this to a value <= 0 will disable the thin lens
     */
    Camera(vec3<float> origin, vec3<float> look_at, vec3<float> up, float focal_length, float height, float aspect_ratio, float aperture_width,
           float aperture_height, std::unique_ptr<ApertureSampler> &&aperture_sampler, float focal_plane_dist = 0.0F) noexcept;

    /**
     * Shoots a ray from the camera aperture out into the scene that contributes to the light measured
     *  for the pixel of the virtual image sensor with the specified coordinates and extent
     *
     * @param x Center coordinate value in the camera "right" direction, should be in range [-1, 1]
     * @param y Center coordinate value in the camera "up" direction, should be in range [-1, 1]
     * @param pixel_width Extent of the pixel on the image sensor in the "right direction"
     * @param pixel_height Extent of the pixel on the image sensor in the "up direction"
     * @param re RandomEngine used to generate random bits
     * @return A ray originating at the camera aperture, pointing out into the scene, that
     *  contributes to the light measured by the virtual image sensor at the specified coordinates
     */
    Ray shootRay(float x, float y, float pixel_width, float pixel_height, RandomEngine &re) const noexcept;
};

#endif /* PATHTRACE_CAMERA_H */
