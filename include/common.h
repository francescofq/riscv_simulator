#pragma once
#include <bits/stdc++.h>

namespace rv {

    class SimError : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    inline std::string hex32(std::uint32_t value) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "0x%08X", static_cast<unsigned>(value));
        return buf;
    }

    inline std::string hex8(std::uint8_t value) {
        char buf[8];
        std::snprintf(buf, sizeof buf, "%02X", static_cast<unsigned>(value));
        return buf;
    }

}