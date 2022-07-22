/******************************************************************************
 * File:             rfc.c
 *
 * Author:           ZhuMon
 * Created:          08/16/20
 *                   Recursive Flow Classification (RFC)
 *****************************************************************************/
/*
 *
obj=rfc
DEBUG=debug
FREE=MY_FREE
CFLAG=-D${DEBUG} -D${FREE}

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
struct ENTRY_INDEX {
    int index;
    struct ENTRY_INDEX *next;
};
struct CHUNK {
    int eqid;
    unsigned index;
    struct ENTRY_INDEX *rule;
    struct ENTRY_INDEX *tail;
};
struct CHUNK *chunk_16bit[7];
struct CHUNK *chunk_p1[3];
struct CHUNK *chunk_p2;
int max_eqid[7] = {};
int max_eqid_p1[3] = {};


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
    struct ENTRY_INDEX *tmp;

    int seven_dim[7];
    seven_dim[0] = node->src_ip_begin >> 16;
    seven_dim[1] = node->src_ip_begin & 0xffff;
    seven_dim[2] = node->dst_ip_begin >> 16;
    seven_dim[3] = node->dst_ip_begin & 0xffff;
    seven_dim[4] = node->src_port >> 16;
    seven_dim[5] = node->dst_port >> 16;
    seven_dim[6] = node->protocol;

    int eqid_p0[7];
    int i;
    for (i = 0; i < 7; i++) {
        eqid_p0[i] = chunk_16bit[i][seven_dim[i]].eqid;
    }
    int p1_index[3] = {eqid_p0[0] * (max_eqid[1] + 1) + eqid_p0[1],
                       eqid_p0[2] * (max_eqid[3] + 1) + eqid_p0[3],
                       eqid_p0[4] * (max_eqid[5] + 1) * (max_eqid[6] + 1) +
                           eqid_p0[5] * (max_eqid[6] + 1) + eqid_p0[6]};
    int eqid_p1[3];
    for (i = 0; i < 3; i++) {
        eqid_p1[i] = chunk_p1[i][p1_index[i]].eqid;
    }

    unsigned int eqid_p2 =
        eqid_p1[0] * (max_eqid_p1[1] + 1) * (max_eqid_p1[2] + 1) +
        eqid_p1[1] * (max_eqid_p1[2] + 1) + eqid_p1[2];

    tmp = chunk_p2[eqid_p2].rule;
    while (tmp) {
        if (node->src_ip_begin == table[tmp->index].src_ip_begin &&
            /*node->src_ip_begin <= table[tmp->index].src_ip_end &&*/
            node->dst_ip_begin == table[tmp->index].dst_ip_begin &&
            /*node->dst_ip_begin <= table[tmp->index].dst_ip_end &&*/
            node->src_port == (table[tmp->index].src_port) &&
            node->dst_port == (table[tmp->index].dst_port) &&
            /*node->src_port >= (table[tmp->index].src_port >> 16) &&*/
            /*node->src_port <= (table[tmp->index].src_port & 0xffff) &&*/
            /*node->dst_port >= (table[tmp->index].dst_port >> 16) &&*/
            /*node->dst_port <= (table[tmp->index].dst_port & 0xffff) &&*/
            node->protocol == table[tmp->index].protocol)
            return 1;
        tmp = tmp->next;
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
    /*unsigned int b, e;*/
    while (fgets(string, 200, fp) != NULL) {
        read_table(string, &query[num_query]);
        /*b = query[num_query].src_ip_begin;*/
        /*e = query[num_query].src_ip_end;*/
        /*query[num_query].src_ip_begin = b + (rand() % (e - b + 1));*/

        /*b = query[num_query].dst_ip_begin;*/
        /*e = query[num_query].dst_ip_end;*/
        /*query[num_query].dst_ip_begin = b + (rand() % (e - b + 1));*/

        /*b = query[num_query].src_port >> 16;*/
        /*e = query[num_query].src_port & 0xffff;*/
        /*query[num_query].src_port = b + (rand() % (e - b + 1));*/

        /*b = query[num_query].dst_port >> 16;*/
        /*e = query[num_query].dst_port & 0xffff;*/
        /*query[num_query].dst_port = b + (rand() % (e - b + 1));*/

        query[num_query].next = NULL;
        my_clock[num_query++] = 10000000;
    }
    fclose(fp);
}

////////////////////////////////////////////////////////////////////////////////////
int comp_eqid(const void *a, const void *b)
{
    struct CHUNK *ca = (struct CHUNK *) a;
    struct CHUNK *cb = (struct CHUNK *) b;

    struct ENTRY_INDEX *ta, *tb;
    ta = ca->rule;
    tb = cb->rule;
    while (ta && tb) {
        if (ta->index == tb->index) {
            ta = ta->next;
            tb = tb->next;
        } else {
            return ta->index - tb->index;
        }
    }

    if (ta) {
        return 1;
    }
    if (tb) {
        return -1;
    }

    return 0;
}

struct ENTRY_INDEX *merge_index_list(struct ENTRY_INDEX *a,
                                     struct ENTRY_INDEX *b)
{
    struct ENTRY_INDEX h;
    struct ENTRY_INDEX *t, *tmp;

    t = &h;
    h.next = NULL;

    if (!a && !b)
        return NULL;

    while (a && b) {
        struct ENTRY_INDEX *new_node;
        new_node = (struct ENTRY_INDEX *) malloc(sizeof(struct ENTRY_INDEX));
        new_node->next = NULL;

        if (a->index < b->index) {
            new_node->index = a->index;
            a = a->next;
        } else if (a->index > b->index) {
            new_node->index = b->index;
            b = b->next;
        } else {
            new_node->index = a->index;
            a = a->next;
            b = b->next;
        }


        t->next = new_node;
        t = t->next;
    }
    tmp = a ? a : (b ? b : NULL);
    while (tmp) {
        struct ENTRY_INDEX *new_node;
        new_node = (struct ENTRY_INDEX *) malloc(sizeof(struct ENTRY_INDEX));
        new_node->index = tmp->index;
        new_node->next = NULL;
        t->next = new_node;
        t = t->next;
        tmp = tmp->next;
    }
    return h.next;
}
void construct_chunk()
{
    int i, k;
    unsigned int j;

    for (i = 0; i < 7; ++i) {
        chunk_16bit[i] = (struct CHUNK *) calloc(65536, sizeof(struct CHUNK));
    }
    unsigned int rule_range[12];
    struct ENTRY_INDEX *new_node, *tmp;
    for (i = 0; i < num_entry; i++) {
        rule_range[0] = table[i].src_ip_begin >> 16;
        rule_range[1] = table[i].src_ip_end >> 16;
        rule_range[2] = table[i].src_ip_begin & 0xffff;
        rule_range[3] = table[i].src_ip_end & 0xffff;
        rule_range[4] = table[i].dst_ip_begin >> 16;
        rule_range[5] = table[i].dst_ip_end >> 16;
        rule_range[6] = table[i].dst_ip_begin & 0xffff;
        rule_range[7] = table[i].dst_ip_end & 0xffff;
        rule_range[8] = table[i].src_port >> 16;
        rule_range[9] = table[i].src_port & 0xffff;
        rule_range[10] = table[i].dst_port >> 16;
        rule_range[11] = table[i].dst_port & 0xffff;


        for (k = 0; k < 6; k++) {
            for (j = rule_range[k * 2]; j <= rule_range[k * 2 + 1]; j++) {
                new_node = NULL;
                new_node =
                    (struct ENTRY_INDEX *) malloc(sizeof(struct ENTRY_INDEX));
                new_node->index = i;
                new_node->next = NULL;
                if (!chunk_16bit[k][j].rule) {
                    chunk_16bit[k][j].rule = new_node;
                    chunk_16bit[k][j].tail = new_node;
                } else {
                    chunk_16bit[k][j].tail->next = new_node;
                    chunk_16bit[k][j].tail = chunk_16bit[k][j].tail->next;
                }
                chunk_16bit[k][j].index = j;
            }
        }
        new_node = NULL;
        new_node = (struct ENTRY_INDEX *) malloc(sizeof(struct ENTRY_INDEX));
        new_node->index = i;
        new_node->next = NULL;
        k = 6;
        j = table[i].protocol;
        if (!chunk_16bit[k][j].rule) {
            chunk_16bit[k][j].rule = new_node;
            chunk_16bit[k][j].tail = new_node;
        } else {
            chunk_16bit[k][j].tail->next = new_node;
            chunk_16bit[k][j].tail = chunk_16bit[k][j].tail->next;
        }
        chunk_16bit[k][j].index = j;
    }

    // Compute eqid
    struct CHUNK *distinct_chunk_p0[7];
    int d;
    for (i = 0; i < 7; ++i) {
        struct CHUNK *sorted_chunk_p0 = NULL;
        sorted_chunk_p0 = (struct CHUNK *) calloc(65536, sizeof(struct CHUNK));
        distinct_chunk_p0[i] =
            (struct CHUNK *) malloc(sizeof(struct CHUNK) * 65536);

        memcpy(sorted_chunk_p0, chunk_16bit[i], sizeof(struct CHUNK) * 65536);
        qsort(sorted_chunk_p0, 65536, sizeof(struct CHUNK), comp_eqid);

        // assign eqid
        d = 0;
        sorted_chunk_p0[0].eqid = 0;
        chunk_16bit[i][sorted_chunk_p0[0].index].eqid = 0;
        memcpy(&distinct_chunk_p0[i][d], &sorted_chunk_p0[0],
               sizeof(struct CHUNK));

        for (j = 1; j < 65536; ++j) {
            if (comp_eqid(&sorted_chunk_p0[j - 1], &sorted_chunk_p0[j]) != 0) {
                d++;
                sorted_chunk_p0[j].eqid = d;
                chunk_16bit[i][sorted_chunk_p0[j].index].eqid = d;
                memcpy(&distinct_chunk_p0[i][d], &sorted_chunk_p0[j],
                       sizeof(struct CHUNK));
            } else {
                sorted_chunk_p0[j].eqid = d;
                chunk_16bit[i][sorted_chunk_p0[j].index].eqid = d;
                // free useless rule
                tmp = sorted_chunk_p0[j].rule;
                while (sorted_chunk_p0[j].rule) {
                    sorted_chunk_p0[j].rule = sorted_chunk_p0[j].rule->next;
                    free(tmp);
                    tmp = sorted_chunk_p0[j].rule;
                }
                sorted_chunk_p0[j].rule = sorted_chunk_p0[j - 1].rule;
                sorted_chunk_p0[j].tail = sorted_chunk_p0[j - 1].tail;
            }
        }
        max_eqid[i] = d;
        distinct_chunk_p0[i] = (struct CHUNK *) realloc(
            distinct_chunk_p0[i], sizeof(struct CHUNK) * (max_eqid[i] + 1));
        free(sorted_chunk_p0);

        if (i % 2 == 1 && i < 4) {
            chunk_p1[i / 2] = (struct CHUNK *) malloc(sizeof(struct CHUNK) *
                                                      (max_eqid[i - 1] + 1) *
                                                      (max_eqid[i] + 1));

            for (d = 0; d < max_eqid[i - 1] + 1; d++) {
                for (k = 0; k < max_eqid[i] + 1; k++) {
                    j = d * (max_eqid[i] + 1) + k;
                    chunk_p1[i / 2][j].index = j;
                    chunk_p1[i / 2][j].rule =
                        merge_index_list(distinct_chunk_p0[i - 1][d].rule,
                                         distinct_chunk_p0[i][k].rule);
                }
#ifdef MY_FREE
                tmp = distinct_chunk_p0[i - 1][d].rule;
                // free rule in distinct_chunk_p0[i-1]
                while (distinct_chunk_p0[i - 1][d].rule) {
                    distinct_chunk_p0[i - 1][d].rule =
                        distinct_chunk_p0[i - 1][d].rule->next;
                    free(tmp);
                    tmp = distinct_chunk_p0[i - 1][d].rule;
                }
#endif
            }

#ifdef MY_FREE
            free(distinct_chunk_p0[i - 1]);
            // free rule in distinct_chunk_p0[i]
            for (k = 0; k < max_eqid[i] + 1; k++) {
                tmp = distinct_chunk_p0[i][k].rule;
                while (distinct_chunk_p0[i][k].rule) {
                    distinct_chunk_p0[i][k].rule =
                        distinct_chunk_p0[i][k].rule->next;
                    free(tmp);
                    tmp = distinct_chunk_p0[i][k].rule;
                }
            }
            free(distinct_chunk_p0[i]);
#endif
        }
    }

    for (i = 0; i < 7; i++) {
        printf("%d ", max_eqid[i]);
    }
    printf("\n");
    // merge chunk
    int l;
    chunk_p1[2] =
        (struct CHUNK *) malloc(sizeof(struct CHUNK) * (max_eqid[4] + 1) *
                                (max_eqid[5] + 1) * (max_eqid[6] + 1));
    for (i = 0; i < max_eqid[4] + 1; i++) {
        for (k = 0; k < max_eqid[5] + 1; k++) {
            for (l = 0; l < max_eqid[6] + 1; l++) {
                j = i * (max_eqid[5] + 1) * (max_eqid[6] + 1) +
                    k * (max_eqid[6] + 1) + l;
                chunk_p1[2][j].index = j;
                chunk_p1[2][j].rule = merge_index_list(
                    distinct_chunk_p0[4][i].rule, distinct_chunk_p0[5][k].rule);
                chunk_p1[2][j].rule = merge_index_list(
                    distinct_chunk_p0[6][l].rule, chunk_p1[2][j].rule);
            }
        }
#ifdef MY_FREE
        tmp = distinct_chunk_p0[4][i].rule;
        // free rule in distinct_chunk_p0[4]
        while (distinct_chunk_p0[4][i].rule) {
            distinct_chunk_p0[4][i].rule = distinct_chunk_p0[4][i].rule->next;
            free(tmp);
            tmp = distinct_chunk_p0[4][i].rule;
        }
#endif
    }
#ifdef MY_FREE
    free(distinct_chunk_p0[4]);
    for (i = 5; i < 7; i++) {
        for (k = 0; k < max_eqid[i] + 1; k++) {
            tmp = distinct_chunk_p0[i][k].rule;
            while (distinct_chunk_p0[i][k].rule) {
                distinct_chunk_p0[i][k].rule =
                    distinct_chunk_p0[i][k].rule->next;
                free(tmp);
                tmp = distinct_chunk_p0[i][k].rule;
            }
        }
        free(distinct_chunk_p0[i]);
    }
#endif

    int chunk_size[3] = {
        (max_eqid[0] + 1) * (max_eqid[1] + 1),
        (max_eqid[2] + 1) * (max_eqid[3] + 1),
        (max_eqid[4] + 1) * (max_eqid[5] + 1) * (max_eqid[6] + 1)};
    struct CHUNK *distinct_chunk_p1[3];

    for (i = 0; i < 3; i++) {
        struct CHUNK *sorted_chunk_p1;
        sorted_chunk_p1 =
            (struct CHUNK *) calloc(chunk_size[i], sizeof(struct CHUNK));
        distinct_chunk_p1[i] =
            (struct CHUNK *) calloc(chunk_size[i], sizeof(struct CHUNK));
        memcpy(sorted_chunk_p1, chunk_p1[i],
               sizeof(struct CHUNK) * chunk_size[i]);
        qsort(sorted_chunk_p1, chunk_size[i], sizeof(struct CHUNK), comp_eqid);

        d = 0;
        sorted_chunk_p1[0].eqid = 0;
        memcpy(&distinct_chunk_p1[i][d], &sorted_chunk_p1[0],
               sizeof(struct CHUNK));
        chunk_p1[i][sorted_chunk_p1[l].index].eqid = d;

        for (l = 1; l < chunk_size[i]; l++) {
            if (comp_eqid(&sorted_chunk_p1[l - 1], &sorted_chunk_p1[l]) != 0) {
                d++;
                sorted_chunk_p1[l].eqid = d;
                memcpy(&distinct_chunk_p1[i][d], &sorted_chunk_p1[l],
                       sizeof(struct CHUNK));
                max_eqid_p1[i] = d;
                chunk_p1[i][sorted_chunk_p1[l].index].eqid = d;
            } else {
                sorted_chunk_p1[l].eqid = d;
                chunk_p1[i][sorted_chunk_p1[l].index].eqid = d;
#ifdef MY_FREE
                tmp = sorted_chunk_p1[l].rule;
                while (sorted_chunk_p1[l].rule) {
                    sorted_chunk_p1[l].rule = sorted_chunk_p1[l].rule->next;
                    free(tmp);
                    tmp = sorted_chunk_p1[l].rule;
                }
#endif
                sorted_chunk_p1[l].rule = sorted_chunk_p1[l - 1].rule;
                sorted_chunk_p1[l].tail = sorted_chunk_p1[l - 1].tail;
            }
        }
        distinct_chunk_p1[i] = (struct CHUNK *) realloc(
            distinct_chunk_p1[i], sizeof(struct CHUNK) * (max_eqid_p1[i] + 1));
        free(sorted_chunk_p1);
    }
    chunk_p2 =
        (struct CHUNK *) malloc(sizeof(struct CHUNK) * (max_eqid_p1[0] + 1) *
                                (max_eqid_p1[1] + 1) * (max_eqid_p1[2] + 1));
    for (i = 0; i < max_eqid_p1[0] + 1; i++) {
        for (k = 0; k < max_eqid_p1[1] + 1; k++) {
            for (d = 0; d < max_eqid_p1[2] + 1; d++) {
                j = i * (max_eqid_p1[1] + 1) * (max_eqid_p1[2] + 1) +
                    k * (max_eqid_p1[2] + 1) + d;
                chunk_p2[j].index = j;
                chunk_p2[j].rule = merge_index_list(distinct_chunk_p1[0]->rule,
                                                    distinct_chunk_p1[1]->rule);
                chunk_p2[j].rule = merge_index_list(distinct_chunk_p1[2]->rule,
                                                    chunk_p2[j].rule);
                /*if (chunk_p2[j].rule) {*/
                /*printf("%d\n", j);*/
                /*}*/
            }
        }
    }
}



////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
void CountClock()
{
    unsigned int i;
    unsigned int *NumCntClock =
        (unsigned int *) malloc(50 * sizeof(unsigned int));
    for (i = 0; i < 50; i++)
        NumCntClock[i] = 0;
    unsigned long long MinClock = 10000000, MaxClock = 0;
    for (i = 0; i < num_query; i++) {
        if (my_clock[i] > MaxClock)
            MaxClock = my_clock[i];
        if (my_clock[i] < MinClock)
            MinClock = my_clock[i];
        if (my_clock[i] / 100 < 50)
            NumCntClock[my_clock[i] / 100]++;
        else
            NumCntClock[49]++;
    }
    printf("(MaxClock, MinClock) =\t(%5llu, %5llu)\n", MaxClock, MinClock);

    /*for (i = 0; i < 50; i++) {
        printf("%f\n", (double)NumCntClock[i]/num_entry);
    }*/
    free(NumCntClock);
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
    /*cut(NULL, num_entry);*/
    construct_chunk();
    end = rdtsc();
    printf("Avg. Insert:\t%llu\n", (end - begin) / num_entry);

    // shuffle(query, num_entry);
    ////////////////////////////////////////////////////////////////////////////
    for (j = 0; j < 100; j++) {
        for (i = 0; i < num_query; i++) {
            begin = rdtsc();
#ifndef debug
            search(&query[i]);
#else
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
    /*printf("number of nodes:\t%d\n", num_node);*/
    /*printf("Total memory requirement:\t%ld KB\n",*/
    /*((num_node * sizeof(struct ENTRY_INDEX) +*/
    /*num_clsr * sizeof(struct classifier)) /*/
    /*1024));*/
    CountClock();
    ////////////////////////////////////////////////////////////////////////////
    // count_node(root);
    // printf("There are %d nodes in binary trie\n",N);
    free(query);
    free(table);
    free(my_clock);
    return 0;
}
