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
	{ 0x23c03251, "i2c_register_driver" },
	{ 0x4c8110b1, "sysfs_remove_group" },
	{ 0x2103abff, "device_unregister" },
	{ 0x9a3d88fa, "class_destroy" },
	{ 0x2c6697b9, "_dev_info" },
	{ 0x7f06074, "i2c_smbus_read_word_data" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xd4925fdd, "devm_kmalloc" },
	{ 0x8f4e90d7, "class_create" },
	{ 0x75f343e5, "device_create" },
	{ 0x4332afa5, "sysfs_create_group" },
	{ 0xa9329c5c, "i2c_del_driver" },
	{ 0x126bac03, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cmycorp,ina226_custom");
MODULE_ALIAS("of:N*T*Cmycorp,ina226_customC*");

MODULE_INFO(srcversion, "DE3BDBF58E1AEA3BD942BAB");
