# SPRD FM (radio) driver -- replaces blob sprd_fm.ko
# Port 2026-10-04 from gpl-source/hadenix_ums512_wcn/drivers/wcn/fm/driver/fm_sdio/
#
# The vendor of_match_table registers BOTH spellings:
#   { .compatible = "sprd,marlin3_fm" },   <- what the .ko binary carries
#   { .compatible = "sprd,marlin3-fm" },   <- what the STOCK DTB actually has
# (verified: node sprd-fm in /data/local/tmp/dtsx/fdt.dts:53 says
#  compatible = "sprd,marlin3-fm"). So the underscore/hyphen difference between
#  the blob's string and the DT is already handled by the vendor source -- do not
#  "fix" it to one form or the driver stops matching.
#
# ccflags: the wlan side of sprdwcn needs the same include roots.
obj-$(CONFIG_WCN_FM) += fmdrv_main.o fmdrv_ops.o fmdrv_rds_parser.o \
                        fm_rf_marlin3.o

ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/include
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/platform
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/sdio
ccflags-y += -I$(src)
