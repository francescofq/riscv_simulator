#include <bits/stdc++.h>
#include "../include/cli.h"
#include "../include/common.h"

namespace rv {

namespace {

constexpr std::uint32_t kDefaultRunLimit = 100000;
constexpr std::uint32_t kMaxMemDump = 4096;

constexpr std::array<const char*, 32> kAbiNames = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::optional<std::uint32_t> parseU32(std::string_view text) {
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) {
        text.remove_prefix(2);
        base = 16;
    }
    if (text.empty()) {
        return std::nullopt;
    }
    std::uint32_t value = 0;
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<unsigned> parseRegister(const std::string& text) {
    const std::string name = toLower(text);
    if (name == "fp") {
        return 8u;
    }
    if (name.size() >= 2 && name[0] == 'x') {
        const auto n = parseU32(std::string_view(name).substr(1));
        if (n && *n < 32) {
            return *n;
        }
        return std::nullopt;
    }
    for (unsigned i = 0; i < kAbiNames.size(); ++i) {
        if (name == kAbiNames[i]) {
            return i;
        }
    }
    return std::nullopt;
}

std::string regLabel(unsigned index) {
    return "x" + std::to_string(index) + " (" + kAbiNames[index] + ")";
}

std::string regLine(const Cpu& cpu, unsigned index) {
    std::ostringstream os;
    os << std::left << std::setw(10) << regLabel(index) << " = " << hex32(cpu.reg(index));
    return os.str();
}

}

void Cli::run(const std::string& initialProgram) {
    std::cout << "Simulador RISC-V RV32I. Escriba 'help' para ver los comandos.\n";

    if (!initialProgram.empty()) {
        cmdLoad({"load", initialProgram});
    }

    std::string line;
    while (!quit_) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        std::istringstream iss(line);
        Args args;
        for (std::string token; iss >> token;) {
            args.push_back(token);
        }
        if (args.empty()) {
            continue;
        }

        try {
            dispatch(args);
        } catch (const SimError& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
}

void Cli::dispatch(const Args& args) {
    const std::string cmd = toLower(args[0]);

    if (cmd == "help") cmdHelp();
    else if (cmd == "load") cmdLoad(args);
    else if (cmd == "step") cmdStep(args);
    else if (cmd == "run") cmdRun(args);
    else if (cmd == "pc") cmdPc(args);
    else if (cmd == "regs") cmdRegs(args);
    else if (cmd == "mem") cmdMem(args);
    else if (cmd == "reset") cmdReset(args);
    else if (cmd == "exit" || cmd == "quit") quit_ = true;
    else std::cout << "Comando desconocido: '" << args[0] << "'. Escriba 'help'.\n";
}


void Cli::cmdHelp() const {
    std::cout <<
        "Comandos disponibles:\n"
        "  help                  Muestra esta ayuda\n"
        "  load archivo.bin      Carga un programa (binario crudo) en la direccion 0x00000000\n"
        "  step                  Ejecuta una instruccion\n"
        "  run [max]             Ejecuta hasta 'max' instrucciones (por defecto 100000);\n"
        "                        se detiene ante un error o un salto a si mismo (fin del programa)\n"
        "  pc                    Muestra el Program Counter\n"
        "  regs                  Muestra los 32 registros\n"
        "  regs x5               Muestra un registro (acepta x5, t0, a0, sp, ...)\n"
        "  mem 0x1000 0x100F     Muestra un rango de memoria (extremos inclusivos, max 4096 bytes)\n"
        "  reset                 Reinicia el programa cargado al estado inicial\n"
        "  exit                  Sale del simulador\n";
}

void Cli::cmdLoad(const Args& args) {
    if (args.size() < 2) {
        std::cout << "Uso: load archivo.bin\n";
        return;
    }
    // Se une el resto de la línea para permitir rutas con espacios.
    std::string path = args[1];
    for (std::size_t i = 2; i < args.size(); ++i) {
        path += " " + args[i];
    }

    const Cpu::LoadInfo info = cpu_.loadProgram(path);
    std::cout << "Programa cargado correctamente (" << info.bytes << " bytes, "
              << (info.bytes + 3) / 4 << " palabras de 32 bits).\n";
    if (info.paddedTo4) {
        std::cout << "Aviso: el tamano no es multiplo de 4; se completo con ceros.\n";
    }
}

void Cli::cmdStep(const Args& args) {
    if (args.size() != 1) {
        std::cout << "Uso: step\n";
        return;
    }
    const Cpu::StepResult r = cpu_.step();
    std::cout << "Instruccion ejecutada: " << disassemble(r.inst, r.pcBefore) << "   (PC "
              << hex32(r.pcBefore) << " -> " << hex32(r.pcAfter) << ")\n";
}

void Cli::cmdRun(const Args& args) {
    if (args.size() > 2) {
        std::cout << "Uso: run [max_instrucciones]\n";
        return;
    }
    std::uint32_t limit = kDefaultRunLimit;
    if (args.size() == 2) {
        const auto parsed = parseU32(args[1]);
        if (!parsed || *parsed == 0) {
            std::cout << "Error: limite invalido '" << args[1] << "'.\n";
            return;
        }
        limit = *parsed;
    }

    std::uint32_t count = 0;
    while (count < limit) {
        Cpu::StepResult r;
        try {
            r = cpu_.step();
        } catch (const SimError& e) {
            std::cout << "Error: " << e.what() << "\n";
            std::cout << "Se ejecutaron " << count << " instrucciones antes del error.\n";
            return;
        }
        ++count;

        if (r.pcAfter == r.pcBefore) {  // la instruccion salta a si misma: bucle infinito
            std::cout << "Fin de ejecucion: la instruccion en " << hex32(r.pcBefore) << " ("
                      << disassemble(r.inst, r.pcBefore) << ") salta a si misma.\n"
                      << "Se ejecutaron " << count << " instrucciones. PC = " << hex32(cpu_.pc())
                      << "\n";
            return;
        }
    }
    std::cout << "Limite de " << limit << " instrucciones alcanzado. PC = " << hex32(cpu_.pc())
              << "\n";
}

void Cli::cmdPc(const Args& args) const {
    if (args.size() != 1) {
        std::cout << "Uso: pc\n";
        return;
    }
    std::cout << "PC = " << hex32(cpu_.pc());
    if (const auto next = cpu_.peek()) {
        std::cout << "   (siguiente: " << disassemble(*next, cpu_.pc()) << ")";
    }
    std::cout << "\n";
}

void Cli::cmdRegs(const Args& args) const {
    if (args.size() > 2) {
        std::cout << "Uso: regs [registro]\n";
        return;
    }
    if (args.size() == 2) {
        const auto idx = parseRegister(args[1]);
        if (!idx) {
            std::cout << "Error: registro invalido '" << args[1] << "' (use x0..x31 o nombre ABI).\n";
            return;
        }
        const std::uint32_t v = cpu_.reg(*idx);
        std::cout << regLabel(*idx) << " = " << hex32(v) << "   (" << static_cast<std::int32_t>(v)
                  << ")\n";
        return;
    }
    for (unsigned i = 0; i < 16; ++i) {  // dos columnas: x0..x15 | x16..x31
        std::cout << regLine(cpu_, i) << "    " << regLine(cpu_, i + 16) << "\n";
    }
}

void Cli::cmdMem(const Args& args) const {
    if (args.size() != 3) {
        std::cout << "Uso: mem inicio fin   (ej: mem 0x1000 0x100F)\n";
        return;
    }
    const auto start = parseU32(args[1]);
    const auto end = parseU32(args[2]);
    if (!start || !end) {
        std::cout << "Error: direccion invalida (use hexadecimal 0x... o decimal, maximo 0xFFFFFFFF).\n";
        return;
    }
    if (*start > *end) {
        std::cout << "Error: la direccion inicial es mayor que la final.\n";
        return;
    }
    const std::uint64_t count = static_cast<std::uint64_t>(*end) - *start + 1;
    if (count > kMaxMemDump) {
        std::cout << "Error: rango demasiado grande (" << count << " bytes, maximo " << kMaxMemDump
                  << ").\n";
        return;
    }

    // uint64_t para que el bucle no se desborde cuando end == 0xFFFFFFFF
    for (std::uint64_t row = *start; row <= *end; row += 16) {
        std::cout << hex32(static_cast<std::uint32_t>(row)) << ":";
        for (std::uint64_t addr = row; addr < row + 16 && addr <= *end; ++addr) {
            std::cout << ' ' << hex8(cpu_.memory().read8(static_cast<std::uint32_t>(addr)));
        }
        std::cout << "\n";
    }
}

void Cli::cmdReset(const Args& args) {
    if (args.size() != 1) {
        std::cout << "Uso: reset\n";
        return;
    }
    if (!cpu_.isLoaded()) {
        std::cout << "No hay ningun programa cargado. Use: load archivo.bin\n";
        return;
    }
    cpu_.reset();
    std::cout << "Simulador reiniciado: PC = 0x00000000, registros en cero y memoria restaurada.\n";
}

}