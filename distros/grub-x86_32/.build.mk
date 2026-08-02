$(BOOTDISK): $(RAMDISK) $(KERNEL_BINARY) $(DISTRO_DIRECTORY)/grub.cfg $(wildcard $(DISTRO_DIRECTORY)/theme/*)
	$(DIRECTORY_GUARD)
	@echo [GRUB-MKRESCUE] $@

	@mkdir -p $(BOOTROOT)/boot/grub
	@mkdir -p $(BOOTROOT)/boot/grub/themes/amberos
	@cp $(DISTRO_DIRECTORY)/grub.cfg $(BOOTROOT)/boot/grub/
	@cp $(DISTRO_DIRECTORY)/theme/* $(BOOTROOT)/boot/grub/themes/amberos/
	@gzip -c $(RAMDISK) > $(BOOTROOT)/boot/ramdisk.tar.gz
	@gzip -c $(KERNEL_BINARY) > $(BOOTROOT)/boot/kernel.bin.gz

	@grub-mkrescue -o $@ $(BOOTROOT) || \
	 grub2-mkrescue -o $@ $(BOOTROOT)
