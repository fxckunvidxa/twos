EFILIB = gnu-efi/x86_64/lib
EFILIB2 = gnu-efi/x86_64/gnuefi
EFIINC = gnu-efi/inc

CFLAGS = -w -I$(EFIINC) -fpermissive -fpic -ffreestanding \
		 -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone
LDFLAGS = -shared -Bsymbolic -L$(EFILIB) -L$(EFILIB2) \
		  -Tgnu-efi/gnuefi/elf_x86_64_efi.lds $(EFILIB2)/crt0-efi-x86_64.o

all: clean twos_loader.efi

clean:
	@rm -f twos_loader.efi

%.o: %.c
	@echo -e "  CC\t$@"
	@$(CC) $(CFLAGS) -c $< -o $@

twos_loader.so: twos_loader.o ldr_util.o
	@echo -e "  LD\t$@"
	@$(LD) $(LDFLAGS) $^ -o $@ -lgnuefi -lefi

twos_loader.efi: twos_loader.so
	@echo -e "  OBJCOPY $@"
	@objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 $< $@
	@rm *.o *.so
