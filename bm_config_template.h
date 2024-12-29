#ifndef __BM_CONFIG_H__
#define __BM_CONFIG_H__

#define bm_app_name "user_app"

#define bm_debug(format, ...) printf(format, ##__VA_ARGS__)

#define bm_configuration_size_bytes (5 * 1024)

#endif
