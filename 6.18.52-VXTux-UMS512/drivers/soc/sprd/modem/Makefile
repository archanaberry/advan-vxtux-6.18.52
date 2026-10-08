obj-$(CONFIG_SPRD_MODEM) += sprd_modem.o

# Port 2026-10-04: the real modem drivers. Without these lines the directory
# built sprd_modem.o alone and none of cproc/cp_dump/modem_loader could ever be
# linked, which is why /proc/cptl never appeared.
obj-$(CONFIG_SPRD_CPROC)      += cproc/
obj-$(CONFIG_SPRD_CP_DUMP)    += cp_dump/
obj-$(CONFIG_SPRD_MODEM_LOADER) += modem_loader/
