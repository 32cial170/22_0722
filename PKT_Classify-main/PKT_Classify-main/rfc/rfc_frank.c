#include<stdlib.h>
#include<stdio.h>
#include<string.h>

////////////////////////////////////////////////////////////////////////////////////
struct ENTRY{
	unsigned int s_ip;
	unsigned int d_ip;
	unsigned char s_len;
	unsigned char d_len;
	unsigned short int s_port_s;
	unsigned short int s_port_e;
	unsigned short int d_port_s;
	unsigned short int d_port_e;
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
int chunk_node_number = 0, rule_node_number = 0, rule_leaf_number = 0, rule_node_array_number = 0;
struct chunk
{
	int eqid;
	struct rule_leaf * leaf_link;
};

struct rule_node{
	struct rule_node * right_link;
	struct rule_leaf * leaf_link;

};

struct rule_node_array{
	struct rule_leaf * leaf_link;
};

struct rule_node_array * phase1_ip_s_up_array = NULL, * phase1_ip_s_down_array = NULL, * phase1_ip_d_up_array = NULL, * phase1_ip_d_down_array = NULL, * phase1_port_s_array = NULL, * phase1_port_d_array = NULL, * phase1_type_array = NULL;
struct rule_node_array * phase2_ip_s_array = NULL, * phase2_ip_d_array = NULL, *phase2_port_type_array = NULL;
struct rule_node_array * phase3_all_array = NULL;
struct rule_node * create_rule_node(){
	struct rule_node * temp = (struct rule_node*)malloc(sizeof(struct rule_node));
	rule_node_number++;
	temp -> right_link = NULL;
	temp -> leaf_link = NULL;
	return temp;
}

struct rule_node * phase1_ip_s_up = NULL, * phase1_ip_s_down = NULL, * phase1_ip_d_up = NULL, * phase1_ip_d_down = NULL, * phase1_port_s = NULL, * phase1_port_d = NULL, * phase1_type = NULL;
struct rule_node * phase2_ip_s = NULL, * phase2_ip_d = NULL, * phase2_port_type = NULL;
struct rule_node * phase3_all = NULL;
int phase1_ip_s_up_eqid = 0, phase1_ip_s_down_eqid = 0, phase1_ip_d_up_eqid = 0, phase1_ip_d_down_eqid = 0, phase1_port_s_eqid = 0, phase1_port_d_eqid = 0, phase1_type_eqid = 0;
int phase2_ip_s_eqid = 0, phase2_ip_d_eqid = 0, phase2_port_type_eqid = 0;
int phase3_all_eqid = 0;
struct rule_leaf{
	int rule_index;
	struct rule_leaf * down_link;
};

struct rule_leaf* create_rule_leaf(){
	struct rule_leaf * temp = (struct rule_leaf*)malloc(sizeof(struct rule_leaf));
	rule_leaf_number++;
	temp -> down_link = NULL;
	temp -> rule_index = -1;
	return temp;
}

struct chunk chunk_s_ip_up[65536], chunk_s_ip_down[65536], chunk_d_ip_up[65536], chunk_d_ip_down[65536], chunk_s_port[65536], chunk_d_port[65536],  chunk_type[256];
struct chunk * chunk_ip_s_phase2, * chunk_ip_d_phase2, * chunk_port_type_phase2;
struct chunk * chunk_all_phase3;

int match_case = 0, loss_case = 0;

////////////////////////////////////////////////////////////////////////////////////
struct list{//structure of binary trie
	unsigned int port;
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
	temp->port=256;//default port
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr=root;
	int i;
	if(len == 0)
		ptr -> port = nexthop;
	for(i=0;i<len;i++){
		if(ip&(1<<(31-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); // Create Node
			ptr=ptr->right;
			if((i==len-1)&&(ptr->port==256))
				ptr->port=nexthop;
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if((i==len-1)&&(ptr->port==256))
				ptr->port=nexthop;
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

void read_table(char *str,unsigned int *s_ip, unsigned int *d_ip, int *s_len, int *d_len, unsigned short int *s_port_s, unsigned short int *s_port_e, unsigned short int *d_port_s, unsigned short int *d_port_e, unsigned int *type){
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
	unsigned short int s_port_s, s_port_e, d_port_s, d_port_e;
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
	unsigned short int s_port_s, s_port_e, d_port_s, d_port_e;
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

		/*
		printf("s_ip : %u\n", s_ip);
		printf("d_ip : %u\n", d_ip);
		printf("s_len : %d\n", s_len);
		printf("d_len : %d\n", d_len);
		printf("s_port_s : %d\n", s_port_s);
		printf("s_port_e : %d\n", s_port_e);
		printf("d_port_s : %d\n", d_port_s);
		printf("d_port_e : %d\n", d_port_e);
		printf("type : %u\n", type);
		*/
		clock[num_query++]=10000000;
	}
}
////////////////////////////////////////////////////////////////////////////////////
/*
void create(){
	int i;
	root=create_node();
	begin=rdtsc();
	for(i=0;i<num_entry/10*9;i++)
		add_node(table[i].ip,table[i].len,table[i].port);
	end=rdtsc();

	printf("Avg. Build: %llu\n",(end-begin)/(num_entry/10*9));


	begin=rdtsc();
	for(i=num_entry/10*9;i<num_entry;i++)
		add_node(table[i].ip,table[i].len,table[i].port);
	end=rdtsc();

	printf("Avg. Insert: %llu\n",(end-begin)/(num_entry/10));
}
*/
////////////////////////////////////////////////////////////////////////////////////
void count_node(btrie r){
	if(r==NULL)
		return;
	count_node(r->left);
	N++;
	count_node(r->right);
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
		temp->type=array[j].type;
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
void free_leaf_link(struct rule_leaf * leaf_node){
	struct rule_leaf * temp_free, * temp = leaf_node;

	while(temp != NULL){
		temp_free = temp;
		temp = temp -> down_link;
		rule_leaf_number--;
		free(temp_free);
	}
}

void create_RFC(){
	int num_entry_size = num_entry/10*9;
	//create phase 1 chunk
	unsigned int input_ip_s_up, input_ip_s_down, input_ip_d_up, input_ip_d_down, input_port_s_start, input_port_s_end, input_port_d_start, input_port_d_end, input_type;
	for(int i = 0; i < num_entry_size; ++i){
		input_ip_s_up = table[i].s_ip >> 16;

		//printf("a\n");
		if(table[i].s_len < 16){
			unsigned int down = input_ip_s_up >> (16 - table[i].s_len) << (16 - table[i].s_len);
			unsigned int up = input_ip_s_up >> (16 - table[i].s_len);

			for(unsigned int j = 0; j < (16 - table[i].s_len); ++j){
				up = up << 1;
				up +=1;
			}
			for(int j = down; j <= up; ++j){
				struct rule_leaf * temp = create_rule_leaf();
				temp -> rule_index = i;
				//printf("rule index : %d\n", temp -> rule_index);
				temp -> down_link = chunk_s_ip_up[j].leaf_link;
				chunk_s_ip_up[j].leaf_link = temp;
			}
		}
		else{
			struct rule_leaf * temp = create_rule_leaf();
			temp -> rule_index = i;
			//printf("rule index : %d\n", temp -> rule_index);
			temp -> down_link = chunk_s_ip_up[input_ip_s_up].leaf_link;
			chunk_s_ip_up[input_ip_s_up].leaf_link = temp;
		}

	}

	struct rule_leaf * temp_leaf = NULL;
	for(int i = 0; i< 65536; ++i){
		struct rule_node * temp_phase_node = phase1_ip_s_up;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_s_ip_up[i].leaf_link == NULL){
			chunk_s_ip_up[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_s_ip_up[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_s_ip_up[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_s_ip_up[i].eqid = quid_num;
				free_leaf_link(chunk_s_ip_up[i].leaf_link);
				chunk_s_ip_up[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase1_ip_s_up;
			if(phase1_ip_s_up == NULL){
				phase1_ip_s_up = create_rule_node();
				temp_phase_node = phase1_ip_s_up;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_s_ip_up[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_s_ip_up[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_s_ip_up[i].eqid);
	}

	struct rule_node * count_eqid = phase1_ip_s_up;
	int eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("ip s up : eqid num: %d\n", eqid_num);
	phase1_ip_s_up_eqid = eqid_num;

	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	//ip s down
	for(int i = 0; i < num_entry_size; ++i){
		input_ip_s_down = table[i].s_ip & 0x0000ffff;

		if(table[i].s_len > 16){
			unsigned int down = input_ip_s_down >> (32 - table[i].s_len) << (32 - table[i].s_len);
			unsigned int up = input_ip_s_down >> (32 - table[i].s_len);
			for(unsigned int j = 0; j < (32 - table[i].s_len); ++j){
				up = up << 1;
				up +=1;
			}
			for(unsigned int j = down; j <= up; ++j){
				struct rule_leaf * temp = create_rule_leaf();
				temp -> rule_index = i;
				temp -> down_link = chunk_s_ip_down[j].leaf_link;
				chunk_s_ip_down[j].leaf_link = temp;
			}
		}
		else
		{
			for(unsigned int j = 0; j < 65536; ++j){
				struct rule_leaf * temp = create_rule_leaf();
				temp -> rule_index = i;
				temp -> down_link = chunk_s_ip_down[j].leaf_link;
				chunk_s_ip_down[j].leaf_link = temp;
			}
		}
	}

	temp_leaf = NULL;
	for(int i = 0; i< 65536; ++i){
		struct rule_node * temp_phase_node = phase1_ip_s_down;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_s_ip_down[i].leaf_link == NULL){
			chunk_s_ip_down[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_s_ip_down[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_s_ip_down[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_s_ip_down[i].eqid = quid_num;
				free_leaf_link(chunk_s_ip_down[i].leaf_link);
				chunk_s_ip_down[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase1_ip_s_down;
			if(phase1_ip_s_down == NULL){
				phase1_ip_s_down = create_rule_node();
				temp_phase_node = phase1_ip_s_down;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_s_ip_down[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_s_ip_down[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_s_ip_down[i].eqid);
	}

	count_eqid = phase1_ip_s_down;
	eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("ip s down : eqid num: %d\n", eqid_num);
	phase1_ip_s_down_eqid = eqid_num;
	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	//ip d up

	for(int i = 0; i < num_entry_size; ++i){
		input_ip_d_up = table[i].d_ip >> 16;

		if(table[i].d_len < 16){
			unsigned int down = input_ip_d_up >> (16 - table[i].d_len) << (16 - table[i].d_len);
			unsigned int up = input_ip_d_up >> (16 - table[i].d_len);
			for(unsigned int j = 0; j < (16 - table[i].d_len); ++j){
				up = up << 1;
				up +=1;
			}
			for(unsigned int j = down; j <= up; ++j){
				struct rule_leaf * temp = create_rule_leaf();
				temp -> rule_index = i;
				temp -> down_link = chunk_d_ip_up[j].leaf_link;
				chunk_d_ip_up[j].leaf_link = temp;
			}
		}
		else{
			struct rule_leaf * temp = create_rule_leaf();
			temp -> rule_index = i;
			temp -> down_link = chunk_d_ip_up[input_ip_d_up].leaf_link;
			chunk_d_ip_up[input_ip_d_up].leaf_link = temp;
		}
	}

	temp_leaf = NULL;
	for(int i = 0; i< 65536; ++i){
		struct rule_node * temp_phase_node = phase1_ip_d_up;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_d_ip_up[i].leaf_link == NULL){
			chunk_d_ip_up[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_d_ip_up[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_d_ip_up[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_d_ip_up[i].eqid = quid_num;
				free_leaf_link(chunk_d_ip_up[i].leaf_link);
				chunk_d_ip_up[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase1_ip_d_up;
			if(phase1_ip_d_up == NULL){
				phase1_ip_d_up = create_rule_node();
				temp_phase_node = phase1_ip_d_up;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_d_ip_up[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_d_ip_up[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_d_ip_up[i].eqid);
	}

	count_eqid = phase1_ip_d_up;
	eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("ip d up : eqid num: %d\n", eqid_num);
	phase1_ip_d_up_eqid = eqid_num;

	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	//ip d down

	for(int i = 0; i < num_entry_size; ++i){
		input_ip_d_down = table[i].d_ip & 0x0000ffff;

		if(table[i].d_len > 16){
			unsigned int down = input_ip_d_down >> (32 - table[i].d_len) << (32 - table[i].d_len);
			unsigned int up = input_ip_d_down >> (32 - table[i].d_len);
			for(unsigned int j = 0; j < (32 - table[i].d_len); ++j){
				up = up << 1;
				up +=1;
			}
			for(unsigned int j = down; j <= up; ++j){
				struct rule_leaf * temp = create_rule_leaf();
				temp -> rule_index = i;
				temp -> down_link = chunk_d_ip_down[j].leaf_link;
				chunk_d_ip_down[j].leaf_link = temp;
			}
		}
		else
		{
			for(unsigned int j = 0; j < 65536; ++j){
				struct rule_leaf * temp = create_rule_leaf();
				temp -> rule_index = i;
				temp -> down_link = chunk_d_ip_down[j].leaf_link;
				chunk_d_ip_down[j].leaf_link = temp;
			}
		}
	}

	temp_leaf = NULL;
	for(int i = 0; i< 65536; ++i){
		struct rule_node * temp_phase_node = phase1_ip_d_down;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_d_ip_down[i].leaf_link == NULL){
			chunk_d_ip_down[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_d_ip_down[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_d_ip_down[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_d_ip_down[i].eqid = quid_num;
				free_leaf_link(chunk_d_ip_down[i].leaf_link);
				chunk_d_ip_down[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase1_ip_d_down;
			if(phase1_ip_d_down == NULL){
				phase1_ip_d_down = create_rule_node();
				temp_phase_node = phase1_ip_d_down;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_d_ip_down[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_d_ip_down[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_d_ip_down[i].eqid);
	}

	count_eqid = phase1_ip_d_down;
	eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("ip d down : eqid num: %d\n", eqid_num);
	phase1_ip_d_down_eqid = eqid_num;

	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	//ip port s 

	for(int i = 0; i < num_entry_size; ++i){
		input_port_s_start = table[i].s_port_s;
		input_port_s_end = table[i].s_port_e;

		for(unsigned int j = input_port_s_start; j <= input_port_s_end; ++j){
			struct rule_leaf * temp = create_rule_leaf();
			temp -> rule_index = i;
			temp -> down_link = chunk_s_port[j].leaf_link;
			chunk_s_port[j].leaf_link = temp;
		}

	}

	temp_leaf = NULL;
	for(int i = 0; i< 65536; ++i){
		struct rule_node * temp_phase_node = phase1_port_s;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_s_port[i].leaf_link == NULL){
			chunk_s_port[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_s_port[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_s_port[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_s_port[i].eqid = quid_num;
				free_leaf_link(chunk_s_port[i].leaf_link);
				chunk_s_port[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase1_port_s;
			if(phase1_port_s == NULL){
				phase1_port_s = create_rule_node();
				temp_phase_node = phase1_port_s;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_s_port[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_s_port[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_s_port[i].eqid);
	}

	count_eqid = phase1_port_s;
	eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("port s : eqid num: %d\n", eqid_num);
	phase1_port_s_eqid = eqid_num;

	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	// ip port d 

	for(int i = 0; i < num_entry_size; ++i){
		input_port_d_start = table[i].d_port_s;
		input_port_d_end = table[i].d_port_e;

		for(unsigned int j = input_port_d_start; j <= input_port_d_end; ++j){
			struct rule_leaf * temp = create_rule_leaf();
			temp -> rule_index = i;
			temp -> down_link = chunk_d_port[j].leaf_link;
			chunk_d_port[j].leaf_link = temp;
		}
	}

	temp_leaf = NULL;
	for(int i = 0; i< 65536; ++i){
		struct rule_node * temp_phase_node = phase1_port_d;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_d_port[i].leaf_link == NULL){
			chunk_d_port[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_d_port[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_d_port[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_d_port[i].eqid = quid_num;
				free_leaf_link(chunk_d_port[i].leaf_link);
				chunk_d_port[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase1_port_d;
			if(phase1_port_d == NULL){
				phase1_port_d = create_rule_node();
				temp_phase_node = phase1_port_d;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_d_port[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_d_port[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_d_port[i].eqid);
	}

	count_eqid = phase1_port_d;
	eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("port d : eqid num: %d\n", eqid_num);
	phase1_port_d_eqid = eqid_num;

	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	//type

	for(int i = 0; i < num_entry_size; ++i){
		input_type = table[i].type;

		struct rule_leaf * temp = create_rule_leaf();
		temp -> rule_index = i;
		temp -> down_link = chunk_type[input_type].leaf_link;
		chunk_type[input_type].leaf_link = temp;
	}

	temp_leaf = NULL;
	for(int i = 0; i< 256; ++i){
		struct rule_node * temp_phase_node = phase1_type;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_type[i].leaf_link == NULL){
			chunk_type[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_type[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_type[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_type[i].eqid = quid_num;
				free_leaf_link(chunk_type[i].leaf_link);
				chunk_type[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase1_type;
			if(phase1_type == NULL){
				phase1_type = create_rule_node();
				temp_phase_node = phase1_type;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_type[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_type[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_type[i].eqid);
	}

	count_eqid = phase1_type;
	eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("protocol : eqid num: %d\n", eqid_num);
	phase1_type_eqid = eqid_num;


	printf("start recycle\n");

	phase1_ip_s_up_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase1_ip_s_up_eqid);
	phase1_ip_s_down_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase1_ip_s_down_eqid);
	phase1_ip_d_up_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase1_ip_d_up_eqid);
	phase1_ip_d_down_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase1_ip_d_down_eqid);
	phase1_port_s_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase1_port_s_eqid);
	phase1_port_d_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase1_port_d_eqid);
	phase1_type_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase1_type_eqid);

	rule_node_array_number += (phase1_ip_s_up_eqid + phase1_ip_s_down_eqid + phase1_ip_d_up_eqid + phase1_ip_d_down_eqid + phase1_port_s_eqid + phase1_port_d_eqid + phase1_type_eqid);
	//ip s up
	for(int i = 0; i < phase1_ip_s_up_eqid; ++i){
		phase1_ip_s_up_array[i].leaf_link = phase1_ip_s_up -> leaf_link;
		struct rule_node * temp = phase1_ip_s_up;
		phase1_ip_s_up = phase1_ip_s_up -> right_link;
		rule_node_number--;
		free(temp);
	}
	//ip s down
	for(int i = 0; i < phase1_ip_s_down_eqid; ++i){
		phase1_ip_s_down_array[i].leaf_link = phase1_ip_s_down -> leaf_link;
		struct rule_node * temp = phase1_ip_s_down;
		phase1_ip_s_down = phase1_ip_s_down -> right_link;
		rule_node_number--;
		free(temp);
	}
	//ip d up
	for(int i = 0; i < phase1_ip_d_up_eqid; ++i){
		phase1_ip_d_up_array[i].leaf_link = phase1_ip_d_up -> leaf_link;
		struct rule_node * temp = phase1_ip_d_up;
		phase1_ip_d_up = phase1_ip_d_up -> right_link;
		rule_node_number--;
		free(temp);
	}
	//ip d down
	for(int i = 0; i < phase1_ip_d_down_eqid; ++i){
		phase1_ip_d_down_array[i].leaf_link = phase1_ip_d_down -> leaf_link;
		struct rule_node * temp = phase1_ip_d_down;
		phase1_ip_d_down = phase1_ip_d_down -> right_link;
		rule_node_number--;
		free(temp);
	}
	//ip port s
	for(int i = 0; i < phase1_port_s_eqid; ++i){
		phase1_port_s_array[i].leaf_link = phase1_port_s -> leaf_link;
		struct rule_node * temp = phase1_port_s;
		phase1_port_s = phase1_port_s -> right_link;
		rule_node_number--;
		free(temp);
	}
	//ip port d
	for(int i = 0; i < phase1_port_d_eqid; ++i){
		phase1_port_d_array[i].leaf_link = phase1_port_d -> leaf_link;
		struct rule_node * temp = phase1_port_d;
		phase1_port_d = phase1_port_d -> right_link;
		rule_node_number--;
		free(temp);
	}
	//type
	for(int i = 0; i < phase1_type_eqid; ++i){
		phase1_type_array[i].leaf_link = phase1_type -> leaf_link;
		struct rule_node * temp = phase1_type;
		phase1_type = phase1_type -> right_link;
		rule_node_number--;
		free(temp);
	}
	printf("end phase 1\n");
	/////// phase 2

}



void create_RFC_phase2(){
	chunk_ip_s_phase2 = (struct chunk *)malloc(sizeof(struct chunk) * phase1_ip_s_up_eqid * phase1_ip_s_down_eqid);
	chunk_ip_d_phase2 = (struct chunk *)malloc(sizeof(struct chunk) * phase1_ip_d_up_eqid * phase1_ip_d_down_eqid);
	chunk_port_type_phase2 = (struct chunk *)malloc(sizeof(struct chunk) * phase1_port_s_eqid * phase1_port_d_eqid * phase1_type_eqid);

	chunk_node_number += (phase1_ip_s_up_eqid * phase1_ip_s_down_eqid + phase1_ip_d_up_eqid * phase1_ip_d_down_eqid + phase1_port_s_eqid * phase1_port_d_eqid * phase1_type_eqid);

	for(int i = 0; i < phase1_ip_s_up_eqid; ++i){
		for(int j = 0; j < phase1_ip_s_down_eqid; ++j){
			chunk_ip_s_phase2[i * phase1_ip_s_down_eqid + j].eqid = -1;
			chunk_ip_s_phase2[i * phase1_ip_s_down_eqid + j].leaf_link = NULL;
		}
	}
	for(int i = 0; i < phase1_ip_d_up_eqid; ++i){
		for(int j = 0; j < phase1_ip_d_down_eqid; ++j){
			chunk_ip_d_phase2[i * phase1_ip_d_down_eqid + j].eqid = -1;
			chunk_ip_d_phase2[i * phase1_ip_d_down_eqid + j].leaf_link = NULL;
		}
	}
	for(int i = 0; i < phase1_port_s_eqid; ++i){
		for(int j = 0; j < phase1_port_d_eqid; ++j){
			for(int k = 0; k < phase1_type_eqid; ++k){
				chunk_port_type_phase2[i * phase1_port_d_eqid * phase1_type_eqid + j * phase1_type_eqid + k].eqid = -1;
				chunk_port_type_phase2[i * phase1_port_d_eqid * phase1_type_eqid + j * phase1_type_eqid + k].leaf_link = NULL;
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//ip s 
	for(int i = 0; i < phase1_ip_s_up_eqid; ++i){
		for(int j = 0; j < phase1_ip_s_down_eqid; ++j){
			struct rule_leaf * temp_leaf_1 = phase1_ip_s_up_array[i].leaf_link;
			struct rule_leaf * temp_leaf_2 = phase1_ip_s_down_array[j].leaf_link;

			while(temp_leaf_1 != NULL && temp_leaf_2 != NULL){
				if(temp_leaf_1 -> rule_index == temp_leaf_2 -> rule_index){
					struct rule_leaf * temp = create_rule_leaf();
					temp -> rule_index = temp_leaf_1 -> rule_index;
					temp -> down_link = chunk_ip_s_phase2[i * phase1_ip_s_down_eqid + j].leaf_link;
					chunk_ip_s_phase2[i * phase1_ip_s_down_eqid + j].leaf_link = temp;

					temp_leaf_1 = temp_leaf_1 -> down_link;
					temp_leaf_2 = temp_leaf_2 -> down_link;
				}
				else if(temp_leaf_1 -> rule_index > temp_leaf_2 -> rule_index){
					temp_leaf_1 = temp_leaf_1 -> down_link;
				}
				else if(temp_leaf_1 -> rule_index < temp_leaf_2 -> rule_index){
					temp_leaf_2 = temp_leaf_2 -> down_link;
				}
			}
		}
	}

	struct rule_leaf * temp_leaf = NULL;
	for(int i = 0; i< phase1_ip_s_up_eqid * phase1_ip_s_down_eqid; ++i){
		struct rule_node * temp_phase_node = phase2_ip_s;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_ip_s_phase2[i].leaf_link == NULL){
			chunk_ip_s_phase2[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_ip_s_phase2[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_ip_s_phase2[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_ip_s_phase2[i].eqid = quid_num;
				free_leaf_link(chunk_ip_s_phase2[i].leaf_link);
				chunk_ip_s_phase2[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase2_ip_s;
			if(phase2_ip_s == NULL){
				phase2_ip_s = create_rule_node();
				temp_phase_node = phase2_ip_s;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_ip_s_phase2[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_ip_s_phase2[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_ip_s_phase2[i].eqid);
	}

	struct rule_node * count_eqid = phase2_ip_s;
	int eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("ip s : eqid num: %d\n", eqid_num);
	phase2_ip_s_eqid = eqid_num;


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ip d

	for(int i = 0; i < phase1_ip_d_up_eqid; ++i){
		for(int j = 0; j < phase1_ip_d_down_eqid; ++j){
			struct rule_leaf * temp_leaf_1 = phase1_ip_d_up_array[i].leaf_link;
			struct rule_leaf * temp_leaf_2 = phase1_ip_d_down_array[j].leaf_link;

			while(temp_leaf_1 != NULL && temp_leaf_2 != NULL){
				if(temp_leaf_1 -> rule_index == temp_leaf_2 -> rule_index){
					struct rule_leaf * temp = create_rule_leaf();
					temp -> rule_index = temp_leaf_1 -> rule_index;
					temp -> down_link = chunk_ip_d_phase2[i * phase1_ip_d_down_eqid + j].leaf_link;
					chunk_ip_d_phase2[i * phase1_ip_d_down_eqid + j].leaf_link = temp;

					temp_leaf_1 = temp_leaf_1 -> down_link;
					temp_leaf_2 = temp_leaf_2 -> down_link;
				}
				else if(temp_leaf_1 -> rule_index > temp_leaf_2 -> rule_index){
					temp_leaf_1 = temp_leaf_1 -> down_link;
				}
				else if(temp_leaf_1 -> rule_index < temp_leaf_2 -> rule_index){
					temp_leaf_2 = temp_leaf_2 -> down_link;
				}
			}
		}
	}

	temp_leaf = NULL;
	for(int i = 0; i< phase1_ip_d_up_eqid * phase1_ip_d_down_eqid; ++i){
		struct rule_node * temp_phase_node = phase2_ip_d;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_ip_d_phase2[i].leaf_link == NULL){
			chunk_ip_d_phase2[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_ip_d_phase2[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_ip_d_phase2[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_ip_d_phase2[i].eqid = quid_num;
				free_leaf_link(chunk_ip_d_phase2[i].leaf_link);
				chunk_ip_d_phase2[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase2_ip_d;
			if(phase2_ip_d == NULL){
				phase2_ip_d = create_rule_node();
				temp_phase_node = phase2_ip_d;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_ip_d_phase2[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_ip_d_phase2[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_ip_d_phase2[i].eqid);
	}

	count_eqid = phase2_ip_d;
	eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("ip d : eqid num: %d\n", eqid_num);
	phase2_ip_d_eqid = eqid_num;

	//printf("build chunk port_type phase2\n");
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//port type
	for(int i = 0; i < phase1_port_s_eqid; ++i){
		for(int j = 0; j < phase1_port_d_eqid; ++j){
			for(int k = 0; k < phase1_type_eqid; ++k){
				struct rule_leaf * temp_leaf_1 = phase1_port_s_array[i].leaf_link;
				struct rule_leaf * temp_leaf_2 = phase1_port_d_array[j].leaf_link;
				struct rule_leaf * temp_leaf_3 = phase1_type_array[k].leaf_link;

				while(temp_leaf_1 != NULL && temp_leaf_2 != NULL && temp_leaf_3 != NULL){
					if(temp_leaf_1 -> rule_index == temp_leaf_2 -> rule_index && temp_leaf_1 -> rule_index == temp_leaf_3 -> rule_index ){
						struct rule_leaf * temp = create_rule_leaf();
						temp -> rule_index = temp_leaf_1 -> rule_index;
						temp -> down_link = chunk_port_type_phase2[i * phase1_port_d_eqid * phase1_type_eqid + j * phase1_type_eqid + k].leaf_link;
						chunk_port_type_phase2[i * phase1_port_d_eqid * phase1_type_eqid + j * phase1_type_eqid + k].leaf_link = temp;

						temp_leaf_1 = temp_leaf_1 -> down_link;
						temp_leaf_2 = temp_leaf_2 -> down_link;
						temp_leaf_3 = temp_leaf_3 -> down_link;
					}
					else if(temp_leaf_1 -> rule_index > temp_leaf_2 -> rule_index && temp_leaf_1 -> rule_index > temp_leaf_3 -> rule_index){
						temp_leaf_1 = temp_leaf_1 -> down_link;
					}
					else if(temp_leaf_2 -> rule_index > temp_leaf_1 -> rule_index && temp_leaf_2 -> rule_index > temp_leaf_3 -> rule_index){
						temp_leaf_2 = temp_leaf_2 -> down_link;
					}
					else if(temp_leaf_3 -> rule_index > temp_leaf_1 -> rule_index && temp_leaf_3 -> rule_index > temp_leaf_2 -> rule_index){
						temp_leaf_3 = temp_leaf_3 -> down_link;
					}
					else if(temp_leaf_1 -> rule_index == temp_leaf_2 -> rule_index && temp_leaf_1 -> rule_index > temp_leaf_3 -> rule_index){
						temp_leaf_1 = temp_leaf_1 -> down_link;
						temp_leaf_2 = temp_leaf_2 -> down_link;
					}
					else if(temp_leaf_1 -> rule_index == temp_leaf_3 -> rule_index && temp_leaf_1 -> rule_index > temp_leaf_2 -> rule_index){
						temp_leaf_1 = temp_leaf_1 -> down_link;
						temp_leaf_3 = temp_leaf_3 -> down_link;
					}
					else if(temp_leaf_2 -> rule_index == temp_leaf_3 -> rule_index && temp_leaf_2 -> rule_index > temp_leaf_1 -> rule_index){
						temp_leaf_2 = temp_leaf_2 -> down_link;
						temp_leaf_3 = temp_leaf_3 -> down_link;
					}
					else{
						printf("error at phase2\n");
					}
				}
			}
		}
	}

	temp_leaf = NULL;
	for(int i = 0; i < phase1_port_s_eqid * phase1_port_d_eqid * phase1_type_eqid; ++i){
		struct rule_node * temp_phase_node = phase2_port_type;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_port_type_phase2[i].leaf_link == NULL){
			chunk_port_type_phase2[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_port_type_phase2[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_port_type_phase2[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_port_type_phase2[i].eqid = quid_num;
				free_leaf_link(chunk_port_type_phase2[i].leaf_link);
				chunk_port_type_phase2[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase2_port_type;
			if(phase2_port_type == NULL){
				phase2_port_type = create_rule_node();
				temp_phase_node = phase2_port_type;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_port_type_phase2[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_port_type_phase2[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_port_type_phase2[i].eqid);
	}

	count_eqid = phase2_port_type;
	eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("port type : eqid num: %d\n", eqid_num);
	phase2_port_type_eqid = eqid_num;


	//recycle link list

	printf("start recycle\n");

	phase2_ip_s_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase2_ip_s_eqid);
	phase2_ip_d_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase2_ip_d_eqid);
	phase2_port_type_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase2_port_type_eqid);

	rule_node_array_number += (phase2_ip_s_eqid + phase2_ip_d_eqid + phase2_port_type_eqid);

	//ip s
	for(int i = 0; i < phase2_ip_s_eqid; ++i){
		phase2_ip_s_array[i].leaf_link = phase2_ip_s -> leaf_link;
		struct rule_node * temp = phase2_ip_s;
		phase2_ip_s = phase2_ip_s -> right_link;
		rule_node_number--;
		free(temp);
	}

	//ip d
	for(int i = 0; i < phase2_ip_d_eqid; ++i){
		phase2_ip_d_array[i].leaf_link = phase2_ip_d -> leaf_link;
		struct rule_node * temp = phase2_ip_d;
		phase2_ip_d = phase2_ip_d -> right_link;
		rule_node_number--;
		free(temp);
	}
	//port & type

	for(int i = 0; i < phase2_port_type_eqid; ++i){
		phase2_port_type_array[i].leaf_link = phase2_port_type -> leaf_link;
		struct rule_node * temp = phase2_port_type;
		phase2_port_type = phase2_port_type -> right_link;
		rule_node_number--;
		free(temp);
	}
	/*
	for(int i = 0; i < phase1_ip_s_up_eqid * phase1_ip_s_down_eqid; ++i){
		printf("i : %d,  eqid : %d\n", i, chunk_ip_s_phase2[i].eqid);
	}
	*/
	/*
	for(int i = 0; i < phase2_port_type_eqid; ++i){
		struct rule_leaf * temp = phase2_port_type_array[i].leaf_link;
		int temp_flag = 0;
		while(temp != NULL){
			printf("%d ", temp -> rule_index);
			temp = temp -> down_link;
			temp_flag = 1;
		}
		if(temp_flag == 1)
			printf("----- i : %d\n", i);
	}
	*/
	printf("end phase2\n");

}

void create_RFC_phase3(){
	chunk_all_phase3 = (struct chunk *)malloc(sizeof(struct chunk) * phase2_ip_s_eqid * phase2_ip_d_eqid * phase2_port_type_eqid);
	chunk_node_number += (phase2_ip_s_eqid * phase2_ip_d_eqid * phase2_port_type_eqid);

	for(int i = 0; i < phase2_ip_s_eqid; ++i){
		for(int j = 0; j < phase2_ip_d_eqid; ++j){
			for(int k = 0; k < phase2_port_type_eqid; ++k){
				chunk_all_phase3[i * phase2_ip_d_eqid * phase2_port_type_eqid + j * phase2_port_type_eqid + k].eqid = -1;
				chunk_all_phase3[i * phase2_ip_d_eqid * phase2_port_type_eqid + j * phase2_port_type_eqid + k].leaf_link = NULL;
			}
		}
	}
	
	printf("build chunk for phase3 \n");
	printf("ip s eqid %d, ip d eqid %d, port type eqid %d\n", phase2_ip_s_eqid, phase2_ip_d_eqid, phase2_port_type_eqid);

	for(int i = 0; i < phase2_ip_s_eqid; ++i){
		for(int j = 0; j < phase2_ip_d_eqid; ++j){
			for(int k = 0; k < phase2_port_type_eqid; ++k){
				struct rule_leaf * temp_leaf_1 = phase2_ip_s_array[i].leaf_link;
				struct rule_leaf * temp_leaf_2 = phase2_ip_d_array[j].leaf_link;
				struct rule_leaf * temp_leaf_3 = phase2_port_type_array[k].leaf_link;

				while(temp_leaf_1 != NULL && temp_leaf_2 != NULL && temp_leaf_3 != NULL){
					if(temp_leaf_1 -> rule_index == temp_leaf_2 -> rule_index && temp_leaf_1 -> rule_index == temp_leaf_3 -> rule_index ){
						struct rule_leaf * temp = create_rule_leaf();
						temp -> rule_index = temp_leaf_1 -> rule_index;
						temp -> down_link = chunk_all_phase3[i * phase2_ip_d_eqid * phase2_port_type_eqid + j * phase2_port_type_eqid + k].leaf_link;
						chunk_all_phase3[i * phase2_ip_d_eqid * phase2_port_type_eqid + j * phase2_port_type_eqid + k].leaf_link = temp;

						temp_leaf_1 = temp_leaf_1 -> down_link;
						temp_leaf_2 = temp_leaf_2 -> down_link;
						temp_leaf_3 = temp_leaf_3 -> down_link;
					}
					else if(temp_leaf_1 -> rule_index < temp_leaf_2 -> rule_index && temp_leaf_1 -> rule_index < temp_leaf_3 -> rule_index){
						temp_leaf_1 = temp_leaf_1 -> down_link;
					}
					else if(temp_leaf_2 -> rule_index < temp_leaf_1 -> rule_index && temp_leaf_2 -> rule_index < temp_leaf_3 -> rule_index){
						temp_leaf_2 = temp_leaf_2 -> down_link;
					}
					else if(temp_leaf_3 -> rule_index < temp_leaf_1 -> rule_index && temp_leaf_3 -> rule_index < temp_leaf_2 -> rule_index){
						temp_leaf_3 = temp_leaf_3 -> down_link;
					}
					else if(temp_leaf_1 -> rule_index == temp_leaf_2 -> rule_index && temp_leaf_1 -> rule_index < temp_leaf_3 -> rule_index){
						temp_leaf_1 = temp_leaf_1 -> down_link;
						temp_leaf_2 = temp_leaf_2 -> down_link;
					}
					else if(temp_leaf_1 -> rule_index == temp_leaf_3 -> rule_index && temp_leaf_1 -> rule_index < temp_leaf_2 -> rule_index){
						temp_leaf_1 = temp_leaf_1 -> down_link;
						temp_leaf_3 = temp_leaf_3 -> down_link;
					}
					else if(temp_leaf_2 -> rule_index == temp_leaf_3 -> rule_index && temp_leaf_2 -> rule_index < temp_leaf_1 -> rule_index){
						temp_leaf_2 = temp_leaf_2 -> down_link;
						temp_leaf_3 = temp_leaf_3 -> down_link;
					}
					else{
						printf("error at phase3\n");
					}
				}
			}
		}
	}
	/*
	for(int i = 0; i < phase2_ip_s_eqid * phase2_ip_d_eqid * phase2_port_type_eqid; ++i){
		struct rule_leaf * temp = chunk_all_phase3[i].leaf_link;
		int temp_flag = 0;
		while(temp != NULL){
			printf("%d ", temp -> rule_index);
			temp = temp -> down_link;
			temp_flag = 1;
		}
		if(temp_flag == 1)
			printf("----- i : %d\n", i);
	}
	*/
	printf("start build eqid array phase 3\n");

	struct rule_leaf * temp_leaf = NULL;
	for(int i = 0; i <  phase2_ip_s_eqid * phase2_ip_d_eqid * phase2_port_type_eqid; ++i){
		struct rule_node * temp_phase_node = phase3_all;
		int quid_num = 0;
		int same_flag = 0;

		if(chunk_all_phase3[i].leaf_link == NULL){
			chunk_all_phase3[i].eqid = -1;
			//printf("i : %d, eqid : %d\n", i, chunk_all_phase3[i].eqid);
			continue;
		}

		while(temp_phase_node != NULL){
			temp_leaf = chunk_all_phase3[i].leaf_link;
			struct rule_leaf * temp = temp_phase_node -> leaf_link;
			

			while(temp != NULL && temp_leaf != NULL){
				if(temp -> rule_index == temp_leaf -> rule_index){
					temp = temp -> down_link;
					temp_leaf = temp_leaf -> down_link;
					same_flag = 1;
				}
				else
				{
					same_flag = 0;
					break;
				}
				if((temp == NULL && temp_leaf != NULL) || (temp != NULL && temp_leaf == NULL)){
					same_flag = 0;
					break;
				}
			}
			if(same_flag == 1){
				chunk_all_phase3[i].eqid = quid_num;
				free_leaf_link(chunk_all_phase3[i].leaf_link);
				chunk_all_phase3[i].leaf_link = temp_phase_node -> leaf_link;
				break;
			}
			quid_num++;
			temp_phase_node = temp_phase_node -> right_link;
		}
		if(same_flag == 0){
			temp_phase_node = phase3_all;
			if(phase3_all == NULL){
				phase3_all = create_rule_node();
				temp_phase_node = phase3_all;
			}
			else
			{
				while(temp_phase_node -> right_link != NULL){
					temp_phase_node = temp_phase_node -> right_link;
				}
				temp_phase_node -> right_link = create_rule_node();
				temp_phase_node = temp_phase_node -> right_link;
			}

			temp_leaf = chunk_all_phase3[i].leaf_link;
			temp_phase_node -> leaf_link =  temp_leaf;
			chunk_all_phase3[i].eqid = quid_num;
		}
		//printf("i : %d, eqid : %d\n", i, chunk_all_phase3[i].eqid);
	}

	struct rule_node * count_eqid = phase3_all;
	int eqid_num = 0;
	while(count_eqid != NULL){
		eqid_num ++;
		count_eqid = count_eqid -> right_link;
	}
	printf("all : eqid num: %d\n", eqid_num);
	phase3_all_eqid = eqid_num;


	//recycle list
	phase3_all_array = (struct rule_node_array *)malloc(sizeof(struct rule_node_array) * phase3_all_eqid);
	rule_node_array_number += (phase3_all_eqid);
	//ip s
	for(int i = 0; i < phase3_all_eqid; ++i){
		phase3_all_array[i].leaf_link = phase3_all -> leaf_link;
		struct rule_node * temp = phase3_all;
		phase3_all = phase3_all -> right_link;
		rule_node_number--;
		free(temp);
	}


	printf("end phase 3\n");
}

////////////////////////////////////////////////////////////////////////////////////

void RCF_search(unsigned int input_ip_s, unsigned int input_ip_d, unsigned int input_port_s, unsigned int input_port_d, unsigned int input_type){
	unsigned int input_ip_s_up, input_ip_s_down, input_ip_d_up, input_ip_d_down;

	input_ip_s_up = input_ip_s >> 16;
	input_ip_s_down = input_ip_s & 0x0000ffff;
	input_ip_d_up = input_ip_d >> 16;
	input_ip_d_down = input_ip_d & 0x0000ffff;

	int search_s_up_eqid = chunk_s_ip_up[input_ip_s_up].eqid;
	int search_s_down_eqid = chunk_s_ip_down[input_ip_s_down].eqid;
	int search_d_up_eqid = chunk_d_ip_up[input_ip_d_up].eqid;
	int search_d_down_eqid = chunk_d_ip_down[input_ip_d_down].eqid;
	int search_port_s_eqid = chunk_s_port[input_port_s].eqid;
	int search_port_d_eqid = chunk_d_port[input_port_d].eqid;
	int search_type_eqid = chunk_type[input_type].eqid;

	if(search_s_up_eqid == -1 || search_s_down_eqid == -1 || search_d_up_eqid == -1 || search_d_down_eqid == -1 || search_port_s_eqid == -1 || search_port_d_eqid == -1 || search_type_eqid == -1){
		loss_case++;
		return;
		printf("-1 occur at phase 1\n");
		printf("search: s_up_eqid : %d, s_down_eqid : %d, d_up_eqid : %d, d_down_eqid : %d, port_s eqid : %d,  port_d eqid : %d,  type eqid : %d\n", search_s_up_eqid, search_s_down_eqid, search_d_up_eqid, search_d_down_eqid, search_port_s_eqid, search_port_d_eqid, search_type_eqid);

	}
	int search_s_eqid = chunk_ip_s_phase2[search_s_up_eqid * phase1_ip_s_down_eqid + search_s_down_eqid].eqid;
	int search_d_eqid = chunk_ip_d_phase2[search_d_up_eqid * phase1_ip_d_down_eqid + search_d_down_eqid].eqid;
	int search_port_type_eqid = chunk_port_type_phase2[search_port_s_eqid * phase1_port_d_eqid * phase1_type_eqid + search_port_d_eqid * phase1_type_eqid + search_type_eqid].eqid;

	if(search_s_eqid == -1 || search_d_eqid == -1 || search_port_type_eqid == -1){
		loss_case++;
		return;
		printf("----------------------------------------------------------------------\n");
		printf("-1 occur at phase 2\n");
		printf("search: s_eqid : %d, d_eqid : %d, port_type_eqid : %d\n", search_s_eqid, search_d_eqid, search_port_type_eqid);
		printf("search: s_up_eqid : %d, s_down_eqid : %d, d_up_eqid : %d, d_down_eqid : %d, port_s eqid : %d,  port_d eqid : %d,  type eqid : %d\n", search_s_up_eqid, search_s_down_eqid, search_d_up_eqid, search_d_down_eqid, search_port_s_eqid, search_port_d_eqid, search_type_eqid);
		printf("ip s : %u, ip d : %u, port s : %u, port d : %u, type : %u\n", input_ip_s, input_ip_d, input_port_s, input_port_d, input_type);
		/*
		struct rule_leaf * temp = phase1_port_s_array[search_port_s_eqid].leaf_link;
		int temp_flag = 0;
		while(temp != NULL){
			printf("%d ", temp -> rule_index);
			temp = temp -> down_link;
			temp_flag = 1;
		}
		if(temp_flag == 1)
			printf("-----\n");
		////

		temp = phase1_port_d_array[search_port_d_eqid].leaf_link;
		temp_flag = 0;
		while(temp != NULL){
			printf("%d ", temp -> rule_index);
			temp = temp -> down_link;
			temp_flag = 1;
		}
		if(temp_flag == 1)
			printf("-----\n");

		////

		temp = phase1_type_array[search_type_eqid].leaf_link;
		temp_flag = 0;
		while(temp != NULL){
			printf("%d ", temp -> rule_index);
			temp = temp -> down_link;
			temp_flag = 1;
		}
		if(temp_flag == 1)
			printf("-----\n");

		printf("----------------------------------------------------------------------\n");
		*/
	}

	int search_all = chunk_all_phase3[search_s_eqid * phase2_ip_d_eqid * phase2_port_type_eqid + search_d_eqid * phase2_port_type_eqid + search_port_type_eqid].eqid;

	if(search_all == -1){
		loss_case++;
		return;
		/*
		printf("----------------------------------------------------------------------\n");
		printf("error occur at phase 3\n");
		printf("search: all : %d\n", search_all);
		printf("search: s_eqid : %d, d_eqid : %d, port_type_eqid : %d\n", search_s_eqid, search_d_eqid, search_port_type_eqid);
		printf("search: s_up_eqid : %d, s_down_eqid : %d, d_up_eqid : %d, d_down_eqid : %d, port_s eqid : %d,  port_d eqid : %d,  type eqid : %d\n", search_s_up_eqid, search_s_down_eqid, search_d_up_eqid, search_d_down_eqid, search_port_s_eqid, search_port_d_eqid, search_type_eqid);
		printf("ip s : %u, ip d : %u, port s : %u, port d : %u, type : %u\n", input_ip_s, input_ip_d, input_port_s, input_port_d, input_type);
		*/
	/*
		struct rule_leaf * temp = phase1_ip_s_down_array[search_s_down_eqid].leaf_link;
		int temp_flag = 0;
		while(temp != NULL){
			printf("%d ", temp -> rule_index);
			temp = temp -> down_link;
			temp_flag = 1;
		}
		if(temp_flag == 1)
			printf("-----\n");
	*/
		printf("----------------------------------------------------------------------\n");
	}

	if(phase3_all_array[search_all].leaf_link != NULL){
		match_case++;
	}
	else
	{
		loss_case++;
	}
	
	
}

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

	printf("start create\n");
	create_RFC();
	create_RFC_phase2();
	create_RFC_phase3();

	//////////////
	/*
	int prefix_array[33] = {0};
	for(i = 0; i < num_entry; ++i){
		prefix_array[table[i].len]++;
	}
	for(i = 0; i < 33; ++i){
		printf("%u\n", prefix_array[i]);
	}
	*/
	//////////////
	
	//printf("number of nodes: %d\n",num_node);
	printf("num entry : %d\n", num_entry);
	printf("Total memory requirement: %d KB\n",((sizeof(struct chunk) * (65536 * 6 + 256 + chunk_node_number) + sizeof(struct rule_node) * rule_node_number + sizeof(struct rule_leaf) * rule_leaf_number + sizeof(struct rule_node_array) * rule_node_array_number)/1024));

	shuffle(query, num_query); 
	printf("start search\n");
	////////////////////////////////////////////////////////////////////////////
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			begin=rdtsc();
			RCF_search(query[i].s_ip, query[i].d_ip, query[i].s_port_s, query[i].d_port_s, query[i].type);
			end=rdtsc();
			if(clock[i]>(end-begin))
				clock[i]=(end-begin);
		}
	}
	total=0;
	for(j=0;j<num_query;j++)
		total+=clock[j];
	printf("Avg. Search: %llu\n",total/num_query);
	CountClock();

	printf("match case : %d, loss case : %d\n", match_case, loss_case);
	////////////////////////////////////////////////////////////////////////////
	//count_node(root);
	//printf("There are %d nodes in binary trie\n",N);
	return 0;
}
