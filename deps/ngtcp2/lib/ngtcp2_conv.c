/*
 * ngtcp2
 *
 * Copyright (c) 2017 ngtcp2 contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "ngtcp2_conv.h"

#include <string.h>
#include <assert.h>

#include "ngtcp2_str.h"

uint64_t ngtcp2_get_uint64(const uint8_t *p) {
  uint64_t n;
  memcpy(&n, p, 8);
  return bswap64(n);
}

uint64_t ngtcp2_get_uint48(const uint8_t *p) {
  uint64_t n = 0;
  memcpy(((uint8_t *)&n) + 2, p, 6);
  return bswap64(n);
}

uint32_t ngtcp2_get_uint32(const uint8_t *p) {
  uint32_t n;
  memcpy(&n, p, 4);
  return ntohl(n);
}

uint32_t ngtcp2_get_uint24(const uint8_t *p) {
  uint32_t n = 0;
  memcpy(((uint8_t *)&n) + 1, p, 3);
  return ntohl(n);
}

uint16_t ngtcp2_get_uint16(const uint8_t *p) {
  uint16_t n;
  memcpy(&n, p, 2);
  return ntohs(n);
}

uint64_t ngtcp2_get_varint(size_t *plen, const uint8_t *p) {
  union {
    char b[8];
    uint16_t n16;
    uint32_t n32;
    uint64_t n64;
  } n;

  *plen = 1u << (*p >> 6);

  switch (*plen) {
  case 1:
    return *p;
  case 2:
    memcpy(&n, p, 2);
    n.b[0] &= 0x3f;
    return ntohs(n.n16);
  case 4:
    memcpy(&n, p, 4);
    n.b[0] &= 0x3f;
    return ntohl(n.n32);
  case 8:
    memcpy(&n, p, 8);
    n.b[0] &= 0x3f;
    return bswap64(n.n64);
  }

  assert(0);
}

uint64_t ngtcp2_get_pkt_num(const uint8_t *p, size_t pkt_numlen) {
  switch (pkt_numlen) {
  case 1:
    return *p;
  case 2:
    return ngtcp2_get_uint16(p);
  case 3:
    return ngtcp2_get_uint24(p);
  case 4:
    return ngtcp2_get_uint32(p);
  default:
    assert(0);
  }
}

uint8_t *ngtcp2_put_uint64be(uint8_t *p, uint64_t n) {
  n = bswap64(n);
  return ngtcp2_cpymem(p, (const uint8_t *)&n, sizeof(n));
}

uint8_t *ngtcp2_put_uint48be(uint8_t *p, uint64_t n) {
  n = bswap64(n);
  return ngtcp2_cpymem(p, ((const uint8_t *)&n) + 2, 6);
}

uint8_t *ngtcp2_put_uint32be(uint8_t *p, uint32_t n) {
  n = htonl(n);
  return ngtcp2_cpymem(p, (const uint8_t *)&n, sizeof(n));
}

uint8_t *ngtcp2_put_uint24be(uint8_t *p, uint32_t n) {
  n = htonl(n);
  return ngtcp2_cpymem(p, ((const uint8_t *)&n) + 1, 3);
}

uint8_t *ngtcp2_put_uint16be(uint8_t *p, uint16_t n) {
  n = htons(n);
  return ngtcp2_cpymem(p, (const uint8_t *)&n, sizeof(n));
}

uint8_t *ngtcp2_put_varint(uint8_t *p, uint64_t n) {
  uint8_t *rv;
  if (n < 64) {
    *p++ = (uint8_t)n;
    return p;
  }
  if (n < 16384) {
    rv = ngtcp2_put_uint16be(p, (uint16_t)n);
    *p |= 0x40;
    return rv;
  }
  if (n < 1073741824) {
    rv = ngtcp2_put_uint32be(p, (uint32_t)n);
    *p |= 0x80;
    return rv;
  }
  assert(n < 4611686018427387904ULL);
  rv = ngtcp2_put_uint64be(p, n);
  *p |= 0xc0;
  return rv;
}

uint8_t *ngtcp2_put_varint14(uint8_t *p, uint16_t n) {
  uint8_t *rv;

  assert(n < 16384);

  rv = ngtcp2_put_uint16be(p, n);
  *p |= 0x40;

  return rv;
}

uint8_t *ngtcp2_put_pkt_num(uint8_t *p, uint64_t pkt_num, size_t len) {
  switch (len) {
  case 1:
    *p++ = (uint8_t)pkt_num;
    return p;
  case 2:
    ngtcp2_put_uint16be(p, (uint16_t)pkt_num);
    return p + 2;
  case 3:
    ngtcp2_put_uint24be(p, (uint32_t)pkt_num);
    return p + 3;
  case 4:
    ngtcp2_put_uint32be(p, (uint32_t)pkt_num);
    return p + 4;
  default:
    assert(0);
  }
}

size_t ngtcp2_get_varint_len(const uint8_t *p) { return 1u << (*p >> 6); }

size_t ngtcp2_put_varint_len(uint64_t n) {
  if (n < 64) {
    return 1;
  }
  if (n < 16384) {
    return 2;
  }
  if (n < 1073741824) {
    return 4;
  }
  assert(n < 4611686018427387904ULL);
  return 8;
}

uint64_t ngtcp2_nth_server_bidi_id(uint64_t n) {
  if (n == 0) {
    return 0;
  }
  return ((n - 1) << 2) | 0x01;
}

uint64_t ngtcp2_nth_client_bidi_id(uint64_t n) {
  if (n == 0) {
    return 0;
  }
  return (n - 1) << 2;
}

uint64_t ngtcp2_nth_server_uni_id(uint64_t n) {
  if (n == 0) {
    return 0;
  }

  return ((n - 1) << 2) | 0x03;
}

uint64_t ngtcp2_nth_client_uni_id(uint64_t n) {
  if (n == 0) {
    return 0;
  }

  return ((n - 1) << 2) | 0x02;
}
