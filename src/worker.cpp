#include <PathTrace/worker.h>

#include <cassert>
#include <cmath>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <queue>

namespace impl {

    float getContribution(Color<float> color) {
        return /* color[3] * */ (color[0] + color[1] + color[2]) / 3.0F;
    }

    float getContribution(Spectrum spectrum) {
        auto color = spectrum.getColor();

        return getContribution(color);
    }

    bool isNonNegative(Spectrum spectrum) {
        return isNonNegative(spectrum.getColor());
    }

    std::tuple<Spectrum, bool> getSample(const WorkItem &item, float x_camera, float y_camera, RandomEngine &re) {
        const auto pixel_width = 1.0F / static_cast<float>(item.job->options.image_width);
        const auto pixel_height = 1.0F / static_cast<float>(item.job->options.image_height);

        const auto epsilon = item.job->options.epsilon;

        std::uniform_real_distribution<float> dist(0, 1);

        Ray ray = item.job->camera.shootRay(x_camera, y_camera, pixel_width, pixel_height, re);
        assertNormalized(ray.dir);

        bool sample_collected = false;
        float contribution_unweighted = static_cast<float>(1);
        double sample_divisor = 1.0F;
        double sample_bounce_pd = 1.0;
        Spectrum sample_spectrum = {Color<float>(1.0F, 1.0F, 1.0F, 1.0F)};
        Spectrum out_spectrum;
        int path_length = 0;
        for(;;) {
            auto [t, object] = item.job->scene.getIntersection(ray);

            if(t < static_cast<float>(0)) {
                break;
            }
            path_length++;

            sample_collected = true;

            auto pos = ray.origin + ray.dir * t;
            auto n = object->getSurfaceNormal(pos);
            assertNormalized(n);

            const auto *material_handler = object->getMaterialHandler();
            const auto *material = material_handler->getMaterial(pos);
            const auto *bsdf = material_handler->getBSDF(pos);

            auto emission = material->getEmission(ray, pos);
            assert(sample_bounce_pd > 0.0);
            out_spectrum = out_spectrum + sample_spectrum * emission / static_cast<float>(sample_divisor * sample_bounce_pd);

            // TODO: Don't use raw contribution, use some relative measure instead
            float bounce_probability = path_length <= 4 ? 1.0F : 0.1F + 0.1F * std::min(contribution_unweighted * getContribution(sample_spectrum), 1.0F);
            assert(bounce_probability >= 0.0F && bounce_probability <= 1.0F);

            bool do_bounce = dist(re) < bounce_probability;
            bool sample_light_sources = true; // !do_bounce;

            if(sample_light_sources) {
                auto lights = item.job->scene.sampleLights(pos, n, re);

                for(const auto &[light_pos, light_spectrum, lpd] : lights) {
                    assert(lpd >= 0.0F);
                    assertNonNegative(light_spectrum);

                    auto to_light = (light_pos - pos);
                    auto light_dir = to_light.normalize();
                    Ray light_ray = {pos + light_dir * epsilon, light_dir};

                    float light_t = std::get<0>(item.job->scene.getIntersection(light_ray));

                    if(light_t < static_cast<float>(0) || (light_t >= to_light.getLength() - epsilon)) {
                        auto [base_spectrum, shading_factor, shadow_ray_pd] = bsdf->getSpectrum(ray, light_ray, pos, n, light_spectrum, material, true);
                        assert(shading_factor >= 0.0F && shading_factor <= 1.0F);
                        assert(shadow_ray_pd >= 0.0F);
                        assertNonNegative(base_spectrum);

                        if(shadow_ray_pd > static_cast<float>(0)) {
                            auto combined_spectrum = base_spectrum * shading_factor * sample_spectrum;
                            assertNonNegative(combined_spectrum);

                            assert(sample_bounce_pd >= 0.0);
                            auto weighed_spectrum = combined_spectrum / static_cast<float>(sample_divisor * sample_bounce_pd * lpd * shadow_ray_pd);
                            assertNonNegative(weighed_spectrum);

                            out_spectrum = out_spectrum + weighed_spectrum;
                        }
                    }
                }
            }

            if(!do_bounce) {
                sample_bounce_pd *= 1.0F - bounce_probability;
                break;
            }
            sample_bounce_pd *= bounce_probability;

            if(sample_bounce_pd <= 1E-20) {
                break;
            }

            // Generate next ray
            auto [next_ray, ray_factor, ray_pd] = bsdf->propagateRay(ray, pos, n, epsilon, re, material);
            assertNormalized(next_ray.dir);
            assert(ray_pd > 0.0F);

            sample_divisor *= ray_pd;
            sample_divisor /= ray_factor;
            contribution_unweighted *= ray_factor;

            auto [shaded_spectrum, shading_factor, shading_pd] = bsdf->getSpectrum(ray, next_ray, pos, n, sample_spectrum, material, false);
            assert(shading_pd > 0.0F);
            assert(shading_factor >= 0.0F && shading_factor <= 1.0F);
            sample_divisor *= shading_pd;
            sample_divisor /= shading_factor;
            contribution_unweighted *= shading_factor;
            sample_spectrum = shaded_spectrum;
            assertNonNegative(sample_spectrum);

            if(sample_divisor <= 1E-20) {
                break;
            }

            ray = next_ray;
        }

        auto out_color = out_spectrum.getColor();
        out_color[3] = sample_collected ? 1.0F : 0.0F;
        out_spectrum = {out_color};

        return std::make_tuple(out_spectrum, sample_collected);
    }
}

Image<> processItem(const WorkItem &item, RandomEngine &re) {
    using namespace impl;

    long total_collected = 0;

    Image<> image(item.width, item.height);

    const float one_half = static_cast<float>(1) / static_cast<float>(2);

    int stats_sample_count = std::min(std::max(item.job->options.min_sample_count / 4, 1), 64);
    int candidate_batch_count = std::max(std::max(item.job->options.min_sample_count, item.job->options.max_sample_count / 4) / stats_sample_count, 2);

    const int check_sample_count = std::min(std::max({item.job->options.min_sample_count / 2,
                                                      (item.job->options.max_sample_count - item.job->options.min_sample_count) / 8, 8, stats_sample_count}),
                                            1024) /
                                   stats_sample_count;

    for(int y = item.offset_y; y < item.offset_y + item.height; y++) {
        for(int x = item.offset_x; x < item.offset_x + item.width; x++) {
            float x_camera = 2 * ((static_cast<float>(x) + one_half) / static_cast<float>(item.job->options.image_width) - one_half);
            float y_camera = 2 * ((static_cast<float>(y) + one_half) / static_cast<float>(item.job->options.image_height) - one_half);
            y_camera = -y_camera;

            Color<float> pixel_value{};
            int collected_sample_count = 0;
            Color<float> previous_pixel_value{};

            Color<float> contribution_mean{};
            Color<float> contribution_m2{};
            int contribution_count = 0;

            int stats_sample_index = 0;
            Color<float> sample_aggregate{};

            std::vector<Color<float>> candidate_means;
            std::vector<Color<float>> candidate_m2s;
            std::vector<int> candidate_counts;

            Color<float> candidate_mean{};
            Color<float> candidate_m2{};
            int candidate_count = 0;

            int remaining_checks = check_sample_count;
            bool accepted_candidate = false;
            for(int pixel_sample = 0; pixel_sample < item.job->options.max_sample_count; pixel_sample++) {
                auto [out_spectrum, sample_collected] = getSample(item, x_camera, y_camera, re);

                if(sample_collected) {
                    assertNonNegative(out_spectrum);

                    auto color_contribution = out_spectrum.getColor();

                    contribution_count++;

                    stats_sample_index++;
                    sample_aggregate += color_contribution;

                    if(stats_sample_index == stats_sample_count) {
                        sample_aggregate /= static_cast<float>(stats_sample_count);

                        auto delta = sample_aggregate - contribution_mean;
                        contribution_mean += delta / static_cast<float>(contribution_count / stats_sample_count); // NOLINT(bugprone-integer-division)
                        auto delta2 = sample_aggregate - contribution_mean;
                        contribution_m2 += delta * delta2;

                        if(candidate_count == candidate_batch_count) {
                            candidate_means.push_back(candidate_mean);
                            candidate_m2s.push_back(candidate_m2);
                            candidate_counts.push_back(candidate_count);

                            candidate_mean = {};
                            candidate_m2 = {};
                            candidate_count = 0;
                        }

                        candidate_count++;
                        auto candidate_delta = sample_aggregate - candidate_mean;
                        candidate_mean += candidate_delta / static_cast<float>(candidate_count);
                        auto candidate_delta2 = sample_aggregate - candidate_mean;
                        candidate_m2 += candidate_delta * candidate_delta2;

                        stats_sample_index = 0;
                        sample_aggregate = {};
                    }

                    previous_pixel_value = pixel_value;
                    pixel_value = pixel_value + color_contribution;

                    collected_sample_count++;

                    if(stats_sample_index == 0 && collected_sample_count >= std::max(item.job->options.min_sample_count, 2)) {
                        bool passed_check = false;
                        if(contribution_count / stats_sample_count >= 2) {
                            auto m2_weighted =
                              contribution_m2 / static_cast<float>(contribution_count / stats_sample_count - 1); // NOLINT(bugprone-integer-division)
                            auto stddev = std::sqrt(m2_weighted[0] + m2_weighted[1] + m2_weighted[2]);
                            if(stddev < 1E-4F || stddev / (3 * 3 * getContribution(contribution_mean) + 1E-5) < 0.2F) {
                                passed_check = true;
                                remaining_checks--;

                                if(remaining_checks <= 0) {
                                    accepted_candidate = true;
                                    break;
                                }
                            }
                        }

                        if(!passed_check) {
                            remaining_checks = check_sample_count;
                        }
                    }
                }
            }

            if(collected_sample_count > 0) {
                pixel_value = pixel_value * (static_cast<float>(1) / static_cast<float>(collected_sample_count));
            }

            if(candidate_count > 0) {
                candidate_means.push_back(candidate_mean);
                candidate_m2s.push_back(candidate_m2);
                candidate_counts.push_back(candidate_count);
            }

            if(!accepted_candidate) {
                struct PixelCandidate {
                    Color<float> color;
                    float stddev;
                };

                std::vector<PixelCandidate> pixel_candidates;

                for(int i = 0; i < static_cast<int>(candidate_means.size()); i++) {
                    auto mean = candidate_means[i];
                    auto m2 = candidate_m2s[i];
                    auto count = candidate_counts[i];

                    if(count < std::max((candidate_batch_count * 3) / 4, 2)) {
                        continue;
                    }

                    auto m2_weighted = m2 / static_cast<float>(count);
                    auto stddev = std::sqrt(m2_weighted[0] + m2_weighted[1] + m2_weighted[2]);

                    pixel_candidates.push_back({mean, stddev});
                }

                if(!pixel_candidates.empty()) {
                    std::sort(pixel_candidates.begin(), pixel_candidates.end(),
                              [](const PixelCandidate &a, const PixelCandidate &b) -> bool { return a.stddev < b.stddev; });

                    pixel_value = pixel_candidates[0].color;
                    auto stddev = pixel_candidates[0].stddev;

                    for(int i = 1; i < static_cast<int>(pixel_candidates.size()); i++) {
                        auto stddev_other = pixel_candidates[i].stddev;
                        auto color_other = pixel_candidates[i].color;

                        // TODO: Respect difference in color and relative stddev
                        if(stddev_other < std::max(stddev + 0.005F, stddev * 1.01F)) {
                            pixel_value += (color_other - pixel_value) / static_cast<float>(i + 1);
                            stddev = stddev_other;
                        }
                        else {
                            break;
                        }
                    }
                }
            }

            image(x - item.offset_x, y - item.offset_y) = pixel_value;

            total_collected += collected_sample_count;
        }
    }

    return image;
}

void doWork(std::queue<WorkItem> &queue, Image<> &output_image, RandomEngine re, const std::function<void(int)> &progress_callback, std::mutex &mutex_queue,
            std::mutex &mutex_callback, std::atomic<int> &processed_tiles) {
    for(;;) {
        WorkItem item;

        // Note: Non-blocking parallel queue implementations are available
        //  The current solution performs sufficiently well for large enough tile sizes
        {
            std::lock_guard<std::mutex> lock(mutex_queue);

            if(queue.empty()) {
                break;
            }

            item = queue.front();
            queue.pop();
        }

        Image<> tile_image = processItem(item, re);

        for(int y = 0; y < item.height; y++) {
            for(int x = 0; x < item.width; x++) {
                output_image(item.offset_x + x, item.offset_y + y) = tile_image(x, y);
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex_callback);

            // Atomics are not strictly required here, due to lock being acquired
            // The lock should still be acquired here nonetheless, to prevent subsequent progress values from decreasing
            //  and to ensure the callback is never called from more than one thread at a time
            int previous_progress = processed_tiles.fetch_add(1, std::memory_order_relaxed);

            progress_callback(previous_progress + 1);
        }
    }
}

void doWorkParallel(std::queue<WorkItem> &queue, Image<> &output_image, const std::function<void(int)> &progress_callback, int worker_count = 0) {
    if(worker_count <= 0) {
        worker_count = static_cast<int>(std::thread::hardware_concurrency()) - 1;
    }

    std::random_device rd;
    RandomEngine re(rd());

    std::mutex mutex_queue;
    std::mutex mutex_callback;
    std::atomic<int> processed_tiles = 0;

    std::vector<std::thread> extra_threads;
    extra_threads.reserve(worker_count - 1);
    for(int i = 0; i < worker_count - 1; i++) {
        extra_threads.emplace_back(doWork, std::ref(queue), std::ref(output_image), RandomEngine{re()}, progress_callback, std::ref(mutex_queue),
                                   std::ref(mutex_callback), std::ref(processed_tiles));
    }
    doWork(queue, output_image, RandomEngine{re()}, progress_callback, std::ref(mutex_queue), std::ref(mutex_callback), std::ref(processed_tiles));

    for(auto &thread : extra_threads) {
        thread.join();
    }
}

Image<> processJob(const FrameRenderJob &job, const std::function<void(int, int)> &progress_callback, int worker_count) {
    auto width = std::max(job.options.image_width, 0);
    auto height = std::max(job.options.image_height, 0);

    Image<> output_image = Image(width, height);
    if(width == 0 || height == 0) {
        return output_image;
    }

    int tile_size = std::max(std::min(std::min(width, height) / 4, 32), 1);

    // Divide by tile_size, rounding up to next integer
    int horizontal_tiles = (width + (tile_size - 1)) / tile_size;
    int vertical_tiles = (height + (tile_size - 1)) / tile_size;

    std::queue<WorkItem> queue;
    for(int tile_y = 0; tile_y < vertical_tiles; tile_y++) {
        for(int tile_x = 0; tile_x < horizontal_tiles; tile_x++) {
            int offset_x = tile_x * tile_size;
            int offset_y = tile_y * tile_size;
            int tile_width = std::min(width - offset_x, tile_size);
            int tile_height = std::min(height - offset_y, tile_size);

            queue.emplace(&job, offset_x, offset_y, tile_width, tile_height);
        }
    }

    int total_tile_count = horizontal_tiles * vertical_tiles;
    const auto bound_progress_callback = [progress_callback, total_tile_count](int completed_tiles) {
        return progress_callback(completed_tiles, total_tile_count);
    };

    doWorkParallel(queue, output_image, bound_progress_callback, worker_count);

    return output_image;
}

WorkItem::WorkItem() noexcept : job(nullptr), offset_x(0), offset_y(0), width(0), height(0) {}

WorkItem::WorkItem(const FrameRenderJob *job, int offset_x, int offset_y, int width, int height) noexcept :
  job(job), offset_x(offset_x), offset_y(offset_y), width(width), height(height) {}
