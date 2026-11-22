// A stand-in for mbedTLS's `mbedtls/platform_util.h` on the computer, where the host tests link no ESP-IDF: the one function the
// widgets use to wipe a working copy (SECRETS §5). It zeroes through a volatile pointer, which the compiler may not
// drop, as mbedTLS's does, and tells a test's spy first what is about to be wiped, so a test can see that a secret's buffer
// was wiped and what it held.
#pragma once

#include <cstddef>

inline void (*mbedtls_test_zeroize_spy)(void *buf, size_t len) = nullptr;

inline void mbedtls_platform_zeroize(void *buf, size_t len)
{
    if (mbedtls_test_zeroize_spy != nullptr) {
        mbedtls_test_zeroize_spy(buf, len);
    }
    volatile unsigned char *p = static_cast<volatile unsigned char *>(buf);
    while (len-- > 0) {
        *p++ = 0;
    }
}
