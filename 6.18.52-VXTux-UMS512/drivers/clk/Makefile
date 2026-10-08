# SPDX-License-Identifier: GPL-2.0
# common clock types
obj-$(CONFIG_HAVE_CLK)		+= clk-devres.o clk-bulk.o clkdev.o
obj-$(CONFIG_COMMON_CLK)	+= clk.o
obj-$(CONFIG_CLK_KUNIT_TEST)	+= clk-test.o
clk-test-y			:= clk_test.o \
				   kunit_clk_assigned_rates_u64_one.dtbo.o \
				   kunit_clk_assigned_rates_u64_one_consumer.dtbo.o \
				   kunit_clk_assigned_rates_u64_multiple.dtbo.o \
				   kunit_clk_assigned_rates_u64_multiple_consumer.dtbo.o \
				   kunit_clk_assigned_rates_multiple.dtbo.o \
				   kunit_clk_assigned_rates_multiple_consumer.dtbo.o \
				   kunit_clk_assigned_rates_null.dtbo.o \
				   kunit_clk_assigned_rates_null_consumer.dtbo.o \
				   kunit_clk_assigned_rates_one.dtbo.o \
				   kunit_clk_assigned_rates_one_consumer.dtbo.o \
				   kunit_clk_assigned_rates_without.dtbo.o \
				   kunit_clk_assigned_rates_without_consumer.dtbo.o \
				   kunit_clk_assigned_rates_zero.dtbo.o \
				   kunit_clk_assigned_rates_zero_consumer.dtbo.o \
				   kunit_clk_hw_get_dev_of_node.dtbo.o \
				   kunit_clk_parent_data_test.dtbo.o
obj-$(CONFIG_COMMON_CLK)	+= clk-divider.o
obj-$(CONFIG_COMMON_CLK)	+= clk-fixed-factor.o
obj-$(CONFIG_COMMON_CLK)	+= clk-fixed-rate.o
obj-$(CONFIG_CLK_FIXED_RATE_KUNIT_TEST)	+= clk-fixed-rate-test.o
clk-fixed-rate-test-y		:= clk-fixed-rate_test.o kunit_clk_fixed_rate_test.dtbo.o
obj-$(CONFIG_COMMON_CLK)	+= clk-gate.o
obj-$(CONFIG_CLK_GATE_KUNIT_TEST) += clk-gate_test.o
obj-$(CONFIG_COMMON_CLK)	+= clk-multiplier.o
obj-$(CONFIG_COMMON_CLK)	+= clk-mux.o
obj-$(CONFIG_COMMON_CLK)	+= clk-composite.o
obj-$(CONFIG_COMMON_CLK)	+= clk-fractional-divider.o
obj-$(CONFIG_CLK_FD_KUNIT_TEST) += clk-fractional-divider_test.o
obj-$(CONFIG_COMMON_CLK)	+= clk-gpio.o
ifeq ($(CONFIG_OF), y)
obj-$(CONFIG_COMMON_CLK)	+= clk-conf.o
endif

# KUnit specific helpers
ifeq ($(CONFIG_COMMON_CLK), y)
obj-$(CONFIG_KUNIT)		+= clk_kunit_helpers.o
endif

# hardware specific clock types
obj-$(CONFIG_COMMON_CLK_FIXED_MMIO)	+= clk-fixed-mmio.o
obj-$(CONFIG_COMMON_CLK_PWM)		+= clk-pwm.o
obj-$(CONFIG_COMMON_CLK_SCMI)           += clk-scmi.o
obj-$(CONFIG_COMMON_CLK_SCPI)           += clk-scpi.o
obj-$(CONFIG_COMMON_CLK_WM831X)		+= clk-wm831x.o

# please keep this section sorted lexicographically by directory path name
obj-y					+= actions/
obj-y					+= analogbits/
obj-$(CONFIG_COMMON_CLK_AT91)		+= at91/
obj-$(CONFIG_ARCH_ARTPEC)		+= axis/
obj-$(CONFIG_ARC_PLAT_AXS10X)		+= axs10x/
obj-y					+= baikal-t1/
obj-y					+= bcm/
obj-$(CONFIG_ARCH_BERLIN)		+= berlin/
obj-$(CONFIG_ARCH_DAVINCI)		+= davinci/
obj-$(CONFIG_ARCH_HISI)			+= hisilicon/
obj-y					+= imgtec/
obj-y					+= imx/
obj-y					+= ingenic/
obj-y					+= keystone/
obj-y					+= mediatek/
obj-$(CONFIG_ARCH_MESON)		+= meson/
obj-y					+= microchip/
ifeq ($(CONFIG_COMMON_CLK), y)
obj-$(CONFIG_ARCH_MMP)			+= mmp/
endif
obj-y					+= mstar/
obj-y					+= mvebu/
obj-$(CONFIG_ARCH_MXS)			+= mxs/
obj-$(CONFIG_ARCH_MA35)			+= nuvoton/
obj-$(CONFIG_COMMON_CLK_NXP)		+= nxp/
obj-$(CONFIG_COMMON_CLK_PISTACHIO)	+= pistachio/
obj-$(CONFIG_COMMON_CLK_PXA)		+= pxa/
obj-$(CONFIG_COMMON_CLK_QCOM)		+= qcom/
obj-y					+= ralink/
obj-y					+= renesas/
obj-$(CONFIG_ARCH_ROCKCHIP)		+= rockchip/
obj-$(CONFIG_COMMON_CLK_SAMSUNG)	+= samsung/
obj-$(CONFIG_CLK_SIFIVE)		+= sifive/
obj-y					+= socfpga/
obj-y					+= sophgo/
obj-y					+= spacemit/
obj-y					+= sprd/
obj-$(CONFIG_PLAT_SPEAR)		+= spear/
obj-$(CONFIG_ARCH_STI)			+= st/
obj-$(CONFIG_ARCH_STM32)		+= stm32/
obj-y					+= starfive/
obj-$(CONFIG_ARCH_SUNXI)		+= sunxi/
obj-y					+= sunxi-ng/
obj-$(CONFIG_ARCH_TEGRA)		+= tegra/
obj-$(CONFIG_ARCH_THEAD)		+= thead/
obj-y					+= ti/
obj-$(CONFIG_CLK_UNIPHIER)		+= uniphier/
obj-$(CONFIG_ARCH_U8500)		+= ux500/
obj-y					+= versatile/
obj-$(CONFIG_COMMON_CLK_VISCONTI)	+= visconti/
ifeq ($(CONFIG_COMMON_CLK), y)
obj-$(CONFIG_X86)			+= x86/
endif
obj-y					+= xilinx/
obj-$(CONFIG_ARCH_ZYNQ)			+= zynq/
obj-$(CONFIG_COMMON_CLK_ZYNQMP)         += zynqmp/
