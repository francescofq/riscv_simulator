#pragma once
#include <bits/stdc++.h>
#include "decoder.h"
#include "memory.h"

namespace rv {

class Cpu {
public:
    static constexpr std::size_t kMaxProgramBytes = 16u * 1024u * 1024u;

    struct StepResult {
        std::uint32_t pcBefore = 0;
        std::uint32_t pcAfter = 0;
        Instruction inst;
    };

    struct LoadInfo {
        std::size_t bytes = 0;
        bool paddedTo4 = false;
    };

    LoadInfo loadProgram(const std::string& path);

    void reset();

    StepResult step();

    std::optional<Instruction> peek() const;

    bool isLoaded() const { return loaded_; }
    bool isHalted() const { return halted_; }
    const std::string& haltReason() const { return haltReason_; }
    const std::string& programPath() const { return programPath_; }
    std::uint32_t programSize() const { return programEnd_; }

    std::uint32_t pc() const { return pc_; }
    std::uint32_t reg(unsigned index) const { return regs_.at(index); }
    const Memory& memory() const { return mem_; }

private:
    std::uint32_t execute(const Instruction& inst);
    void writeReg(unsigned index, std::uint32_t value);

    std::uint32_t pc_ = 0;
    std::array<std::uint32_t, 32> regs_{};
    Memory mem_;

    std::vector<std::uint8_t> image_;
    std::uint32_t programEnd_ = 0;
    std::string programPath_;
    bool loaded_ = false;
    bool halted_ = false;
    std::string haltReason_;
};

}