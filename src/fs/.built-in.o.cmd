cmd_src/fs/built-in.o :=  ld -z max-page-size=0x1000 -melf_x86_64 -dp  -r -o src/fs/built-in.o src/fs/fat32/built-in.o
