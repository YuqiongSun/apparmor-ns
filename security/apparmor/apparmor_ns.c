#include <linux/export.h>
#include <linux/user_namespace.h>
#include <linux/proc_ns.h>
#include <linux/apparmor_namespace.h>

struct apparmor_namespace init_apparmor_ns = {
	.kref = KREF_INIT(2),
	.user_ns = &init_user_ns,
	.ns.inum = PROC_APPARMOR_INIT_INO,
#ifdef CONFIG_IMA_NS
	.ns.ops = &apparmorns_operations,
#endif
	.parent = NULL,
};
EXPORT_SYMBOL(init_apparmor_ns);
