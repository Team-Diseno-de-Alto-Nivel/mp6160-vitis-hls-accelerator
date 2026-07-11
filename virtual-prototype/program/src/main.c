/*
 * ARM64 driver program skeleton, run by the simulated core in gem5.
 * Mirrors the flow implemented by CPU::run() in
 * ../../systemc-model/src/cpu/cpu.cpp (load -> store -> configure ->
 * poll -> fetch -> save), but here it is real software issuing loads/stores
 * to the accelerator's memory-mapped registers instead of SystemC TLM calls.
 *
 * Scaffolding only - register addresses and I/O calls are TODO once the
 * gem5 memory map (virtual-prototype/gem5/configs/kv260_arm64.py) is defined.
 */

#include <stdint.h>
#include <stdio.h>

/* TODO: replace with the addresses fixed in the gem5 memory map / README. */
#define ACCEL_BASE_ADDR   0x00000000ULL
#define ACCEL_CTRL_OFFSET 0x00
#define ACCEL_SRC_OFFSET  0x10
#define ACCEL_DST_OFFSET  0x18
#define ACCEL_NPIX_OFFSET 0x20

int main(void) {
    printf("driver: TODO load input image from disk\n");
    printf("driver: TODO store image in RAM\n");
    printf("driver: TODO configure accelerator (src, dst, num_pixels)\n");
    printf("driver: TODO poll ap_done / status\n");
    printf("driver: TODO read result from RAM\n");
    printf("driver: TODO save result to disk\n");
    return 0;
}
