#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<iostream>
#include<time.h>
#include<algorithm>


using namespace std;
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY{
	unsigned int s_ip;
	unsigned int d_ip;
	unsigned char s_len;
	unsigned char d_len;
	unsigned  int s_port_s;
	unsigned  int s_port_e;
	unsigned  int d_port_s;
	unsigned  int d_port_e;
	unsigned int type;

};
////////////////////////////////////////////////////////////////////////////////////
static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

////////////////////////////////////////////////////////////////////////////////////
int match_case = 0, loss_case = 0, modnum = 0;
int leaf_node_num = 0, hicut_node_num = 0;
struct hicut_leaf{
	unsigned int s_ip;
	unsigned int d_ip;
	unsigned char s_len;
	unsigned char d_len;
	unsigned  int s_port_s;
	unsigned  int s_port_e;
	unsigned  int d_port_s;
	unsigned  int d_port_e;
	unsigned int type;

	struct hicut_leaf * down_link;
};

struct hicut_leaf* create_hicut_leaf(){
	struct hicut_leaf * temp;
	temp = (struct hicut_leaf * )malloc(sizeof(struct hicut_leaf));
	if(temp == NULL){
		printf("out of memory\n");
	}
	temp -> s_ip = 0;
	temp -> d_ip = 0;
	temp -> s_len = 0;
	temp -> d_len = 0;
	temp -> s_port_s = 0;
	temp -> s_port_e = 0;
	temp -> d_port_s = 0;
	temp -> d_port_e = 0;
	temp -> type = 0;
	temp -> down_link = NULL;
	leaf_node_num++;
	return temp;
}

struct hicut_node{
	int dimension;
	unsigned int cut_value;
	int rule_num;
	struct hicut_node * down_pointer_left;
	struct hicut_node * down_pointer_right;
	struct hicut_leaf * hicut_leafnode;
};

struct hicut_node * root_hicut;

struct hicut_node* create_hicut_node(){
	struct hicut_node * temp;
	temp = (struct hicut_node * )malloc(sizeof(struct hicut_node));
	if(temp == NULL){
		printf("out of memory\n");
	}
	temp -> dimension = -1;
	temp -> cut_value = 0;
	temp -> rule_num = 0;
	temp -> down_pointer_left = NULL;
	temp -> down_pointer_right = NULL;
	temp -> hicut_leafnode = NULL;
	hicut_node_num ++;
	return temp;
}

int max(int a, int b){
	return (a > b)? a:b;
}

int min(int a, int b){
	return (a < b)? a:b;
}

int compare (unsigned int a, unsigned int b){
	return a < b;
}
////////////////////////////////////////////////////////////////////////////////////
struct list{//structure of binary trie
	unsigned int port;
	int count;
	struct list *left,*right;
};
typedef struct list node;
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;
int num_entry=0;
int num_query=0;
struct ENTRY *table, *query;
int N=0;//number of nodes
unsigned long long int time_begin,time_end,total=0;
unsigned long long int *time_clock;
int num_node=0;//total number of nodes in the binary trie
////////////////////////////////////////////////////////////////////////////////////
btrie create_node(){
	btrie temp;
	num_node++;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->count = 0;
	temp->port=256;//default port
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(btrie input_ptr, unsigned int ip,unsigned char len){
	btrie ptr=input_ptr;
	int i;
	if(len == 0){
		if(ptr->port==256)
			ptr->port=0;
		else 
			ptr->count++;
	}
	for(i=0;i<len;i++){
		if(ip&(1<<(31-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); // Create Node
			ptr=ptr->right;
			if(i==len-1){
				if(ptr->port==256)
					ptr->port=0;
				else 
					ptr->count++;
			}
				
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if(i==len-1){
				if(ptr->port==256)
					ptr->port=0;
				else 
					ptr->count++;
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////

int getIndexOfSigns(char ch)
{
    if(ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if(ch >= 'A' && ch <='F') 
    {
        return ch - 'A' + 10;
    }
    if(ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    return 0;
}


unsigned long long int hexToDec(char *source)
{
    unsigned long long int sum = 0;
    unsigned long long int t = 1;
    int i, len;
 
    len = strlen(source);
    for(i=len-1; i>=0; i--)
    {
        sum += t * getIndexOfSigns(*(source + i));
        t *= 16;
    }
	//printf("sum : %llu\n", sum);
    return sum;
}

void read_table(char *str,unsigned int *s_ip, unsigned int *d_ip, int *s_len, int *d_len, unsigned  int *s_port_s, unsigned  int *s_port_e, unsigned  int *d_port_s, unsigned  int *d_port_e, unsigned int *type){
	char *buf1, *buf2, *buf3, *buf4, *buf5;
	buf1 = strtok(str,"\t");
	buf2 = strtok(NULL,"\t");
	buf3 = strtok(NULL,"\t");
	buf4 = strtok(NULL,"\t");
	buf5 = strtok(NULL,"\t");

	for(int i = 0; i < 2; ++i){
		char tok[]="./";
		char buf[100],*str1;
		unsigned int n[4];
		if( i == 0){
			sprintf(buf,"%s\0",strtok(buf1,tok));
			n[0]=atoi(buf+1);
			//printf("%s\n", buf+1);
		}
		else{
			sprintf(buf,"%s\0",strtok(buf2,tok));
			n[0]=atoi(buf);
			//printf("%s\n", buf);
		}
			
		
		
		sprintf(buf,"%s\0",strtok(NULL,tok));
		n[1]=atoi(buf);
		sprintf(buf,"%s\0",strtok(NULL,tok));
		n[2]=atoi(buf);
		sprintf(buf,"%s\0",strtok(NULL,tok));
		n[3]=atoi(buf);
		str1=(char *)strtok(NULL,tok);
		if( i == 0){
			if(str1!=NULL){
				sprintf(buf,"%s\0",str1);
				*s_len=atoi(buf);
			}
			else{
				if(n[1]==0&&n[2]==0&&n[3]==0)
					*s_len=8;
				else
					if(n[2]==0&&n[3]==0)
						*s_len=16;
					else
						if(n[3]==0)
							*s_len=24;
			}
			*s_ip=n[0];
			*s_ip<<=8;
			*s_ip+=n[1];
			*s_ip<<=8;
			*s_ip+=n[2];
			*s_ip<<=8;
			*s_ip+=n[3];
		}
		else
		{
			if(str1!=NULL){
				sprintf(buf,"%s\0",str1);
				*d_len=atoi(buf);
			}
			else{
				if(n[1]==0&&n[2]==0&&n[3]==0)
					*d_len=8;
				else
					if(n[2]==0&&n[3]==0)
						*d_len=16;
					else
						if(n[3]==0)
							*d_len=24;
			}
			*d_ip=n[0];
			*d_ip<<=8;
			*d_ip+=n[1];
			*d_ip<<=8;
			*d_ip+=n[2];
			*d_ip<<=8;
			*d_ip+=n[3];
		}
		
		
	}


	*s_port_s = atoi(strtok(buf3,":"));
	*s_port_e = atoi(strtok(NULL,":"));
	*d_port_s = atoi(strtok(buf4,":"));
	*d_port_e = atoi(strtok(NULL,":"));
	*type = hexToDec(strtok(buf5,"/"));

	//printf("s_ip %u, d_ip : %u, s_port_s : %u, s_port_e : %u, d_port_s : %u, d_port_e : %u, type : %u\n", *s_ip, *d_ip, *s_port_s, *s_port_e, *d_port_s, *d_port_e, *type);

}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned int ip){
	int j;
	btrie current=root,temp=NULL;
	for(j=31;j>=(-1);j--){
		if(current==NULL)
			break;
		if(current->port!=256)
			temp=current;
		if(ip&(1<<j)){
			current=current->right;
		}
		else{
			current=current->left; 
		}
	}
	if(temp==NULL)
	  printf("default: %u\n", ip);
	  /*else
	  printf("%u\n",temp->port);*/
}
////////////////////////////////////////////////////////////////////////////////////
void set_table(char *file_name){
	FILE *fp;
	int s_len, d_len;
	char string[150];
	unsigned  int s_port_s, s_port_e, d_port_s, d_port_e;
	unsigned int s_ip, d_ip, type;
	fp=fopen(file_name,"r");
	while(fgets(string,100,fp)!=NULL){
		read_table(string,&s_ip, &d_ip,&s_len, &d_len, &s_port_s, &s_port_e, &d_port_s, &d_port_e, &type);
		num_entry++;
	}
	rewind(fp);
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	while(fgets(string,100,fp)!=NULL){
		read_table(string,&s_ip, &d_ip,&s_len, &d_len, &s_port_s, &s_port_e, &d_port_s, &d_port_e, &type);
		table[num_entry].s_ip=s_ip;
		table[num_entry].d_ip=d_ip;
		table[num_entry].s_len=s_len;
		table[num_entry].d_len=d_len;
		table[num_entry].s_port_s=s_port_s;
		table[num_entry].s_port_e=s_port_e;
		table[num_entry].d_port_s=d_port_s;
		table[num_entry].d_port_e=d_port_e;
		table[num_entry++].type=type;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_query(char *file_name){
	FILE *fp;
	int s_len, d_len;
	char string[150];
	unsigned  int s_port_s, s_port_e, d_port_s, d_port_e;
	unsigned int s_ip, d_ip, type;
	fp=fopen(file_name,"r");
	while(fgets(string,100,fp)!=NULL){
		read_table(string,&s_ip, &d_ip,&s_len, &d_len, &s_port_s, &s_port_e, &d_port_s, &d_port_e, &type);
		num_query++;
	}
	rewind(fp);
	query=(struct ENTRY *)malloc(num_query*sizeof(struct ENTRY));
	time_clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));
	num_query=0;
	while(fgets(string,100,fp)!=NULL){
		read_table(string,&s_ip, &d_ip,&s_len, &d_len, &s_port_s, &s_port_e, &d_port_s, &d_port_e, &type);
		query[num_query].s_ip=s_ip;
		query[num_query].d_ip=d_ip;
		query[num_query].s_len=s_len;
		query[num_query].d_len=d_len;
		query[num_query].s_port_s=s_port_s;
		query[num_query].s_port_e=s_port_e;
		query[num_query].d_port_s=d_port_s;
		query[num_query].d_port_e=d_port_e;
		query[num_query].type=type;
		time_clock[num_query++]=10000000;
	}
}
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
void count_node(btrie r){
	if(r==NULL)
		return;
	count_node(r->left);
	N++;
	count_node(r->right);
}

int count_active_node(btrie r){
	if(r==NULL)
		return 0;
	if(r->port != 256){
		return 1 + count_active_node(r->left) + count_active_node(r->right);
	}
	else
	{
		return count_active_node(r->left) + count_active_node(r->right);
	}

}

int count_collide_num(btrie r){
	if(r==NULL)
		return 0;
	if(r->port != 256){
		return r->count + count_collide_num(r->left) + count_collide_num(r->right);
	}
	else
	{
		return count_collide_num(r->left) + count_collide_num(r->right);
	}

}
////////////////////////////////////////////////////////////////////////////////////
void CountClock()
{
	unsigned int i;
	unsigned int* NumCntClock = (unsigned int* )malloc(50 * sizeof(unsigned int ));
	for(i = 0; i < 50; i++) NumCntClock[i] = 0;
	unsigned long long MinClock = 10000000, MaxClock = 0;
	for(i = 0; i < num_query; i++)
	{
		if(time_clock[i] > MaxClock) MaxClock = time_clock[i];
		if(time_clock[i] < MinClock) MinClock = time_clock[i];
		if(time_clock[i] / 100 < 50) NumCntClock[time_clock[i] / 100]++;
		else NumCntClock[49]++;
	}
	printf("(MaxClock, MinClock) = (%5llu, %5llu)\n", MaxClock, MinClock);
	
	for(i = 0; i < 50; i++)
	{
		printf("%d\n",NumCntClock[i]);
	}
	return;
}

void shuffle(struct ENTRY *array, int n) {
	
    srand((unsigned)time(NULL));
    struct ENTRY *temp=(struct ENTRY *)malloc(sizeof(struct ENTRY));
    
    for (int i = 0; i < n - 1; i++) {
        ssize_t j = i + rand() / (RAND_MAX / (n - i) + 1);

        temp->s_ip=array[j].s_ip;
		temp->d_ip=array[j].d_ip;
        temp->s_len=array[j].s_len;
		temp->d_len=array[j].d_len;
        temp->s_port_s=array[j].s_port_s;
		temp->s_port_e=array[j].s_port_e;
		temp->d_port_s=array[j].d_port_s;
		temp->d_port_e=array[j].d_port_e;
		temp->type = array[j].type;
        array[j].s_ip = array[i].s_ip;
		array[j].d_ip = array[i].d_ip;
        array[j].s_len = array[i].s_len;
		array[j].d_len = array[i].d_len;
        array[j].s_port_s = array[i].s_port_s;
		array[j].s_port_e = array[i].s_port_e;
		array[j].d_port_s = array[i].d_port_s;
		array[j].d_port_e = array[i].d_port_e;
		array[j].type = array[i].type;
        array[i].s_ip = temp->s_ip;
		array[i].d_ip = temp->d_ip;
        array[i].s_len = temp->s_len;
		array[i].d_len = temp->d_len;
        array[i].s_port_s = temp->s_port_s;
		array[i].s_port_e = temp->s_port_e;
		array[i].d_port_s = temp->d_port_s;
		array[i].d_port_e = temp->d_port_e;
		array[i].type = temp->type;
    }
	free(temp);
}

////////////////////////////////////////////////////////////////////////////////////
void create_root_hicut(){
	root_hicut = create_hicut_node();
	root_hicut -> hicut_leafnode = create_hicut_leaf();
	struct hicut_leaf * temp = root_hicut -> hicut_leafnode;
	for(int i=0; i< num_entry; ++i){
		temp -> s_ip = table[i].s_ip;
		temp -> d_ip = table[i].d_ip;
		temp -> s_len = table[i].s_len;
		temp -> d_len = table[i].d_len;
		temp -> s_port_s = table[i].s_port_s;
		temp -> s_port_e = table[i].s_port_e;
		temp -> d_port_s = table[i].d_port_s;
		temp -> d_port_e = table[i].d_port_e;
		temp -> type = table[i].type;

		if(temp -> down_link == NULL && i != num_entry - 1){
			temp -> down_link = create_hicut_leaf();
			temp = temp -> down_link;
		}
	}
	root_hicut -> rule_num = num_entry;
}

void free_binary_tree(btrie input_ptr){
	if(input_ptr == NULL)
		return;
	
	free_binary_tree(input_ptr -> right);
	free_binary_tree(input_ptr -> left);
	
	free(input_ptr);
}

int count_distinct_rule_num(struct hicut_leaf * input_node, int dimension){
	if(input_node == NULL)	return -1;
	//printf("in count_distinct_rule_num\n");
	if(dimension == 1){
		btrie temp_root = create_node();
		while(input_node != NULL){
			add_node(temp_root, input_node->s_ip, input_node->s_len);
			input_node = input_node -> down_link;
		}
		int node_num = count_active_node(temp_root);
		free_binary_tree(temp_root);
		return node_num;
	}
	else if(dimension == 2){
		btrie temp_root = create_node();
		while(input_node != NULL){
			add_node(temp_root, input_node->d_ip, input_node->d_len);
			input_node = input_node -> down_link;
		}
		int node_num = count_active_node(temp_root);
		free_binary_tree(temp_root);
		return node_num;
	}
	else if(dimension == 3){
		btrie temp_root[65536];
		for(int i=0; i<65536; ++i){
			temp_root[i] = create_node();
		}
		while(input_node != NULL){
			add_node(temp_root[input_node->s_port_e], input_node->s_port_s, 32);
			input_node = input_node -> down_link;
		}
		int node_num = 0;
		for(int i=0; i<65536; ++i){
			node_num += count_active_node(temp_root[i]);
			free_binary_tree(temp_root[i]);
		}

		return node_num;
	}
	else if(dimension == 4)
	{
		btrie temp_root[65536];
		for(int i=0; i<65536; ++i){
			temp_root[i] = create_node();
		}
		while(input_node != NULL){
			add_node(temp_root[input_node->d_port_e], input_node->d_port_s, 32);
			input_node = input_node -> down_link;
		}
		int node_num = 0;
		for(int i=0; i<65536; ++i){
			node_num += count_active_node(temp_root[i]);
			free_binary_tree(temp_root[i]);
		}

		return node_num;
	}
	else
	{
		int count_array[32] = {0};
		while(input_node != NULL){
			count_array[input_node -> type]++;
			input_node = input_node -> down_link;
		}
		int node_num = 0;
		for(int i=0; i<32; ++i){
			if(count_array[i] != 0)
				node_num ++;
		}
		return node_num;
	}
	
	
}

unsigned int get_cut_bit_number(struct hicut_leaf * input_node, int dimension, int rule_num){

	struct hicut_leaf * temp;
	int array_size = rule_num * 2;
	unsigned int value_array[array_size];
	for(int i=0; i<array_size; ++i){
		value_array[i] = 0;
	}
	int array_index = 0;

	if(dimension == 1){
		temp = input_node;

		while(temp != NULL){
			unsigned int temp_ip_down = ((temp -> s_ip) >> (32 - temp -> s_len)) << (32 - temp -> s_len);
			value_array[array_index++] = temp_ip_down;
			
			unsigned int temp_ip_up = ((temp -> s_ip) >> (32 - temp -> s_len));
			for(int i=0; i < (32 - temp -> s_len); ++i){
				temp_ip_up = temp_ip_up << 1;
				temp_ip_up = temp_ip_up + 1;
			}
			value_array[array_index++] = temp_ip_up;

			temp = temp -> down_link;
		}
	}
	else if(dimension == 2){
		temp = input_node;
		while(temp != NULL){
			unsigned int temp_ip_down = ((temp -> d_ip) >> (32 - temp -> d_len)) << (32 - temp -> d_len);
			unsigned int temp_ip_up = ((temp -> d_ip) >> (32 - temp -> d_len));
			
			for(int i=0; i < (32 - temp -> d_len); ++i){
				temp_ip_up = temp_ip_up << 1;
				temp_ip_up = temp_ip_up + 1;
			}
			
			value_array[array_index++] = temp_ip_down;
			value_array[array_index++] = temp_ip_up;
			temp = temp -> down_link;
		}
	}
	else if(dimension == 3){
		temp = input_node;
		while(temp != NULL){
			unsigned int down = temp -> s_port_s;
			unsigned int top = temp -> s_port_e;

			temp = temp -> down_link;
			value_array[array_index++] = down;
			value_array[array_index++] = top;
		}
	}
	else if(dimension == 4)
	{
		temp = input_node;
		while(temp != NULL){
			unsigned int down = temp -> d_port_s;
			unsigned int top = temp -> d_port_e;

			temp = temp -> down_link;
			value_array[array_index++] = down;
			value_array[array_index++] = top;
		}
	}
	else
	{
		temp = input_node;
		while(temp != NULL){

			value_array[array_index++] = (unsigned int) temp -> type;
			value_array[array_index++] = temp -> type;
			temp = temp -> down_link;
			
		}
	}

	if(array_index == 0){
		printf("rule num error\n");
	}

	sort(value_array, value_array + array_size, compare);

	/*
	for(int i = 0; i < array_size; ++i){
		printf("%u\n", value_array[i]);
	}
	*/

	//printf("value_array[rule_num/2] : %u, array_index: %d, rulenum : %d\n", value_array[rule_num/2], array_index, rule_num);
	/*
	if(value_array[rule_num/2] == 1700){
		for(int i = 0; i < rule_num; ++i){
			printf("%u\n", value_array[i]);
		}
		printf("-----------------------------\n");
	}
	*/
	return value_array[array_size/2];
}

struct hicut_node * move_leaf_node(struct hicut_node * input_node, struct hicut_leaf * input_node2){

	if(input_node2 == NULL)
		return input_node;
	//move 2 to 1
	if(input_node == NULL){
		input_node = create_hicut_node();
	}
	


	struct hicut_leaf * temp = NULL;


	if(input_node -> hicut_leafnode == NULL){
		input_node -> hicut_leafnode = create_hicut_leaf();
		temp = input_node -> hicut_leafnode;
	}
	else{

		temp = input_node -> hicut_leafnode;
		while(temp -> down_link != NULL){
			temp = temp-> down_link;
		}
		//printf("s_ip : %u, d_ip : %u, s_len : %d, d_len : %d\n", temp ->s_ip, temp -> d_ip, temp -> s_len,  temp -> d_len);
		temp -> down_link = create_hicut_leaf();
		temp = temp-> down_link;

	}

	temp -> s_ip = input_node2 -> s_ip;
	temp -> d_ip = input_node2 -> d_ip;
	temp -> s_len = input_node2 -> s_len;
	temp -> d_len = input_node2 -> d_len;
	temp -> s_port_s = input_node2 -> s_port_s;
	temp -> s_port_e = input_node2 -> s_port_e;
	temp -> d_port_s = input_node2 -> d_port_s;
	temp -> d_port_e = input_node2 -> d_port_e;
	temp -> type = input_node2 -> type;


	if(input_node -> hicut_leafnode == NULL){
		printf("input_node -> hicut_leafnode is null\n");
	}
	if(temp == NULL){
		printf("temp is null\n");
	}
	return input_node;
}


void free_leaf_node(struct hicut_leaf * input_node){
	struct hicut_leaf * temp = input_node, *for_free;

	while(temp != NULL){
		for_free = temp;
		temp = temp -> down_link;
		leaf_node_num--;
		free(for_free);
	}
}

void build_hicut(struct hicut_node * cut_node,int run_count){
	
	if(cut_node == NULL || cut_node -> rule_num < 100 || run_count >= 15){
		return ;
	}
	//choose dimension
	int dimension_distinct_num[5];
	int max_dimension_num = -1, max_dimension = 0;
	for(int i=0; i<5; ++i){
		dimension_distinct_num[i] = count_distinct_rule_num(cut_node->hicut_leafnode, i+1);
		//printf("dimension_distinct_num[i] : %d\n", dimension_distinct_num[i]);
		if(dimension_distinct_num[i] > max_dimension_num){
			max_dimension_num = dimension_distinct_num[i];
			max_dimension = i+1;
		}
	}
	//choost number of cut;
	if(max_dimension_num <= 5 || max_dimension <= 0)
		return;

	unsigned int this_cut_value = get_cut_bit_number(cut_node->hicut_leafnode, max_dimension, cut_node -> rule_num);
	
	//printf("cut_value : %u, max dimension : %d, max dimension num : %d, rule_num : %d\n", this_cut_value, max_dimension, max_dimension_num, cut_node -> rule_num);
	//printf("----------------------------\n");

	cut_node -> dimension = max_dimension;
	cut_node -> cut_value = this_cut_value;

	cut_node -> down_pointer_right = create_hicut_node();
	cut_node -> down_pointer_left = create_hicut_node();

	struct hicut_leaf * temp = cut_node -> hicut_leafnode;
	while (temp != NULL){
		if(cut_node -> dimension == 1){
			unsigned int temp_ip_down = ((temp -> s_ip) >> (32 - temp -> s_len)) << (32 - temp -> s_len);
			unsigned int temp_ip_up = ((temp -> s_ip) >> (32 - temp -> s_len));
			for(int i=0; i < (32 - temp -> s_len); ++i){
				temp_ip_up = temp_ip_up << 1;
				temp_ip_up = temp_ip_up + 1;
			}
			
			if(temp_ip_down <= this_cut_value && temp_ip_up >= this_cut_value){
				cut_node -> down_pointer_left =  move_leaf_node(cut_node -> down_pointer_left, temp);
				cut_node -> down_pointer_right =  move_leaf_node(cut_node -> down_pointer_right, temp);
				cut_node -> down_pointer_left -> rule_num += 1;
				cut_node -> down_pointer_right -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_in_s : %u, go to left & right\n", this_cut_value, temp -> s_ip);
				}
				*/
			}
			else if(temp_ip_up < this_cut_value){
				cut_node -> down_pointer_left =  move_leaf_node(cut_node -> down_pointer_left, temp);
				cut_node -> down_pointer_left -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_in_s : %u, go to left\n", this_cut_value, temp -> s_ip);
				}
				*/
			}
			else if(temp_ip_down > this_cut_value){
				cut_node -> down_pointer_right =  move_leaf_node(cut_node -> down_pointer_right, temp);
				cut_node -> down_pointer_right -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_in_s : %u, go to right\n", this_cut_value, temp -> s_ip);
				}
				*/
			}
			else{
				printf("not happen\n");
			}
			
		}
		else if(cut_node -> dimension == 2){
			unsigned int temp_ip_down = ((temp -> d_ip) >> (32 - temp -> d_len)) << (32 - temp -> d_len);
			unsigned int temp_ip_up = ((temp -> d_ip) >> (32 - temp -> d_len));
			for(int i=0; i < (32 - temp -> d_len); ++i){
				temp_ip_up = temp_ip_up << 1;
				temp_ip_up = temp_ip_up + 1;
			}

			if(temp_ip_down <= this_cut_value && temp_ip_up >= this_cut_value){
				cut_node -> down_pointer_left =  move_leaf_node(cut_node -> down_pointer_left, temp);
				cut_node -> down_pointer_right =  move_leaf_node(cut_node -> down_pointer_right, temp);
				cut_node -> down_pointer_left -> rule_num += 1;
				cut_node -> down_pointer_right -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_in_d : %u, go to left & right\n", this_cut_value, temp -> d_ip);
				}
				*/
			}
			else if(temp_ip_up < this_cut_value){
				cut_node -> down_pointer_left =  move_leaf_node(cut_node -> down_pointer_left, temp);
				cut_node -> down_pointer_left -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_in_d : %u, go to left\n", this_cut_value, temp -> d_ip);
				}
				*/
				
			}
			else if(temp_ip_down > this_cut_value){
				cut_node -> down_pointer_right =  move_leaf_node(cut_node -> down_pointer_right, temp);
				cut_node -> down_pointer_right -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_in_d : %u, go to right\n", this_cut_value, temp -> d_ip);
				}
				*/
			}
			else{
				printf("not happen\n");
			}

		}
		else if(cut_node -> dimension == 3){
			unsigned int temp_ip_down = temp -> s_port_s;
			unsigned int temp_ip_up = temp -> s_port_e;

			if(temp_ip_down <= this_cut_value && temp_ip_up >= this_cut_value){
				cut_node -> down_pointer_left =  move_leaf_node(cut_node -> down_pointer_left, temp);
				cut_node -> down_pointer_right =  move_leaf_node(cut_node -> down_pointer_right, temp);
				cut_node -> down_pointer_left -> rule_num += 1;
				cut_node -> down_pointer_right -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_s_port_s : %u, go to left & right\n", this_cut_value, temp -> s_port_s);
				}
				*/
			}
			else if(temp_ip_up < this_cut_value){
				cut_node -> down_pointer_left =  move_leaf_node(cut_node -> down_pointer_left, temp);
				cut_node -> down_pointer_left -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_s_port_s : %u, go to left\n", this_cut_value, temp -> s_port_s);
				}
				*/
			}
			else if(temp_ip_down > this_cut_value){
				cut_node -> down_pointer_right =  move_leaf_node(cut_node -> down_pointer_right, temp);
				cut_node -> down_pointer_right -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_s_port_s : %u, go to right\n", this_cut_value, temp -> s_port_s);
				}
				*/
			}
			else{
				printf("not happen\n");
			}
		}
		else if(cut_node -> dimension == 4)
		{
			unsigned int temp_ip_down = temp -> d_port_s;
			unsigned int temp_ip_up = temp -> d_port_e;

			if(temp_ip_down <= this_cut_value && temp_ip_up >= this_cut_value){
				cut_node -> down_pointer_left =  move_leaf_node(cut_node -> down_pointer_left, temp);
				cut_node -> down_pointer_right =  move_leaf_node(cut_node -> down_pointer_right, temp);
				cut_node -> down_pointer_left -> rule_num += 1;
				cut_node -> down_pointer_right -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_d_port_s : %u, go to left & right\n", this_cut_value, temp -> d_port_s);
				}
				*/
			}
			else if(temp_ip_up < this_cut_value){
				cut_node -> down_pointer_left =  move_leaf_node(cut_node -> down_pointer_left, temp);
				cut_node -> down_pointer_left -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_d_port_s : %u, go to left\n", this_cut_value, temp -> d_port_s);
				}
				*/
			}
			else if(temp_ip_down > this_cut_value){
				cut_node -> down_pointer_right =  move_leaf_node(cut_node -> down_pointer_right, temp);
				cut_node -> down_pointer_right -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, input_d_port_s : %u, go to right\n", this_cut_value, temp -> d_port_s);
				}
				*/
			}
			else{
				printf("not happen\n");
			}
		}
		else
		{
			if(this_cut_value >= temp -> type){
				cut_node -> down_pointer_left =  move_leaf_node(cut_node -> down_pointer_left, temp);
				cut_node -> down_pointer_left -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, type : %u, go to left\n", this_cut_value, temp -> type);
				}
				*/
			}
			else
			{
				cut_node -> down_pointer_right =  move_leaf_node(cut_node -> down_pointer_right, temp);
				cut_node -> down_pointer_right -> rule_num += 1;
				/*
				if(temp -> s_ip == 1128583766 && temp -> d_ip == 2711100294 && temp -> d_port_s == 1704 && temp -> d_port_e == 1704 && temp -> type == 6){
					printf("this cut : %u, type : %u, go to right\n", this_cut_value, temp -> type);
				}
				*/
			}
			
		}

		temp = temp -> down_link;
	}

	//printf("left : %u, right : %u\n", cut_node -> down_pointer_left -> rule_num, cut_node -> down_pointer_right -> rule_num);
	//printf("-----------------------------\n");

	/*
	if(this_cut_value == 1700){
		struct hicut_leaf * temp = cut_node -> down_pointer_right -> hicut_leafnode;
		while(temp != NULL){
			printf("ports :%u, %u\n", temp -> d_port_s, temp -> d_port_e);
			temp = temp -> down_link;
		}
	}
	*/
	int temp_node_num = cut_node -> rule_num;
	if(cut_node -> hicut_leafnode != NULL){
		free_leaf_node(cut_node -> hicut_leafnode);
		cut_node -> rule_num = 0;
	}

	//printf("go to next\n");
	if(temp_node_num != cut_node -> down_pointer_left ->rule_num)
		build_hicut(cut_node -> down_pointer_left, run_count + 1);
	if(temp_node_num != cut_node -> down_pointer_right ->rule_num)
		build_hicut(cut_node -> down_pointer_right, run_count + 1);
}

void hicut_search(unsigned int input_ip_s, unsigned int input_ip_d, unsigned  int input_s_port_s, unsigned  int input_d_port_s, unsigned int input_type){
	struct hicut_node * temp = root_hicut;

	while(temp -> rule_num == 0){
		if(temp == NULL){
			//printf("temp is null\n");
			loss_case++;
			return;
 
		}
		int this_cut_value = temp -> cut_value;
		if(temp -> dimension == 1){
			if(this_cut_value >= input_ip_s){
				temp = temp -> down_pointer_left;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, input_in_s : %u, go to left\n", this_cut_value, input_ip_s);
				}
				*/
			}
			else{
				temp = temp -> down_pointer_right;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, input_in_s : %u, go to right\n", this_cut_value, input_ip_s);
				}
				*/
			}
		}
		else if(temp -> dimension == 2){
			if(this_cut_value >= input_ip_d){
				temp = temp -> down_pointer_left;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, input_in_d : %u, go to left\n", this_cut_value, input_ip_d);
				}
				*/
			}
			else{
				temp = temp -> down_pointer_right;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, input_in_d : %u, go to right\n", this_cut_value, input_ip_d);
				}
				*/
			}
		}
		else if(temp -> dimension == 3){
			if(this_cut_value >= input_s_port_s){
				temp = temp -> down_pointer_left;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, input_s_port_s : %u, go to left\n", this_cut_value, input_s_port_s);
				}
				*/
			}
			else{
				temp = temp -> down_pointer_right;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, input_s_port_s : %u, go to right\n", this_cut_value, input_s_port_s);
				}
				*/
			}
		}
		else if(temp -> dimension == 4){
			if(this_cut_value >= input_d_port_s){
				temp = temp -> down_pointer_left;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, input_d_port_s : %u, go to left\n", this_cut_value, input_d_port_s);
				}
				*/
			}
			else{
				temp = temp -> down_pointer_right;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, input_d_port_s : %u, go to right\n", this_cut_value, input_d_port_s);
				}
				*/
			}
		}
		else if(temp -> dimension == 5){
			if(this_cut_value >= input_type){
				temp = temp -> down_pointer_left;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, type : %u, go to left\n", this_cut_value, input_type);
				}
				*/
			}
			else{
				temp = temp -> down_pointer_right;
				/*
				if(input_ip_s == 1128583766 && input_ip_d == 2711100294 && input_d_port_s == 1704 && input_d_port_e == 1704 && input_type == 6){
					printf("this cut : %u, type : %u, go to right\n", this_cut_value, input_type);
				}
				*/
			}
		}
		else
		{
			printf("not happened in hicut search , dimension : %d\n", temp -> dimension);
		}
		
		if(temp == NULL){
			//printf("temp is null\n");
			loss_case++;
			return;

		}
		
	}

	struct hicut_leaf* temp_leaf = temp -> hicut_leafnode;

	while(temp_leaf != NULL){
		if(	input_ip_s >> (32 - temp_leaf -> s_len) == temp_leaf -> s_ip >> (32 - temp_leaf -> s_len) &&
			input_ip_d >> (32 - temp_leaf -> d_len) == temp_leaf -> d_ip >> (32 - temp_leaf -> d_len) &&
			input_s_port_s == temp_leaf -> s_port_s &&
			input_d_port_s == temp_leaf -> d_port_s &&
			input_type == temp_leaf -> type
			)
		{
			match_case++;
			return;
		}
		else
		{
			temp_leaf = temp_leaf -> down_link;
		}
	}
	//temp_leaf = temp -> hicut_leafnode;
	loss_case++;
	
	//printf("ip_s : %u, ip_d : %u, s_port_s : %u, s_port_e: %u, d_port_s : %u, d_port_e : %u, type : %u\n", input_ip_s, input_ip_d, input_s_port_s, input_s_port_e, input_d_port_s, input_d_port_e, input_type);
	//printf("temp_rulenum : %d-------------------------------------------\n", temp -> rule_num);
	/*
	while(temp_leaf != NULL){
		printf("s_ip : %u, s_len : %d, d_ip : %u, d_len : %d, s_port_s : %u, s_port_e : %u, d_port_s : %u, d_port_e : %u, type : %u\n", temp_leaf ->s_ip , temp_leaf ->s_len, temp_leaf ->d_ip, temp_leaf ->d_len, temp_leaf ->s_port_s, temp_leaf ->s_port_e, temp_leaf ->d_port_s, temp_leaf ->d_port_e, temp_leaf ->type);
		temp_leaf = temp_leaf -> down_link;
	}	
	*/
	return;

}
/*
void hicut_insert(unsigned int input_ip_s, unsigned int input_ip_d, unsigned  int input_s_port_s, unsigned  int input_s_port_e, unsigned  int input_d_port_s, unsigned  int input_d_port_e, unsigned int input_type, int input_s_len, int input_d_len){
	if(root_hicut == NULL){
		printf("root is null\n");
		return;
	}

	struct hicut_node * temp = root_hicut;

	while(temp -> rule_num == 0){
		if(temp == NULL){
			printf("temp is null\n");
			return;

		}
		int back_bit_len = temp -> back_cut_len[temp->dimension-1];
		if(temp -> dimension == 1){
			unsigned int temp_ip = ((input_ip_s << back_bit_len) >> back_bit_len) >> (32 - temp -> bit_len - back_bit_len);
			if(temp -> down_pointer[temp_ip] == NULL){
				temp -> down_pointer[temp_ip] = create_hicut_node();
				temp = temp -> down_pointer[temp_ip];
				break;
			}
			temp = temp -> down_pointer[temp_ip];

		}
		else if(temp -> dimension == 2){
			unsigned int temp_ip = ((input_ip_d << back_bit_len) >> back_bit_len) >> (32 - temp -> bit_len - back_bit_len);
			if(temp -> down_pointer[temp_ip] == NULL){
				temp -> down_pointer[temp_ip] = create_hicut_node();
				temp = temp -> down_pointer[temp_ip];
				break;
			}
			temp = temp -> down_pointer[temp_ip];
		}
		else if(temp -> dimension == 3){
			unsigned int temp_ip = ((input_s_port_s << back_bit_len) >> back_bit_len) >> (32 - temp -> bit_len - back_bit_len);
			if(temp -> down_pointer[temp_ip] == NULL){
				temp -> down_pointer[temp_ip] = create_hicut_node();
				temp = temp -> down_pointer[temp_ip];
				break;
			}
			temp = temp -> down_pointer[temp_ip];
		}
		else if(temp -> dimension == 4){
			unsigned int temp_ip = ((input_d_port_s << back_bit_len) >> back_bit_len) >> (32 - temp -> bit_len - back_bit_len);
			if(temp -> down_pointer[temp_ip] == NULL){
				temp -> down_pointer[temp_ip] = create_hicut_node();
				temp = temp -> down_pointer[temp_ip];
				break;
			}
			temp = temp -> down_pointer[temp_ip];
		}
		else if(temp -> dimension == 5){
			temp = temp -> down_pointer[input_type];
			if(temp -> down_pointer[input_type] == NULL){
				temp -> down_pointer[input_type] = create_hicut_node();
				temp = temp -> down_pointer[input_type];
				break;
			}
		}
		else
		{
			printf("not happened in hicut search , dimension : %d\n", temp -> dimension);
		}
		
	}

	if(temp == NULL){
		printf("will not happen\n");
	}

	struct hicut_leaf* temp_leaf = temp -> hicut_leafnode;

	if(temp_leaf == NULL){
		temp -> hicut_leafnode = create_hicut_leaf();
		temp -> hicut_leafnode -> s_ip = input_ip_s;
		temp -> hicut_leafnode -> d_ip = input_ip_d;
		temp -> hicut_leafnode -> s_len = input_s_len;
		temp -> hicut_leafnode -> d_len = input_d_len;
		temp -> hicut_leafnode -> s_port_s = input_s_port_s;
		temp -> hicut_leafnode -> s_port_e = input_s_port_e;
		temp -> hicut_leafnode -> d_port_s = input_d_port_s;
		temp -> hicut_leafnode -> d_port_e = input_d_port_e;
		temp -> hicut_leafnode -> type = input_type;
		temp -> rule_num ++;
		if( temp -> rule_num > 50)
			build_hicut(temp);
		return;
	}


	while(temp_leaf != NULL){
		if(	input_ip_s >> (32 - temp_leaf -> s_len) == temp_leaf -> s_ip >> (32 - temp_leaf -> s_len) &&
			input_ip_d >> (32 - temp_leaf -> d_len) == temp_leaf -> d_ip >> (32 - temp_leaf -> d_len) &&
			input_s_port_s == temp_leaf -> s_port_s &&
			input_s_port_e == temp_leaf -> s_port_e &&
			input_d_port_s == temp_leaf -> d_port_s &&
			input_d_port_e == temp_leaf -> d_port_e &&
			input_type == temp_leaf -> type
			)
		{
			return;
		}
		else
		{
			if(temp_leaf -> down_link == NULL){
				break;
			}
			temp_leaf = temp_leaf -> down_link;
		}
	}
	temp_leaf -> down_link = create_hicut_leaf();
	temp_leaf -> down_link -> s_ip = input_ip_s;
	temp_leaf -> down_link -> d_ip = input_ip_d;
	temp_leaf -> down_link -> s_len = input_s_len;
	temp_leaf -> down_link -> d_len = input_d_len;
	temp_leaf -> down_link -> s_port_s = input_s_port_s;
	temp_leaf -> down_link -> s_port_e = input_s_port_e;
	temp_leaf -> down_link -> d_port_s = input_d_port_s;
	temp_leaf -> down_link -> d_port_e = input_d_port_e;
	temp_leaf -> down_link -> type = input_type;
	temp -> rule_num ++;
	if( temp -> rule_num > 50)
		build_hicut(temp);

	return;
}
*/
////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char *argv[]){
	/*if(argc!=3){
		printf("Please execute the file as the following way:\n");
		printf("%s  routing_table_file_name  query_table_file_name\n",argv[0]);
		exit(1);
	}*/
	int i,j;
	//set_query(argv[2]);
	//set_table(argv[1]);

	set_query(argv[1]);
	set_table(argv[1]);

	if(num_entry <= 0)	return 0;

	

	create_root_hicut();
	build_hicut(root_hicut, 0);

	printf("endbuild \n");
	/*
	if(0){
		time_begin=rdtsc();
		for(i = num_entry/10 * 9; i < num_entry; ++i){
			hicut_insert(table[i].s_ip, table[i].d_ip, table[i].s_port_s, table[i].s_port_e, table[i].d_port_s, table[i].d_port_e, table[i].type, table[i].s_len, table[i].d_len);
		}
		time_end=rdtsc();
		printf("insert time : %d\n", (time_end- time_begin) / (num_entry/10));
		return 0;
	}
	*/

	
	//printf("number of nodes: %d\n",num_node);
	//printf("num entry : %d\n", num_entry);
	printf("Total memory requirement: %d KB\n",((hicut_node_num *sizeof(struct hicut_node) + leaf_node_num * sizeof(struct hicut_leaf))/1024));

	shuffle(query, num_query); 
	////////////////////////////////////////////////////////////////////////////
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			time_begin=rdtsc();
			hicut_search(query[i].s_ip, query[i].d_ip, query[i].s_port_s, query[i].d_port_s, query[i].type);
			time_end=rdtsc();
			if(time_clock[i]>(time_end-time_begin))
				time_clock[i]=(time_end-time_begin);
		}
	}
	total=0;
	for(j=0;j<num_query;j++)
		total+=time_clock[j];
	printf("Avg. Search: %llu\n",total/num_query);
	printf("match : %d, loss : %d\n", match_case, loss_case);
	CountClock();
	
	////////////////////////////////////////////////////////////////////////////
	//count_node(root);
	//printf("There are %d nodes in binary trie\n",N);
	return 0;
}
