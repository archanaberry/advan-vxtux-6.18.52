# SPRD camera sensor framework (blob: sprd_sensor)
# Port 2026-10-04 from work/donorcatalog/mods-sprd/sprd/common/camera/sensor/
#
# Source notes:
#  - The donor Kbuild pulled a third source group from
#    csi2/sprd/$(BSP_BOARD_CAMERA_MODULE_CSI_VERSION)/. This board resolves to
#    receiver_r3p0, so that directory is copied in rather than searched for at
#    build time. The extra "sprd/" level of the donor layout is preserved so the
#    donor's own -I paths still resolve.
#  - Only sprd,sensor-main and sprd,sensor-sub exist in the STOCK DTB; main2 /
#    sub2 / main3 / sub3 have 0 nodes. All six tables stay in the source (they
#    are upstream and cost nothing built-in) but the DT only ever matches two.
obj-$(CONFIG_SPRD_SENSOR) += sprd_sensor.o

sprd_sensor-y := sprd_sensor_core.o sprd_sensor_drv.o
sprd_sensor-y += csi2/sprd/receiver_r3p0/csi_api.o csi2/sprd/receiver_r3p0/csi_driver.o

ccflags-y += -I$(src)
ccflags-y += -I$(src)/csi2/sprd/receiver_r3p0
