#pragma once

/*
 * Single source of truth for the system memory map and the accelerator's
 * AXI4-Lite register layout.
 *
 * Included by the SystemC model (C++), by the ARM64 driver (C), and mirrored
 * by gem5/configs/kv260_arm64.py. Plain #define rather than constexpr so the
 * C driver can include it unchanged.
 */

#include <stdint.h>

/* ── Accelerator ─────────────────────────────────────────────────────────── */
/* Same base in the standalone SystemC model and in the gem5 system. */
#define ACCEL_BASE          0x10000000ULL
#define ACCEL_END           0x1FFFFFFFULL
#define ACCEL_REGION_SIZE   0x1000ULL /* one page covers the control block */

/*
 * Register offsets from ACCEL_BASE. This layout is what Vitis HLS generates
 * from grayscale_accel()'s s_axilite bundle, so the SystemC model, the driver
 * and the HLS kernel all share one programming model.
 *
 * Addresses are 32-bit: run_hls.tcl sets `config_interface -m_axi_addr64=0`.
 * Flipping that to 1 would add upper-half registers at +0x14/+0x1c and this
 * table would have to grow with it.
 */
#define ACCEL_REG_CTRL      0x00ULL /* bit0 ap_start (W), bit1 ap_done (R) */
#define ACCEL_REG_SRC       0x10ULL /* base address of the input RGB buffer  */
#define ACCEL_REG_DST       0x18ULL /* base address of the output gray buffer */
#define ACCEL_REG_NPIX      0x20ULL /* total pixels to process */

#define ACCEL_CTRL_AP_START (1u << 0)
#define ACCEL_CTRL_AP_DONE  (1u << 1)

/* ── Standalone SystemC model ────────────────────────────────────────────── */
/* RAM_IMG_* and DISK_IMG_* are offsets from their module's base. */
#define RAM_BASE            0x00000000ULL
#define RAM_END             0x03FFFFFFULL /* 64 MB */
#define RAM_IMG_IN          0x00000000ULL
#define RAM_IMG_OUT         0x00600000ULL

#define DISK_BASE           0x20000000ULL
#define DISK_END            0x2FFFFFFFULL
#define DISK_IMG_IN         0x00000000ULL
#define DISK_IMG_OUT        0x01000000ULL

/* ── gem5 virtual prototype ──────────────────────────────────────────────── */
/*
 * The driver's buffers live in gem5's memory, not in the SystemC RAM module,
 * so they get their own absolute addresses. The accelerator reaches them via
 * whatever it is handed in ACCEL_REG_SRC/DST at runtime, which is why the same
 * module works unchanged in both topologies.
 */
#define GEM5_IMG_IN         0x80000000ULL
#define GEM5_IMG_OUT        0x80600000ULL
