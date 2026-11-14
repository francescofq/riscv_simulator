#include <bits/stdc++.h>
#include "../include/decoder.h"
#include "../include/common.h"

namespace rv {

namespace {

constexpr std::uint32_t bits(std::uint32_t w, unsigned hi, unsigned lo) {
    return (w >> lo) & ((1u << (hi - lo + 1)) - 1u);
}

constexpr std::uint32_t kOpLoad = 0x03;
constexpr std::uint32_t kOpImm = 0x13;
constexpr std::uint32_t kOpAuipc = 0x17;
constexpr std::uint32_t kOpStore = 0x23;
constexpr std::uint32_t kOpReg = 0x33;
constexpr std::uint32_t kOpLui = 0x37;
constexpr std::uint32_t kOpBranch = 0x63;
constexpr std::uint32_t kOpJalr = 0x67;
constexpr std::uint32_t kOpJal = 0x6F;


std::int32_t immI(std::uint32_t w) {
    return signExtend(bits(w, 31, 20), 12);
}

std::int32_t immS(std::uint32_t w) {
    return signExtend((bits(w, 31, 25) << 5) | bits(w, 11, 7), 12);
}

// imm[12|10:5|4:1|11], el bit 0 siempre es 0
std::int32_t immB(std::uint32_t w) {
    return signExtend((bits(w, 31, 31) << 12) | (bits(w, 7, 7) << 11) |
                          (bits(w, 30, 25) << 5) | (bits(w, 11, 8) << 1),
                      13);
}

// imm[31:12] en las posiciones altas, 12 ceros abajo
std::int32_t immU(std::uint32_t w) {
    return static_cast<std::int32_t>(w & 0xFFFFF000u);
}

// imm[20|10:1|11|19:12], el bit 0 siempre es 0
std::int32_t immJ(std::uint32_t w) {
    return signExtend((bits(w, 31, 31) << 20) | (bits(w, 19, 12) << 12) |
                          (bits(w, 20, 20) << 11) | (bits(w, 30, 21) << 1),
                      21);
}

bool isLoad(Op op) {
    return op == Op::LB || op == Op::LH || op == Op::LW || op == Op::LBU || op == Op::LHU;
}

std::string reg(unsigned n) {
    return "x" + std::to_string(n);
}

}

Instruction decode(std::uint32_t w) {
    Instruction in;
    in.raw = w;

    const std::uint32_t opcode = bits(w, 6, 0);
    const std::uint32_t funct3 = bits(w, 14, 12);
    const std::uint32_t funct7 = bits(w, 31, 25);

    Op op = Op::ILLEGAL;
    Format fmt = Format::Invalid;

    switch (opcode) {
        case kOpLui:
            op = Op::LUI;
            fmt = Format::U;
            break;
        case kOpAuipc:
            op = Op::AUIPC;
            fmt = Format::U;
            break;
        case kOpJal:
            op = Op::JAL;
            fmt = Format::J;
            break;
        case kOpJalr:
            if (funct3 == 0) {
                op = Op::JALR;
                fmt = Format::I;
            }
            break;
        case kOpBranch:
            fmt = Format::B;
            switch (funct3) {
                case 0: op = Op::BEQ; break;
                case 1: op = Op::BNE; break;
                case 4: op = Op::BLT; break;
                case 5: op = Op::BGE; break;
                case 6: op = Op::BLTU; break;
                case 7: op = Op::BGEU; break;
                default: fmt = Format::Invalid; break;
            }
            break;
        case kOpLoad:
            fmt = Format::I;
            switch (funct3) {
                case 0: op = Op::LB; break;
                case 1: op = Op::LH; break;
                case 2: op = Op::LW; break;
                case 4: op = Op::LBU; break;
                case 5: op = Op::LHU; break;
                default: fmt = Format::Invalid; break;
            }
            break;
        case kOpStore:
            fmt = Format::S;
            switch (funct3) {
                case 0: op = Op::SB; break;
                case 1: op = Op::SH; break;
                case 2: op = Op::SW; break;
                default: fmt = Format::Invalid; break;
            }
            break;
        case kOpImm:
            fmt = Format::I;
            switch (funct3) {
                case 0: op = Op::ADDI; break;
                case 2: op = Op::SLTI; break;
                case 3: op = Op::SLTIU; break;
                case 4: op = Op::XORI; break;
                case 6: op = Op::ORI; break;
                case 7: op = Op::ANDI; break;
                case 1:  // slli: funct7 debe ser 0000000
                    if (funct7 == 0x00) op = Op::SLLI;
                    break;
                case 5:  // srli (0000000) o srai (0100000)
                    if (funct7 == 0x00) op = Op::SRLI;
                    else if (funct7 == 0x20) op = Op::SRAI;
                    break;
                default: break;
            }
            if (op == Op::ILLEGAL) fmt = Format::Invalid;
            break;
        case kOpReg:
            fmt = Format::R;
            if (funct7 == 0x00) {
                switch (funct3) {
                    case 0: op = Op::ADD; break;
                    case 1: op = Op::SLL; break;
                    case 2: op = Op::SLT; break;
                    case 3: op = Op::SLTU; break;
                    case 4: op = Op::XOR; break;
                    case 5: op = Op::SRL; break;
                    case 6: op = Op::OR; break;
                    case 7: op = Op::AND; break;
                    default: break;
                }
            } else if (funct7 == 0x20) {
                if (funct3 == 0) op = Op::SUB;
                else if (funct3 == 5) op = Op::SRA;
            }
            if (op == Op::ILLEGAL) fmt = Format::Invalid;
            break;
        default:
            break;
    }

    if (op == Op::ILLEGAL) {
        return in;
    }

    in.op = op;
    in.format = fmt;

    switch (fmt) {
        case Format::R:
            in.rd = static_cast<std::uint8_t>(bits(w, 11, 7));
            in.rs1 = static_cast<std::uint8_t>(bits(w, 19, 15));
            in.rs2 = static_cast<std::uint8_t>(bits(w, 24, 20));
            break;
        case Format::I:
            in.rd = static_cast<std::uint8_t>(bits(w, 11, 7));
            in.rs1 = static_cast<std::uint8_t>(bits(w, 19, 15));
            in.imm = (op == Op::SLLI || op == Op::SRLI || op == Op::SRAI)
                         ? static_cast<std::int32_t>(bits(w, 24, 20))  // shamt
                         : immI(w);
            break;
        case Format::S:
            in.rs1 = static_cast<std::uint8_t>(bits(w, 19, 15));
            in.rs2 = static_cast<std::uint8_t>(bits(w, 24, 20));
            in.imm = immS(w);
            break;
        case Format::B:
            in.rs1 = static_cast<std::uint8_t>(bits(w, 19, 15));
            in.rs2 = static_cast<std::uint8_t>(bits(w, 24, 20));
            in.imm = immB(w);
            break;
        case Format::U:
            in.rd = static_cast<std::uint8_t>(bits(w, 11, 7));
            in.imm = immU(w);
            break;
        case Format::J:
            in.rd = static_cast<std::uint8_t>(bits(w, 11, 7));
            in.imm = immJ(w);
            break;
        case Format::Invalid:
            break;
    }
    return in;
}

std::string_view mnemonic(Op op) {
    switch (op) {
        case Op::LB: return "lb";
        case Op::LH: return "lh";
        case Op::LW: return "lw";
        case Op::LBU: return "lbu";
        case Op::LHU: return "lhu";
        case Op::ADDI: return "addi";
        case Op::SLLI: return "slli";
        case Op::SLTI: return "slti";
        case Op::SLTIU: return "sltiu";
        case Op::XORI: return "xori";
        case Op::SRLI: return "srli";
        case Op::SRAI: return "srai";
        case Op::ORI: return "ori";
        case Op::ANDI: return "andi";
        case Op::SB: return "sb";
        case Op::SH: return "sh";
        case Op::SW: return "sw";
        case Op::ADD: return "add";
        case Op::SUB: return "sub";
        case Op::SLL: return "sll";
        case Op::SLT: return "slt";
        case Op::SLTU: return "sltu";
        case Op::XOR: return "xor";
        case Op::SRL: return "srl";
        case Op::SRA: return "sra";
        case Op::OR: return "or";
        case Op::AND: return "and";
        case Op::LUI: return "lui";
        case Op::AUIPC: return "auipc";
        case Op::BEQ: return "beq";
        case Op::BNE: return "bne";
        case Op::BLT: return "blt";
        case Op::BGE: return "bge";
        case Op::BLTU: return "bltu";
        case Op::BGEU: return "bgeu";
        case Op::JAL: return "jal";
        case Op::JALR: return "jalr";
        case Op::ILLEGAL: return "???";
    }
    return "???";
}

std::string disassemble(const Instruction& in, std::uint32_t pc) {
    if (in.op == Op::ILLEGAL) {
        return "<instruccion no reconocida " + hex32(in.raw) + ">";
    }

    const std::string name(mnemonic(in.op));
    const auto uimm = static_cast<std::uint32_t>(in.imm);

    switch (in.format) {
        case Format::R:
            return name + " " + reg(in.rd) + ", " + reg(in.rs1) + ", " + reg(in.rs2);
        case Format::I:
            if (isLoad(in.op) || in.op == Op::JALR) {
                return name + " " + reg(in.rd) + ", " + std::to_string(in.imm) + "(" +
                       reg(in.rs1) + ")";
            }
            return name + " " + reg(in.rd) + ", " + reg(in.rs1) + ", " + std::to_string(in.imm);
        case Format::S:
            return name + " " + reg(in.rs2) + ", " + std::to_string(in.imm) + "(" +
                   reg(in.rs1) + ")";
        case Format::B:
            return name + " " + reg(in.rs1) + ", " + reg(in.rs2) + ", " + hex32(pc + uimm);
        case Format::U:
            return name + " " + reg(in.rd) + ", " + hex32(uimm >> 12);
        case Format::J:
            return name + " " + reg(in.rd) + ", " + hex32(pc + uimm);
        case Format::Invalid:
            break;
    }
    return "???";
}

}