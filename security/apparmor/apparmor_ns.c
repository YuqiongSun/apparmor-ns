#include <linux/export.h>
#include <linux/user_namespace.h>
#include <linux/proc_ns.h>
#include <linux/apparmor_namespace.h>
#include "include/lib.h"

struct apparmor_namespace init_apparmor_ns = {
	.kref = KREF_INIT(2),
	.user_ns = &init_user_ns,
	.ns.inum = PROC_APPARMOR_INIT_INO,
#ifdef CONFIG_IMA_NS
	.ns.ops = &apparmorns_operations,
#endif
	.parent = NULL,
	.root_ns = NULL,
};
EXPORT_SYMBOL(init_apparmor_ns);


struct aa_global_policy global_policy = {
	.globals = LIST_HEAD_INIT(global_policy.globals),
	.size = 0,
};
EXPORT_SYMBOL(global_policy);
