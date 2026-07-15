/*
 * ARM64 driver program, run by the simulated core in gem5.
 *
 * Mirrors the flow implemented by CPU::run() in
 * ../../systemc-model/src/cpu/cpu.cpp (load -> store -> configure ->
 * poll -> fetch -> save), but here it is real software issuing loads/stores
 * through /dev/mem to the accelerator's AXI4-Lite control registers and to
 * the DRAM regions holding the image, instead of SystemC TLM calls.
 *
 * Memory map (must match virtual-prototype/gem5/configs/kv260_arm64.py and
 * README.md "GEM5 MMIO map"):
 *
 *   0x10000000            Accelerator AXI4-Lite control registers
 *     +0x00 CTRL           bit0 ap_start, bit1 ap_done
 *     +0x10 RGB_IN_ADDR    physical address of the input RGB buffer
 *     +0x18 GRAY_OUT_ADDR  physical address of the output grayscale buffer
 *     +0x20 NUM_PIXELS     total pixel count
 *   0x80000000            Input RGB image buffer  (DRAM)
 *   0x80600000            Output grayscale buffer (DRAM)
 *
 * Requires root (or CAP_SYS_RAWIO) to mmap /dev/mem.
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define WIDTH               1920u
#define HEIGHT              1080u
#define PIXEL_COUNT         (WIDTH * HEIGHT)
#define INPUT_IMAGE_BYTES   (PIXEL_COUNT * 3u)
#define OUTPUT_IMAGE_BYTES  (PIXEL_COUNT)

#define ACCEL_BASE_ADDR     0x10000000ULL
#define ACCEL_REGION_SIZE   0x1000u /* one page is enough for the control block */
#define ACCEL_CTRL_OFFSET   0x00u
#define ACCEL_SRC_OFFSET    0x10u
#define ACCEL_DST_OFFSET    0x18u
#define ACCEL_NPIX_OFFSET   0x20u
#define ACCEL_CTRL_AP_START (1u << 0)
#define ACCEL_CTRL_AP_DONE  (1u << 1)

#define INPUT_RAM_ADDR      0x80000000ULL
#define OUTPUT_RAM_ADDR     0x80600000ULL

#define DEFAULT_INPUT_PATH  "images/input/input_1080p.raw"
#define DEFAULT_OUTPUT_PATH "images/output/output_1080p.raw"

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static volatile uint32_t *map_phys(int mem_fd, uint64_t phys_addr, size_t size) {
    void *m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (off_t)phys_addr);
    if (m == MAP_FAILED) {
        die("mmap");
    }
    return (volatile uint32_t *)m;
}

static void load_image_from_disk(const char *path, unsigned char *buffer, size_t size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        die("fopen (input image)");
    }
    if (fread(buffer, 1, size, f) != size) {
        fprintf(stderr, "driver: input image is smaller than expected (%zu bytes)\n", size);
        exit(1);
    }
    fclose(f);
    printf("driver: loaded %zu bytes from %s\n", size, path);
}

static void save_result_to_disk(const char *path, const unsigned char *buffer, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        die("fopen (output image)");
    }
    if (fwrite(buffer, 1, size, f) != size) {
        die("fwrite (output image)");
    }
    fclose(f);
    printf("driver: saved %zu bytes to %s\n", size, path);
}

static void store_image_to_ram(volatile uint32_t *ram_in, const unsigned char *buffer, size_t size) {
    memcpy((void *)ram_in, buffer, size);
}

static void read_result_from_ram(unsigned char *buffer, volatile uint32_t *ram_out, size_t size) {
    memcpy(buffer, (const void *)ram_out, size);
}

static void configure_accelerator(volatile uint32_t *ctrl, uint64_t src_addr, uint64_t dst_addr, uint32_t num_pixels) {
    *(volatile uint32_t *)((uint8_t *)ctrl + ACCEL_SRC_OFFSET)  = (uint32_t)src_addr;
    *(volatile uint32_t *)((uint8_t *)ctrl + ACCEL_DST_OFFSET)  = (uint32_t)dst_addr;
    *(volatile uint32_t *)((uint8_t *)ctrl + ACCEL_NPIX_OFFSET) = num_pixels;
    printf("driver: configured accelerator (src=0x%08llx, dst=0x%08llx, pixels=%u)\n",
           (unsigned long long)src_addr, (unsigned long long)dst_addr, num_pixels);
}

static void start_accelerator(volatile uint32_t *ctrl) {
    *(volatile uint32_t *)((uint8_t *)ctrl + ACCEL_CTRL_OFFSET) = ACCEL_CTRL_AP_START;
}

static void wait_accelerator_done(volatile uint32_t *ctrl) {
    volatile uint32_t *ctrl_reg = (volatile uint32_t *)((uint8_t *)ctrl + ACCEL_CTRL_OFFSET);
    printf("driver: waiting for accelerator (ap_done)...\n");
    while ((*ctrl_reg & ACCEL_CTRL_AP_DONE) == 0) {
        usleep(1000);
    }
    printf("driver: accelerator done\n");
}

int main(int argc, char **argv) {
    const char *input_path  = (argc > 1) ? argv[1] : DEFAULT_INPUT_PATH;
    const char *output_path = (argc > 2) ? argv[2] : DEFAULT_OUTPUT_PATH;

    unsigned char *input_image  = malloc(INPUT_IMAGE_BYTES);
    unsigned char *output_image = malloc(OUTPUT_IMAGE_BYTES);
    if (!input_image || !output_image) {
        die("malloc");
    }

    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        die("open /dev/mem (are you root?)");
    }

    volatile uint32_t *accel_ctrl = map_phys(mem_fd, ACCEL_BASE_ADDR, ACCEL_REGION_SIZE);
    volatile uint32_t *ram_in     = map_phys(mem_fd, INPUT_RAM_ADDR, INPUT_IMAGE_BYTES);
    volatile uint32_t *ram_out    = map_phys(mem_fd, OUTPUT_RAM_ADDR, OUTPUT_IMAGE_BYTES);

    /* 1. Load RGB RAW image from the disk. */
    load_image_from_disk(input_path, input_image, INPUT_IMAGE_BYTES);

    /* 2. Store the image in RAM. */
    store_image_to_ram(ram_in, input_image, INPUT_IMAGE_BYTES);

    /* 3. Configure the accelerator: source, destination, pixel count. */
    configure_accelerator(accel_ctrl, INPUT_RAM_ADDR, OUTPUT_RAM_ADDR, PIXEL_COUNT);

    /* 4. Start it and wait for completion. */
    start_accelerator(accel_ctrl);
    wait_accelerator_done(accel_ctrl);

    /* 5. Read the processed image back from RAM. */
    read_result_from_ram(output_image, ram_out, OUTPUT_IMAGE_BYTES);

    /* 6. Save the result to disk. */
    save_result_to_disk(output_path, output_image, OUTPUT_IMAGE_BYTES);

    munmap((void *)accel_ctrl, ACCEL_REGION_SIZE);
    munmap((void *)ram_in, INPUT_IMAGE_BYTES);
    munmap((void *)ram_out, OUTPUT_IMAGE_BYTES);
    close(mem_fd);
    free(input_image);
    free(output_image);

    return 0;
}
