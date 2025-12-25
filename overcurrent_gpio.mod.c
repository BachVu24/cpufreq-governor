#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xd44f612c, "__platform_driver_register" },
	{ 0x122c3a7e, "_printk" },
	{ 0x4c8110b1, "sysfs_remove_group" },
	{ 0x2103abff, "device_unregister" },
	{ 0x9a3d88fa, "class_destroy" },
	{ 0x2c6697b9, "_dev_info" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xe28177c3, "gpiod_get_value" },
	{ 0xcfb1ca3f, "_dev_warn" },
	{ 0xd4925fdd, "devm_kmalloc" },
	{ 0x2ff415eb, "devm_gpiod_get" },
	{ 0x20f6ad9a, "gpiod_to_irq" },
	{ 0xbb6ea478, "devm_request_threaded_irq" },
	{ 0x8f4e90d7, "class_create" },
	{ 0x75f343e5, "device_create" },
	{ 0x4332afa5, "sysfs_create_group" },
	{ 0x846d2c82, "_dev_err" },
	{ 0xec09d16, "platform_driver_unregister" },
	{ 0x126bac03, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Ccustom,overcurrent-gpio");
MODULE_ALIAS("of:N*T*Ccustom,overcurrent-gpioC*");

MODULE_INFO(srcversion, "1323293E2D0A0EDD5492F07");
