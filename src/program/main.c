/*
 * ARM64 driver program, run by the simulated core in gem5.
 *
 * Mirrors the flow implemented by CPU::run() in
 * ../../model/cpu/cpu.cpp (load -> store -> configure -> start ->
 * poll -> fetch -> save), but here it is real software issuing loads/stores to
 * the accelerator's AXI4-Lite control registers and to the DRAM regions
 * holding the image, instead of SystemC TLM calls.
 *
 * The memory map and register layout come from ../../common/memory_map.h,
 * which the SystemC model and the gem5 config share.
 *
 * Two ways to reach physical memory, selected at build time:
 *
 *   GEM5_SE=ON (default)  — gem5 SE mode. There is no /dev/mem: gem5's mmap
 *                           emulation with a real fd copies file contents into
 *                           anonymous pages instead of mapping physical
 *                           memory. The gem5 config identity-maps the
 *                           accelerator page and the image buffers into the
 *                           process page table, so physical addresses can be
 *                           dereferenced directly.
 *   GEM5_SE=OFF           — real hardware (Kria KV260). mmap /dev/mem, needs
 *                           root or CAP_SYS_RAWIO.
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "image_config.h"
#include "memory_map.h"

#define DEFAULT_INPUT_PATH  "images/input/image.raw"
#define DEFAULT_OUTPUT_PATH "images/output/output_gem5.raw"

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

#ifdef GEM5_SE
static volatile uint32_t *map_phys(int mem_fd, uint64_t phys_addr, size_t size) {
    (void)mem_fd;
    (void)size;
    return (volatile uint32_t *)(uintptr_t)phys_addr;
}
#else
static volatile uint32_t *map_phys(int mem_fd, uint64_t phys_addr, size_t size) {
    void *m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (off_t)phys_addr);
    if (m == MAP_FAILED) {
        die("mmap");
    }
    return (volatile uint32_t *)m;
}
#endif

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
    *(volatile uint32_t *)((uint8_t *)ctrl + ACCEL_REG_SRC)  = (uint32_t)src_addr;
    *(volatile uint32_t *)((uint8_t *)ctrl + ACCEL_REG_DST)  = (uint32_t)dst_addr;
    *(volatile uint32_t *)((uint8_t *)ctrl + ACCEL_REG_NPIX) = num_pixels;
    printf("driver: configured accelerator (src=0x%08llx, dst=0x%08llx, pixels=%u)\n",
           (unsigned long long)src_addr, (unsigned long long)dst_addr, num_pixels);
}

static void start_accelerator(volatile uint32_t *ctrl) {
    *(volatile uint32_t *)((uint8_t *)ctrl + ACCEL_REG_CTRL) = ACCEL_CTRL_AP_START;
}

static void wait_accelerator_done(volatile uint32_t *ctrl) {
    volatile uint32_t *ctrl_reg = (volatile uint32_t *)((uint8_t *)ctrl + ACCEL_REG_CTRL);
    printf("driver: waiting for accelerator (ap_done)...\n");
    while ((*ctrl_reg & ACCEL_CTRL_AP_DONE) == 0) {
#ifndef GEM5_SE
        /*
         * Back off on real hardware. Not in gem5 SE: it ignores nanosleep
         * (so the sleep buys nothing) and has no clock_nanosleep handler at
         * all, which glibc's usleep may route to — that would be a fatal
         * "syscall unimplemented". Spinning on the volatile read is both
         * correct and cheaper there, since simulated time only advances
         * through the accelerator's own transactions anyway.
         */
        usleep(1000);
#endif
    }
    printf("driver: accelerator done\n");
}

int main(int argc, char **argv) {
    const char *input_path  = (argc > 1) ? argv[1] : DEFAULT_INPUT_PATH;
    const char *output_path = (argc > 2) ? argv[2] : DEFAULT_OUTPUT_PATH;

    unsigned char *input_image  = malloc(IMAGE_BYTES_RGB);
    unsigned char *output_image = malloc(IMAGE_BYTES_GRAY);
    if (!input_image || !output_image) {
        die("malloc");
    }

    int mem_fd = -1;
#ifndef GEM5_SE
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        die("open /dev/mem (are you root?)");
    }
#endif

    volatile uint32_t *accel_ctrl = map_phys(mem_fd, ACCEL_BASE, ACCEL_REGION_SIZE);
    volatile uint32_t *ram_in     = map_phys(mem_fd, GEM5_IMG_IN, IMAGE_BYTES_RGB);
    volatile uint32_t *ram_out    = map_phys(mem_fd, GEM5_IMG_OUT, IMAGE_BYTES_GRAY);

    /* 1. Load RGB RAW image from the disk. */
    load_image_from_disk(input_path, input_image, IMAGE_BYTES_RGB);

    /* 2. Store the image in RAM. */
    store_image_to_ram(ram_in, input_image, IMAGE_BYTES_RGB);

    /* 3. Configure the accelerator: source, destination, pixel count. */
    configure_accelerator(accel_ctrl, GEM5_IMG_IN, GEM5_IMG_OUT, (uint32_t)IMAGE_PIXEL_COUNT);

    /* 4. Start it and wait for completion. */
    start_accelerator(accel_ctrl);
    wait_accelerator_done(accel_ctrl);

    /* 5. Read the processed image back from RAM. */
    read_result_from_ram(output_image, ram_out, IMAGE_BYTES_GRAY);

    /* 6. Save the result to disk. */
    save_result_to_disk(output_path, output_image, IMAGE_BYTES_GRAY);

#ifndef GEM5_SE
    munmap((void *)accel_ctrl, ACCEL_REGION_SIZE);
    munmap((void *)ram_in, IMAGE_BYTES_RGB);
    munmap((void *)ram_out, IMAGE_BYTES_GRAY);
    close(mem_fd);
#endif
    free(input_image);
    free(output_image);

    return 0;
}
