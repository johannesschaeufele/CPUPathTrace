#include "test_utils.h"

#include <random>

namespace test {

    Color<float> getColor(RandomEngine &re) {
        std::uniform_real_distribution<float> dist(0, 1);

        return Color<float>(dist(re), dist(re), dist(re), dist(re));
    }

    Image<> getTestImage(int width, int height, long seed) {
        Image<> image(width, height);

        RandomEngine random_engine(seed);

        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                image(x, y) = getColor(random_engine);
            }
        }

        return image;
    }

}
