#ifndef __BM_PACKET_H__
#define __BM_PACKET_H__

#include "messages.h"
#include "util.h"
#include "ip_addr.h"
#include "ll.h"
#include <stdint.h>

#define bcmp_serialize_data(err, buf, info) \
do { \
    BCMPSerializeInfo serialize = info; \
    err = bcmp_serialize(buf, serialize) \
} while (0);

typedef struct {
    BCMPHeader *header;
    uint8_t *payload;
    struct *pbuf pbuf;
    ip_addr_t *src;
    ip_addr_t *dst;
    uint8_t ingest_port;
} BCMPParserData;

typedef BMErr (*BCMPParserCb)(BCMPParserData data);

typedef struct {
    struct {
        BCMPMessageType *types;
        uint32_t count;
    } msg;
    BCMPParserCb cb;
} BCMPMessageParserCfg;

typedef struct {
    BCMPMessageParserCfg cfg;
    LLItem object;
} BCMPParserItem;

typedef enum {
    BCMP_PARSER_8BIT,
    BCMP_PARSER_16BIT,
    BCMP_PARSER_32BIT,
    BCMP_PARSER_64BIT,
} BCMPVarLength;

typedef struct {
    BCMPVarLength *lengths;
    uint32_t size;
} BCMPSerializeInfo;

BMErr bcmp_parser_init(void);
BMErr bcmp_parser_add(BCMPParserItem *item);
BMErr bcmp_parse_update(struct pbuf *pbuf, ip_addr_t *src, ip_addr_t *dst);
BMErr bcmp_serialize(void *buf, BCMPSerializeInfo info);

#endif
