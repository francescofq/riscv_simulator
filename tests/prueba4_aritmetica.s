.global _start
_start:
    addi x1, x0, 5        # x1 = 5
    addi x2, x0, 7        # x2 = 7
    add  x3, x1, x2       # x3 = 12
    sub  x4, x2, x1       # x4 = 2
    slli x5, x3, 2        # x5 = 48 (0x30)
    lui  x6, 1            # x6 = 0x00001000
    sw   x5, 0(x6)        # mem[0x1000] = 0x30
    lw   x7, 0(x6)        # x7 = 0x30
    addi x8, x0, -1       # x8 = 0xFFFFFFFF
    sb   x8, 4(x6)        # mem[0x1004] = 0xFF
    lb   x9, 4(x6)        # x9 = 0xFFFFFFFF (con signo)
    lbu  x10, 4(x6)       # x10 = 0x000000FF (con ceros)
    addi x0, x0, 99       # intento de escribir x0: debe seguir en 0
fin:
    jal  x0, fin          # bucle infinito