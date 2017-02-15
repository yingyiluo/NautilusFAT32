cmd_src/test/built-in.o :=  ld -z max-page-size=0x1000 -melf_x86_64 -dp  -r -o src/test/built-in.o src/test/ipi.o
