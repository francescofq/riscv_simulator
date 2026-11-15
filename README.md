# Ejecutar el simulador RISC-V

Desde la carpeta raíz del proyecto, ejecutar:

```bash
cmake -S . -B build
cmake --build build
./build/riscv-sim
```

Una vez iniciado el simulador, cargar un archivo `.bin` ubicado en la carpeta del proyecto y ejecutarlo:

```text
load riscv_test.bin
run
```

Para iniciar nuevamente el simulador sin recompilar:

```bash
./build/riscv-sim
```
