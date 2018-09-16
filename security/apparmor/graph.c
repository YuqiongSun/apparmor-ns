#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/string.h>
#include "include/graph.h"


/* basic directed graph type */
/* the implementation uses adjacency lists
 * represented as variable-length arrays */

/* these arrays may or may not be sorted: if one gets long enough
 * and you call graph_has_edge on its source, it will be */

struct graph {
    int n;              /* number of vertices */
    int m;              /* number of edges */
    struct successors {
        int d;          /* number of successors */
        int len;        /* number of slots in array */
        char is_sorted; /* true if list is already sorted */
        int list[1];    /* actual list of successors */
    } *alist[1];
};

struct transition {
    struct trans {
	char list[1];
    } *alist[1];
};

/* create a new graph with n vertices labeled 0..n-1 and no edges */
Graph
graph_create(int n)
{
    Graph g;
    int i;

    g = kmalloc(sizeof(struct graph) + sizeof(struct successors *) * (n-1), 
		GFP_KERNEL);

    g->n = n;
    g->m = 0;

    for(i = 0; i < n; i++) {
        g->alist[i] = kmalloc(sizeof(struct successors), GFP_KERNEL);

        g->alist[i]->d = 0;
        g->alist[i]->len = 1;
        g->alist[i]->is_sorted= 1;
    }
    
    return g;
}

Tran tran_create(int n) {
    Tran tr;
    int i;
    tr = kmalloc(sizeof(struct transition) + sizeof(struct trans *) * (n - 1), 
		GFP_KERNEL);
    for(i = 0; i < n; i++) {
        tr->alist[i] = kmalloc(sizeof(struct trans), GFP_KERNEL);
    }
    return tr;
}

/* free all space used by graph */
void
graph_destroy(Graph g, Tran tr)
{
    int i;

    for(i = 0; i < g->n; i++) {
	kfree(g->alist[i]);
	kfree(tr->alist[i]);
    }
    kfree(g);
    kfree(tr);
}

/* add an edge to an existing graph */
void
graph_add_edge(Graph g, Tran tr, int u, int v, char c)
{

    // SYQ: do not consider multiple edges connecting same nodes
    //      to reduce memory footprint and computing time
    int i;
    for (i = 0; i < g->alist[u]->d; i++) {
    	if (v == g->alist[u]->list[i]) return;
    }

    /* do we need to grow the list? */
    while(g->alist[u]->d >= g->alist[u]->len) {
        g->alist[u]->len *= 2;
        g->alist[u] =
            krealloc(g->alist[u], 
                sizeof(struct successors) + sizeof(int) * (g->alist[u]->len - 1), 
		GFP_KERNEL);
        tr->alist[u] =
            krealloc(tr->alist[u], 
                sizeof(struct trans) + sizeof(char) * (g->alist[u]->len - 1),
		GFP_KERNEL);
    }

    /* now add the new sink */
    g->alist[u]->list[g->alist[u]->d] = v;
    tr->alist[u]->list[g->alist[u]->d] = c;
    g->alist[u]->d++;
    g->alist[u]->is_sorted = 0;
    /* bump edge count */
    g->m++;
}

/* return the number of vertices in the graph */
int
graph_vertex_count(Graph g)
{
    return g->n;
}

/* return the number of vertices in the graph */
int
graph_edge_count(Graph g)
{
    return g->m;
}

/* return the out-degree of a vertex */
int
graph_out_degree(Graph g, int source)
{
    return g->alist[source]->d;
}

/* when we are willing to call bsearch */
#define BSEARCH_THRESHOLD (10)

static int
intcmp(const void *a, const void *b)
{
    return *((const int *) a) - *((const int *) b);
}

/* return 1 if edge (source, sink) exists), 0 otherwise */
int
graph_has_edge(Graph g, int source, int sink)
{
    int i;

        /* just do a simple linear search */
        /* we could call lfind for this, but why bother? */
        for(i = 0; i < g->alist[source]->d; i++) {
            if(g->alist[source]->list[i] == sink) return 1;
        }
        /* else */
        return 0;
}

/* invoke f on all edges (u,v) with source u */
/* supplying data as final parameter to f */
void
graph_foreach(Graph g, int source,
    void (*f)(Graph g, int source, int sink, void *data),
    void *data)
{
    int i;


    for(i = 0; i < g->alist[source]->d; i++) {
        f(g, source, g->alist[source]->list[i], data);
    }
}

struct node {
	int *path;
	int len;
	struct node *next;
};

typedef struct node* Node;

Node head = NULL;
Node tail = NULL;

int isNotVisited(int state, Node cur) {
	int i;
	for (i = 0; i < cur->len; i++) {
		if (state == cur->path[i])
			return 0;
	}
	return 1;
}


void print_path(Graph g, Tran tr, int src, int dst) {
	Node start = kmalloc(sizeof(struct node), GFP_KERNEL);
	int last = -1;
	int i, j;

	start->len = 1;
	start->path = kmalloc(sizeof(int) * start->len, GFP_KERNEL);
	// Modified to reduce memory footprint
	//start->path_str = kmalloc(g->n, GFP_KERNEL);
	//memset(start->path_str, '\0', g->n);
	start->path[start->len - 1] = src;
	start->next = NULL;

	head = start;
	tail = start;

	while (head != NULL) {
		Node cur = head;

		/*
		printk("now: ");
		for (int i = 0; i < cur->len; i++) {
			printk("%d ", cur->path[i]);
		}
		printk("\n");
		*/
			
		last = cur->path[cur->len - 1];
		if (last == dst) {
			//printk("SYQ| Path: %s\n", cur->path_str);
			/*
			for (i = 0; i < cur->len; i++) {
				printk("%d ", cur->path[i]);
			}
			printk("(%s)\n", cur->path_str);
			*/
			// Reconstruct path
			// TODO this is not entirely correct when
			//	there are more than 1 edges connecting
			//	two nodes
			char *path = kmalloc(cur->len, GFP_KERNEL);
			memset(path, '\0', cur->len);
			for (i = 0; i < cur->len - 1; i++) {
				for (j = 0; j < g->alist[cur->path[i]]->d; j++) {
					if( cur->path[i + 1] == g->alist[cur->path[i]]->list[j]) { 
						*(path + i) = tr->alist[cur->path[i]]->list[j];
						continue;
					}
				}
			}
			printk("SYQ| Path: %s\n", path);
			kfree(path);	
		}
		for (i = 0; i < g->alist[last]->d; i++) {
			if (isNotVisited(g->alist[last]->list[i], cur)) {
				Node new = kmalloc(sizeof(struct node), GFP_KERNEL);
				new->len = cur->len + 1;
				new->path = kmalloc(sizeof(int) * new->len, GFP_KERNEL);
				//new->path_str = kmalloc(g->n, GFP_KERNEL);
				//memset(new->path_str, '\0', g->n);
				memcpy(new->path, cur->path, sizeof(int) * cur->len);
				//memcpy(new->path_str, cur->path_str, cur->len - 1);
				new->path[cur->len] = g->alist[last]->list[i];
				//new->path_str[cur->len - 1] = tr->alist[last]->list[i];
				new->next = NULL;
				tail->next = new;
				tail = new;	
			}
		}		

		head = head->next;
		kfree(cur->path);
		//kfree(cur->path_str);
		kfree(cur);
	}
}
// TODO
// This does not handle the scenario
// where there are more than 1 edges
// connecting two nodes
int generate_path(Node cur, Graph g, Tran tr, char **path) {
	int i, j;
	*path = kmalloc(cur->len, GFP_KERNEL);
	if (!(*path)) return -ENOMEM;
	memset(*path, '\0', cur->len);
        for (i = 0; i < cur->len - 1; i++) {
        	for (j = 0; j < g->alist[cur->path[i]]->d; j++) {
                	if( cur->path[i + 1] == g->alist[cur->path[i]]->list[j]) {
                        	*(*path + i) = tr->alist[cur->path[i]]->list[j];
                        }
                }
        }
	return 0;
}

void print_all_path(Graph g, Tran tr, int src) {
	Node start = kmalloc(sizeof(struct node), GFP_KERNEL);
	int last = -1;
	int i, j;
	char *path;
	int err;
	start->len = 1;
	start->path = kmalloc(sizeof(int) * start->len, GFP_KERNEL);
	start->path[start->len - 1] = src;
	start->next = NULL;

	head = start;
	tail = start;

	while (head != NULL) {
		Node cur = head;
		err = generate_path(cur, g, tr, &path);
		if (err == 0) {
			printk("SYQ| path: %s", path);
			kfree(path);
		}
		last = cur->path[cur->len - 1];
		for (i = 0; i < g->alist[last]->d; i++) {
			if (isNotVisited(g->alist[last]->list[i], cur)) {
				Node new = kmalloc(sizeof(struct node), GFP_KERNEL);
				new->len = cur->len + 1;
				new->path = kmalloc(sizeof(int) * new->len, GFP_KERNEL);
				memcpy(new->path, cur->path, sizeof(int) * cur->len);
				new->path[cur->len] = g->alist[last]->list[i];
				new->next = NULL;
				tail->next = new;
				tail = new;	
			}
		}		

		head = head->next;
		kfree(cur->path);
		kfree(cur);
	}
}


int check_state_match(Graph g, Tran tr, struct aa_profile *new_profile, 
		      struct aa_profile *existing) {
	Node start = kmalloc(sizeof(struct node), GFP_KERNEL);
	int last = -1;
	int i;
	int ret = 0;
	char *path;
	int err;	
	struct aa_perms perms_e = {};
	struct aa_perms perms_n = {};
	struct aa_perms perms_p_e = {};
	struct aa_perms perms_p_n = {};
	int p_state; 


	start->len = 1;
	start->path = kmalloc(sizeof(int) * start->len, GFP_KERNEL);
	start->path[start->len - 1] = new_profile->file.start;
	start->next = NULL;
	head = start;
	tail = start;

	while (head != NULL) {
		Node cur = head;
		err = generate_path(cur, g, tr, &path);
		if (err != 0) {
			printk("SYQ| error allocating memory");
			return -1;
		}
		//printk("SYQ| Checking Path: %s\n", path);
		p_state = aa_dfa_match(existing->file.dfa, existing->file.start, path);
		perms_e = aa_compute_fperms_simple(new_profile->file.dfa, cur->path[cur->len - 1], 1);
		perms_n = aa_compute_fperms_simple(new_profile->file.dfa, cur->path[cur->len - 1], 0);	
		perms_p_e = aa_compute_fperms_simple(existing->file.dfa, p_state, 1);
		perms_p_n = aa_compute_fperms_simple(existing->file.dfa, p_state, 0);
		if ((perms_e.allow & perms_p_e.allow) != perms_e.allow || (perms_n.allow & perms_p_n.allow) != perms_n.allow) {
			ret = 1;
			printk("SYQ| conflicts detected at %s\n", path);
		}

		last = cur->path[cur->len - 1];
		for (i = 0; i < g->alist[last]->d; i++) {
			if (isNotVisited(g->alist[last]->list[i], cur)) {
				Node new = kmalloc(sizeof(struct node), GFP_KERNEL);
				new->len = cur->len + 1;
				new->path = kmalloc(sizeof(int) * new->len, GFP_KERNEL);
				memcpy(new->path, cur->path, sizeof(int) * cur->len);
				new->path[cur->len] = g->alist[last]->list[i];
				new->next = NULL;
				tail->next = new;
				tail = new;	
			}
		}		

		head = head->next;
		kfree(cur->path);
		kfree(path);
		kfree(cur);
	}
	return ret;
}


int check_global_state_match(Graph g, Tran tr, struct aa_profile *local, 
		      struct aa_profile *global) {
	Node start = kmalloc(sizeof(struct node), GFP_KERNEL);
	int last = -1;
	int i;
	int ret = 0;
	char *path;
	int err;	
	struct aa_perms perms_e = {};
	struct aa_perms perms_n = {};
	struct aa_perms perms_g_e = {};
	struct aa_perms perms_g_n = {};
	struct aa_perms testperms_e = {};
	struct aa_perms testperms_n = {};
	char testnames[] = "/";
	int local_state; 
	int test_state;	
	

	start->len = 1;
	start->path = kmalloc(sizeof(int) * start->len, GFP_KERNEL);
	start->path[start->len - 1] = global->file.start;
	start->next = NULL;
	head = start;
	tail = start;

	test_state = aa_dfa_match(global->file.dfa, global->file.start, testnames);
	testperms_e = aa_compute_fperms_simple(global->file.dfa, test_state, 1);
	testperms_n = aa_compute_fperms_simple(global->file.dfa, test_state, 1);

	while (head != NULL) {
		Node cur = head;
		err = generate_path(cur, g, tr, &path);
		if (err != 0) {
			printk("SYQ| error allocating memory");
			return -1;
		}
		//printk("SYQ| Checking Path: %s\n", path);
		local_state = aa_dfa_match(local->file.dfa, local->file.start, path);
		perms_e = aa_compute_fperms_simple(local->file.dfa, local_state, 1);
		perms_n = aa_compute_fperms_simple(local->file.dfa, local_state, 0);	
		perms_g_e = aa_compute_fperms_simple(global->file.dfa, cur->path[cur->len - 1], 1);
		perms_g_n = aa_compute_fperms_simple(global->file.dfa, cur->path[cur->len - 1], 0);
		if (testperms_e.allow != perms_g_e.allow || testperms_n.allow != perms_g_n.allow) {
			if ((perms_e.allow & perms_g_e.allow) != perms_e.allow || (perms_n.allow & perms_g_n.allow) != perms_n.allow) {
				ret = 1;
				printk("SYQ| conflicts detected at %s\n", path);
			}
		}

		last = cur->path[cur->len - 1];
		for (i = 0; i < g->alist[last]->d; i++) {
			if (isNotVisited(g->alist[last]->list[i], cur)) {
				Node new = kmalloc(sizeof(struct node), GFP_KERNEL);
				new->len = cur->len + 1;
				new->path = kmalloc(sizeof(int) * new->len, GFP_KERNEL);
				memcpy(new->path, cur->path, sizeof(int) * cur->len);
				new->path[cur->len] = g->alist[last]->list[i];
				new->next = NULL;
				tail->next = new;
				tail = new;	
			}
		}		

		head = head->next;
		kfree(cur->path);
		kfree(path);
		kfree(cur);
	}
	return ret;
}
