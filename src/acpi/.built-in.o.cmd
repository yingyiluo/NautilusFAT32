cmd_src/acpi/built-in.o :=  ld -z max-page-size=0x1000 -melf_x86_64 -dp  -r -o src/acpi/built-in.o src/acpi/osl.o src/acpi/tables.o src/acpi/acpica/built-in.o
