// SPDX-License-Identifier: Apache-2.0
#include "applets.h"
#include "device.h"
#include "ndef.h"
#include "oath.h"
#include "openpgp.h"
#include "piv.h"
#include <admin.h>
#include <aes.h>
#include <apdu.h>
#include <assert.h>
#include <bd/lfs_filebd.h>
#include <ctap.h>
#include <fs.h>
#include <lfs.h>

static struct lfs_config cfg;
static lfs_filebd_t bd;

uint8_t private_key[] = {0x46, 0x5b, 0x44, 0x5d, 0x8e, 0x78, 0x34, 0x53, 0xf7, 0x4b, 0x90,
                         0x00, 0xd2, 0x20, 0x32, 0x51, 0x99, 0x5e, 0x12, 0xdc, 0xd1, 0x21,
                         0xa1, 0x9c, 0xea, 0x09, 0x5a, 0xfc, 0xf8, 0xe9, 0xeb, 0x75};
uint8_t cert[] = {
    0x30, 0x82, 0x02, 0x77, 0x30, 0x82, 0x01, 0x5f, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x15, 0x30, 0x0d, 0x06,
    0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x30, 0x31, 0x31, 0x2f, 0x30, 0x2d, 0x06,
    0x03, 0x55, 0x04, 0x03, 0x0c, 0x26, 0x43, 0x61, 0x6e, 0x6f, 0x4b, 0x65, 0x79, 0x73, 0x20, 0x46, 0x49, 0x44, 0x4f,
    0x20, 0x41, 0x74, 0x74, 0x65, 0x73, 0x74, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x20, 0x43,
    0x41, 0x20, 0x4e, 0x6f, 0x2e, 0x31, 0x30, 0x1e, 0x17, 0x0d, 0x32, 0x31, 0x31, 0x30, 0x32, 0x37, 0x31, 0x36, 0x33,
    0x31, 0x31, 0x39, 0x5a, 0x17, 0x0d, 0x33, 0x31, 0x30, 0x37, 0x32, 0x37, 0x31, 0x36, 0x33, 0x31, 0x31, 0x39, 0x5a,
    0x30, 0x66, 0x31, 0x20, 0x30, 0x1e, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x17, 0x43, 0x61, 0x6e, 0x6f, 0x4b, 0x65,
    0x79, 0x20, 0x53, 0x65, 0x72, 0x69, 0x61, 0x6c, 0x20, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x22,
    0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c, 0x19, 0x41, 0x75, 0x74, 0x68, 0x65, 0x6e, 0x74, 0x69, 0x63, 0x61,
    0x74, 0x6f, 0x72, 0x20, 0x41, 0x74, 0x74, 0x65, 0x73, 0x74, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x31, 0x11, 0x30, 0x0f,
    0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x08, 0x43, 0x61, 0x6e, 0x6f, 0x4b, 0x65, 0x79, 0x73, 0x31, 0x0b, 0x30, 0x09,
    0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x43, 0x4e, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce,
    0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x2c, 0x11,
    0x4a, 0x50, 0x45, 0x41, 0x4a, 0x6b, 0x22, 0x8c, 0x0c, 0xc4, 0xf7, 0x7a, 0x18, 0xfc, 0x5d, 0x1c, 0x6e, 0x97, 0x54,
    0xe7, 0xaf, 0x94, 0x72, 0x44, 0xfe, 0xc7, 0x60, 0x7c, 0xed, 0x5a, 0xc8, 0xa0, 0x3a, 0x74, 0xe3, 0x80, 0x86, 0xb1,
    0xb5, 0xf2, 0xd7, 0x2e, 0x5d, 0x2a, 0xcf, 0x51, 0x77, 0x38, 0x2e, 0x2f, 0x60, 0x76, 0xe5, 0x25, 0xe7, 0x9a, 0x92,
    0xa5, 0xa1, 0xa6, 0x0b, 0x2c, 0xa3, 0x30, 0x30, 0x2e, 0x30, 0x09, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x04, 0x02, 0x30,
    0x00, 0x30, 0x21, 0x06, 0x0b, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xe5, 0x1c, 0x01, 0x01, 0x04, 0x04, 0x12, 0x04,
    0x10, 0x24, 0x4e, 0xb2, 0x9e, 0xe0, 0x90, 0x4e, 0x49, 0x81, 0xfe, 0x1f, 0x20, 0xf8, 0xd3, 0xb8, 0xf4, 0x30, 0x0d,
    0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x7c,
    0x28, 0x27, 0x67, 0x06, 0x65, 0xd6, 0x31, 0x78, 0xda, 0xe8, 0x9e, 0xc0, 0xac, 0x93, 0xc1, 0xb5, 0xd2, 0x56, 0xaf,
    0xf6, 0x1d, 0x0b, 0x01, 0x5d, 0xc1, 0x1a, 0x04, 0xf5, 0xc2, 0xf7, 0x00, 0x9b, 0xac, 0xf6, 0xaf, 0xe0, 0xc9, 0x23,
    0x93, 0xb4, 0x9f, 0xe0, 0x7e, 0xb1, 0x22, 0xd8, 0xbe, 0x0f, 0xa8, 0x9d, 0x32, 0x5f, 0x53, 0x78, 0xe4, 0x11, 0x90,
    0xb2, 0x58, 0xa1, 0x0c, 0x0f, 0x0d, 0x07, 0x68, 0xdb, 0xea, 0x4e, 0xb5, 0x0c, 0xff, 0x7e, 0x7a, 0x93, 0x80, 0xcc,
    0x51, 0xa0, 0x6d, 0x49, 0x1b, 0x28, 0x34, 0x57, 0xb5, 0xcd, 0xf0, 0xc3, 0x1c, 0x32, 0x9e, 0xcb, 0x5a, 0x9d, 0x32,
    0x44, 0xd0, 0x6b, 0x9a, 0x7b, 0x7b, 0x56, 0xfe, 0x6f, 0xac, 0xb7, 0xa8, 0x51, 0x55, 0x57, 0x39, 0x5d, 0xe2, 0x0e,
    0xba, 0xdd, 0xe8, 0x25, 0x64, 0x72, 0x9f, 0x88, 0xfc, 0x93, 0x1b, 0xff, 0x62, 0x3e, 0xf2, 0xd3, 0x1c, 0x6f, 0xd4,
    0xf7, 0xbe, 0xfc, 0xea, 0x51, 0x86, 0xbd, 0xff, 0x78, 0xda, 0xef, 0x92, 0x3b, 0xc2, 0x3e, 0xe5, 0x5c, 0xb9, 0x3c,
    0x4c, 0xf2, 0xff, 0x09, 0xf8, 0x77, 0xa7, 0x2d, 0x87, 0x7e, 0x8d, 0x3a, 0x08, 0xa2, 0xec, 0x1d, 0xf4, 0x5c, 0xb9,
    0x7c, 0x8b, 0x86, 0xda, 0xc0, 0xfb, 0x5b, 0xb9, 0x22, 0x80, 0x19, 0x18, 0x31, 0xe4, 0x69, 0xa2, 0x92, 0xa4, 0x7e,
    0x83, 0x75, 0xd0, 0x60, 0x5d, 0x7d, 0x41, 0x9c, 0xbf, 0x56, 0x74, 0xcb, 0x6a, 0xc4, 0x48, 0x96, 0x8c, 0x8a, 0x63,
    0xdf, 0xe2, 0x1c, 0xaf, 0x49, 0xb9, 0x3d, 0xaf, 0x29, 0x86, 0x0a, 0x7a, 0xc7, 0x8c, 0x4e, 0x73, 0x05, 0xa9, 0x8d,
    0x1d, 0xb4, 0xd4, 0x33, 0x6c, 0x0b, 0x64, 0xaf, 0x3a, 0xb1, 0xe1, 0xb7, 0x29, 0xde, 0x3b, 0xe6, 0x6b, 0xdb, 0xf2,
    0x59, 0xa4, 0x60, 0x4c, 0x68, 0x72, 0x47, 0x7a};

static void fake_fido_personalization() {

  uint8_t c_buf[1024], r_buf[1024];
  CAPDU capdu;
  RAPDU rapdu;
  capdu.data = c_buf;
  rapdu.data = r_buf;

  build_capdu(&capdu, (uint8_t *)"\x00\x20\x00\x00\x06\x31\x32\x33\x34\x35\x36", 11);
  admin_process_apdu(&capdu, &rapdu);
  assert(rapdu.sw == 0x9000);

  capdu.cla = 0x00;
  capdu.ins = ADMIN_INS_WRITE_FIDO_PRIVATE_KEY;
  capdu.data = private_key;
  capdu.lc = 32;

  admin_process_apdu(&capdu, &rapdu);
  assert(rapdu.sw == 0x9000);

  capdu.ins = ADMIN_INS_WRITE_FIDO_CERT;
  capdu.data = cert;
  capdu.lc = sizeof(cert);
  admin_process_apdu(&capdu, &rapdu);
  assert(rapdu.sw == 0x9000);
}

static void fido2_init() { fake_fido_personalization(); }

static void oath_init() {
  uint8_t r_buf[1024] = {0};
  // name: abc, algo: HOTP+SHA1, digit: 6, key: 0x00 0x01 0x02
  uint8_t data[] = {0x71, 0x03, 'a', 'b', 'c', 0x73, 0x05, 0x11, 0x06, 0x00, 0x01, 0x02};
  CAPDU C = {.data = data, .ins = OATH_INS_PUT, .lc = sizeof(data)};
  RAPDU R = {.data = r_buf};
  CAPDU *capdu = &C;
  RAPDU *rapdu = &R;
  oath_process_apdu(capdu, rapdu);
  // set default
  uint8_t data2[] = {0x71, 0x03, 'a', 'b', 'c'};
  capdu->data = data2;
  capdu->ins = OATH_INS_SET_DEFAULT;
  capdu->lc = sizeof(data2);
  oath_process_apdu(capdu, rapdu);
}

void card_fs_init(const char *lfs_root) {
  memset(&cfg, 0, sizeof(cfg));
  cfg.context = &bd;
  cfg.read = &lfs_filebd_read;
  cfg.prog = &lfs_filebd_prog;
  cfg.erase = &lfs_filebd_erase;
  cfg.sync = &lfs_filebd_sync;
  cfg.read_size = 1;
  cfg.prog_size = 512;
  cfg.block_size = 512;
  cfg.block_count = 256;
  cfg.block_cycles = 50000;
  cfg.cache_size = 512;
  cfg.lookahead_size = 16;
  lfs_filebd_create(&cfg, lfs_root);

  assert(fs_init(&cfg) == 0);
}

int card_fabrication_procedure(const char *lfs_root) {
  card_fs_init(lfs_root);
  applets_install();

  // reset state of applets
  uint8_t c_buf[1024] = "RESET", r_buf[1024];
  RAPDU rapdu = {.data = r_buf};
  CAPDU capdu = {.data = c_buf, .cla = 0x00, .ins = ADMIN_INS_VERIFY, .p1 = 0, .p2 = 0, .lc = 6};
  admin_process_apdu(&capdu, &rapdu);
  admin_process_apdu(&capdu, &rapdu);
  admin_process_apdu(&capdu, &rapdu);
  // now the admin PIN has been locked
  capdu.ins = ADMIN_INS_FACTORY_RESET;
  capdu.lc = 5;
  admin_process_apdu(&capdu, &rapdu);
  assert(rapdu.sw == 0x9000);
  oath_init();
  fido2_init();

  return 0;
}

int card_read(const char *lfs_root) {
  card_fs_init(lfs_root);

  applets_install();
  return 0;
}

// constants for vendor
#ifdef VIRT_VENDOR
#include "git-rev.h"
int admin_vendor_version(const CAPDU *capdu, RAPDU *rapdu) {
  LL = strlen(GIT_REV);
  memcpy(RDATA, GIT_REV, LL);
  if (LL > LE) LL = LE;

  return 0;
}

int admin_vendor_hw_variant(const CAPDU *capdu, RAPDU *rapdu) {
  UNUSED(capdu);

  static const char *const hw_variant_str = "CanoKey Virt-Card";
  size_t len = strlen(hw_variant_str);
  memcpy(RDATA, hw_variant_str, len);
  LL = len;
  if (LL > LE) LL = LE;

  return 0;
}

int admin_vendor_hw_sn(const CAPDU *capdu, RAPDU *rapdu) {
  UNUSED(capdu);

  static const char *const hw_sn = "\x00";
  memcpy(RDATA, hw_sn, 1);
  LL = 1;
  if (LL > LE) LL = LE;

  return 0;
}
#endif // VIRT_VENDOR
