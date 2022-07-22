#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<limits.h>

#define BINTH 50
////////////////////////////////////////////////////////////////////////////////////

static __inline__ unsigned long long rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

////////////////////////////////////////////////////////////////////////////////////s
enum fiveDim {
	NONE,
	SRCIP,
	DESIP,
	SRCPORT,
	DESPORT,
	PROTOCOL
};
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY {
	unsigned int src_ip;
	unsigned char src_len;
	unsigned int des_ip;
	unsigned char des_len;
	unsigned short int src_port_start;
	unsigned short int src_port_end;
	unsigned short int des_port_start;
	unsigned short int des_port_end;
	unsigned char protocol;
};
////////////////////////////////////////////////////////////////////////////////////
struct list {
	unsigned int port;
	struct list *left, *right;
};
typedef struct list node;
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
struct bucket {
	enum fiveDim cut_dim;
	struct ENTRY *internal_array;
	unsigned int arraySize;
	struct bucket *children;
	unsigned int num_children;
	unsigned int cut_point;
};
typedef struct bucket bnode;
typedef bnode *ctrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
ctrie root;
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
unsigned long long int begin, end, total = 0;
unsigned long long int *clock;
int counter;
int dim_count[5] = {0,0,0,0,0};
int num_leave = 0;
////////////////////////////////////////////////////////////////////////////////////
void update(struct ENTRY input) {
	struct bucket *ptr = root;
	int i;
	int find = 0;
	while (find != 1) {
		if (ptr->cut_dim == SRCIP) {
			if (input.src_ip >= ptr->cut_point)
				ptr = &(ptr->children[1]);
			else if (input.src_ip < ptr->cut_point)
				ptr = &(ptr->children[0]);
		}
		else if (ptr->cut_dim == DESIP) {
			if (input.des_ip >= ptr->cut_point)
				ptr = &(ptr->children[1]);
			else if (input.des_ip < ptr->cut_point) {
				ptr = &(ptr->children[0]);
			}
		}
		else if (ptr->cut_dim == SRCPORT) {
			if (input.src_port_end >= ptr->cut_point) {
				ptr = &(ptr->children[1]);
			}
			else if (input.src_port_end < ptr->cut_point) {
				ptr = &(ptr->children[0]);
			}
		}
		else if (ptr->cut_dim == DESPORT) {
			if (input.des_port_end >= ptr->cut_point) {
				ptr = &(ptr->children[1]);
			}
			else if (input.des_port_end < ptr->cut_point) {
				ptr = &(ptr->children[0]);
			}
		}
		else if (ptr->cut_dim == PROTOCOL) {
			if (input.protocol >= ptr->cut_point) {
				ptr = &(ptr->children[1]);
			}
			else if (input.protocol < ptr->cut_point) {
				ptr = &(ptr->children[0]);
			}
		}
		else
			break;
	}

	struct ENTRY *temp = malloc(sizeof(struct ENTRY) * (ptr->arraySize + 1));
	for (i = 0; i < ptr->arraySize; i++) {
		temp[i].src_ip = ptr->internal_array[i].src_ip;
		temp[i].src_len = ptr->internal_array[i].src_len;
		temp[i].des_ip = ptr->internal_array[i].des_ip;
		temp[i].des_len = ptr->internal_array[i].des_len;
		temp[i].src_port_start = ptr->internal_array[i].src_port_start;
		temp[i].src_port_end = ptr->internal_array[i].src_port_end;
		temp[i].des_port_start = ptr->internal_array[i].des_port_start;
		temp[i].des_port_end = ptr->internal_array[i].des_port_end;
		temp[i].protocol = ptr->internal_array[i].protocol;
	}
	temp[i].src_ip = input.src_ip;
	temp[i].src_len = input.src_len;
	temp[i].des_ip = input.des_ip;
	temp[i].des_len = input.des_len;
	temp[i].src_port_start = input.src_port_start;
	temp[i].src_port_end = input.src_port_end;
	temp[i].des_port_start = input.des_port_start;
	temp[i].des_port_end = input.des_port_end;
	temp[i].protocol = input.protocol;
	ptr->arraySize++;
	free(ptr->internal_array);
	ptr->internal_array = temp;
}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned int src_ip, unsigned int des_ip, unsigned int src_port, unsigned int des_port, unsigned int protocol) {
	struct bucket *ptr = root;
	int find = 0;
	int target = -1;
	unsigned int index;
	//find leave node
	while (find != 1) {
		if (ptr->cut_dim == SRCIP) {
			if (src_ip >= ptr->cut_point)
				ptr = &(ptr->children[1]);
			else if (src_ip < ptr->cut_point)
				ptr = &(ptr->children[0]);
		}
		else if (ptr->cut_dim == DESIP) {
			if (des_ip >= ptr->cut_point)
				ptr = &(ptr->children[1]);
			else if (des_ip < ptr->cut_point)
				ptr = &(ptr->children[0]);
		}
		else if (ptr->cut_dim == SRCPORT) {
			if (src_port >= ptr->cut_point)
				ptr = &(ptr->children[1]);
			else if (src_port < ptr->cut_point)
				ptr = &(ptr->children[0]);
		}
		else if (ptr->cut_dim == DESPORT) {
			if (des_port >= ptr->cut_point)
				ptr = &(ptr->children[1]);
			else if (des_port < ptr->cut_point)
				ptr = &(ptr->children[0]);
		}
		else if (ptr->cut_dim == PROTOCOL) {
			if (protocol >= ptr->cut_point)
				ptr = &(ptr->children[1]);
			else if (protocol < ptr->cut_point)
				ptr = &(ptr->children[0]);
		}
		else
			break;
	}
	
	//sequence search
	for (int i = 0; i < ptr->arraySize; i++) {
		if (ptr->internal_array[i].src_ip << (32 - ptr->internal_array[i].src_len) == src_ip << (32 - ptr->internal_array[i].src_len)) {
			if (ptr->internal_array[i].des_ip << (32 - ptr->internal_array[i].des_len) == des_ip << (32 - ptr->internal_array[i].des_len)) {
				if (ptr->internal_array[i].src_port_start <= src_port && ptr->internal_array[i].src_port_end >= src_port && ptr->internal_array[i].des_port_start <= des_port && ptr->internal_array[i].des_port_end >= des_port) {
					if (ptr->internal_array[i].protocol == protocol) {
						target = ptr->internal_array[i].des_ip;
						break;
					}
				}
			}
		}
	}
	/*
	if (target == -1)
		printf("**********************not find****************************************\n");
	else
		printf("%x\n", target);
	*/
}
////////////////////////////////////////////////////////////////////////////////////
int cmpfunc(const void *a, const void *b) {
	unsigned int pa = *(unsigned int*)a;
	unsigned int pb = *(unsigned int*)b;
	if (pa < pb) return -1;
	else if (pa == pb) return 0;
	else return 1;
}
////////////////////////////////////////////////////////////////////////////////////
btrie create_node() {
	btrie temp;
	temp = (btrie)malloc(sizeof(node));
	temp->right = NULL;
	temp->left = NULL;
	temp->port = 65536;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
ctrie create_bnode() {
	ctrie temp;
	temp = (ctrie)malloc(sizeof(bnode));
	temp->cut_dim = NONE;
	temp->internal_array = NULL;
	temp->arraySize = 0;
	temp->children = NULL;
	temp->num_children = 0;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
int addr_tree_addnode(btrie r, unsigned int ip, unsigned char len, unsigned short int hop) {
	btrie ptr = r;
	int i;
	for (i = 0; i < len; i++) {
		if (ip&(1 << (31 - i))) {
			if (ptr->right == NULL)
				ptr->right = create_node();
			ptr = ptr->right;
			if ((i == len - 1) && (ptr->port == 65536))
				ptr->port = hop;
		}
		else {
			if (ptr->left == NULL)
				ptr->left = create_node();
			ptr = ptr->left;
			if ((i == len - 1) && (ptr->port == 65536))
				ptr->port = hop;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void port_seg_addnode(btrie* port_seg, unsigned short int port_start, unsigned short int port_end) {
	btrie ptr = port_seg[port_start];
	int i;
	for (i = 0; i < 16; i++) {
		if (ptr->port == 65536) {
			ptr->port = port_end;
			return;
		}
		else if (port_end > ptr->port) {
			if (ptr->right == NULL)
				ptr->right = create_node();
			ptr = ptr->right;
		}
		else if (port_end < ptr->port) {
			if (ptr->left == NULL)
				ptr->left = create_node();
			ptr = ptr->left;
		}
		else
			return;
	}
	ptr->port = port_end;
}
////////////////////////////////////////////////////////////////////////////////////
void count_tree_distinct(btrie p) {
	if (p == NULL)return;
	count_tree_distinct(p->left);
	count_tree_distinct(p->right);
	if (p->port != 65536)counter++;
}
////////////////////////////////////////////////////////////////////////////////////
void free_tree(btrie p) {
	if (p == NULL)return;
	free_tree(p->left);
	free_tree(p->right);
	free(p);
}
////////////////////////////////////////////////////////////////////////////////////
void find_cutOfDim(ctrie p) {
	int i;
	int index;
	int src_addr_DN, des_addr_DN, src_port_DN, des_port_DN, ptc_DN;
	btrie src_root, des_root, src_port_seg[65536], des_port_seg[65536];
	unsigned int ptc_seg[256];

	//initialize
	src_root = create_node();
	des_root = create_node();
	for (i = 0; i < 65536; i++) {
		src_port_seg[i] = create_node();
		des_port_seg[i] = create_node();
	}
	for (i = 0; i < 256; i++)
		ptc_seg[i] = 0;

	//add node with 5 dim
	for (i = 0; i < p->arraySize; i++) {
		addr_tree_addnode(src_root, p->internal_array[i].src_ip, p->internal_array[i].src_len, p->internal_array[i].src_port_start);
		addr_tree_addnode(des_root, p->internal_array[i].des_ip, p->internal_array[i].des_len, p->internal_array[i].des_port_start);
		port_seg_addnode(src_port_seg, p->internal_array[i].src_port_start, p->internal_array[i].src_port_end);
		port_seg_addnode(des_port_seg, p->internal_array[i].des_port_start, p->internal_array[i].des_port_end);
		ptc_seg[p->internal_array[i].protocol]++;
	}

	//count src address distinct number
	counter = 0;
	count_tree_distinct(src_root);
	src_addr_DN = counter;
	//count des address distinct number
	counter = 0;
	count_tree_distinct(des_root);
	des_addr_DN = counter;
	//count src port distinct number
	counter = 0;
	for (i = 0; i < 65536; i++) {
		if (src_port_seg[i]->port != 65536)
			count_tree_distinct(src_port_seg[i]);
	}
	src_port_DN = counter;
	//count des port distinct number
	counter = 0;
	for (i = 0; i < 65536; i++) {
		if (des_port_seg[i]->port != 65536)
			count_tree_distinct(des_port_seg[i]);
	}
	des_port_DN = counter;
	//count ptc distinct number
	counter = 0;
	for (i = 0; i < 256; i++) {
		if (ptc_seg[i] != 0)
			counter++;
	}
	ptc_DN = counter;

	//select max distinct number dim
	enum fiveDim max_dim = NONE;
	int max_number = INT_MIN;
	if (src_addr_DN >= des_addr_DN) {
		max_dim = SRCIP;
		max_number = src_addr_DN;
		dim_count[0]++;
	}
	else {
		max_dim = DESIP;
		max_number = des_addr_DN;
		dim_count[1]++;
	}
	if (src_port_DN > max_number) {
		max_dim = SRCPORT;
		max_number = src_port_DN;
		dim_count[2]++;
	}
	if (des_port_DN > max_number) {
		max_dim = DESPORT;
		max_number = des_port_DN;
		dim_count[3]++;
	}
	if (ptc_DN > max_number) {
		max_dim = PROTOCOL;
		max_number = ptc_DN;
		dim_count[4]++;
	}
	//free memory space
	free_tree(src_root);
	free_tree(des_root);
	for (i = 0; i < 65536; i++) {
		free_tree(src_port_seg[i]);
		free_tree(des_port_seg[i]);
	}
	p->cut_dim = max_dim;
}
////////////////////////////////////////////////////////////////////////////////////
void choose_numOfpart(ctrie ptr) {
	int i, j, k;
	unsigned int s_index, e_index, mid;
	unsigned int *seq_array = malloc(sizeof(unsigned int) * ptr->arraySize * 2);
	int count_array[2] = {0,0};//record num of left part and right part

	if (ptr->cut_dim == SRCIP) {
		//set seq array
		for (i = 0, j = 0; i < ptr->arraySize; i++) {
			if (ptr->internal_array[i].src_len < 32) {
				seq_array[j++] = ptr->internal_array[i].src_ip & (0xFFFFFFFF << (32 - ptr->internal_array[i].src_len));
				seq_array[j++] = ptr->internal_array[i].src_ip | (0xFFFFFFFF >> ptr->internal_array[i].src_len);
			}
			else {
				seq_array[j++] = ptr->internal_array[i].src_ip;
				seq_array[j++] = ptr->internal_array[i].src_ip;
			}
		}
		qsort(seq_array, ptr->arraySize * 2, sizeof(unsigned int), cmpfunc);
		mid = seq_array[ptr->arraySize];//cut point
		ptr->cut_point = mid;
		//count children internal array size
		for (i = 0; i < ptr->arraySize; i++) {
			if (ptr->internal_array[i].src_len < 32) {
				s_index  = ptr->internal_array[i].src_ip & (0xFFFFFFFF << (32 - ptr->internal_array[i].src_len));
				e_index = ptr->internal_array[i].src_ip | (0xFFFFFFFF >> ptr->internal_array[i].src_len);
			}
			else {
				s_index = e_index = ptr->internal_array[i].src_ip;
			}
			//count left part and right part
			if (s_index < mid && e_index < mid)
				count_array[0]++;
			else if (s_index >= mid && e_index >= mid)
				count_array[1]++;
			else {
				count_array[0]++;
				count_array[1]++;
			}
		}
		if ((count_array[0] >= ptr->arraySize) || count_array[1] >= ptr->arraySize) {
			return;
		}
		//new children
		ptr->num_children = 0;
		ptr->children = malloc(sizeof(struct bucket) * 2);
		(&ptr->children[0])->cut_dim = NONE;
		(&ptr->children[0])->num_children = 0;
		(&ptr->children[0])->cut_point = 0;
		(&ptr->children[0])->children = NULL;
		(&ptr->children[0])->arraySize = 0;
		(&ptr->children[0])->internal_array = malloc(sizeof(struct ENTRY) * count_array[0]);
		ptr->num_children++;
		(&ptr->children[1])->cut_dim = NONE;
		(&ptr->children[1])->num_children = 0;
		(&ptr->children[1])->cut_point = 0;
		(&ptr->children[1])->children = NULL;
		(&ptr->children[1])->arraySize = 0;
		(&ptr->children[1])->internal_array = malloc(sizeof(struct ENTRY) * count_array[1]);
		ptr->num_children++;
		//assign children
		for (i = 0; i < ptr->arraySize; i++) {
			if (ptr->internal_array[i].src_len < 32) {
				s_index = ptr->internal_array[i].src_ip & (0xFFFFFFFF << (32 - ptr->internal_array[i].src_len));
				e_index = ptr->internal_array[i].src_ip | (0xFFFFFFFF >> ptr->internal_array[i].src_len);
			}
			else {
				s_index = e_index = ptr->internal_array[i].src_ip;
			}
		
			if (s_index < mid && e_index < mid)
				(&ptr->children[0])->internal_array[(&ptr->children[0])->arraySize++] = ptr->internal_array[i];
			else if (s_index >= mid && e_index >= mid)
				(&ptr->children[1])->internal_array[(&ptr->children[1])->arraySize++] = ptr->internal_array[i];
			else {
				(&ptr->children[0])->internal_array[(&ptr->children[0])->arraySize++] = ptr->internal_array[i];
				(&ptr->children[1])->internal_array[(&ptr->children[1])->arraySize++] = ptr->internal_array[i];
			}
		}
	}
	else if (ptr->cut_dim == DESIP) {
		//set seq array
		for (i = 0, j = 0; i < ptr->arraySize; i++) {
			if (ptr->internal_array[i].des_len < 32) {
				seq_array[j++] = ptr->internal_array[i].des_ip & (0xFFFFFFFF << (32 - ptr->internal_array[i].des_len));
				seq_array[j++] = ptr->internal_array[i].des_ip | (0xFFFFFFFF >> ptr->internal_array[i].des_len);
			}
			else {
				seq_array[j++] = ptr->internal_array[i].des_ip;
				seq_array[j++] = ptr->internal_array[i].des_ip;
			}
		}
		qsort(seq_array, ptr->arraySize * 2, sizeof(unsigned int), cmpfunc);

		mid = seq_array[ptr->arraySize];//cut point
		ptr->cut_point = mid;
		//count children internal array size
		for (i = 0; i < ptr->arraySize; i++) {
			if (ptr->internal_array[i].des_len < 32) {
				s_index = ptr->internal_array[i].des_ip & (0xFFFFFFFF << (32 - ptr->internal_array[i].des_len));
				e_index = ptr->internal_array[i].des_ip | (0xFFFFFFFF >> ptr->internal_array[i].des_len);
			}
			else {
				s_index = e_index = ptr->internal_array[i].des_ip;
			}
			//count left part and right part
			if (s_index < mid && e_index < mid)
				count_array[0]++;
			else if (s_index >= mid && e_index >= mid)
				count_array[1]++;
			else {
				count_array[0]++;
				count_array[1]++;
			}
		}
		if ((count_array[0] >= ptr->arraySize) || count_array[1] >= ptr->arraySize) {
			ptr->num_children = 0;
			return;
		}
		//new children
		ptr->num_children = 0;
		ptr->children = malloc(sizeof(struct bucket) * 2);
		(&ptr->children[0])->cut_dim = NONE;
		(&ptr->children[0])->num_children = 0;
		(&ptr->children[0])->cut_point = 0;
		(&ptr->children[0])->children = NULL;
		(&ptr->children[0])->arraySize = 0;
		(&ptr->children[0])->internal_array = malloc(sizeof(struct ENTRY) * count_array[0]);
		ptr->num_children++;
		(&ptr->children[1])->cut_dim = NONE;
		(&ptr->children[1])->num_children = 0;
		(&ptr->children[1])->cut_point = 0;
		(&ptr->children[1])->children = NULL;
		(&ptr->children[1])->arraySize = 0;
		(&ptr->children[1])->internal_array = malloc(sizeof(struct ENTRY) * count_array[1]);
		ptr->num_children++;
		//assign children
		for (i = 0; i < ptr->arraySize; i++) {
			if (ptr->internal_array[i].des_len < 32) {
				s_index = ptr->internal_array[i].des_ip & (0xFFFFFFFF << (32 - ptr->internal_array[i].des_len));
				e_index = ptr->internal_array[i].des_ip | (0xFFFFFFFF >> ptr->internal_array[i].des_len);
			}
			else {
				s_index = e_index = ptr->internal_array[i].des_ip;
			}

			if (s_index < mid && e_index < mid)
				(&ptr->children[0])->internal_array[(&ptr->children[0])->arraySize++] = ptr->internal_array[i];
			else if (s_index >= mid && e_index >= mid)
				(&ptr->children[1])->internal_array[(&ptr->children[1])->arraySize++] = ptr->internal_array[i];
			else {
				(&ptr->children[0])->internal_array[(&ptr->children[0])->arraySize++] = ptr->internal_array[i];
				(&ptr->children[1])->internal_array[(&ptr->children[1])->arraySize++] = ptr->internal_array[i];
			}
		}
	}
	else if (ptr->cut_dim == SRCPORT) {
		for (i = 0, j = 0; i < ptr->arraySize; i++) {
			seq_array[j++] = (unsigned int)ptr->internal_array[i].src_port_start;
			seq_array[j++] = (unsigned int)ptr->internal_array[i].src_port_end;
		}
		qsort(seq_array, ptr->arraySize * 2, sizeof(unsigned int), cmpfunc);
		mid = seq_array[ptr->arraySize];//cut point
		ptr->cut_point = mid;
		//count children internal array size
		for (i = 0; i < ptr->arraySize; i++) {
			s_index = (unsigned int)ptr->internal_array[i].src_port_start;
			e_index = (unsigned int)ptr->internal_array[i].src_port_end;
			//count left part and right part
			if (s_index < mid && e_index < mid)
				count_array[0]++;
			else if (s_index >= mid && e_index >= mid)
				count_array[1]++;
			else {
				count_array[0]++;
				count_array[1]++;
			}
		}
		if ((count_array[0] >= ptr->arraySize) || count_array[1] >= ptr->arraySize) {
			ptr->num_children = 0;
			return;
		}
		//new children
		ptr->num_children = 0;
		ptr->children = malloc(sizeof(struct bucket) * 2);
		(&ptr->children[0])->cut_dim = NONE;
		(&ptr->children[0])->num_children = 0;
		(&ptr->children[0])->cut_point = 0;
		(&ptr->children[0])->children = NULL;
		(&ptr->children[0])->arraySize = 0;
		(&ptr->children[0])->internal_array = malloc(sizeof(struct ENTRY) * count_array[0]);
		ptr->num_children++;
		(&ptr->children[1])->cut_dim = NONE;
		(&ptr->children[1])->num_children = 0;
		(&ptr->children[1])->cut_point = 0;
		(&ptr->children[1])->children = NULL;
		(&ptr->children[1])->arraySize = 0;
		(&ptr->children[1])->internal_array = malloc(sizeof(struct ENTRY) * count_array[1]);
		ptr->num_children++;
		//assign children
		for (i = 0; i < ptr->arraySize; i++) {
			s_index = (unsigned int)ptr->internal_array[i].src_port_start;
			e_index = (unsigned int)ptr->internal_array[i].src_port_end;
			if (s_index < mid && e_index < mid)
				(&ptr->children[0])->internal_array[(&ptr->children[0])->arraySize++] = ptr->internal_array[i];
			else if (s_index >= mid && e_index >= mid)
				(&ptr->children[1])->internal_array[(&ptr->children[1])->arraySize++] = ptr->internal_array[i];
			else {
				(&ptr->children[0])->internal_array[(&ptr->children[0])->arraySize++] = ptr->internal_array[i];
				(&ptr->children[1])->internal_array[(&ptr->children[1])->arraySize++] = ptr->internal_array[i];
			}
		}
	}
	else if (ptr->cut_dim == DESPORT) {
		for (i = 0, j = 0; i < ptr->arraySize; i++) {
			seq_array[j++] = (unsigned int)ptr->internal_array[i].des_port_start;
			seq_array[j++] = (unsigned int)ptr->internal_array[i].des_port_end;
		}
		qsort(seq_array, ptr->arraySize * 2, sizeof(unsigned int), cmpfunc);
		mid = seq_array[ptr->arraySize];//cut point
		ptr->cut_point = mid;
		//count children internal array size
		for (i = 0; i < ptr->arraySize; i++) {
			s_index = (unsigned int)ptr->internal_array[i].des_port_start;
			e_index = (unsigned int)ptr->internal_array[i].des_port_end;
			//count left part and right part
			if (s_index < mid && e_index < mid)
				count_array[0]++;
			else if (s_index >= mid && e_index >= mid)
				count_array[1]++;
			else {
				count_array[0]++;
				count_array[1]++;
			}
		}
		if ((count_array[0] >= ptr->arraySize) || count_array[1] >= ptr->arraySize) {
			ptr->num_children = 0;
			return;
		}
		//new children
		ptr->num_children = 0;
		ptr->children = malloc(sizeof(struct bucket) * 2);
		(&ptr->children[0])->cut_dim = NONE;
		(&ptr->children[0])->num_children = 0;
		(&ptr->children[0])->cut_point = 0;
		(&ptr->children[0])->children = NULL;
		(&ptr->children[0])->arraySize = 0;
		(&ptr->children[0])->internal_array = malloc(sizeof(struct ENTRY) * count_array[0]);
		ptr->num_children++;
		(&ptr->children[1])->cut_dim = NONE;
		(&ptr->children[1])->num_children = 0;
		(&ptr->children[1])->cut_point = 0;
		(&ptr->children[1])->children = NULL;
		(&ptr->children[1])->arraySize = 0;
		(&ptr->children[1])->internal_array = malloc(sizeof(struct ENTRY) * count_array[1]);
		ptr->num_children++;
		//assign children
		for (i = 0; i < ptr->arraySize; i++) {
			s_index = (unsigned int)ptr->internal_array[i].des_port_start;
			e_index = (unsigned int)ptr->internal_array[i].des_port_end;
			if (s_index < mid && e_index < mid)
				(&ptr->children[0])->internal_array[(&ptr->children[0])->arraySize++] = ptr->internal_array[i];
			else if (s_index >= mid && e_index >= mid)
				(&ptr->children[1])->internal_array[(&ptr->children[1])->arraySize++] = ptr->internal_array[i];
			else {
				(&ptr->children[0])->internal_array[(&ptr->children[0])->arraySize++] = ptr->internal_array[i];
				(&ptr->children[1])->internal_array[(&ptr->children[1])->arraySize++] = ptr->internal_array[i];
			}
		}
	}
	else if (ptr->cut_dim == PROTOCOL) {
		for (i = 0, j = 0; i < ptr->arraySize; i++)
			seq_array[j++] = (unsigned int)ptr->internal_array[i].protocol;
		qsort(seq_array, ptr->arraySize, sizeof(unsigned int), cmpfunc);
		mid = seq_array[ptr->arraySize / 2];//cut point
		ptr->cut_point = mid;
		//count children internal array size
		for (i = 0; i < ptr->arraySize; i++) {
			s_index = (unsigned int)ptr->internal_array[i].protocol;
			//count left part and right part
			if (s_index < mid)
				count_array[0]++;
			else if (s_index >= mid)
				count_array[1]++;
		}
		if ((count_array[0] >= ptr->arraySize) || count_array[1] >= ptr->arraySize) {
			ptr->num_children = 0;
			return;
		}
		//new children
		ptr->num_children = 0;
		ptr->children = malloc(sizeof(struct bucket) * 2);
		(&ptr->children[0])->cut_dim = NONE;
		(&ptr->children[0])->num_children = 0;
		(&ptr->children[0])->cut_point = 0;
		(&ptr->children[0])->children = NULL;
		(&ptr->children[0])->arraySize = 0;
		(&ptr->children[0])->internal_array = malloc(sizeof(struct ENTRY) * count_array[0]);
		ptr->num_children++;
		(&ptr->children[1])->cut_dim = NONE;
		(&ptr->children[1])->num_children = 0;
		(&ptr->children[1])->cut_point = 0;
		(&ptr->children[1])->children = NULL;
		(&ptr->children[1])->arraySize = 0;
		(&ptr->children[1])->internal_array = malloc(sizeof(struct ENTRY) * count_array[1]);
		ptr->num_children++;
		//assign children
		for (i = 0; i < ptr->arraySize; i++) {
			s_index = (unsigned int)ptr->internal_array[i].protocol;
			if (s_index < mid)
				(&ptr->children[0])->internal_array[(&ptr->children[0])->arraySize++] = ptr->internal_array[i];
			else if (s_index >= mid)
				(&ptr->children[1])->internal_array[(&ptr->children[1])->arraySize++] = ptr->internal_array[i];
		}
	}
	else {
		printf("******************CUT DIM ERROR*********************\n");
	}
}
////////////////////////////////////////////////////////////////////////////////////
void split(ctrie p) {
	ctrie ptr = p;
	find_cutOfDim(p);
	choose_numOfpart(ptr);
	for (int i = 0; i < ptr->num_children; i++) {
		if (ptr->children[i].arraySize > BINTH)
			split(&(ptr->children[i]));
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create() {
	root = create_bnode();
	root->internal_array = malloc(sizeof(struct ENTRY) * num_entry);
	root->arraySize = num_entry;
	for (int i = 0; i < num_entry; i++)
		root->internal_array[i] = table[i];
}
////////////////////////////////////////////////////////////////////////////////////
void read_table(char *str, unsigned int *src_ip, unsigned char *src_len, unsigned int *des_ip, unsigned char *des_len, unsigned short int *src_port_start, unsigned short int *src_port_end, unsigned short int *des_port_start, unsigned short int *des_port_end, unsigned char *protocol) {
	char tok_space[] = "\t";
	char tok_ip[] = "@./ ";
	char tok_port[] = ":\t ";
	char tok_protocol[] = "\tx/ ";
	char buf[200];
	char *sa, *da, *sp, *dp, *pt;

	//source address
	sa = strtok(str, tok_space);
	//destination address
	da = strtok(NULL, tok_space);
	//source port
	sp = strtok(NULL, tok_space);
	//destination address
	dp = strtok(NULL, tok_space);
	//protocol
	pt = strtok(NULL, tok_space);
	//final field
	sprintf(buf, "%s\0", strtok(NULL, tok_space));

	//deal with source ip
	*src_ip = 0;
	sprintf(buf, "%s\0", strtok(sa, tok_ip));
	*src_ip += atoi(buf);
	*src_ip <<= 8;
	sprintf(buf, "%s\0", strtok(NULL, tok_ip));
	*src_ip += atoi(buf);
	*src_ip <<= 8;
	sprintf(buf, "%s\0", strtok(NULL, tok_ip));
	*src_ip += atoi(buf);
	*src_ip <<= 8;
	sprintf(buf, "%s\0", strtok(NULL, tok_ip));
	*src_ip += atoi(buf);
	sprintf(buf, "%s\0", strtok(NULL, tok_ip));
	*src_len = atoi(buf);
	//deal with destination ip
	*des_ip = 0;
	sprintf(buf, "%s\0", strtok(da, tok_ip));
	*des_ip += atoi(buf);
	*des_ip <<= 8;
	sprintf(buf, "%s\0", strtok(NULL, tok_ip));
	*des_ip += atoi(buf);
	*des_ip <<= 8;
	sprintf(buf, "%s\0", strtok(NULL, tok_ip));
	*des_ip += atoi(buf);
	*des_ip <<= 8;
	sprintf(buf, "%s\0", strtok(NULL, tok_ip));
	*des_ip += atoi(buf);
	sprintf(buf, "%s\0", strtok(NULL, tok_ip));
	*des_len = atoi(buf);
	//deal with source port
	sprintf(buf, "%s\0", strtok(sp, tok_port));
	*src_port_start = atoi(buf);
	sprintf(buf, "%s\0", strtok(NULL, tok_port));
	*src_port_end = atoi(buf);
	//deal with destination port
	sprintf(buf, "%s\0", strtok(dp, tok_port));
	*des_port_start = atoi(buf);
	sprintf(buf, "%s\0", strtok(NULL, tok_port));
	*des_port_end = atoi(buf);
	//deal with protocol
	sprintf(buf, "%s\0", strtok(pt, tok_protocol));
	sprintf(buf, "%s\0", strtok(NULL, tok_protocol));
	*protocol = buf[0] >= 'a' ? (buf[0] - 'a' + 10) * 16 : (buf[0] - '0') * 16;
	*protocol += buf[1] >= 'a' ? (buf[1] - 'a' + 10) : (buf[1] - '0');
}
////////////////////////////////////////////////////////////////////////////////////
void set_table(char *file_name) {
	FILE *fp;
	char string[200];
	unsigned int src_ip;
	unsigned char src_len;
	unsigned int des_ip;
	unsigned char des_len;
	unsigned short int src_port_start;
	unsigned short int src_port_end;
	unsigned short int des_port_start;
	unsigned short int des_port_end;
	unsigned char protocol;

	fp = fopen(file_name, "r");
	while (fgets(string, 200, fp) != NULL) {
		read_table(string, &src_ip, &src_len, &des_ip, &des_len, &src_port_start, &src_port_end, &des_port_start, &des_port_end, &protocol);
		num_entry++;
	}
	rewind(fp);
	table = (struct ENTRY *)malloc(num_entry * sizeof(struct ENTRY));
	num_entry = 0;
	while (fgets(string, 200, fp) != NULL) {
		read_table(string, &src_ip, &src_len, &des_ip, &des_len, &src_port_start, &src_port_end, &des_port_start, &des_port_end, &protocol);
		table[num_entry].src_ip = src_ip;
		table[num_entry].src_len = src_len;
		table[num_entry].des_ip = des_ip;
		table[num_entry].des_len = des_len;
		table[num_entry].src_port_start = src_port_start;
		table[num_entry].src_port_end = src_port_end;
		table[num_entry].des_port_start = des_port_start;
		table[num_entry].des_port_end = des_port_end;
		table[num_entry++].protocol = protocol;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_query(char *file_name) {
	FILE *fp;
	char string[200];
	unsigned int src_ip;
	unsigned char src_len;
	unsigned int des_ip;
	unsigned char des_len;
	unsigned short int src_port_start;
	unsigned short int src_port_end;
	unsigned short int des_port_start;
	unsigned short int des_port_end;
	unsigned char protocol;

	fp = fopen(file_name, "r");
	while (fgets(string, 200, fp) != NULL) {
		read_table(string, &src_ip, &src_len, &des_ip, &des_len, &src_port_start, &src_port_end, &des_port_start, &des_port_end, &protocol);
		num_query++;
	}
	rewind(fp);
	query = (struct ENTRY *)malloc(num_query * sizeof(struct ENTRY));
	clock = (unsigned long long int *)malloc(num_query * sizeof(unsigned long long int));
	num_query = 0;
	while (fgets(string, 200, fp) != NULL) {
		read_table(string, &src_ip, &src_len, &des_ip, &des_len, &src_port_start, &src_port_end, &des_port_start, &des_port_end, &protocol);
		query[num_query].src_ip = src_ip;
		query[num_query].src_len = src_len;
		query[num_query].des_ip = des_ip;
		query[num_query].des_len = des_len;
		query[num_query].src_port_start = src_port_start;
		query[num_query].src_port_end = src_port_end;
		query[num_query].des_port_start = des_port_start;
		query[num_query].des_port_end = des_port_end;
		query[num_query].protocol = protocol;
		clock[num_query++] = 10000000;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_input(char *file_name) {
	FILE *fp;
	char string[200];
	unsigned int src_ip;
	unsigned char src_len;
	unsigned int des_ip;
	unsigned char des_len;
	unsigned short int src_port_start;
	unsigned short int src_port_end;
	unsigned short int des_port_start;
	unsigned short int des_port_end;
	unsigned char protocol;

	fp = fopen(file_name, "r");
	while (fgets(string, 200, fp) != NULL) {
		read_table(string, &src_ip, &src_len, &des_ip, &des_len, &src_port_start, &src_port_end, &des_port_start, &des_port_end, &protocol);
		num_input++;
	}
	rewind(fp);
	input = (struct ENTRY *)malloc(num_input * sizeof(struct ENTRY));
	num_input = 0;
	while (fgets(string, 200, fp) != NULL) {
		read_table(string, &src_ip, &src_len, &des_ip, &des_len, &src_port_start, &src_port_end, &des_port_start, &des_port_end, &protocol);
		input[num_input].src_ip = src_ip;
		input[num_input].src_len = src_len;
		input[num_input].des_ip = des_ip;
		input[num_input].des_len = des_len;
		input[num_input].src_port_start = src_port_start;
		input[num_input].src_port_end = src_port_end;
		input[num_input].des_port_start = des_port_start;
		input[num_input].des_port_end = des_port_end;
		input[num_input++].protocol = protocol;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void CountClock()
{
	unsigned int i;
	unsigned int* NumCntClock = (unsigned int*)malloc(50 * sizeof(unsigned int));
	for (i = 0; i < 50; i++) NumCntClock[i] = 0;
	unsigned long long MinClock = 10000000, MaxClock = 0;
	for (i = 0; i < num_query; i++)
	{
		if (clock[i] > MaxClock) MaxClock = clock[i];
		if (clock[i] < MinClock) MinClock = clock[i];
		if (clock[i] / 100 < 50) NumCntClock[clock[i] / 100]++;
		else NumCntClock[49]++;
	}
	printf("(MaxClock, MinClock) = (%5llu, %5llu)\n", MaxClock, MinClock);

	for (i = 0; i < 50; i++)
	{
		printf("%d\n", NumCntClock[i]);
	}
	return;
}
////////////////////////////////////////////////////////////////////////////////////
void shuffle(struct ENTRY *array, int n) {
	srand((unsigned)time(NULL));
	struct ENTRY *temp = (struct ENTRY *)malloc(sizeof(struct ENTRY));

	for (int i = 0; i < n - 1; i++) {
		size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
		temp->src_ip = array[j].src_ip;
		temp->des_ip = array[j].des_ip;
		temp->src_port_start = array[j].src_port_start;
		temp->src_port_end = array[j].src_port_end;
		temp->des_port_start = array[j].des_port_start;
		temp->des_port_end = array[j].des_port_end;
		temp->src_len = array[j].src_len;
		temp->des_len = array[j].des_len;
		temp->protocol = array[j].protocol;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void count_leave(struct bucket *p) {
	if (p == NULL)
		return;
	if (p->num_children == 0) {
		num_leave += p->arraySize;
		return;
	}
	count_leave(&p->children[0]);
	count_leave(&p->children[1]);
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
	int i, j;
	char filename[50] = "test.txt";
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	
	//build
	begin = rdtsc();
	create();
	split(root);
	end = rdtsc();
	
	printf("Avg. Build Time: %llu\n", (end - begin) / num_entry);
	printf("BINTH:%d\n", BINTH);
	printf("dim count:\n");
	for (i = 0; i < 5; i++)
		printf("	%d\n", dim_count[i]);

	count_leave(root);
	printf("num of leave: %d\n", num_leave);
	
	shuffle(query, num_query);
	for (j = 0; j < 100; j++) {
		for (i = 0; i < num_query; i++) {
			begin = rdtsc();
			search(query[i].src_ip, query[i].des_ip, (query[i].src_port_start + query[i].src_port_end) / 2, (query[i].des_port_start + query[i].des_port_end) / 2, query[i].protocol);
			end = rdtsc();
			if (clock[i] > (end - begin))
				clock[i] = (end - begin);
		}
	}
	total = 0;
	for (j = 0; j < num_query; j++)
		total += clock[j];
	printf("Avg. Search: %lld\n", total / num_query);
	CountClock();

	//update
	begin = rdtsc();
	for (i = 0; i < num_input; i++)
		update(input[i]);
	end = rdtsc();
	printf("Avg. Build Time: %llu\n", (end - begin) / num_input);
	
	return 0;
}

