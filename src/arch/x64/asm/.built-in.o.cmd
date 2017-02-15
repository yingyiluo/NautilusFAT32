cmd_src/arch/x64/asm/built-in.o :=  ld -z max-page-size=0x1000 -melf_x86_64 -dp  -r -o src/arch/x64/asm/built-in.o src/arch/x64/asm/boot.o
