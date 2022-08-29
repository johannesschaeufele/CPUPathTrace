#include <PathTrace/image/image_io.h>
#include <PathTrace/image/image.h>

#include "test_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <random>
#include <sstream>

TEST(ImageIOTest, EncodeDecodeTest) { // NOLINT
    const auto test_image = test::getTestImage();

    // Encode image
    std::ostringstream ostream;
    io::writeRGBImage(ostream, test_image);

    // Decode image
    std::istringstream istream(ostream.str());
    const auto decoded_image = io::readRGBImage(istream);

    // Compare original and decoded image
    EXPECT_THAT(decoded_image.getWidth(), testing::Eq(test_image.getWidth()));
    EXPECT_THAT(decoded_image.getHeight(), testing::Eq(test_image.getHeight()));

    if((decoded_image.getWidth() == test_image.getWidth()) && (decoded_image.getHeight() == test_image.getHeight())) {
        // Note: Typically 256 different possible values per color channel are used,
        //  1 / 256 is just barely below 0.004, which is enough of a difference to account for floating point imprecision
        float color_eps = 0.004F;

        for(int y = 0; y < test_image.getHeight(); y++) {
            for(int x = 0; x < test_image.getWidth(); x++) {
                for(int c = 0; c < Color<float>::SIZE; c++) {
                    EXPECT_THAT(decoded_image(x, y)[c], testing::FloatNear(test_image(x, y)[c], color_eps)) << "(" << x << ", " << y << ")[" << c << "]";
                }
            }
        }
    }
}
