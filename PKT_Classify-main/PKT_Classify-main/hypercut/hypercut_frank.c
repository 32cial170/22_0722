#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

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
int match_case = 0, loss_case = 0;
int leaf_node_num = 0, hicut_node_num = 0, pointer_num = 0;;
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
	int second_dimension;
	int bit_len;
	int second_bit_len;
	int back_cut_len[5];
	int rule_num;
	struct hicut_node ** down_pointer;
	struct hicut_leaf * hicut_leafnode;
};

struct hicut_node * root_hicut;

struct hicut_node* create_hicut_node(){
	struct hicut_node * temp;
	temp = (struct hicut_node * )malloc(sizeof(struct hicut_node));
	temp -> dimension = -1;
	temp -> second_dimension = -1;
	temp -> bit_len = -1;
	temp -> second_bit_len = -1;
	for(int i=0; i<5; ++i)
		temp -> back_cut_len[i] = 0;
	temp -> rule_num = 0;
	temp -> down_pointer = NULL;
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
struct port_range{
	int range_start;
	int range_end;
};
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
unsigned long long int begin,end,total=0;
unsigned long long int *clock;
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
		}
		else{
			sprintf(buf,"%s\0",strtok(buf2,tok));
			n[0]=atoi(buf);
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

	//printf("s_ip %u, d_ip : %u\n", *s_ip, *d_ip);

	*s_port_s = atoi(strtok(buf3,":"));
	*s_port_e = atoi(strtok(NULL,":"));
	*d_port_s = atoi(strtok(buf4,":"));
	*d_port_e = atoi(strtok(NULL,":"));
	*type = hexToDec(strtok(buf5,"/"));
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
		if(s_port_e != 65535)
			printf("s_port_e : %d\n", s_port_e);
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
	clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));
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
		clock[num_query++]=10000000;
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
		if(clock[i] > MaxClock) MaxClock = clock[i];
		if(clock[i] < MinClock) MinClock = clock[i];
		if(clock[i] / 100 < 50) NumCntClock[clock[i] / 100]++;
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

int get_cut_bit_number(struct hicut_leaf * input_node, int dimension, int back_bit_len, int rule_num){
	if(input_node == NULL)	return -1;
	if(dimension <= 0) return -1;
	int spfac = 2;

	struct hicut_leaf * temp;

	if(rule_num <= 50){
		return -1;
	}
	int spmf = spfac * sqrt(rule_num);
	if(dimension == 1){
		temp = input_node;
		
		int bit_cut_count = 0, rule_child = 0;;
		while(1){
			while(temp != NULL){
				//printf("part3\n");
				if(temp -> s_len >= (bit_cut_count + 1 + back_bit_len)){
					//printf("part4.1\n");
					++rule_child;
				}
				else
				{
					//printf("part5.1\n");
					//printf("rule count : %d\n", rule_count);

					unsigned int temp_ip_down = ((temp -> s_ip << back_bit_len) >> back_bit_len) >> (32-temp->s_len) << min((bit_cut_count+1 + back_bit_len  - temp->s_len), bit_cut_count+1);
					unsigned int temp_ip_up = (((temp -> s_ip << back_bit_len) >> back_bit_len) >> (32-temp->s_len));
					for(int i=0; i < min((bit_cut_count+1 + back_bit_len - temp->s_len), bit_cut_count+1); ++i){
						temp_ip_up = temp_ip_up << 1;
						temp_ip_up = temp_ip_up + 1;
					}
					//printf("down : %u, top : %u, rule count : %d\n", temp_ip_down, temp_ip_up, rule_count);
					for(unsigned int i = temp_ip_down; i <= temp_ip_up; ++ i){
						++rule_child;
					}
					//printf("part5.2\n");
				}
				temp = temp -> down_link;
				//free(free_temp);
			}	

			if(spmf >= sqrt(rule_child) + pow(2, bit_cut_count+1)){
				bit_cut_count ++ ;
				rule_child = 0;
				temp = input_node;
			}
			else
			{
				//printf("rule_num : %d, spmf : %d, rule_child : %d, bit_cut_count: %d\n", rule_num, spmf, rule_child, bit_cut_count);
				return bit_cut_count;
			}
			if(bit_cut_count + back_bit_len >= 32){
				//printf("rule_num : %d, spmf : %d, rule_child : %d, bit_cut_count: %d\n", rule_num, spmf, rule_child, bit_cut_count);
				return bit_cut_count;
			}
		}
		
	}
	else if(dimension == 2){
		temp = input_node;
		int bit_cut_count = 0, rule_child = 0;
		while(1){
			while(temp != NULL){
				//printf("part3\n");
				if(temp -> s_len >= (bit_cut_count + 1 + back_bit_len)){
					//printf("part4.1\n");
					++rule_child;
				}
				else
				{
					//printf("part5.1\n");
					unsigned int temp_ip_down = ((temp -> d_ip << back_bit_len) >> back_bit_len) >> (32-temp->d_len) << min((bit_cut_count+1 + back_bit_len  - temp->d_len), bit_cut_count+1);
					unsigned int temp_ip_up = ((temp -> d_ip << back_bit_len) >> back_bit_len) >> (32-temp->d_len);
					for(int i=0; i < min((bit_cut_count+1 + back_bit_len - temp->d_len), bit_cut_count+1); ++i){
						temp_ip_up = temp_ip_up << 1;
						temp_ip_up = temp_ip_up + 1;
					}
					//printf("down : %u, top : %u, rule count : %d\n", temp_ip_down, temp_ip_up, rule_count);
					for(unsigned int i = temp_ip_down; i <= temp_ip_up; ++ i){
						++rule_child;
					}
					//printf("part5.2\n");
				}
				temp = temp -> down_link;
				//free(free_temp);
			}
			
			if(spmf >= sqrt(rule_child) + pow(2, bit_cut_count+1)){
				bit_cut_count ++ ;
				rule_child = 0;
				temp = input_node;
			}
			else
			{
				//printf("rule_num : %d, spmf : %d, rule_child : %d, bit_cut_count: %d\n", rule_num, spmf, rule_child, bit_cut_count);
				return bit_cut_count;
			}	
			if(bit_cut_count + back_bit_len >= 32){
				//printf("rule_num : %d, spmf : %d, rule_child : %d, bit_cut_count: %d\n", rule_num, spmf, rule_child, bit_cut_count);
				return bit_cut_count;
			}
		}
	}
	else if(dimension == 3){
		temp = input_node;
		int bit_cut_count = 0, rule_child = 0;
		while(1){
			while(temp != NULL){
				unsigned int down = ((temp -> s_port_s << back_bit_len) >> back_bit_len) >> (32 - (bit_cut_count + 1) - back_bit_len);
				unsigned int top = ((temp -> s_port_e << back_bit_len) >> back_bit_len) >> (32 - (bit_cut_count + 1) - back_bit_len);
				if(top < down)
					top = down;
				for(unsigned  int i = down; i <= top; ++i){
					++rule_child;
				}
				temp = temp -> down_link;

			}
			if(spmf >= sqrt(rule_child) + pow(2, bit_cut_count+1)){
				bit_cut_count ++ ;
				rule_child = 0;
				temp = input_node;
			}
			else
			{
				//printf("rule_num : %d, spmf : %d, rule_child : %d, bit_cut_count: %d\n", rule_num, spmf, rule_child, bit_cut_count);
				return bit_cut_count;
			}
			if(bit_cut_count + back_bit_len >= 32){
				//printf("rule_num : %d, spmf : %d, rule_child : %d, bit_cut_count: %d\n", rule_num, spmf, rule_child, bit_cut_count);
				return bit_cut_count;
			}
		}
	}
	else if(dimension == 4)
	{
		temp = input_node;
		int bit_cut_count = 0, rule_child = 0;
		unsigned int down = 0, top  = 0;
		while(1){
			while(temp != NULL){
				down = ((temp -> d_port_s << back_bit_len) >> back_bit_len) >> (32 - (bit_cut_count + 1) - back_bit_len);
				top = ((temp -> d_port_e << back_bit_len) >> back_bit_len) >> (32 - (bit_cut_count + 1) - back_bit_len);
				//printf("down : %u, top : %u, bit cut : %d, back : %d\n", down, top, bit_cut_count, back_bit_len);
				//printf("half top : %u, shift num : %d\n", ((temp -> d_port_e << back_bit_len) >> back_bit_len), (32 - (bit_cut_count + 1) - back_bit_len));
				if(top < down)
					top = down;
				for(unsigned  int i = down; i <= top; ++i){
					++rule_child;
				}
				temp = temp -> down_link;

			}
			if(spmf >= sqrt(rule_child) + pow(2, bit_cut_count+1)){
				bit_cut_count ++ ;
				rule_child = 0;
				temp = input_node;
			}
			else
			{
				//printf("rule_num : %d, spmf : %d, rule_child : %d, bit_cut_count: %d, back: %d\n", rule_num, spmf, rule_child, bit_cut_count, back_bit_len);
				return bit_cut_count;
			}
			if(bit_cut_count + back_bit_len >= 32){
				//printf("rule_num : %d, spmf : %d, rule_child : %d, bit_cut_count: %d\n", rule_num, spmf, rule_child, bit_cut_count);
				return bit_cut_count;
			}
		}
	}
	else
	{
		if(back_bit_len >= 8){
				return 0;
		}
		else
		{
			return 8;
		}
		
	}
}

struct hicut_node * move_leaf_node(struct hicut_node * input_node, struct hicut_leaf * input_node2){
	if(input_node2 == NULL)
		return input_node;
	//move 2 to 1
	if(input_node == NULL){
		input_node = create_hicut_node();
	}
		
	
	struct hicut_leaf * temp;
	//printf("part a\n");
	if(input_node -> hicut_leafnode == NULL){
		input_node -> hicut_leafnode = create_hicut_leaf();
		temp = input_node -> hicut_leafnode;
	}
	else{
		temp = input_node -> hicut_leafnode;
		while(temp -> down_link != NULL){
			if(temp != NULL){
				
			}
			temp = temp-> down_link;
		}
		//printf("s_ip : %u, d_ip : %u, s_len : %d, d_len : %d\n", temp ->s_ip, temp -> d_ip, temp -> s_len,  temp -> d_len);
		temp -> down_link = create_hicut_leaf();
		temp = temp-> down_link;
	}
	
	//printf("part b\n");

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
	if(input_node -> down_link != NULL){
		free_leaf_node(input_node -> down_link);
	}
	leaf_node_num--;
	free(input_node);
}

void build_hicut(struct hicut_node * cut_node){
	//choose dimension
	int dimension_distinct_num[5];
	int max_dimension_num = -1, max_dimension = 0, second_dimension_num = -1, second_dimension = 0, dimension_mean = 0;
	for(int i=0; i<5; ++i){
		dimension_distinct_num[i] = count_distinct_rule_num(cut_node->hicut_leafnode, i+1);
		//printf("dimension_distinct_num[i] : %d\n", dimension_distinct_num[i]);
		if(dimension_distinct_num[i] > max_dimension_num){
			max_dimension_num = dimension_distinct_num[i];
			max_dimension = i+1;
		}
		dimension_mean += dimension_distinct_num[i];
	}
	for(int i=0; i < 5; ++i){
		if(i != max_dimension - 1){
			if(dimension_distinct_num[i] > second_dimension_num){
				second_dimension_num = dimension_distinct_num[i];
				second_dimension = i + 1;
			}
		}
	}
	//
	/*
	if(cut_node -> rule_num > 50){
		printf("rulenum : %d\n", cut_node ->rule_num);
		printf("max: %d, max dimension : %d, second : %d, second dimension : %d, dimension mean : %d\n", max_dimension_num, max_dimension, second_dimension_num, second_dimension, dimension_mean/5);
	}
	*/
	if(max_dimension == -1 || max_dimension == 0 || second_dimension_num == -1 || second_dimension == 0){
		return;
	}
	if(max_dimension_num < dimension_mean/5 || second_dimension_num < dimension_mean/5){
		return;
	}
	if(max_dimension_num < 3 || second_dimension_num < 3){
		return;
	}
	//choost number of cut;

	int back_bit_len = cut_node -> back_cut_len[max_dimension-1];
	int back_bit_len_second = cut_node -> back_cut_len[second_dimension -1];

	int cut_bit_number = get_cut_bit_number(cut_node->hicut_leafnode, max_dimension, back_bit_len, cut_node -> rule_num);
	int cut_bit_number_2 = get_cut_bit_number(cut_node->hicut_leafnode, second_dimension, back_bit_len_second, cut_node -> rule_num);
	
	
	

	
	if(cut_bit_number <= 0 || cut_bit_number_2 <= 0)
		return;
	/*
	if(cut_node -> rule_num > 50){
		printf("cut_number : %d, max dimension : %d, back_num : %d, max dimension num : %d, rule_num : %d\n", cut_bit_number, max_dimension, back_bit_len, max_dimension_num, cut_node -> rule_num);
		printf("cut_number_2 : %d, max dimension_2 : %d, back_num_2 : %d, max dimension num_2 : %d, rule_num : %d\n", cut_bit_number_2, second_dimension, back_bit_len_second, second_dimension_num);
		printf("-------------------------------------\n");		
	}
	*/

	cut_node -> dimension = max_dimension;
	cut_node -> second_dimension = second_dimension;
	cut_node -> bit_len = cut_bit_number;
	cut_node -> second_bit_len = cut_bit_number_2;
	
	int pownum = pow(2, cut_node -> bit_len );
	int second_pownum = pow(2, cut_node -> second_bit_len );
	cut_node -> down_pointer = (struct hicut_node **)malloc(sizeof(struct hicut_node *) * pownum * second_pownum);
	pointer_num += pownum * second_pownum;
	//printf("pownum = %d\n", pownum);
	for(int i = 0; i<pownum * second_pownum; ++i){
		cut_node -> down_pointer[i] = NULL;
	}
	struct hicut_leaf * temp = cut_node -> hicut_leafnode;
	while (temp != NULL){
		unsigned int temp_ip_down, temp_ip_up, temp_ip_down_second, temp_ip_up_second;
		if(cut_node -> dimension == 1 || cut_node -> dimension == 2){
			if(cut_node -> bit_len + back_bit_len > 32){
				return;
			}
			if(cut_node -> dimension == 1){
				if(temp -> s_len >= (cut_node -> bit_len + back_bit_len)){
					temp_ip_down = ((temp -> s_ip << back_bit_len) >> back_bit_len) >> (32 - cut_node -> bit_len - back_bit_len);
					temp_ip_up = ((temp -> s_ip << back_bit_len) >> back_bit_len) >> (32 - cut_node -> bit_len - back_bit_len);
				}
				else
				{
					temp_ip_down = ((temp -> s_ip << back_bit_len) >> back_bit_len) >> (32-temp->s_len) << min((cut_node -> bit_len + back_bit_len  - temp->s_len), cut_node -> bit_len);
					temp_ip_up = (((temp -> s_ip << back_bit_len) >> back_bit_len) >> (32-temp->s_len));
					for(int i=0; i < min((cut_node -> bit_len + back_bit_len - temp->s_len), cut_node -> bit_len); ++i){
						temp_ip_up = temp_ip_up << 1;
						temp_ip_up = temp_ip_up + 1;
					}

				}
			}
			else if(cut_node -> dimension == 2){
				if(temp -> d_len >= (cut_node -> bit_len + back_bit_len)){
					temp_ip_down = ((temp -> d_ip << back_bit_len) >> back_bit_len) >> (32 - cut_node -> bit_len - back_bit_len);
					temp_ip_up = ((temp -> d_ip << back_bit_len) >> back_bit_len) >> (32 - cut_node -> bit_len - back_bit_len);
				}
				else
				{
					temp_ip_down = ((temp -> d_ip << back_bit_len) >> back_bit_len) >> (32-temp->d_len) << min((cut_node -> bit_len + back_bit_len  - temp->d_len), cut_node -> bit_len);
					temp_ip_up = (((temp -> d_ip << back_bit_len) >> back_bit_len) >> (32-temp->d_len));
					for(int i=0; i < min((cut_node -> bit_len + back_bit_len - temp->d_len), cut_node -> bit_len); ++i){
						temp_ip_up = temp_ip_up << 1;
						temp_ip_up = temp_ip_up + 1;
					}

				}
			}
		}
		else if(cut_node -> dimension == 3 || cut_node -> dimension == 4){
			if(cut_node -> bit_len + back_bit_len > 32){
				return;
			}
			if(cut_node -> dimension == 3){
				temp_ip_down = ((temp -> s_port_s << back_bit_len) >> back_bit_len) >> (32 - cut_node -> bit_len - back_bit_len);
				temp_ip_up = ((temp -> s_port_e << back_bit_len) >> back_bit_len) >> (32 - cut_node -> bit_len  - back_bit_len);
				if(temp_ip_up < temp_ip_down)
					temp_ip_up = temp_ip_down;
			}
			else if(cut_node -> dimension == 4){
				temp_ip_down = ((temp -> d_port_s << back_bit_len) >> back_bit_len) >> (32 - cut_node -> bit_len - back_bit_len);
				temp_ip_up = ((temp -> d_port_e << back_bit_len) >> back_bit_len) >> (32 - cut_node -> bit_len - back_bit_len);
				if(temp_ip_up < temp_ip_down)
					temp_ip_up = temp_ip_down;
			}
		}
		else if(cut_node -> dimension == 5){
			if(cut_node -> bit_len + back_bit_len > 8){
				return;
			}
			temp_ip_down = 0;
			temp_ip_up = 7;
		}
		else {
			printf("not happen in dimension 1\n");
		}

		if(cut_node -> second_dimension == 1 || cut_node -> second_dimension == 2){
			if(cut_node -> second_bit_len + back_bit_len_second > 32){
				return;
			}
			if(cut_node -> second_dimension == 1){
				if(temp -> s_len >= (cut_node -> second_bit_len + back_bit_len_second)){
					temp_ip_down_second = ((temp -> s_ip << back_bit_len_second) >> back_bit_len_second) >> (32 - cut_node -> second_bit_len - back_bit_len_second);
					temp_ip_up_second = ((temp -> s_ip << back_bit_len_second) >> back_bit_len_second) >> (32 - cut_node -> second_bit_len - back_bit_len_second);
				}
				else
				{
					temp_ip_down_second = ((temp -> s_ip << back_bit_len_second) >> back_bit_len_second) >> (32-temp->s_len) << min((cut_node -> second_bit_len + back_bit_len_second  - temp->s_len), cut_node -> second_bit_len);
					temp_ip_up_second = (((temp -> s_ip << back_bit_len_second) >> back_bit_len_second) >> (32-temp->s_len));
					for(int i=0; i < min((cut_node -> second_bit_len + back_bit_len_second - temp->s_len), cut_node -> second_bit_len); ++i){
						temp_ip_up_second = temp_ip_up_second << 1;
						temp_ip_up_second = temp_ip_up_second + 1;
					}

				}
			}
			else if(cut_node -> second_dimension == 2){
				if(temp -> d_len >= (cut_node -> second_bit_len + back_bit_len_second)){
					temp_ip_down_second = ((temp -> d_ip << back_bit_len_second) >> back_bit_len_second) >> (32 - cut_node -> second_bit_len - back_bit_len_second);
					temp_ip_up_second = ((temp -> d_ip << back_bit_len_second) >> back_bit_len_second) >> (32 - cut_node -> second_bit_len - back_bit_len_second);
				}
				else
				{
					temp_ip_down_second = ((temp -> d_ip << back_bit_len_second) >> back_bit_len_second) >> (32-temp->d_len) << min((cut_node -> second_bit_len + back_bit_len_second  - temp->d_len), cut_node -> second_bit_len);
					temp_ip_up_second = (((temp -> d_ip << back_bit_len_second) >> back_bit_len_second) >> (32-temp->d_len));
					for(int i=0; i < min((cut_node -> second_bit_len + back_bit_len_second - temp->d_len), cut_node -> second_bit_len); ++i){
						temp_ip_up_second = temp_ip_up_second << 1;
						temp_ip_up_second = temp_ip_up_second + 1;
					}

				}
			}
		}
		else if(cut_node -> second_dimension == 3 || cut_node -> second_dimension == 4){
			if(cut_node -> second_bit_len + back_bit_len_second > 32){
				return;
			}
			if(cut_node -> second_dimension == 3){
				temp_ip_down_second = ((temp -> s_port_s << back_bit_len_second) >> back_bit_len_second) >> (32 - cut_node -> second_bit_len - back_bit_len_second);
				temp_ip_up_second = ((temp -> s_port_e << back_bit_len_second) >> back_bit_len_second) >> (32 - cut_node -> second_bit_len  - back_bit_len_second);
				if(temp_ip_up_second < temp_ip_down_second)
					temp_ip_up_second = temp_ip_down_second;
			}
			else if(cut_node -> second_dimension == 4){
				temp_ip_down_second = ((temp -> d_port_s << back_bit_len_second) >> back_bit_len_second) >> (32 - cut_node -> second_bit_len - back_bit_len_second);
				temp_ip_up_second = ((temp -> d_port_e << back_bit_len_second) >> back_bit_len_second) >> (32 - cut_node -> second_bit_len - back_bit_len_second);
				if(temp_ip_up_second < temp_ip_down_second)
					temp_ip_up_second = temp_ip_down_second;
			}
		}
		else if(cut_node -> second_dimension == 5){
			if(cut_node -> second_bit_len + back_bit_len_second > 8){
				return;
			}
			temp_ip_down_second = 0;
			temp_ip_up_second = 7;
		}
		else {
			printf("not happen in dimension 2\n");
		}


		for(unsigned int i = temp_ip_down; i <= temp_ip_up; ++ i){
			for(unsigned int j = temp_ip_down_second; j <= temp_ip_up_second; ++j){
				cut_node -> down_pointer[i * second_pownum + j] = move_leaf_node(cut_node -> down_pointer[i * second_pownum + j], temp);
				cut_node -> down_pointer[i * second_pownum + j]->rule_num += 1;
				for(int k=0; k<5; ++k){
					cut_node -> down_pointer[i * second_pownum + j]->back_cut_len[k] = cut_node -> back_cut_len[k];
				}
				cut_node -> down_pointer[i * second_pownum + j]->back_cut_len[cut_node -> dimension -1] += cut_bit_number;
				cut_node -> down_pointer[i * second_pownum + j]->back_cut_len[cut_node -> second_dimension -1] += cut_bit_number_2;
			}
		}
		/*
		if(temp_ip_down != temp_ip_up){
			printf("ip down : %u, ip up : %u, ip2 down : %u, ip2 up : %u\n", temp_ip_down, temp_ip_up, temp_ip_down_second, temp_ip_up_second);
			printf("bit : %d, back : %d, bit 2 : %d, back 2 : %d\n", cut_bit_number, back_bit_len, cut_bit_number_2, back_bit_len_second);
		}
		*/
		temp = temp -> down_link;
	}


	if(cut_node -> hicut_leafnode != NULL){
		free_leaf_node(cut_node -> hicut_leafnode);
		cut_node -> rule_num = 0;
	}

	//printf("go to next\n");

	for(int i = 0; i < pownum * second_pownum; ++i){
		if(cut_node -> down_pointer[i] != NULL){
			//printf("a\n");
			//printf("rulenum : %d\n", cut_node -> down_pointer[i] ->rule_num);
			build_hicut(cut_node -> down_pointer[i]);
			//printf("B\n");
		}
			
	}
}

void hicut_search(unsigned int input_ip_s, unsigned int input_ip_d, unsigned  int input_s_port_s, unsigned  int input_s_port_e, unsigned  int input_d_port_s, unsigned  int input_d_port_e, unsigned int input_type){
	/*
	if(root_hicut == NULL){
		printf("root is null\n");
		return;
	}
	*/
	struct hicut_node * temp = root_hicut;
	//printf("into hicut node\n");
	while(temp -> rule_num == 0){
		/*
		if(temp == NULL){
			loss_case ++;
			return;
		}
		*/
		int back_bit_len = temp -> back_cut_len[temp->dimension-1];
		int back_bit_len_second = temp -> back_cut_len[temp->second_dimension-1];
		unsigned int temp_ip = 0, temp_ip_second = 0;

		if(temp -> dimension == 1){
			temp_ip = ((input_ip_s << back_bit_len) >> back_bit_len) >> (32 - temp -> bit_len - back_bit_len);
		}
		else if(temp -> dimension == 2){
			temp_ip = ((input_ip_d << back_bit_len) >> back_bit_len) >> (32 - temp -> bit_len - back_bit_len);
		}
		else if(temp -> dimension == 3){
			temp_ip = ((input_s_port_s << back_bit_len) >> back_bit_len) >> (32 - temp -> bit_len - back_bit_len);
		}
		else if(temp -> dimension == 4){
			temp_ip = ((input_d_port_s << back_bit_len) >> back_bit_len) >> (32 - temp -> bit_len - back_bit_len);
		}
		else if(temp -> dimension == 5){
			temp_ip = input_type;
		}
		else
		{
			printf("not happened in hicut search , dimension 1 : %d\n", temp -> dimension);
		}

		if(temp -> second_dimension == 1){
			temp_ip_second = ((input_ip_s << back_bit_len_second) >> back_bit_len_second) >> (32 - temp -> second_bit_len - back_bit_len_second);
		}
		else if(temp -> second_dimension == 2){
			temp_ip_second = ((input_ip_d << back_bit_len_second) >> back_bit_len_second) >> (32 - temp -> second_bit_len - back_bit_len_second);
		}
		else if(temp -> second_dimension == 3){
			temp_ip_second = ((input_s_port_s << back_bit_len_second) >> back_bit_len_second) >> (32 - temp -> second_bit_len - back_bit_len_second);
		}
		else if(temp -> second_dimension == 4){
			temp_ip_second = ((input_d_port_s << back_bit_len_second) >> back_bit_len_second) >> (32 - temp -> second_bit_len - back_bit_len_second);
		}
		else if(temp -> second_dimension == 5){
			temp_ip_second = input_type;
		}
		else
		{
			printf("not happened in hicut search , dimension 2 : %d\n", temp -> second_dimension);
		}

		int pownum_second = pow(2, temp -> second_bit_len);
		//printf("bit len : %d, bit len 2 : %d, temp_ip : %u, temp_ip_2 : %u, dimension : %d, dimension 2 : %d\n", temp -> bit_len, temp -> second_bit_len, temp_ip, temp_ip_second, temp -> dimension, temp -> second_dimension);
		//printf("index : %d, dimension : %d , dimension 2\n", temp_ip * pownum_second + temp_ip_second, temp -> dimension, temp -> second_dimension);
		//printf('next rule num : %d\n', temp -> down_pointer[temp_ip * pownum_second + temp_ip_second]->rule_num);
		temp = temp -> down_pointer[temp_ip * pownum_second + temp_ip_second];
		/*
		if(temp == NULL){
			loss_case ++;
			return;
		}
		*/
		//printf("safe\n");
	}

	//printf("into hicut leaf\n");

	struct hicut_leaf* temp_leaf = temp -> hicut_leafnode;
	/*
	if(temp -> bit_len > 0 || temp -> second_bit_len > 0){
		printf("temp bit error : %d, second : %d\n", temp -> bit_len, temp -> second_bit_len);
	}
	if(temp -> dimension > 0 || temp -> second_dimension > 0){
		printf("temp dimension error : %d, second : %d\n",temp -> dimension, temp -> second_dimension);
	}
	*/
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
	//printf("====================================\n");
	/*
	printf("ip_s : %u, ip_d : %u, s_port_s : %u, s_port_e: %u, d_port_s : %u, d_port_e : %u, type : %u\n", input_ip_s, input_ip_d, input_s_port_s, input_s_port_e, input_d_port_s, input_d_port_e, input_type);
	printf("temp_rulenum : %d, dimension : %d, %d, %d, %d, %d\n", temp -> rule_num, temp -> back_cut_len[0], temp -> back_cut_len[1], temp -> back_cut_len[2], temp -> back_cut_len[3], temp -> back_cut_len[4]);
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

	create_root_hicut();
	build_hicut(root_hicut);

	printf("endbuild \n");

	/*
	if(0){
		begin=rdtsc();
		for(i = num_entry/10 * 9; i < num_entry; ++i){
			hicut_insert(table[i].s_ip, table[i].d_ip, table[i].s_port_s, table[i].s_port_e, table[i].d_port_s, table[i].d_port_e, table[i].type, table[i].s_len, table[i].d_len);
		}
		end=rdtsc();
		printf("insert time : %d\n", (end- begin) / (num_entry/10));
		return 0;
	}
	*/

	
	//printf("number of nodes: %d\n",num_node);
	//printf("num entry : %d\n", num_entry);
	printf("Total memory requirement: %d KB\n",((hicut_node_num *sizeof(struct hicut_node) + leaf_node_num * sizeof(struct hicut_leaf) + pointer_num * sizeof(struct hicut_node*))/1024));

	shuffle(query, num_query); 
	////////////////////////////////////////////////////////////////////////////
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			begin=rdtsc();
			hicut_search(query[i].s_ip, query[i].d_ip, query[i].s_port_s, query[i].s_port_e, query[i].d_port_s, query[i].d_port_e, query[i].type);
			end=rdtsc();
			if(clock[i]>(end-begin))
				clock[i]=(end-begin);
		}
	}
	total=0;
	for(j=0;j<num_query;j++)
		total+=clock[j];
	printf("Avg. Search: %llu\n",total/num_query);
	printf("match : %d, loss : %d\n", match_case, loss_case);
	CountClock();
	
	////////////////////////////////////////////////////////////////////////////
	//count_node(root);
	//printf("There are %d nodes in binary trie\n",N);
	return 0;
}
