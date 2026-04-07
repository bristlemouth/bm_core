#pragma once

/// @file pcap.h
/// @brief Sink-agnostic pcap stream formatter.
///
/// Formats L2 Ethernet frames as pcap records and pushes the raw bytes
/// through a caller-supplied write callback.  The callback can target a
/// file, serial port, ring buffer, or any other byte sink.
///
/// Thread safety is the caller's responsibility — if the write callback
/// can be invoked from multiple threads (e.g. separate RX and TX paths),
/// the callback itself must synchronise access to its sink.

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Callback invoked by the pcap formatter to emit raw bytes.
///
/// @param data  Pointer to the bytes to write.
/// @param len   Number of bytes.
/// @param ctx   Opaque context supplied at init time.
typedef void (*PcapWriteCb)(const uint8_t *data, size_t len, void *ctx);

/// Initialise the pcap stream.
///
/// Stores the write callback and immediately writes the 24-byte pcap
/// global header through it.
///
/// @param write_cb  Byte-sink callback.
/// @param ctx       Opaque context forwarded to every @p write_cb call.
void pcap_init(PcapWriteCb write_cb, void *ctx);

/// Write one L2 Ethernet frame as a pcap packet record.
///
/// Timestamps are derived from bm_get_tick_count() / bm_ticks_to_ms().
///
/// @param frame  Pointer to the raw L2 Ethernet frame.
/// @param len    Length of the frame in bytes.
void pcap_write_packet(const uint8_t *frame, size_t len);

#ifdef __cplusplus
}
#endif
