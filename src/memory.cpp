#include "../include/memory.h"

namespace rv {

    std::uint8_t Memory::read8(std::uint32_t addr) const {
        const auto it = pages_.find(addr / kPageSize);
        if (it == pages_.end()) {
            return 0;  // memoria nunca escrita
        }
        return it->second[addr % kPageSize];
    }

    std::uint16_t Memory::read16(std::uint32_t addr) const {
        return static_cast<std::uint16_t>(read8(addr) | (read8(addr + 1) << 8));
    }

    std::uint32_t Memory::read32(std::uint32_t addr) const {
        return static_cast<std::uint32_t>(read8(addr)) |
               (static_cast<std::uint32_t>(read8(addr + 1)) << 8) |
               (static_cast<std::uint32_t>(read8(addr + 2)) << 16) |
               (static_cast<std::uint32_t>(read8(addr + 3)) << 24);
    }

    void Memory::write8(std::uint32_t addr, std::uint8_t value) {
        auto [it, inserted] = pages_.try_emplace(addr / kPageSize);
        if (inserted) {
            it->second.fill(0);
        }
        it->second[addr % kPageSize] = value;
    }

    void Memory::write16(std::uint32_t addr, std::uint16_t value) {
        write8(addr, static_cast<std::uint8_t>(value & 0xFF));
        write8(addr + 1, static_cast<std::uint8_t>((value >> 8) & 0xFF));
    }

    void Memory::write32(std::uint32_t addr, std::uint32_t value) {
        write8(addr, static_cast<std::uint8_t>(value & 0xFF));
        write8(addr + 1, static_cast<std::uint8_t>((value >> 8) & 0xFF));
        write8(addr + 2, static_cast<std::uint8_t>((value >> 16) & 0xFF));
        write8(addr + 3, static_cast<std::uint8_t>((value >> 24) & 0xFF));
    }

    void Memory::clear() {
        pages_.clear();
    }

    void Memory::loadBytes(std::uint32_t base, const std::vector<std::uint8_t>& data) {
        for (std::size_t i = 0; i < data.size(); ++i) {
            write8(base + static_cast<std::uint32_t>(i), data[i]);
        }
    }

}