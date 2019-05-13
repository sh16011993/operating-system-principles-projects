#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x21c14206, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x7fe8433e, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x145d814, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x5fc4c1a7, __VMLINUX_SYMBOL_STR(misc_register) },
	{ 0x93067ca2, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x8baf6d18, __VMLINUX_SYMBOL_STR(mutex_lock_interruptible) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x1000e51, __VMLINUX_SYMBOL_STR(schedule) },
	{ 0x9f9291fa, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0xa9d91e8e, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xde990047, __VMLINUX_SYMBOL_STR(misc_deregister) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "9AFA46ADC5BA140FFCD4F26");
