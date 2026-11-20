#include "pcf8563.h"

namespace {
constexpr int kI2cTimeoutMs = 20;
constexpr uint8_t kControlStatus1Reg = 0x00;
constexpr uint8_t kClkoutControlReg = 0x0D;
constexpr uint8_t kTimeReg = 0x02;
constexpr uint8_t kClkoutDisabled = 0x00;

uint8_t bcd_to_dec(uint8_t value)
{
    return static_cast<uint8_t>(((value >> 4) * 10U) + (value & 0x0FU));
}

uint8_t dec_to_bcd(uint8_t value)
{
    return static_cast<uint8_t>(((value / 10U) << 4) | (value % 10U));
}

bool valid_bcd(uint8_t value)
{
    return (value & 0x0F) <= 9 && (value >> 4) <= 9;
}
bool valid_calendar(const std::tm &value)
{
    if (value.tm_year < 0 || value.tm_year > 199 || value.tm_mon < 0 || value.tm_mon > 11 ||
        value.tm_mday < 1 || value.tm_hour < 0 || value.tm_hour > 23 || value.tm_min < 0 ||
        value.tm_min > 59 || value.tm_sec < 0 || value.tm_sec > 59 || value.tm_wday < 0 ||
        value.tm_wday > 6) {
        return false;
    }
    constexpr int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const int year = value.tm_year + 1900;
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    return value.tm_mday <= days[value.tm_mon] + (value.tm_mon == 1 && leap ? 1 : 0);
}

bool write_bytes(i2c_master_dev_handle_t i2c_dev, const uint8_t *data, size_t len)
{
    if (i2c_dev == nullptr) {
        return false;
    }
    return i2c_master_transmit(i2c_dev, data, len, kI2cTimeoutMs) == ESP_OK;
}

bool read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg, uint8_t *data, size_t len)
{
    if (i2c_dev == nullptr) {
        return false;
    }
    return i2c_master_transmit_receive(i2c_dev, &reg, sizeof(reg), data, len, kI2cTimeoutMs) ==
           ESP_OK;
}

} // namespace

bool pcf8563_probe(i2c_master_dev_handle_t i2c_dev)
{
    uint8_t dummy = 0;
    return read_reg(i2c_dev, kControlStatus1Reg, &dummy, 1);
}

bool pcf8563_disable_clkout(i2c_master_dev_handle_t i2c_dev)
{
    const uint8_t payload[] = {
        kClkoutControlReg,
        kClkoutDisabled,
    };
    return write_bytes(i2c_dev, payload, sizeof(payload));
}

bool pcf8563_get_time(i2c_master_dev_handle_t i2c_dev, std::tm &timeinfo)
{
    uint8_t raw[7] = {};
    if (!read_reg(i2c_dev, kTimeReg, raw, sizeof(raw))) {
        return false;
    }

    if ((raw[0] & 0x80U) != 0U) {
        return false; // VL: clock integrity lost.
    }
    const uint8_t masks[] = {0x7F, 0x7F, 0x3F, 0x3F, 0x07, 0x1F, 0xFF};
    for (size_t i = 0; i < sizeof(raw); ++i) {
        if (!valid_bcd(raw[i] & masks[i])) {
            return false;
        }
    }
    std::tm value = {};
    value.tm_sec = bcd_to_dec(raw[0] & 0x7FU);
    value.tm_min = bcd_to_dec(raw[1] & 0x7FU);
    value.tm_hour = bcd_to_dec(raw[2] & 0x3FU);
    value.tm_mday = bcd_to_dec(raw[3] & 0x3FU);
    value.tm_wday = bcd_to_dec(raw[4] & 0x07U);
    value.tm_mon = static_cast<int>(bcd_to_dec(raw[5] & 0x1FU)) - 1;

    const int year = bcd_to_dec(raw[6]);
    value.tm_year = ((raw[5] & 0x80U) != 0U) ? year : (year + 100);
    value.tm_isdst = -1;

    if (!valid_calendar(value)) {
        return false;
    }
    timeinfo = value;

    return true;
}

bool pcf8563_set_time(i2c_master_dev_handle_t i2c_dev, const std::tm &timeinfo)
{
    if (!valid_calendar(timeinfo)) {
        return false;
    }

    const int full_year = timeinfo.tm_year + 1900;
    if (full_year < 1900 || full_year > 2099) {
        return false;
    }

    const uint8_t century_bit = full_year < 2000 ? 0x80U : 0x00U;
    const uint8_t payload[] = {
        kTimeReg,
        dec_to_bcd(static_cast<uint8_t>(timeinfo.tm_sec)),
        dec_to_bcd(static_cast<uint8_t>(timeinfo.tm_min)),
        dec_to_bcd(static_cast<uint8_t>(timeinfo.tm_hour)),
        dec_to_bcd(static_cast<uint8_t>(timeinfo.tm_mday)),
        dec_to_bcd(static_cast<uint8_t>(timeinfo.tm_wday)),
        static_cast<uint8_t>(dec_to_bcd(static_cast<uint8_t>(timeinfo.tm_mon + 1)) | century_bit),
        dec_to_bcd(static_cast<uint8_t>(full_year % 100)),
    };

    return write_bytes(i2c_dev, payload, sizeof(payload));
}
