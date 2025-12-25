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
	{ 0x3928efe9, "__per_cpu_offset" },
	{ 0xb58aeaab, "kernel_cpustat" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x5e605235, "cpufreq_register_governor" },
	{ 0x122c3a7e, "_printk" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x37a0cba, "kfree" },
	{ 0xc5ffd7d3, "cpufreq_unregister_governor" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0x17de3d5, "nr_cpu_ids" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x53a1e8d9, "_find_next_bit" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0xbf20decd, "cpufreq_driver_target" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x9ae47436, "_find_last_bit" },
	{ 0x5e5ac9ba, "kmalloc_caches" },
	{ 0x3ae23b9a, "kmalloc_trace" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x24a5627, "param_ops_uint" },
	{ 0x126bac03, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "CF07458D28F70B63C99335A");
