cmd_src/fs/fat32/built-in.o :=  ld -z max-page-size=0x1000 -melf_x86_64 -dp  -r -o src/fs/fat32/built-in.o src/fs/fat32/fat32.o
