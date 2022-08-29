#include <PathTrace/post_processing.h>
#include <PathTrace/image/image.h>

#include "test_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>

TEST(PostProcessingTest, ToneMapTest) { // NOLINT
    const auto image = test::getTestImage();

    auto image_copy = image;
    toneMap(image_copy);

    EXPECT_THAT(image_copy.getWidth(), testing::Eq(image.getWidth()));
    EXPECT_THAT(image_copy.getHeight(), testing::Eq(image.getHeight()));
}

TEST(PostProcessingTest, GammaTest) { // NOLINT
    const auto image = test::getTestImage();

    std::array<float, 3> gammas{1.0F, 0.1F, 2.0F};

    for(const auto gamma : gammas) {
        auto image_copy = image;
        gammaCorrect(image_copy, gamma);

        EXPECT_THAT(image_copy.getWidth(), testing::Eq(image.getWidth()));
        EXPECT_THAT(image_copy.getHeight(), testing::Eq(image.getHeight()));

        if((image_copy.getWidth() == image.getWidth()) && (image_copy.getHeight() == image.getHeight())) {
            if(gamma == 1.0F) {
                for(int y = 0; y < image.getHeight(); y++) {
                    for(int x = 0; x < image.getWidth(); x++) {
                        for(int c = 0; c < Color<float>::SIZE; c++) {
                            EXPECT_THAT(image_copy(x, y)[c], testing::FloatEq(image(x, y)[c])) << "(" << x << ", " << y << ")[" << c << "]";
                        }
                    }
                }
            }
        }
    }
}

TEST(PostProcessingTest, PostProcessTest) { // NOLINT
    const auto image = test::getTestImage();

    auto image_copy = image;
    postProcess(image_copy);

    EXPECT_THAT(image_copy.getWidth(), testing::Eq(image.getWidth()));
    EXPECT_THAT(image_copy.getHeight(), testing::Eq(image.getHeight()));
}
