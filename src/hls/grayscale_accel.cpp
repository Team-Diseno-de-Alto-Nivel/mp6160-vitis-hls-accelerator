#include "grayscale_accel.h"

#include <cmath>

// hls::stream lives in the Vitis HLS headers. When this file is compiled by
// plain host g++ (CI's functional check and `make hls-host`, where Vitis is not
// installed), fall back to a minimal std::queue-backed shim so the exact same
// source runs as software. The #pragma HLS lines are simply ignored by g++.
#if __has_include(<hls_stream.h>)
#include <hls_stream.h>
#else
#include <queue>
namespace hls {
template <typename T>
class stream {
    std::queue<T> q_;
public:
    stream() {}
    explicit stream(const char *) {}
    void write(const T &v) { q_.push(v); }
    T read() { T v = q_.front(); q_.pop(); return v; }
    bool empty() const { return q_.empty(); }
};
}  // namespace hls
#endif

namespace {

// One RGB pixel carried through the dataflow pipeline.
struct rgb_t {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

// ── Input stage ───────────────────────────────────────────────────────────────
// Burst-reads the RGB image from DRAM over the m_axi port and streams one pixel
// at a time to the compute stage. No arithmetic here: I/O only.
void read_rgb(const unsigned char *rgb_in, hls::stream<rgb_t> &out, uint32_t num_pixels) {
read_loop:
    for (uint32_t i = 0; i < num_pixels; ++i) {
#pragma HLS PIPELINE II=1
        rgb_t p;
        p.r = rgb_in[3 * i + 0];
        p.g = rgb_in[3 * i + 1];
        p.b = rgb_in[3 * i + 2];
        out.write(p);
    }
}

// ── Compute stage ─────────────────────────────────────────────────────────────
// Pure RGB->grayscale. Same BT.601 arithmetic as the SystemC golden model
// (src/model/utils/conversion.h): identical float weights and rounding, so the
// hardware output is bit-for-bit identical to the reference.
void compute_gray(hls::stream<rgb_t> &in, hls::stream<unsigned char> &out, uint32_t num_pixels) {
compute_loop:
    for (uint32_t i = 0; i < num_pixels; ++i) {
#pragma HLS PIPELINE II=1
        rgb_t p = in.read();
        float gray = 0.299f * p.r + 0.587f * p.g + 0.114f * p.b;
        out.write(static_cast<unsigned char>(std::round(gray)));
    }
}

// ── Output stage ──────────────────────────────────────────────────────────────
// Burst-writes the grayscale image back to DRAM over the m_axi port. I/O only.
void write_gray(unsigned char *gray_out, hls::stream<unsigned char> &in, uint32_t num_pixels) {
write_loop:
    for (uint32_t i = 0; i < num_pixels; ++i) {
#pragma HLS PIPELINE II=1
        gray_out[i] = in.read();
    }
}

}  // namespace

void grayscale_accel(
    const unsigned char *rgb_in,
    unsigned char       *gray_out,
    uint32_t             num_pixels
) {
    // Data movers: burst read/write to DDR. depth= sizes the memory model for
    // C/RTL co-simulation (1080p: 3 B/pixel in, 1 B/pixel out); it does not
    // constrain synthesis.
    //
    // max_widen_bitwidth=64 is what keeps read_rgb at II=1. Both pointers are
    // `unsigned char*`, so Vitis would otherwise infer 8-bit AXI ports moving
    // 1 B/cycle -- and since read_rgb consumes 3 B/pixel, the read stage would
    // be stuck at II=3 and throttle the whole DATAFLOW region to 3 cycles per
    // pixel. Widening to 64 bits gives 8 B/cycle, comfortably above the 3 B and
    // 1 B per cycle the read and write stages need to sustain II=1.
    // Automatic port width resizing only rewrites how bytes are fetched, so the
    // arithmetic -- and thus bit-exactness with the SystemC model -- is
    // untouched, and plain g++ ignores these pragmas so `make hls-host` still
    // builds the same source.
#pragma HLS INTERFACE m_axi     port=rgb_in     offset=slave bundle=gmem0 depth=6220800 max_widen_bitwidth=64
#pragma HLS INTERFACE m_axi     port=gray_out   offset=slave bundle=gmem1 depth=2073600 max_widen_bitwidth=64
    // AXI4-Lite control: base addresses, pixel count, and ap_start/ap_done.
#pragma HLS INTERFACE s_axilite port=rgb_in     bundle=control
#pragma HLS INTERFACE s_axilite port=gray_out   bundle=control
#pragma HLS INTERFACE s_axilite port=num_pixels bundle=control
#pragma HLS INTERFACE s_axilite port=return     bundle=control

    // Run the three stages concurrently as a pipeline: while the compute stage
    // works on pixel i, the input stage is already fetching i+1 and the output
    // stage is draining i-1. The hls::stream FIFOs decouple the stages.
#pragma HLS DATAFLOW
    hls::stream<rgb_t> rgb_stream("rgb_stream");
    hls::stream<unsigned char> gray_stream("gray_stream");
#pragma HLS STREAM variable=rgb_stream  depth=64
#pragma HLS STREAM variable=gray_stream depth=64

    read_rgb(rgb_in, rgb_stream, num_pixels);
    compute_gray(rgb_stream, gray_stream, num_pixels);
    write_gray(gray_out, gray_stream, num_pixels);
}
