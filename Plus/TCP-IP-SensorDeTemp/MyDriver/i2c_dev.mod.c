#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xd9c05eac, "module_layout" },
	{ 0xa07bd7fd, "__platform_driver_register" },
	{ 0x145db5b1, "device_create" },
	{ 0x35ba6ee8, "__class_create" },
	{ 0xdc5a85b3, "cdev_add" },
	{ 0xb7643085, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x260d8d1d, "kmem_cache_alloc_trace" },
	{ 0x9dd116e1, "kmalloc_caches" },
	{ 0xf4fa543b, "arm_copy_to_user" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0x963e0acd, "finish_wait" },
	{ 0x93f6bff1, "prepare_to_wait_event" },
	{ 0x1000e51, "schedule" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x37a0cba, "kfree" },
	{ 0xf81b935d, "platform_driver_unregister" },
	{ 0x4ad0e0a6, "device_destroy" },
	{ 0x73ccaaeb, "class_destroy" },
	{ 0x89933be5, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xf9a482f9, "msleep" },
	{ 0xe97c4103, "ioremap" },
	{ 0xd6b8e852, "request_threaded_irq" },
	{ 0x325f1409, "platform_get_irq" },
	{ 0xedc03953, "iounmap" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x413c0466, "__wake_up" },
	{ 0x27e1a049, "printk" },
	{ 0x822137e2, "arm_heavy_mb" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

