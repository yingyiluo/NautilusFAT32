cmd_src/acpi/acpica/built-in.o :=  ld -z max-page-size=0x1000 -melf_x86_64 -dp  -r -o src/acpi/acpica/built-in.o src/acpi/acpica/acpi.o
