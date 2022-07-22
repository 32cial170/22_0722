/*
 *
BINTH=100
H=h3
M=m2
obj=hisplit
CFLAG=-DBINTH=${BINTH} -D${H} -D${M}

all: clean ${obj}

${obj}: ${obj}.o
	gcc -g -o ${obj} ${obj}.o

${obj}.o: ${obj}.c
	gcc -g -c ${obj}.c ${CFLAG}

clean:
	rm -f ${obj}
	rm -f *.o
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define max(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY {
    unsigned int nexthop;
    unsigned int src_ip_begin;
    unsigned int dst_ip_begin;
    unsigned int src_ip_end;
    unsigned int dst_ip_end;
    unsigned int src_port;  // short + short
    unsigned int dst_port;
    unsigned short protocol;  // char + char
    struct ENTRY *next;
};
////////////////////////////////////////////////////////////////////////////////////
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}
////////////////////////////////////////////////////////////////////////////////////
struct bt_node {  // structure of binary trie
    unsigned int port;
    unsigned int num;
    struct bt_node *left, *right;
};
struct port_root {
    int num;  // num of child
    struct bt_node *child;
};
struct classifier {
    int dim;
    unsigned int ep;
    void **child;
    int *num_child;
};
struct ENTRY_INDEX {
    int index;
    struct ENTRY_INDEX *next;
};
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
int num_entry = 0;
unsigned int num_query = 0;
struct ENTRY *table, *query;
int N = 0;  // number of nodes
unsigned long long int begin, end, total = 0;
unsigned long long int *my_clock;
int num_node = 0;  // total number of rule in linked list
int num_clsr = 1;  // total number of classifier
#ifndef BINTH
#define BINTH 100
#endif

struct classifier *root;
////////////////////////////////////////////////////////////////////////////////////
struct bt_node *create_node()
{
    struct bt_node *temp;
    temp = (struct bt_node *) malloc(sizeof(struct bt_node));
    temp->right = NULL;
    temp->left = NULL;
    temp->port = 256;  // default port
    temp->num = 0;
    return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_bt_node(struct bt_node *ptr,
                 unsigned int ip,
                 unsigned char len,
                 unsigned char nexthop)
{
    int i;
    for (i = 0; i < len; i++) {
        if (ip & (1 << (31 - i))) {
            if (ptr->right == NULL)
                ptr->right = create_node();  // Create Node
            ptr = ptr->right;
            if (i == len - 1) {
                ptr->port = nexthop;
                ptr->num += 1;
            }

        } else {
            if (ptr->left == NULL)
                ptr->left = create_node();
            ptr = ptr->left;
            if (i == len - 1) {
                ptr->port = nexthop;
                ptr->num += 1;
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////
void read_table(char *str, struct ENTRY *node)
{
    char tok[] = "./\t:";
    char buf[100], *str1;
    unsigned int n[4];
    int i, len;
    sprintf(buf, "%s", strtok(++str, tok));  // ignore @
    n[0] = atoi(buf);
    for (i = 1; i < 4; i++) {
        sprintf(buf, "%s", strtok(NULL, tok));
        n[i] = atoi(buf);
    }
    node->nexthop = n[2];
    str1 = (char *) strtok(NULL, tok);
    sprintf(buf, "%s", str1);
    len = atoi(buf);
    node->src_ip_begin = (n[0] << 24) + (n[1] << 16) + (n[2] << 8) + n[3];
    node->src_ip_end = node->src_ip_begin + (1 << (32 - len)) - 1;

    for (i = 0; i < 4; i++) {
        sprintf(buf, "%s", strtok(NULL, tok));
        n[i] = atoi(buf);
    }
    str1 = (char *) strtok(NULL, tok);
    sprintf(buf, "%s", str1);
    len = atoi(buf);
    node->dst_ip_begin = (n[0] << 24) + (n[1] << 16) + (n[2] << 8) + n[3];
    node->dst_ip_end = node->dst_ip_begin + (1 << (32 - len)) - 1;

    str1 = (char *) strtok(NULL, tok);
    sprintf(buf, "%s", str1);
    node->src_port = atoi(buf) << 16;

    str1 = (char *) strtok(NULL, tok);
    sprintf(buf, "%s", str1);
    node->src_port += atoi(buf);

    str1 = (char *) strtok(NULL, tok);
    sprintf(buf, "%s", str1);
    node->dst_port = atoi(buf) << 16;

    str1 = (char *) strtok(NULL, tok);
    sprintf(buf, "%s", str1);
    node->dst_port += atoi(buf);

    str1 = (char *) strtok(NULL, tok);
    sprintf(buf, "%s", str1);
    node->protocol = strtoul(buf, NULL, 0) << 8;

    str1 = (char *) strtok(NULL, tok);
    sprintf(buf, "%s", str1);
    node->protocol += strtoul(buf, NULL, 0);
}
////////////////////////////////////////////////////////////////////////////////////
int search(struct ENTRY *node)
{
    int index;
    struct classifier *current = root;
    struct ENTRY_INDEX *tmp;
    int flag = 0;
    while (current) {
        switch (current->dim) {
        case (1):
            index = node->src_ip_begin > current->ep;
            break;
        case (2):
            index = node->dst_ip_begin > current->ep;
            break;
        case (3):
            index = node->src_port > current->ep;
            break;
        case (4):
            index = node->dst_port > current->ep;
            break;
        case (5):
            index = node->protocol > current->ep;
            break;
        }
        if (current->child[index] == NULL) {
            // Not Found
            return 0;
        } else if (current->num_child[index] == 0) {
            // next classifier
            current = (struct classifier *) current->child[index];
        } else {
            flag = 1;
            break;
        }
    }
    if (flag) {
        for (tmp = (struct ENTRY_INDEX *) (current->child[index]); tmp;
             tmp = tmp->next) {
            if (node->src_ip_begin == table[tmp->index].src_ip_begin &&
                /*node->src_ip_begin <= table[tmp->index].src_ip_end &&*/
                node->dst_ip_begin == table[tmp->index].dst_ip_begin &&
                /*node->dst_ip_begin <= table[tmp->index].dst_ip_end &&*/
                node->src_port >= (table[tmp->index].src_port >> 16) &&
                node->src_port <= (table[tmp->index].src_port & 0xffff) &&
                node->dst_port >= (table[tmp->index].dst_port >> 16) &&
                node->dst_port <= (table[tmp->index].dst_port & 0xffff) &&
                node->protocol == table[tmp->index].protocol)
                return 1;
        }
        return 0;
    }

    return 0;
    /*for (j = 31; j >= (-1); j--) {
        if (current == NULL)
            break;
        if (current->port != 256)
            temp = current;
        if (ip & (1 << j)) {
            current = current->right;
        } else {
            current = current->left;
        }
    }*/

    /*if(temp==NULL)
      printf("default\n");
      else
      printf("%u\n",temp->port);*/
}
////////////////////////////////////////////////////////////////////////////////////
void set_table(char *file_name)
{
    FILE *fp;
    char string[200];
    fp = fopen(file_name, "r");
    while (fgets(string, 200, fp) != NULL) {
        num_entry++;
    }
    rewind(fp);
    table = (struct ENTRY *) malloc(num_entry * sizeof(struct ENTRY));
    num_entry = 0;
    while (fgets(string, 200, fp) != NULL) {
        read_table(string, &table[num_entry]);
        table[num_entry].next = NULL;
        num_entry++;
    }
    fclose(fp);
}
////////////////////////////////////////////////////////////////////////////////////
void set_query(char *file_name)
{
    FILE *fp;
    char string[100];
    fp = fopen(file_name, "r");
    while (fgets(string, 200, fp) != NULL) {
        num_query++;
    }
    rewind(fp);
    query = (struct ENTRY *) malloc(num_query * sizeof(struct ENTRY));
    my_clock = (unsigned long long int *) malloc(num_query *
                                              sizeof(unsigned long long int));
    num_query = 0;
    srand(time(NULL));
    unsigned int b, e;
    while (fgets(string, 200, fp) != NULL) {
        read_table(string, &query[num_query]);
        /*b = query[num_query].src_ip_begin;*/
        /*e = query[num_query].src_ip_end;*/
        /*query[num_query].src_ip_begin = b + (rand() % (e - b + 1));*/

        /*b = query[num_query].dst_ip_begin;*/
        /*e = query[num_query].dst_ip_end;*/
        /*query[num_query].dst_ip_begin = b + (rand() % (e - b + 1));*/

        b = query[num_query].src_port >> 16;
        e = query[num_query].src_port & 0xffff;
        query[num_query].src_port = b + (rand() % (e - b + 1));

        b = query[num_query].dst_port >> 16;
        e = query[num_query].dst_port & 0xffff;
        query[num_query].dst_port = b + (rand() % (e - b + 1));

        query[num_query].next = NULL;
        my_clock[num_query++] = 10000000;
    }
    fclose(fp);
}
////////////////////////////////////////////////////////////////////////////////////
void count_distinct_node(struct bt_node *ptr)
{
    if (ptr == NULL) {
        return;
    }
    count_distinct_node(ptr->left);
    if (ptr->port != 256) {
        N++;
    }
    count_distinct_node(ptr->right);
}
////////////////////////////////////////////////////////////////////////////////////
void add_port_node(struct port_root *begin, unsigned int end)
{
    if (begin->child == NULL) {
        begin->child = (struct bt_node *) malloc(sizeof(struct bt_node));
        begin->child->port = end;
        begin->child->left = NULL;
        begin->child->right = NULL;
        begin->num = 1;
        return;
    }
    struct bt_node *now, *new_node;
    new_node = (struct bt_node *) malloc(sizeof(struct bt_node));
    new_node->port = end;
    new_node->left = NULL;
    new_node->right = NULL;
    new_node->num = 1;
    now = begin->child;
    while (now) {
        if (end > now->port) {
            if (now->right == NULL) {
                now->right = new_node;
                begin->num++;
                return;
            } else {
                now = now->right;
            }
        } else if (end < now->port) {
            if (now->left == NULL) {
                now->left = new_node;
                begin->num++;
                return;
            } else {
                now = now->left;
            }
        } else {
            free(new_node);
            now->num += 1;
            return;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////
void free_bt_tree(struct bt_node *node)
{
    if (node == NULL) {
        return;
    }
    free_bt_tree(node->left);
    free_bt_tree(node->right);
    free(node);
}
void free_port_tree(struct port_root *node)
{
    int i;
    for (i = 0; i < 65536; i++) {
        free_bt_tree(node[i].child);
        /*if (node[i].child) {
            free_bt_tree(node[i].child->left);
            free_bt_tree(node[i].child->right);
            free(node[i].child);
        }*/
    }
    free(node);
}
////////////////////////////////////////////////////////////////////////////////////

int comp_ep(const void *r1, const void *r2)
{
    int a = *(int *) r1;
    int b = *(int *) r2;
    return a - b;
}
////////////////////////////////////////////////////////////////////////////////////
unsigned int *sort_endpoint(struct ENTRY_INDEX *index,
                            int num_rule,
                            int dim,
                            int *num_endpoint)
{
    struct ENTRY_INDEX *now = index;
    unsigned int *new_table;
    new_table = (unsigned int *) malloc(sizeof(unsigned int) * num_rule * 2);
    int i;
    switch (dim) {
    case (1):
        for (i = 0; i < num_rule * 2; i += 2) {
            new_table[i] = table[now->index].src_ip_begin;
            new_table[i + 1] = table[now->index].src_ip_end;
            now = now->next;
        }
        break;
    case (2):
        for (i = 0; i < num_rule * 2; i += 2) {
            new_table[i] = table[now->index].dst_ip_begin;
            new_table[i + 1] = table[now->index].dst_ip_end;
            now = now->next;
        }
        break;
    case (3):
        for (i = 0; i < num_rule * 2; i += 2) {
            new_table[i] = table[now->index].src_port >> 16;
            new_table[i + 1] = table[now->index].src_port & 0xffff;
            now = now->next;
        }
        break;
    case (4):
        for (i = 0; i < num_rule * 2; i += 2) {
            new_table[i] = table[now->index].dst_port >> 16;
            new_table[i + 1] = table[now->index].dst_port & 0xffff;
            now = now->next;
        }
        break;
    case (5):
        for (i = 0; i < num_rule; i++) {
            new_table[i] = table[now->index].protocol;
            now = now->next;
        }
        break;
    }
    if (dim < 5)
        qsort(new_table, num_rule * 2, sizeof(unsigned int), comp_ep);
    else
        qsort(new_table, num_rule, sizeof(unsigned int), comp_ep);


    // delete repeat pattern
    int j = 0;
    unsigned int *distinct_table;
    distinct_table = (unsigned int *) malloc(sizeof(unsigned int) * (num_rule));
    distinct_table[0] = new_table[0];
    *num_endpoint = 1;
    for (i = 1; i < num_rule; i++) {
        if (new_table[i] != new_table[i - 1]) {
            distinct_table[j++] = new_table[i];
            *num_endpoint += 1;
        }
    }
    distinct_table = (unsigned int *) realloc(
        distinct_table, sizeof(unsigned int) * (*num_endpoint));

    free(new_table);
    return distinct_table;
}
int cal_sr(int dim,
           int num_endpoint,
           struct ENTRY_INDEX *node_index,
           unsigned int *distinct_table,
           unsigned int *sr)
{
    int i;
    unsigned int sum_sr = 0;
    struct ENTRY_INDEX *now = node_index;

    switch (dim) {
    case (1):
        for (i = 0; i < num_endpoint - 1; i++) {
            sr[i] = 0;
            for (now = node_index; now; now = now->next) {
                if (!((table[now->index].src_ip_begin < distinct_table[i]) &&
                      (table[now->index].src_ip_end < distinct_table[i])) ||
                    !((table[now->index].src_ip_begin >
                       distinct_table[i + 1]) &&
                      (table[now->index].src_ip_end > distinct_table[i + 1]))) {
                    sr[i]++;
                }
            }
            sum_sr += sr[i];
        }
        break;
    case (2):
        for (i = 0; i < num_endpoint - 1; i++) {
            sr[i] = 0;
            for (now = node_index; now; now = now->next) {
                if (!((table[now->index].dst_ip_begin < distinct_table[i]) &&
                      (table[now->index].dst_ip_end < distinct_table[i])) ||
                    !((table[now->index].dst_ip_begin >
                       distinct_table[i + 1]) &&
                      (table[now->index].dst_ip_end > distinct_table[i + 1]))) {
                    sr[i]++;
                }
            }
            sum_sr += sr[i];
        }
        break;
    case (3):
        for (i = 0; i < num_endpoint - 1; i++) {
            sr[i] = 0;
            for (now = node_index; now; now = now->next) {
                if (!(((table[now->index].src_port >> 16) <
                       distinct_table[i]) &&
                      ((table[now->index].src_port & 0xffff) <
                       distinct_table[i])) ||
                    !(((table[now->index].src_port >> 16) >
                       distinct_table[i + 1]) &&
                      ((table[now->index].src_port & 0xffff) >
                       distinct_table[i + 1]))) {
                    sr[i]++;
                }
            }
            sum_sr += sr[i];
        }
        break;
    case (4):
        for (i = 0; i < num_endpoint - 1; i++) {
            sr[i] = 0;
            for (now = node_index; now; now = now->next) {
                if (!(((table[now->index].dst_port >> 16) <
                       distinct_table[i]) &&
                      ((table[now->index].dst_port & 0xffff) <
                       distinct_table[i])) ||
                    !(((table[now->index].dst_port >> 16) >
                       distinct_table[i + 1]) &&
                      ((table[now->index].dst_port & 0xffff) >
                       distinct_table[i + 1]))) {
                    sr[i]++;
                }
            }
            sum_sr += sr[i];
        }
        break;
    case (5):
        for (i = 0; i < num_endpoint; i++) {
            sr[i] = 0;
            for (now = node_index; now; now = now->next) {
                if (table[now->index].protocol == distinct_table[i]) {
                    sr[i]++;
                }
            }
            sum_sr += sr[i];
        }
        break;
    }

    return sum_sr;
}
int pick_endpoint(int dim,
                  int num_endpoint,
                  unsigned int *distinct_table,
                  int num_rule,
                  struct ENTRY_INDEX *node_index)
{
#ifdef h1
#undef h2
#undef h3
    // find M/2
    return distinct_table[num_endpoint / 2];
#endif
    int i;
#ifdef h2
#undef h3
    // find m that num of rules overlap 0~m == (total num of rule)/2
    unsigned int sr;  // num of rule overlap the endpoint
    int index;
    unsigned int goal_num = num_rule / 2;
    struct ENTRY_INDEX *now = node_index;
    switch (dim) {
    case (1):
        for (i = 0; i < num_endpoint; i++) {
            sr = 0;
            for (now = node_index; now; now = now->next) {
                if (table[now->index].src_ip_begin <= distinct_table[i]) {
                    sr++;
                }
            }
            if (sr > goal_num) {
                return distinct_table[i - 1];
            }
        }
    case (2):
        for (i = 0; i < num_endpoint; i++) {
            sr = 0;
            for (now = node_index; now; now = now->next) {
                if (table[now->index].dst_ip_begin <= distinct_table[i]) {
                    sr++;
                }
            }
            if (sr > goal_num) {
                return distinct_table[i - 1];
            }
        }
    case (3):
        for (i = 0; i < num_endpoint; i++) {
            sr = 0;
            for (now = node_index; now; now = now->next) {
                if (table[now->index].src_port >> 16 <= distinct_table[i]) {
                    sr++;
                }
            }
            if (sr > goal_num) {
                return distinct_table[i - 1];
            }
        }
    case (4):
        for (i = 0; i < num_endpoint; i++) {
            sr = 0;
            for (now = node_index; now; now = now->next) {
                if (table[now->index].dst_port >> 16 <= distinct_table[i]) {
                    sr++;
                }
            }
            if (sr > goal_num) {
                return distinct_table[i - 1];
            }
        }
    case (5):
        for (i = 0; i < num_endpoint; i++) {
            sr = 0;
            for (now = node_index; now; now = now->next) {
                if (table[now->index].protocol <= distinct_table[i]) {
                    sr++;
                }
            }
            if (sr > goal_num) {
                return distinct_table[i - 1];
            }
        }
    }
#endif
#ifndef h3
#ifndef h2
#ifndef h1
#define h3
#endif
#endif
#endif
#ifdef h3
    unsigned int *sr;
    sr = (unsigned int *) malloc(sizeof(unsigned int) * num_endpoint);
    unsigned int sum_sr =
        cal_sr(dim, num_endpoint, node_index, distinct_table, sr);
    unsigned int tmp_sum = 0;
    for (i = 0; i < num_endpoint; i++) {
        tmp_sum += sr[i];
        if (tmp_sum > sum_sr / 2) {
            return distinct_table[i];
        }
    }
    printf("Found no endpoint %d\n", num_endpoint);
    exit(1);
#endif
    return -1;
}
////////////////////////////////////////////////////////////////////////////////////
void pick(struct ENTRY_INDEX *node_rule,
          int num_rule,
          int *dimension,
          unsigned int *endpoint)
{
    ///////// pick dimension
    unsigned int *(eps[5]);
    int M[5];
    // method 1: choose the most number of distinct endpoint
    int i;
#ifdef m1
    int max = 0;
#endif
    for (i = 0; i < 5; i++) {
        eps[i] = sort_endpoint(node_rule, num_rule, i + 1, &M[i]);
#ifdef m1
        if (M[i] > max) {
            *dimension = i + 1;
            max = M[i];
        }
#endif
    }

    ////// pick endpoint
#ifndef m1
    // method 2: find the field with minimum (1/M)sum(∑Sr[j])
    double entropy[5];
    unsigned int *sr;
    double min_entropy = 999999;
    for (i = 0; i < 5; i++) {
        sr = (unsigned int *) malloc(sizeof(unsigned int) * M[i]);
        entropy[i] = cal_sr(i + 1, M[i], node_rule, eps[i], sr) / (double) M[i];
        if (entropy[i] < min_entropy && entropy[i] != 0) {
            min_entropy = entropy[i];
            *dimension = i + 1;
        }
        free(sr);
    }

#endif
    *endpoint = pick_endpoint(*dimension, M[(*dimension) - 1],
                              eps[(*dimension) - 1], num_rule, node_rule);
}

////////////////////////////////////////////////////////////////////////////////////

void classify(struct ENTRY_INDEX *node, struct classifier *clsr)
{
    unsigned int index = 0;
    int i, copy_time = 1;
    switch (clsr->dim) {
    case (1):
        index = (table[node->index].src_ip_begin <= clsr->ep) ? 0 : 1;
        copy_time =
            (table[node->index].src_ip_end > clsr->ep && index == 0) ? 2 : 1;
        break;
    case (2):
        index = (table[node->index].dst_ip_begin <= clsr->ep) ? 0 : 1;
        copy_time =
            (table[node->index].dst_ip_end > clsr->ep && index == 0) ? 2 : 1;
        break;
    case (3):
        index = ((table[node->index].src_port >> 16) <= clsr->ep) ? 0 : 1;
        copy_time =
            ((table[node->index].src_port & 0xffff) > clsr->ep && index == 0)
                ? 2
                : 1;
        break;
    case (4):
        index = ((table[node->index].dst_port >> 16) <= clsr->ep) ? 0 : 1;
        copy_time =
            ((table[node->index].dst_port & 0xffff) > clsr->ep && index == 0)
                ? 2
                : 1;
        break;
    case (5):
        index = (table[node->index].protocol <= clsr->ep) ? 0 : 1;
        break;
    }

    for (i = 0; i < copy_time; i++) {
        struct ENTRY_INDEX *new_node;
        new_node = (struct ENTRY_INDEX *) malloc(sizeof(struct ENTRY_INDEX));
        new_node->index = node->index;
        new_node->next = (struct ENTRY_INDEX *) clsr->child[index + i];
        clsr->child[index + i] = new_node;
        clsr->num_child[index + i] += 1;
        num_node++;
    }
}


////////////////////////////////////////////////////////////////////////////////////
void *cut(struct ENTRY_INDEX *node, int num_rule)
{
    struct ENTRY_INDEX *new_table, *tmp;
    struct classifier *new_clsr;
    unsigned int i;
    new_clsr = (struct classifier *) malloc(sizeof(struct classifier));

    // preprocess
    if (root == NULL) {  // first cut
        struct ENTRY_INDEX *new_node;
        new_node = (struct ENTRY_INDEX *) malloc(sizeof(struct ENTRY_INDEX));
        new_node->index = 0;
        new_table = new_node;
        tmp = new_node;
        new_node = NULL;

        for (i = 1; i < (unsigned int) num_rule; i++) {
            new_node =
                (struct ENTRY_INDEX *) malloc(sizeof(struct ENTRY_INDEX));
            new_node->index = i;
            tmp->next = new_node;
            tmp = new_node;
            new_node = NULL;
        }


        root = new_clsr;
    } else {
        new_table = node;
    }

    // pick dimension and endpoint to cut
    pick(new_table, num_rule, &(new_clsr->dim), &(new_clsr->ep));
    new_clsr->child = (void **) malloc(sizeof(struct ENTRY_INDEX *) * 2);

    new_clsr->num_child = (int *) malloc(sizeof(int) * 2);
    for (i = 0; i < 2; i++) {
        new_clsr->child[i] = NULL;
        new_clsr->num_child[i] = 0;
    }
    while (new_table) {
        tmp = new_table;
        classify(new_table, new_clsr);
        new_table = new_table->next;
        free(tmp);
    }
    /*for (i = 0; i < new_clsr->np; i++) {*/
    /*printf("%d ", new_clsr->num_child[i]);*/
    /*}*/

    for (i = 0; i < 2; i++) {
        if (new_clsr->num_child[i] > BINTH &&
            new_clsr->num_child[i] < num_rule) {
            new_clsr->child[i] =
                (void *) cut((struct ENTRY_INDEX *) new_clsr->child[i],
                             new_clsr->num_child[i]);
#ifdef cut
            printf("new_cut: dim: %d, rules: %d, ori_rules: %d\n",
                   ((struct classifier *) (new_clsr->child[i]))->dim,
                   new_clsr->num_child[i], num_rule);
#endif
            new_clsr->num_child[i] = 0;
            num_clsr++;
        } else if (new_clsr->num_child[i] > BINTH && new_clsr->num_child[i] >= num_rule){
            printf("could not cut rules: %d, dim: %d, ep: %u\n",
                   new_clsr->num_child[i], new_clsr->dim, new_clsr->ep);
        }
    }
    return new_clsr;
}
////////////////////////////////////////////////////////////////////////////////////
void count_node(struct bt_node *r)
{
    if (r == NULL)
        return;
    count_node(r->left);
    N++;
    count_node(r->right);
}
////////////////////////////////////////////////////////////////////////////////////
void CountClock()
{
    unsigned int i;
    unsigned int *NumCntClock =
        (unsigned int *) malloc(100 * sizeof(unsigned int));
    for (i = 0; i < 100; i++)
        NumCntClock[i] = 0;
    unsigned long long MinClock = 10000000, MaxClock = 0;
    for (i = 0; i < num_query; i++) {
        if (my_clock[i] > MaxClock)
            MaxClock = my_clock[i];
        if (my_clock[i] < MinClock)
            MinClock = my_clock[i];
        if (my_clock[i] / 100 < 100)
            NumCntClock[my_clock[i] / 100]++;
        else
            NumCntClock[99]++;
    }
    printf("(MaxClock, MinClock) =\t(%5llu, %5llu)\n", MaxClock, MinClock);

    for (i = 0; i < 100; i++) {
        printf("%f\n", (double)NumCntClock[i]/num_entry);
    }
    return;
}

void shuffle(struct ENTRY *array, int n)
{
    srand((unsigned) time(NULL));
    struct ENTRY *temp = (struct ENTRY *) malloc(sizeof(struct ENTRY));

    int i;
    for (i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        memcpy(temp, &array[j], sizeof(struct ENTRY));
        memcpy(&array[j], &array[i], sizeof(struct ENTRY));
        memcpy(&array[i], temp, sizeof(struct ENTRY));
    }
}
void free_classifier(struct classifier *node)
{
    if (node == NULL) {
        return;
    }
    struct ENTRY_INDEX *tmp, *for_free;
    unsigned int i;
    for (i = 0; i < 2; i++) {
        if (node->num_child[i] == 0) {
            free_classifier((struct classifier *) node->child[i]);
        } else if (node->child[i]) {
            tmp = (struct ENTRY_INDEX *) (node->child[i]);
            while (tmp) {
                for_free = tmp;
                tmp = tmp->next;
                for_free->next = NULL;
                free(for_free);
            }
        }
    }
    free(node->child);
    free(node->num_child);
    free(node);
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Please execute the file as the following way:\n");
        printf("%s  routing_table_file_name  query_table_file_name\n",argv[0]);
        exit(1);
    }
    unsigned int i, j;
    // set_query(argv[2]);
    // set_table(argv[1]);
    set_query(argv[1]);
    set_table(argv[1]);
    begin = rdtsc();
    cut(NULL, num_entry);
    end = rdtsc();
    printf("Avg. Insert:\t%llu\n", (end - begin) / num_entry);

    // shuffle(query, num_entry);
    ////////////////////////////////////////////////////////////////////////////
    for (j = 0; j < 100; j++) {
        for (i = 0; i < num_query; i++) {
            begin = rdtsc();
            search(&query[i]);
#ifdef debug
            if (search(&query[i]) == 0) {
                printf("Not Found %d\n", i);
                exit(1);
            }
#endif
            end = rdtsc();
            if (my_clock[i] > (end - begin))
                my_clock[i] = (end - begin);
        }
    }
    total = 0;
    for (j = 0; j < num_query; j++)
        total += my_clock[j];
    printf("Avg. Search:\t%llu\n", total / num_query);
    printf("number of nodes:\t%d\n", num_node);
    printf("Total memory requirement:\t%ld KB\n",
           ((num_node * sizeof(struct ENTRY_INDEX) +
             num_clsr * sizeof(struct classifier)) /
            1024));
    CountClock();
    ////////////////////////////////////////////////////////////////////////////
    // count_node(root);
    // printf("There are %d nodes in binary trie\n",N);
    free_classifier(root);
    free(query);
    free(table);
    free(my_clock);
    return 0;
}
