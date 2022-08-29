#include <PathTrace/base.h>
#include <PathTrace/image/image.h>
#include <PathTrace/util/color.h>

namespace test {

    /**
     * Generates a random Color, with values in the range [0, 1]
     *
     * @param re RandomEngine to use for random bits
     * @return Randomly generated Color
     */
    Color<float> getColor(RandomEngine &re);

    /**
     * Generates a test image of the given dimensions, randomly populated with values
     *  based on the given seed
     *
     * @param width Target width of the image, should be greater than 0
     * @param height Target height of the image, should be greater than 0
     * @param seed Random seed used to populate the image with values
     * @return Randomly populated test image
     */
    Image<> getTestImage(int width = 256, int height = 128, long seed = 1234);

}
