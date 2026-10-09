# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_CHARGER_SPRD_MANAGER)	+= sprd_charger_manager.o
# unisoc_battery_id menyediakan fgauge_get_profile_id() yang dipanggil
# sprd_battery_info_main.c (donor: unisoc_battery.c). Dimasukkan ke objek
# yang sama lewat -y supaya satu modul sprd_battery_info.ko, konsisten
# dengan blob vendor sprd_battery_info.ko. File utama di-rename
# sprd_battery_info.c -> sprd_battery_info_main.c supaya nama modul tidak
# bentrok dengan nama objek sumber (circular -y dependency; pola zram/vsp).
sprd_battery_info-y := sprd_battery_info_main.o unisoc_battery_id.o
obj-$(CONFIG_SPRD_BATTERY_INFO)		+= sprd_battery_info.o
