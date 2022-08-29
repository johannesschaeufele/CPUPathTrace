#ifndef PATHTRACE_WORKER_H
#define PATHTRACE_WORKER_H

#include <PathTrace/base.h>
#include <PathTrace/camera.h>
#include <PathTrace/scene/scene.h>
#include <PathTrace/image/image.h>

#include <functional>

/**
 * POD struct specifying various render options
 */
struct RenderOptions {
    //! Target width of the output image
    int image_width;
    //! Target height of the output image
    int image_height;
    //! Minimum sample count for each pixel
    int min_sample_count;
    //! Maximum sample count for each pixel
    int max_sample_count;

    //! Small epsilon distance value used to offset rays and compare distances
    //! Distances with with differences smaller than this value will be treated as equal
    float epsilon;

    //! Whether to allow bias when rendering in order to improve the perceived quality of an image
    //!  rendered with a smaller number of samples, such as by reducing noise and artifacts
    bool allow_bias = false;
};

/**
 * POD struct containing all information necessary to render an image
 */
struct FrameRenderJob {
    const Camera &camera;
    const Scene &scene;
    const RenderOptions &options;
};

/**
 * Struct describing a specific tile of an image to be rendered
 */
struct WorkItem {
    //! Non-owning raw pointer to the descriptor of the render target
    const FrameRenderJob *job;

    //! Tile-offset in the x-direction
    int offset_x;
    //! Tile-offset in the y-direction
    int offset_y;
    //! Width of the tile
    int width;
    //! Height of the tile
    int height;

    WorkItem() noexcept;
    WorkItem(const FrameRenderJob *job, int offset_x, int offset_y, int width, int height) noexcept;
};

/**
 * Processes a single WorkItem sequentially
 *
 * @param item the item
 * @param re Random engine to use for sampling, must be exclusive to this function and its thread while this function is being executed
 * @return rendered tile
 */
Image<> processItem(const WorkItem &item, RandomEngine &re);

/**
 * Renders FrameRenderJob in parallel
 *
 * @param job the job
 * @param progress_callback A callback to be called for each tile after rendering,
 *  with the number of finished tiles and total tiles as arguments.
 *  This callback may be called by any of the worked threads,
 *  but will not be called by more than one worked thread at a time.
 * @param worker_count The number of parallel workers to use.
 *  Will be set based on the number of logical system cores if the value is <= 0
 * @return rendered image
 */
Image<> processJob(
  const FrameRenderJob &job, const std::function<void(int, int)> &progress_callback = [](int, int) {}, int worker_count = 0);

#endif // PATHTRACE_WORKER_H
