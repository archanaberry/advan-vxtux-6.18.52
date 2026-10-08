# SPRD CPROC (Communication Processor) -- the /proc/cptl provider.
#
# Port 2026-10-04 from gpl-source/fork_t618/extract/soc/drivers/soc/sprd/modem/cproc/
# (Samsung T618 kernel tree). This is the driver /vendor/bin/modem_control
# actually talks to: it creates /proc/cptl/{start,stop,ldinfo,modemassert,mini_dump}
# and matches "sprd,scproc" / "sprd,scproc_pubcp".
#
# Why this matters: the modem_loader port supplies the loader, but modem_control
# does NOT use ioctl (zero _IO* magics in the binary) -- it writes to procfs.
# So this file, not the loader, is what the vendor userland needs.
obj-$(CONFIG_SPRD_CPROC) += sprd_cproc.o

ccflags-y += -I$(srctree)/drivers/soc/sprd/modem/include
ccflags-y += -I$(srctree)/drivers/soc/sprd/modem/include
