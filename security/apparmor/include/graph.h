#include "file.h"
#include "policy.h"

typedef struct graph *Graph;
typedef struct transition *Tran;

/* create a new graph with n vertices labeled 0..n-1 and no edges */
Graph graph_create(int n);
Tran tran_create(int n);

/* free all space used by graph */
void graph_destroy(Graph, Tran);

/* add an edge to an existing graph */
/* doing this more than once may have unpredictable results */
void graph_add_edge(Graph, Tran, int source, int sink, char c);

/* return the number of vertices/edges in the graph */
int graph_vertex_count(Graph);
int graph_edge_count(Graph);

/* return the out-degree of a vertex */
int graph_out_degree(Graph, int source);

/* return 1 if edge (source, sink) exists), 0 otherwise */
int graph_has_edge(Graph, int source, int sink);

/* invoke f on all edges (u,v) with source u */
/* supplying data as final parameter to f */
/* no particular order is guaranteed */
void graph_foreach(Graph g, int source,
        void (*f)(Graph g, int source, int sink, void *data),
        void *data);

void print_path(Graph g, Tran tr, int src, int dst);

void print_all_path(Graph g, Tran tr, int start);

int check_state_match(Graph g, Tran tr, struct aa_profile *new_profile,
                      struct aa_profile *existing);
