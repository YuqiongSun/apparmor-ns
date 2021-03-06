#include <linux/export.h>
#include <linux/apparmor_namespace.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/user_namespace.h>
#include <linux/proc_ns.h>
#include <linux/sched/task.h>
#include <linux/capability.h>
#include <linux/cred.h>
#include <linux/random.h>

//extern struct aa_ns *aa_alloc_namespace_root_ns(const char *name);
extern void destroy_namespace_root_ns(struct aa_ns *ns);
extern struct aa_ns *aa_prepare_ns(struct aa_ns *parent, const char *name);
extern int apparmor_prepare_label(struct cred *cred, struct aa_ns *ns);

static struct apparmor_namespace *create_apparmor_ns(void)
{
	struct apparmor_namespace *apparmor_ns;

	apparmor_ns = kmalloc(sizeof(*apparmor_ns), GFP_KERNEL);
	if (apparmor_ns)
		kref_init(&apparmor_ns->kref);

	return apparmor_ns;
}

static struct apparmor_namespace *clone_apparmor_ns(struct user_namespace *user_ns,
					  struct apparmor_namespace *old_ns,
					  struct task_struct *tsk)
{
	struct apparmor_namespace *ns;
	int err;
	int random;
	struct cred *cred;	

	ns = create_apparmor_ns();
	if (!ns)
		return ERR_PTR(-ENOMEM);

	err = ns_alloc_inum(&ns->ns);
	if (err) {
		kfree(ns);
		return ERR_PTR(err);
	}

	ns->ns.ops = &apparmorns_operations;
	get_apparmor_ns(old_ns);
	ns->parent = old_ns;
	ns->user_ns = get_user_ns(user_ns);

	get_random_bytes(&random, sizeof(random));
	memset(ns->root_ns_name, '\0', APPARMOR_ROOT_NS_NAME_SIZE);	
	sprintf(ns->root_ns_name, "%s_%08x", "root",random);

	ns->root_ns = aa_prepare_ns(ns->parent->root_ns, ns->root_ns_name);
	if (ns->root_ns < 0) {
		kfree(ns);
		printk("SYQ: error allocating aa_ns for namespace\n");
		return ERR_PTR(ns->root_ns);
	}

	cred = get_task_cred(tsk);
	// Reset labels for the new cred
	apparmor_prepare_label(cred, ns->root_ns);

	return ns;
}

struct apparmor_namespace *copy_apparmor(unsigned long flags,
			       struct user_namespace *user_ns,
			       struct apparmor_namespace *old_ns, 
			       struct task_struct *tsk)
{
	struct apparmor_namespace *new_ns;

	BUG_ON(!old_ns);
	get_apparmor_ns(old_ns);

	if (!(flags & CLONE_NEWAPPARMOR))
		return old_ns;

	new_ns = clone_apparmor_ns(user_ns, old_ns, tsk);
	put_apparmor_ns(old_ns);

	return new_ns;
}

static void destroy_apparmor_ns(struct apparmor_namespace *ns)
{
	put_user_ns(ns->user_ns);
	ns_free_inum(&ns->ns);
	destroy_namespace_root_ns(ns->root_ns);
	kfree(ns);
}

// This seems to be wrong
/*
void free_apparmor_ns(struct kref *kref)
{
	struct apparmor_namespace *ns;
	struct apparmor_namespace *parent;

	ns = container_of(kref, struct apparmor_namespace, kref);

	while (ns != &init_apparmor_ns) {
		parent = ns->parent;
		destroy_apparmor_ns(ns);
		put_apparmor_ns(parent);
		ns = parent;
	}
}
*/

void free_apparmor_ns(struct kref *kref)
{
	struct apparmor_namespace *ns;
	struct apparmor_namespace *parent;

	ns = container_of(kref, struct apparmor_namespace, kref);
	parent = ns->parent;
	destroy_apparmor_ns(ns);
	put_apparmor_ns(parent);
}

static inline struct apparmor_namespace *to_apparmor_ns(struct ns_common *ns)
{
	return container_of(ns, struct apparmor_namespace, ns);
}

static struct ns_common *apparmorns_get(struct task_struct *task)
{
	struct apparmor_namespace *ns = NULL;
	struct nsproxy *nsproxy;

	task_lock(task);
	nsproxy = task->nsproxy;
	if (nsproxy) {
		ns = nsproxy->apparmor_ns;
		get_apparmor_ns(ns);
	}
	task_unlock(task);

	return ns ? &ns->ns : NULL;
}

static void apparmorns_put(struct ns_common *ns)
{
	put_apparmor_ns(to_apparmor_ns(ns));
}

static int apparmorns_install(struct nsproxy *nsproxy, struct ns_common *new)
{
	struct apparmor_namespace *ns = to_apparmor_ns(new);

	if (!ns_capable(ns->user_ns, CAP_SYS_ADMIN) ||
	    !ns_capable(current_user_ns(), CAP_SYS_ADMIN))
		return -EPERM;

	get_apparmor_ns(ns);
	put_apparmor_ns(nsproxy->apparmor_ns);
	nsproxy->apparmor_ns = ns;
	return 0;
}

const struct proc_ns_operations apparmorns_operations = {
	.name    = "apparmor",
	.type    = CLONE_NEWAPPARMOR,
	.get     = apparmorns_get,
	.put     = apparmorns_put,
	.install = apparmorns_install,
};
