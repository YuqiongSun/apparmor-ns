#ifndef __LINUX_APPARMOR_NAMESPACE_H__
#define __LINUX_APPARMOR_NAMESPACE_H__

#include <linux/kref.h>
#include <linux/ns_common.h>
#include <linux/nsproxy.h>
#include <linux/rculist.h>
#include <linux/sched.h>

struct apparmor_namespace {
	struct kref kref;
	struct user_namespace *user_ns;
	struct ns_common ns;
	struct apparmor_namespace *parent;
};

extern struct apparmor_namespace init_apparmor_ns;

#ifdef CONFIG_APPARMOR_NS
void free_apparmor_ns(struct kref *kref);

static inline void get_apparmor_ns(struct apparmor_namespace *ns)
{
	kref_get(&ns->kref);
}

static inline void put_apparmor_ns(struct apparmor_namespace *ns)
{
	kref_put(&ns->kref, free_apparmor_ns);
}

struct apparmor_namespace *copy_apparmor(unsigned long flags,
			       struct user_namespace *user_ns,
			       struct apparmor_namespace *old_ns);

#else
static inline void get_apparmor_ns(struct apparmor_namespace *ns)
{
}

static inline void put_apparmor_ns(struct apparmor_namespace *ns)
{
}

static inline struct apparmor_namespace *copy_apparmor(unsigned long flags,
					     struct user_namespace *user_ns,
					     struct apparmor_namespace *old_ns)
{
	if (flags & CLONE_NEWAPPARMOR)
		return ERR_PTR(-EINVAL);
	return old_ns;
}
#endif

#endif /* __LINUX_APPARMOR_NAMESPACE_H__ */

