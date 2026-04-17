#include "pcap.h"
#include "bm_os.h"
#include <string.h>

// https://wiki.wireshark.org/Development/LibpcapFileFormat

typedef struct {
  uint32_t magic_number;
  uint16_t version_major;
  uint16_t version_minor;
  int32_t thiszone;
  uint32_t sigfigs;
  uint32_t snaplen;
  uint32_t network;
} __attribute__((packed)) PcapHeader;

typedef struct {
  uint32_t ts_sec;
  uint32_t ts_usec;
  uint32_t incl_len;
  uint32_t orig_len;
} __attribute__((packed)) PcapRecordHeader;

static PcapWriteCb s_write_cb;
static void *s_write_ctx;

void pcap_init(PcapWriteCb write_cb, void *ctx) {
  s_write_cb = write_cb;
  s_write_ctx = ctx;

  static const PcapHeader header = {
      .magic_number = 0xA1B2C3D4,
      .version_major = 2,
      .version_minor = 4,
      .thiszone = 0,
      .sigfigs = 0,
      .snaplen = 65535,
      .network = 1, // LINKTYPE_ETHERNET
  };

  s_write_cb((const uint8_t *)&header, sizeof(header), s_write_ctx);
}

void pcap_write_packet(const uint8_t *frame, size_t len) {
  if (!s_write_cb || !frame || len == 0) {
    return;
  }

  uint32_t ms = bm_ticks_to_ms(bm_get_tick_count());

  PcapRecordHeader rec = {
      .ts_sec = ms / 1000,
      .ts_usec = (ms % 1000) * 1000,
      .incl_len = (uint32_t)len,
      .orig_len = (uint32_t)len,
  };

  s_write_cb((const uint8_t *)&rec, sizeof(rec), s_write_ctx);
  s_write_cb(frame, len, s_write_ctx);
}
