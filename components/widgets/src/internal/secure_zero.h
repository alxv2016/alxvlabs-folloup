// Wiping a working copy of a typed secret (SECRETS §5): `mbedtls_platform_zeroize`, which mbedTLS (bundled with
// ESP-IDF, a private requirement of the widgets component) writes through a volatile pointer so the compiler cannot drop it as a
// dead store — a plain `memset` before a free or a scope's end can be removed. Private to the widgets component.

#pragma once

#include <cstddef>

#include "mbedtls/platform_util.h"

namespace widgets::detail {

inline void SecureZero(void *buf, size_t len)
{
    if (buf != nullptr && len != 0) {
        mbedtls_platform_zeroize(buf, len);
    }
}

}  // namespace widgets::detail
