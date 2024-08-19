CFLAGS = -w -I/usr/include/efi -fpermissive -fPIE -ffreestanding \
		 -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone \
		 -mno-avx -maccumulate-outgoing-args -DGNU_EFI_USE_MS_ABI
LDFLAGS = -shared -Bsymbolic -L/usr/lib \
		  -T/usr/lib/elf_x86_64_efi.lds

all: clean twos_loader.efi

clean:
	@rm -f twos_loader.efi

twos_loader.so: twos_loader.o ldr_util.o

%.o: %.c
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -c $< -o $@

%.so: %.o
	@echo "  LD      $@"
	@$(LD) $(LDFLAGS) /usr/lib/crt0-efi-x86_64.o $^ -o $@ -lefi -lgnuefi

%.efi: %.so
	@echo "  OBJCOPY $@"
	@objcopy -j .text -j .sdata -j .data -j .dynamic -j .rodata -j .rel \
		    -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* \
		    -j .areloc -j .reloc --target efi-app-x86_64 $< $@
	@rm *.o *.so
