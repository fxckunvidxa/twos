CFLAGS = -I"./include" -pipe -m64 -O2 -w -fpermissive -ffreestanding \
		 -fno-pic -fno-stack-protector -mcmodel=kernel -mno-red-zone \
		 -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx
LDFLAGS = -T link.ld -static
ASFLAGS = --64

SOURCES = $(shell find ./src -type f \( -name "*.c" -o -name "*.s" \) -printf "build/%P\n")
OBJS = $(patsubst %.c, %.o, $(patsubst %.s, %.o, $(SOURCES)))

.PHONY: run image clean rundbg

all: clean image

clean:
	@rm -rf build
	@mkdir -p build/arch/x86_64 build/bin build/sys build/misc \
			build/video/vt_font

build/bin/twos_kernel: $(OBJS)
	@echo "  LD      $@"
	@$(LD) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	@echo "  CC      $@"
	@$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: src/%.s
	@echo "  AS      $@"
	@$(AS) $(ASFLAGS) $< -o $@

image: build/bin/twos_kernel
	@mkdir -p image/root/EFI/Boot image/root/twos/data
	@cp -f build/bin/twos_kernel image/root/twos/kernel
	@cp -f loader/twos_loader.efi image/root/EFI/Boot/bootx64.efi
	@cd image && ./make_image.sh

QEMUARGS = -m 196M -machine type=q35,accel=kvm -device usb-ehci \
		   -device usb-tablet,id=ut -device VGA,vgamem_mb=8 \
		   -drive file=build/bin/boot.img,format=raw,if=virtio,readonly=on \
		   -bios /usr/share/ovmf/x64/OVMF.fd -monitor stdio

run:
	qemu-system-x86_64 $(QEMUARGS)

rundbg:
	qemu-system-x86_64 $(QEMUARGS) -s -S
