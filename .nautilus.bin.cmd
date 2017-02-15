cmd_nautilus.bin := ld -z max-page-size=0x1000 -melf_x86_64 -dp  -o nautilus.bin -T link/nautilus.ld  src/built-in.o --start-group  lib/built-in.o --end-group
