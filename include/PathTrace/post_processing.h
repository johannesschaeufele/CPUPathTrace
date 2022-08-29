#ifndef PATHTRACE_POST_PROCESSING_H
#define PATHTRACE_POST_PROCESSING_H

#include <PathTrace/image/image.h>

/**
 * Maps an image with an arbitrary value range to an image with a value range of [0, 1] inplace
 * This function attempts to find a typically non-linear mapping of values monotonically increasing in brightness
 *  that yields good contrast and use of the available dynamic range in the output image.
 *
 * @param image The input image with an arbitrary (finite) value range
 */
void toneMap(Image<> &image);

/**
 * Inversely corrects an image for a gamma value that will later be applied inplace
 *
 * @param image Image to gamma correct
 * @param gamma Value of gamma to correct for
 */
void gammaCorrect(Image<> &image, float gamma = 1.8F);

/**
 * Maps a raw image with arbitrary finite value range to a final post-processed image by
 *  performing tone mapping followed by gamma correction inplace
 *
 * @param image The image to post-process
 */
void postProcess(Image<> &image);

#endif // PATHTRACE_POST_PROCESSING_H
