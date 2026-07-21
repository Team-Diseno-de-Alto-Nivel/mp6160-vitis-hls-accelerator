// Co-simulation testbench skeleton for grayscale_accel.
// TODO: team fills in comparison against the golden model (see
// virtual-prototype/systemc-model/src/utils/conversion.h) and reports pass/fail.
// Co-simulation testbench for grayscale_accel

#include "../src/grayscale_accel.h"

// Golden model from the SystemC virtual prototype
#include "../../virtual-prototype/systemc-model/src/utils/conversion.h"

#include <cstdio>
#include <vector>
#include <cstdint>

int main(int argc, char **argv)
{
    const char *input_path =
        (argc > 1)
            ? argv[1]
            : "../../images/input/input_1080p.raw";

    const uint32_t width = 1920;
    const uint32_t height = 1080;
    const uint32_t num_pixels = width * height;

    std::vector<unsigned char> rgb_in(num_pixels * 3);
    std::vector<unsigned char> gray_out(num_pixels, 0);
    std::vector<unsigned char> golden(num_pixels, 0);

    //---------------------------------------------------------
    // Read input image
    //---------------------------------------------------------
    FILE *f = fopen(input_path, "rb");

    if (!f)
    {
        fprintf(stderr,
                "ERROR: could not open input image '%s'\n",
                input_path);
        return 1;
    }

    size_t read_bytes =
        fread(rgb_in.data(),
              1,
              rgb_in.size(),
              f);

    fclose(f);

    if (read_bytes != rgb_in.size())
    {
        fprintf(stderr,
                "ERROR: expected %zu bytes, got %zu\n",
                rgb_in.size(),
                read_bytes);
        return 1;
    }

    //---------------------------------------------------------
    // Run accelerator
    //---------------------------------------------------------
    grayscale_accel(
        rgb_in.data(),
        gray_out.data(),
        num_pixels);

    //---------------------------------------------------------
    // Compute golden model
    //---------------------------------------------------------
    for (uint32_t i = 0; i < num_pixels; i++)
    {
        uint8_t r = rgb_in[3 * i];
        uint8_t g = rgb_in[3 * i + 1];
        uint8_t b = rgb_in[3 * i + 2];

        golden[i] = rgb_to_gray(r, g, b);
    }

    //---------------------------------------------------------
    // Compare outputs
    //---------------------------------------------------------
    uint32_t errors = 0;

    for (uint32_t i = 0; i < num_pixels; i++)
    {
        if (gray_out[i] != golden[i])
        {
            errors++;

            if (errors <= 10)
            {
                uint8_t r = rgb_in[3 * i];
                uint8_t g = rgb_in[3 * i + 1];
                uint8_t b = rgb_in[3 * i + 2];

                printf(
                    "Mismatch at pixel %u\n"
                    "RGB      = (%u, %u, %u)\n"
                    "Expected = %u\n"
                    "Obtained = %u\n\n",
                    i,
                    r,
                    g,
                    b,
                    golden[i],
                    gray_out[i]);
            }
        }
    }

    //---------------------------------------------------------
    // Write output image
    //---------------------------------------------------------
    FILE *out =
        fopen("../../images/output/output_1080p.raw", "wb");

    if (!out)
    {
        fprintf(stderr,
                "WARNING: could not write output image.\n");
    }
    else
    {
        fwrite(
            gray_out.data(),
            1,
            gray_out.size(),
            out);

        fclose(out);
    }

    //---------------------------------------------------------
    // Final result
    //---------------------------------------------------------
    if (errors == 0)
    {
        printf("\n=====================================\n");
        printf("TEST PASSED\n");
        printf("Processed %u pixels successfully.\n",
               num_pixels);
        printf("Output image written to images/output/output_1080p.raw\n");
        printf("=====================================\n");

        return 0;
    }
    else
    {
        printf("\n=====================================\n");
        printf("TEST FAILED\n");
        printf("Total mismatches: %u\n", errors);
        printf("=====================================\n");

        return 1;
    }
}