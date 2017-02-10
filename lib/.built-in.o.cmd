cmd_lib/built-in.o :=  ld -z max-page-size=0x1000 -melf_x86_64 -dp  -r -o lib/built-in.o lib/bitmap.o
