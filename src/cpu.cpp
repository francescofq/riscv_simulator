#include <bits/stdc++.h>
#include "../include/cpu.h"
#include "../include/common.h"

namespace rv {

namespace {

bool looksLikeHexText(const std::vector<std::uint8_t>& data) {
    if (data.size() < 8) {
        return false;
    }
    return std::all_of(data.begin(), data.end(), [](std::uint8_t c) {
        return std::isxdigit(c) != 0 || c == ' ' || c == '\n' || c == '\r' || c == '\t' ||
               c == 'x' || c == 'X';
    });
}

}

Cpu::LoadInfo Cpu::loadProgram(const std::string& path) {
    namespace fs = std::filesystem;

    std::error_code ec;
    if (!fs::exists(path, ec)) {
        throw SimError("El archivo '" + path + "' no existe.");
    }
    if (!fs::is_regular_file(path, ec)) {
        throw SimError("'" + path + "' no es un archivo regular.");
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw SimError("No se pudo abrir '" + path + "' (revise los permisos).");
    }
    const std::streamsize size = file.tellg();
    if (size < 0) {
        throw SimError("No se pudo determinar el tamano de '" + path + "'.");
    }
    if (size == 0) {
        throw SimError("El archivo '" + path + "' esta vacio.");
    }
    if (static_cast<std::uintmax_t>(size) > kMaxProgramBytes) {
        throw SimError("El archivo es demasiado grande (maximo 16 MiB).");
    }

    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), size);
    if (!file) {
        throw SimError("Error de lectura al leer '" + path + "'.");
    }
    if (looksLikeHexText(data)) {
        throw SimError(
            "'" + path +
            "' parece un archivo de texto hexadecimal, no un binario crudo. "
            "En CPUlator exporte con formato Raw.");
    }

    // Todo fue bien: recién ahora se modifica el estado del simulador.
    LoadInfo info;
    info.bytes = data.size();
    info.paddedTo4 = (data.size() % 4) != 0;

    image_ = std::move(data);
    programEnd_ = static_cast<std::uint32_t>((image_.size() + 3) & ~static_cast<std::size_t>(3));
    programPath_ = path;
    loaded_ = true;
    reset();
    return info;
}

void Cpu::reset() {
    mem_.clear();
    regs_.fill(0);
    pc_ = 0;
    halted_ = false;
    haltReason_.clear();
    if (loaded_) {
        mem_.loadBytes(0x00000000, image_);
    }
}

Cpu::StepResult Cpu::step() {
    if (!loaded_) {
        throw SimError("No hay ningun programa cargado. Use: load archivo.bin");
    }
    if (halted_) {
        throw SimError("La ejecucion esta detenida (" + haltReason_ +
                       "). Use 'reset' para reiniciar.");
    }

    try {
        StepResult result;
        result.pcBefore = pc_;

        // --- FETCH ---
        if ((pc_ & 3u) != 0) {
            throw SimError("PC desalineado: " + hex32(pc_) + " (debe ser multiplo de 4).");
        }
        if (pc_ >= programEnd_) {
            throw SimError("Ejecucion fuera del programa: PC = " + hex32(pc_) +
                           ", el codigo cargado termina en " + hex32(programEnd_) + ".");
        }
        const std::uint32_t word = mem_.read32(pc_);

        // --- DECODE ---
        result.inst = decode(word);
        if (result.inst.op == Op::ILLEGAL) {
            throw SimError("Instruccion no reconocida " + hex32(word) + " en PC = " +
                           hex32(pc_) + ".");
        }

        // --- EXECUTE + actualización de registros/memoria/PC ---
        pc_ = execute(result.inst);
        result.pcAfter = pc_;
        return result;
    } catch (const SimError& e) {
        halted_ = true;  // el PC se queda en la instrucción que falló
        haltReason_ = e.what();
        throw;
    }
}

std::optional<Instruction> Cpu::peek() const {
    if (!loaded_ || halted_ || (pc_ & 3u) != 0 || pc_ >= programEnd_) {
        return std::nullopt;
    }
    return decode(mem_.read32(pc_));
}

void Cpu::writeReg(unsigned index, std::uint32_t value) {
    if (index != 0) {  // x0 está cableado a cero: las escrituras se ignoran
        regs_[index] = value;
    }
}

std::uint32_t Cpu::execute(const Instruction& in) {
    const std::uint32_t a = regs_[in.rs1];
    const std::uint32_t b = regs_[in.rs2];
    const auto sa = static_cast<std::int32_t>(a);  // misma palabra vista con signo
    const auto sb = static_cast<std::int32_t>(b);
    const auto imm = static_cast<std::uint32_t>(in.imm);  // inmediato ya con signo extendido
    const auto shamt = static_cast<unsigned>(in.imm) & 31u;

    std::uint32_t nextPc = pc_ + 4;

    const auto jumpTo = [&](std::uint32_t target) {
        if ((target & 3u) != 0) {
            throw SimError("Salto a direccion desalineada " + hex32(target) +
                           " (sin extension C, el destino debe ser multiplo de 4).");
        }
        nextPc = target;
    };
    const auto branchIf = [&](bool taken) {
        if (taken) {
            jumpTo(pc_ + imm);
        }
    };

    switch (in.op) {
        // ----- Loads: dirección = rs1 + imm -----
        case Op::LB:
            writeReg(in.rd, static_cast<std::uint32_t>(signExtend(mem_.read8(a + imm), 8)));
            break;
        case Op::LH:
            writeReg(in.rd, static_cast<std::uint32_t>(signExtend(mem_.read16(a + imm), 16)));
            break;
        case Op::LW:
            writeReg(in.rd, mem_.read32(a + imm));
            break;
        case Op::LBU:
            writeReg(in.rd, mem_.read8(a + imm));  // extensión con ceros
            break;
        case Op::LHU:
            writeReg(in.rd, mem_.read16(a + imm));
            break;

        // ----- Operaciones con inmediato -----
        case Op::ADDI:  writeReg(in.rd, a + imm); break;
        case Op::SLTI:  writeReg(in.rd, sa < in.imm ? 1u : 0u); break;
        case Op::SLTIU: writeReg(in.rd, a < imm ? 1u : 0u); break;
        case Op::XORI:  writeReg(in.rd, a ^ imm); break;
        case Op::ORI:   writeReg(in.rd, a | imm); break;
        case Op::ANDI:  writeReg(in.rd, a & imm); break;
        case Op::SLLI:  writeReg(in.rd, a << shamt); break;
        case Op::SRLI:  writeReg(in.rd, a >> shamt); break;
        case Op::SRAI:  writeReg(in.rd, static_cast<std::uint32_t>(sa >> shamt)); break;

        // ----- Stores: dirección = rs1 + imm, el dato viene de rs2 -----
        case Op::SB: mem_.write8(a + imm, static_cast<std::uint8_t>(b & 0xFFu)); break;
        case Op::SH: mem_.write16(a + imm, static_cast<std::uint16_t>(b & 0xFFFFu)); break;
        case Op::SW: mem_.write32(a + imm, b); break;

        // ----- Operaciones entre registros -----
        case Op::ADD:  writeReg(in.rd, a + b); break;
        case Op::SUB:  writeReg(in.rd, a - b); break;
        case Op::SLL:  writeReg(in.rd, a << (b & 31u)); break;
        case Op::SLT:  writeReg(in.rd, sa < sb ? 1u : 0u); break;
        case Op::SLTU: writeReg(in.rd, a < b ? 1u : 0u); break;
        case Op::XOR:  writeReg(in.rd, a ^ b); break;
        case Op::SRL:  writeReg(in.rd, a >> (b & 31u)); break;
        case Op::SRA:  writeReg(in.rd, static_cast<std::uint32_t>(sa >> (b & 31u))); break;
        case Op::OR:   writeReg(in.rd, a | b); break;
        case Op::AND:  writeReg(in.rd, a & b); break;

        // ----- Upper immediate -----
        case Op::LUI:   writeReg(in.rd, imm); break;
        case Op::AUIPC: writeReg(in.rd, pc_ + imm); break;

        // ----- Branches: destino = PC + imm (si se toma) -----
        case Op::BEQ:  branchIf(a == b); break;
        case Op::BNE:  branchIf(a != b); break;
        case Op::BLT:  branchIf(sa < sb); break;
        case Op::BGE:  branchIf(sa >= sb); break;
        case Op::BLTU: branchIf(a < b); break;
        case Op::BGEU: branchIf(a >= b); break;

        // ----- Jumps: primero se valida el destino y luego se escribe rd = PC + 4 -----
        case Op::JAL:
            jumpTo(pc_ + imm);
            writeReg(in.rd, pc_ + 4);
            break;
        case Op::JALR:
            jumpTo((a + imm) & ~1u);  // el bit 0 del destino se pone en 0
            writeReg(in.rd, pc_ + 4);
            break;

        case Op::ILLEGAL:
            throw SimError("Instruccion no reconocida " + hex32(in.raw) + ".");
    }

    return nextPc;
}

}