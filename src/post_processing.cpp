#include <PathTrace/post_processing.h>
#include <PathTrace/base.h>
#include <PathTrace/util/color.h>

#include <cassert>
#include <cmath>
#include <iterator>
#include <algorithm>
#include <vector>

template<typename T>
T gaussian(T t, T mu, T sigma) {
    constexpr T pi = static_cast<T>(M_PI);
    auto fac = static_cast<T>(1) / (std::sqrt(2 * pi));

    auto exponent_part = (t - mu) / (sigma);
    auto gaussian = fac * std::exp(-(exponent_part * exponent_part) / static_cast<T>(2)) / sigma;

    return gaussian;
}

template<typename T>
T getBrightness(Color<T> color) {
    return std::max({color[0], color[1], color[2]});
}

template<typename T>
T getBrightnessHeuristic(Color<T> color) {
    return color[3] * ((color[0] + color[1] + color[2]) / static_cast<T>(3) + std::max({color[0], color[1], color[2]})) / static_cast<T>(2);
}

void toneMap(Image<> &image) {
    // This entire function could be parallelized, though it typically only accounts for a small portion of a render's runtime

    float min_brightness = 0.0F;
    float max_brightness = 1E-4F;

    for(int y = 0; y < image.getHeight(); y++) {
        for(int x = 0; x < image.getWidth(); x++) {
            auto brightness = getBrightnessHeuristic(image(x, y));
            assertFinite(brightness);

            min_brightness = std::min(min_brightness, brightness);
            max_brightness = std::max(max_brightness, brightness);
        }
    }
    assert(max_brightness > min_brightness);

    // Sort brightness values by a single step of bucketing, following by a standard sort, and a final merge
    // Fully sorting is not strictly needed, but cheap and convenient enough to go for

    // Initialize bins
    const int pixel_count = image.getHeight() * image.getWidth();
    const int bin_count = std::min(1024, pixel_count);
    const float step = (max_brightness - min_brightness) / static_cast<float>(bin_count);
    std::vector<std::vector<float>> bins;
    bins.reserve(bin_count);
    for(int i = 0; i < bin_count; i++) {
        std::vector<float> bin;
        bin.reserve(pixel_count / bin_count);
        bins.emplace_back(std::move(bin));
    }

    // Fill bins
    for(int y = 0; y < image.getHeight(); y++) {
        for(int x = 0; x < image.getWidth(); x++) {
            auto brightness = getBrightnessHeuristic(image(x, y));
            assertFinite(brightness);

            int bin_index = std::min(static_cast<int>((brightness - min_brightness) / step), bin_count - 1);

            assert(bin_index >= 0);
            assert(bin_index < bin_count);

            bins[bin_index].push_back(brightness);
        }
    }

    // Sort bins
    for(auto &bin : bins) {
        std::sort(bin.begin(), bin.end());
    }

    // Merge bins
    std::vector<float> brightness_values;
    brightness_values.reserve(pixel_count);
    for(auto &bin : bins) {
        std::move(std::begin(bin), std::end(bin), std::back_inserter(brightness_values));
    }

    int brightness_segments = std::min(1024, pixel_count);
    std::vector<float> segment_weights;
    segment_weights.reserve(brightness_segments);
    auto segments_total_weight = 0.0F;
    for(int i = 0; i < brightness_segments; i++) {
        auto x = (static_cast<float>(i) + 0.5F) / static_cast<float>(brightness_segments);
        x = 2.0F * (x - 0.5F);

        auto segment_weight = 0.1F + gaussian(x, 0.0F, 0.3F);

        segment_weights.push_back(segment_weight);
        segments_total_weight += segment_weight;
    }

    std::vector<float> segment_ceilings;
    segment_ceilings.reserve(brightness_segments);

    int previous_index = 0;
    float missed_contribution = 0.0F;
    for(int i = 0; i < brightness_segments - 1; i++) {
        int segment_item_count =
          static_cast<int>(std::round(segment_weights[i] * static_cast<float>(pixel_count) / segments_total_weight + missed_contribution));

        if(segment_item_count > 0) {
            int brightness_index = std::min(previous_index + segment_item_count - 1, pixel_count - 1);
            assert(brightness_index >= 0);
            assert(brightness_index < pixel_count);
            segment_ceilings.push_back(brightness_values[brightness_index]);
            previous_index += segment_item_count;

            missed_contribution = 0.0F;
        }
        else {
            segment_ceilings.push_back(i > 0 ? segment_ceilings[i - 1] : min_brightness);
            missed_contribution += segment_weights[i] * static_cast<float>(pixel_count) / segments_total_weight;
        }
    }
    segment_ceilings.push_back(max_brightness);

    for(int y = 0; y < image.getHeight(); y++) {
        for(int x = 0; x < image.getWidth(); x++) {
            auto brightness = std::max(getBrightness(image(x, y)), std::numeric_limits<float>::min());
            assertFinite(brightness);
            assert(brightness != 0);
            auto brightness_heuristic = getBrightnessHeuristic(image(x, y));
            assertFinite(brightness_heuristic);

            auto it = std::lower_bound(segment_ceilings.begin(), segment_ceilings.end(), brightness_heuristic);
            assert(it != segment_ceilings.end());

            int segment_index = static_cast<int>(it - segment_ceilings.begin());
            auto segment_upper = segment_ceilings[segment_index];
            auto segment_lower = segment_index > 0 ? segment_ceilings[segment_index - 1] : min_brightness;
            auto segment_span = std::max(segment_upper - segment_lower, std::numeric_limits<float>::min());
            assert(brightness_heuristic >= segment_lower);
            assert(brightness_heuristic <= segment_upper);

            auto segment_value = (brightness_heuristic - segment_lower) / segment_span;

            auto mapped_upper = static_cast<float>(segment_index + 1) / static_cast<float>(brightness_segments);
            auto mapped_lower = static_cast<float>(segment_index) / static_cast<float>(brightness_segments);
            auto mapped_span = mapped_upper - mapped_lower;

            auto mapped_value = mapped_lower + segment_value * mapped_span;

            auto factor = mapped_value / brightness;

            image(x, y)[0] *= factor;
            image(x, y)[1] *= factor;
            image(x, y)[2] *= factor;
        }
    }
}

void gammaCorrect(Image<> &image, float gamma) {
    for(int y = 0; y < image.getHeight(); y++) {
        for(int x = 0; x < image.getWidth(); x++) {
            auto brightness = getBrightness(image(x, y));
            assertFinite(brightness);
            auto factor = std::pow(brightness, 1.0F / gamma - 1.0F);

            image(x, y)[0] *= factor;
            image(x, y)[1] *= factor;
            image(x, y)[2] *= factor;
        }
    }
}

void postProcess(Image<> &image) {
    toneMap(image);
    gammaCorrect(image);
}
