#include "device.h"
#include <string.h>

#define mac_address_size_bytes 6

static DeviceCfg CFG;

/*!
  @brief Generate 48-bit MAC Address From Device's Node ID

  @details First 16 bits are common to all BM devices (0's for now)
           Last 32 bits the 32 least significant bits of the node id

  @param *buff - 6 byte buffer to store MAC address
  @param size of buffer as a safety check (>= mac_address_size_bytes)

  @return BmOK on success
  @return BmErr on failure
*/
BmErr mac_address(uint8_t *mac, uint32_t size) {
  BmErr err = BmEINVAL;
  uint64_t id = 0;

  if (size >= mac_address_size_bytes) {
    err = BmOK;
    id = node_id();
    mac[0] = 0x00;
    mac[1] = 0x00;
    mac[2] = (id >> 24) & 0xFF;
    mac[3] = (id >> 16) & 0xFF;
    mac[4] = (id >> 8) & 0xFF;
    mac[5] = (id >> 0) & 0xFF;
  }

  return err;
}

/*!
  @brief Get The ID Of This Node

  @details This must be a unique value

  @return User configured node ID
 */
uint64_t node_id(void) { return CFG.node_id; }

/*!
  @brief Get The GIT/Version Control System Build Hash

  @return User configured SHA
 */
uint32_t git_sha(void) { return CFG.git_sha; }

/*!
  @brief Get The Vendor ID Of This Node

  @return User configured vendor ID
 */
uint16_t vendor_id(void) { return CFG.vendor_id; }

/*!
  @brief Get The Product ID Of This Node

  @return User configured product ID
 */
uint16_t product_id(void) { return CFG.product_id; }

/*!
  @brief Get The Product Hardware Revision

  @return User configured hardware revision
 */
uint8_t hardware_revision(void) { return CFG.hw_ver; }

/*!
  @brief Get The Device Name Of This Node

  @details This must be NULL terminated

  @return user configured device name string
 */
const char *device_name(void) { return CFG.device_name; }

/*!
  @brief Get The Device's Version String

  @details This must be NULL terminated

  @return user configured device version string
 */
const char *version_string(void) { return CFG.version_string; }

/*!
  @brief Get The Device's Version Information

  @param major version number of device major fw version
  @param minor version number of device minor fw version
  @param patch version number of device patch fw version

  @return BmOK on success
  @return BmErr on failure
 */
BmErr firmware_version(uint8_t *major, uint8_t *minor, uint8_t *patch) {
  BmErr err = BmEINVAL;

  if (major && minor && patch) {
    err = BmOK;
    *major = CFG.ver_major;
    *minor = CFG.ver_minor;
    *patch = CFG.ver_patch;
  }

  return err;
}

/*!
  @brief Get The Device's Serial Number

  @details Buffer must be 16 bytes long

  @param sn pointer to serial number reference
  @param size

  @return BmOK on success
  @return BmErr on failure
 */
BmErr serial_number(uint8_t *sn, uint32_t size) {
  BmErr err = BmEINVAL;

  if (sn && size >= member_size(DeviceCfg, sn)) {
    err = BmOK;
    memcpy(sn, CFG.sn, member_size(DeviceCfg, sn));
  }

  return err;
}

/*!
  @brief Initialize Device Information

  @param cfg configuration for user device information

  @return BmOK
 */
BmErr device_init(DeviceCfg cfg) {
  memset(&CFG, 0, sizeof(CFG));
  CFG = cfg;
  return BmOK;
}
