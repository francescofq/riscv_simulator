#pragma once
#include <bits/stdc++.h>

namespace rv {

    enum class Op : std::uint8_t {
        // Loads
        LB, LH, LW, LBU, LHU,
        // Operaciones con inmediato
        ADDI, SLLI, SLTI, SLTIU, XORI, SRLI, SRAI, ORI, ANDI,
        // Stores
        SB, SH, SW,
        // Operaciones entre registros
        ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
        // Upper immediate
        LUI, AUIPC,
        // Branches
        BEQ, BNE, BLT, BGE, BLTU, BGEU,
        // Jumps
        JAL, JALR,
        // Cualquier otra palabra de 32 bits
        ILLEGAL
    };

    enum class Format : std::uint8_t { R, I, S, B, U, J, Invalid };

    struct Instruction {
        Op op = Op::ILLEGAL;
        Format format = Format::Invalid;
        std::uint8_t rd = 0;
        std::uint8_t rs1 = 0;
        std::uint8_t rs2 = 0;
        std::int32_t imm = 0;
        std::uint32_t raw = 0;
    };

    constexpr std::int32_t signExtend(std::uint32_t value, unsigned width) {
        const std::uint32_t signBit = 1u << (width - 1);
        return static_cast<std::int32_t>((value ^ signBit) - signBit);
    }

    Instruction decode(std::uint32_t word);
    std::string_view mnemonic(Op op);
    std::string disassemble(const Instruction& inst, std::uint32_t pc);

}