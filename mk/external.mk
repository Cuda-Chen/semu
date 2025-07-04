# For each external target, the following must be defined in advance:
#   _DATA_URL : the hyperlink which points to archive.
#   _DATA : the file to be read by specific executable.
#   _DATA_SHA1 : the checksum of the content in _DATA

COMMON_URL = https://github.com/sysprog21/semu/raw/blob

# kernel
KERNEL_DATA_URL = $(COMMON_URL)/Image.bz2
KERNEL_DATA = Image
KERNEL_DATA_SHA1 = 72a8454575508d09210f067b25239cd51b929d27

# initrd
INITRD_DATA_URL = $(COMMON_URL)/rootfs.cpio.bz2
INITRD_DATA = rootfs.cpio
INITRD_DATA_SHA1 = 18bd639c17fc299c317f8fed1c845e7dda296748

define download
$($(T)_DATA):
	$(VECHO) "  GET\t$$@\n"
	$(Q)curl --progress-bar -O -L -C - "$(strip $($(T)_DATA_URL))"
	$(Q)echo "$(strip $$($(T)_DATA_SHA1))  $$@.bz2" | $(SHA1SUM) -c -
	$(Q)bunzip2 $$@.bz2
endef

EXTERNAL_DATA = KERNEL INITRD
$(foreach T,$(EXTERNAL_DATA),$(eval $(download)))
