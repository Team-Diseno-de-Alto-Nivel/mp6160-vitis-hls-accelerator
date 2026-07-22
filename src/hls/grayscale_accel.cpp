#include "grayscale_accel.h"

void grayscale_accel(
    const unsigned char *rgb_in,
    unsigned char       *gray_out,
    uint32_t             num_pixels
) {
#pragma HLS INTERFACE m_axi     port=rgb_in     offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi     port=gray_out   offset=slave bundle=gmem1
#pragma HLS INTERFACE s_axilite port=rgb_in     bundle=control
#pragma HLS INTERFACE s_axilite port=gray_out   bundle=control
#pragma HLS INTERFACE s_axilite port=num_pixels bundle=control
#pragma HLS INTERFACE s_axilite port=return     bundle=control

    // TODO: implement the pipeline here, split into:
    //   - input stage:  burst-read RGB pixels from rgb_in
    //   - compute stage: RGB -> grayscale (see src/model/utils/conversion.h
    //     for the BT.601 reference formula used by the golden model)
    //   - output stage: burst-write grayscale pixels to gray_out
    // Requirement: stages must be clearly separated (e.g. hls::stream + #pragma HLS DATAFLOW).
}
