# folloup firmware, built from the board's hardware spec (STICKY_HARDWARE_SPECS) — ESP-IDF v6.1.1, pinned in
# main/idf_component.yml; the chip is set in sdkconfig.defaults.
cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Build only main and what it depends on: a component is built only when something names it in REQUIRES or
# PRIV_REQUIRES, so deleting a feature's line in main/app_features.def leaves nothing of it in the build
# (APP_ARCHITECTURE §1).
idf_build_set_property(MINIMAL_BUILD ON)

# The firmware's version, in semver. IDF writes it into the image header: esp_app_get_description() reads it back,
# and an OTA's header check sees the same string. Left unset, IDF would take `git describe`. Set before project().
set(PROJECT_VER "0.1.0")

project(folloup)
