#pragma once
#include <bits/stdc++.h>

namespace rv {

    class Memory {
    public:
        static constexpr std::uint32_t kPageSize = 4096;

        std::uint8_t read8(std::uint32_t addr) const;
        std::uint16_t read16(std::uint32_t addr) const;
        std::uint32_t read32(std::uint32_t addr) const;

        void write8(std::uint32_t addr, std::uint8_t value);
        void write16(std::uint32_t addr, std::uint16_t value);
        void write32(std::uint32_t addr, std::uint32_t value);

        void clear();
        void loadBytes(std::uint32_t base, const std::vector<std::uint8_t>& data);

    private:
        using Page = std::array<std::uint8_t, kPageSize>;
        std::unordered_map<std::uint32_t, Page> pages_;
    };

}