cmd_src/nautilus/mm/built-in.o :=  ld -z max-page-size=0x1000 -melf_x86_64 -dp  -r -o src/nautilus/mm/built-in.o src/nautilus/mm/boot_mm.o src/nautilus/mm/buddy.o src/nautilus/mm/kmem.o
