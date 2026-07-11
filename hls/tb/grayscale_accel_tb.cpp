// Co-simulation testbench skeleton for grayscale_accel.
// TODO: team fills in comparison against the golden model (see
// virtual-prototype/systemc-model/src/utils/conversion.h) and reports pass/fail.

#include "../src/grayscale_accel.h"
#include <cstdio>
#include <vector>

int main(int argc, char **argv) {
    const char *input_path = (argc > 1) ? argv[1] : "../../images/input/input_1080p.raw";
    const uint32_t width = 1920, height = 1080;
    const uint32_t num_pixels = width * height;

    std::vector<unsigned char> rgb_in(num_pixels * 3);
    std::vector<unsigned char> gray_out(num_pixels, 0);

    FILE *f = fopen(input_path, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: could not open input image '%s'\n", input_path);
        return 1;
    }
    size_t read_bytes = fread(rgb_in.data(), 1, rgb_in.size(), f);
    fclose(f);
    if (read_bytes != rgb_in.size()) {
        fprintf(stderr, "ERROR: expected %zu bytes, got %zu\n", rgb_in.size(), read_bytes);
        return 1;
    }

    grayscale_accel(rgb_in.data(), gray_out.data(), num_pixels);

    // TODO: compare gray_out against the golden model pixel by pixel,
    // write gray_out to images/output/, and return non-zero on mismatch.

    printf("grayscale_accel_tb: ran on %u pixels (validation TODO)\n", num_pixels);
    return 0;
}
