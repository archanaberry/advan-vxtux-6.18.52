# SPDX-License-Identifier: GPL-2.0
# Makefile modul BFKRT -- dibangun sebagai modul EKSTERNAL terhadap pohon
# kernel yang sudah pernah di-make (KDIR berisi Module.symvers).
#
#   make KDIR=/path/ke/pohon/kernel            -> tiga .ko
#   make KDIR=... sign=1                       -> tanda tangan bila kernel
#                                                 memaksa (MODULE_SIG_FORCE)
#   make clean
#
# Vermagic .ko diambil otomatis dari pohon KDIR (include/linux/vermagic.h),
# jadi modul yang di-build dari pohon berbeda akan DITOLAK kernel saat insmod.
# Cek dulu tanpa memuat:  bfkrt mod check --ko <file>
ifneq ($(KERNELRELEASE),)

obj-m := berryfetcher_kernel.o berry_trig_all.o berry_m_virtkrn_obs.o
ccflags-y := -Wall

else

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all: module

module:
	@test -e "$(KDIR)/Makefile" || { \
	  echo "KDIR tidak valid: $(KDIR)"; \
	  echo "Isikan pohon kernel tujuan, mis.:"; \
	  echo "  make KDIR=../../vxtux_build_repo/6.18.52-VXTux-UMS512"; \
	  exit 1; }
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo "---"
	@for f in *.ko; do \
	   printf "%-28s " "$$f"; \
	   strings -a "$$f" | grep -m1 '^vermagic=' || echo "(tanpa vermagic)"; \
	done

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean 2>/dev/null || true

.PHONY: all module clean
endif
