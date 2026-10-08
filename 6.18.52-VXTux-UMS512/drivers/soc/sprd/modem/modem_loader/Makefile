# SPRD modem loader -- ports work/donors_20260930/iscle_ums512/
#	drivers/soc/sprd/modem/modem_loader/
#
# This is the driver the LTE stack is missing: it is what powers on the modem,
# loads SCI1/MECPV images from l_modem into the CP, and handles dump and reset.
# The stock vendor .ko set for this device contains no equivalent, which is why
# no modem node appears in the DTB and rild is absent.
obj-$(CONFIG_SPRD_MODEM_LOADER) += sprd_modem_loader.o
obj-$(CONFIG_SPRD_MODEM_LOADER) += sprd_modem_pcie_ep_ctrl.o
