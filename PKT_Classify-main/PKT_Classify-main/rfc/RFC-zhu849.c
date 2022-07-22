#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

#define CHUNK0 16
#define CHUNK1 16
#define CHUNK2 16
#define CHUNK3 16
#define CHUNK4 16
#define CHUNK5 16
#define CHUNK6 8

////////////////////////////////////////////////////////////////////////////////////

static __inline__ unsigned long long rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}
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
struct bucket_node {
	int bucketSize;
	int *index_bucket;
	int eqID;
};
typedef struct bucket_node node;
////////////////////////////////////////////////////////////////////////////////////
struct list_node {
	struct list_node *next;
	int eqID;
	int index_size;
	int *bucket;
};
////////////////////////////////////////////////////////////////////////////////////
struct list_node* create_listnode() {
	struct list_node *temp = malloc(sizeof(struct list_node));
	temp->bucket = NULL;
	temp->index_size = 0;
	temp->next = NULL;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
unsigned long long int begin, end, total = 0;
unsigned long long int *clock;
struct list_node *eqID_array[7];
struct list_node* *simple_storage[7];
int eqID_size[7] = { 0 };
struct list_node *eqID_array_second[3];
struct list_node* *simple_storage_second[3];
int eqID_size_second[3] = { 0 };
struct list_node *eqID_array_third[1];
struct list_node* *simple_storage_third[1];
int eqID_size_third[1] = { 0 };
int num_pointer = 0;
int num_element = 0;
////////////////////////////////////////////////////////////////////////////////////
node* intersection(int *a, int a_size, int *b, int b_size) {
	int i, j;
	int bucketSize = 0;
	int *isbucket;
	node *temp = malloc(sizeof(node));
	//count array size
	for (i = 0; i < a_size; i++) {
		for (j = 0; j < b_size; j++) {
			if (a[i] == b[j])
				bucketSize++;
		}
	}
	//malloc memory space
	isbucket = malloc(sizeof(int) * bucketSize);
	//assign a new array
	bucketSize = 0;
	for (i = 0; i < a_size; i++) {
		for (j = 0; j < b_size; j++) {
			if (a[i] == b[j])
				isbucket[bucketSize++] = a[i];
		}
	}
	temp->bucketSize = bucketSize;
	temp->index_bucket = isbucket;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned int src_ip, unsigned int des_ip, unsigned int src_port, unsigned int des_port, unsigned int protocol) {
	int i;
	unsigned int index[7];
	int eqID[7];
	struct list_node* target = NULL;
	index[0] = (src_ip & 0xFFFF0000) >> CHUNK0;
	index[1] = src_ip & 0x0000FFFF;
	index[2] = (des_ip & 0xFFFF0000) >> CHUNK2;
	index[3] = des_ip & 0x0000FFFF;
	index[4] = src_port;
	index[5] = des_port;
	index[6] = protocol;

	for (i = 0; i < 7; i++)
		eqID[i] = simple_storage[i][index[i]]->eqID;

	index[0] = eqID[0] * eqID_size[1] + eqID[1];
	index[1] = eqID[2] * eqID_size[3] + eqID[3];
	index[2] = eqID[4] * eqID_size[5] * eqID_size[6] + eqID[5] * eqID_size[6] + eqID[6];
	for (i = 0; i < 3; i++)
		eqID[i] = simple_storage_second[i][index[i]]->eqID;

	index[0] = eqID[0] * eqID_size_second[1] * eqID_size_second[2] + eqID[1] * eqID_size_second[2] + eqID[2];
	target = simple_storage_third[0][index[0]];
	/*
	if (target == NULL)
		printf("********************** ERROR *******************");
	else {
		for (i = 0; i < target->index_size; i++)
			printf("R%d", target->bucket[i]);
		printf("****************\n\n");
	}
	*/
}
////////////////////////////////////////////////////////////////////////////////////
void parse_first() {
	int i, j, k;
	int counter;
	struct list_node *count_p, *pointer_eqID;
	int index, count;
	node *temp_storage;
	node *temp, *tempB;
	struct list_node *pa, *pb, *pc;
	int arraySize;
	//count size
	for (i = 0; i < 3; i++) {
		counter = 0;
		count_p = eqID_array_second[i];
		while (count_p != NULL) {
			count_p = count_p->next;
			counter++;
		}
		eqID_size_second[i] = counter;
	}

	temp_storage = malloc(sizeof(node) * eqID_size_second[0] * eqID_size_second[1] * eqID_size_second[2]);
	//set phase 2 CHUNK0
	i = j = k = 0;

	for (i = 0, pa = eqID_array_second[0]; i < eqID_size_second[0]; i++, pa = pa->next) {
		for (j = 0, pb = eqID_array_second[1]; j < eqID_size_second[1]; j++, pb = pb->next) {
			tempB = intersection(pa->bucket, pa->index_size, pb->bucket, pb->index_size);
			for (k = 0, pc = eqID_array_second[2]; k < eqID_size_second[2]; k++, pc = pc->next) {
				index = i * eqID_size_second[1] * eqID_size_second[2] + j * eqID_size_second[2] + k;
				temp = intersection(tempB->index_bucket, tempB->bucketSize, pc->bucket, pc->index_size);
				temp_storage[index] = *temp;
				free(temp);
			}
			free(tempB);
		}
	}

	arraySize = eqID_size_second[0] * eqID_size_second[1] * eqID_size_second[2];
	simple_storage_third[0] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	//assign eqID
	count = 0;
	eqID_array_third[0] = create_listnode();
	eqID_array_third[0]->index_size = temp_storage[0].bucketSize;
	eqID_array_third[0]->bucket = temp_storage[0].index_bucket;
	eqID_array_third[0]->eqID = count++;
	simple_storage_third[0][0] = eqID_array_third[0];

	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array_third[0];

		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage_third[0][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}

		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array_third[0];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage_third[0][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);
	/*
	for (j = 0; j < arraySize; j++)
		printf("storage_third[0][%d].eqID = %d \n", j, simple_storage_third[0][j]->eqID);
	printf("**********************************************************************\n");
	*/

	counter = 0;
	count_p = eqID_array_third[i];
	while (count_p != NULL) {
		count_p = count_p->next;
		counter++;
	}
	eqID_size_third[0] = counter;
	
}
////////////////////////////////////////////////////////////////////////////////////
void parse_zero() {
	int i, j, k;
	int counter;
	struct list_node *count_p, *pointer_eqID;
	int index, count;
	node *temp_storage;
	node *temp, *tempB;
	struct list_node *pa, *pb, *pc;
	int arraySize;
	//count size
	for (i = 0; i < 7; i++) {
		counter = 0;
		count_p = eqID_array[i];
		while (count_p != NULL) {
			count_p = count_p->next;
			counter++;
		}
		eqID_size[i] = counter;
	}

	//**************************************deal with src_ip*********************************************
	temp_storage = malloc(sizeof(node) * eqID_size[0] * eqID_size[1]);
	//set phase 1 CHUNK
	i = j = 0;
	
	for (i = 0, pa = eqID_array[0]; i < eqID_size[0]; i++, pa = pa->next) {
		for (j = 0, pb = eqID_array[1]; j < eqID_size[1]; j++, pb = pb->next) {
			index = i * eqID_size[1] + j;
			temp = intersection(pa->bucket, pa->index_size, pb->bucket, pb->index_size);
			temp_storage[index] = *temp;
			free(temp);
		}
	}

	arraySize = eqID_size[0] * eqID_size[1];
	simple_storage_second[0] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	//assign eqID
	count = 0;
	eqID_array_second[0] = create_listnode();
	eqID_array_second[0]->index_size = temp_storage[0].bucketSize;
	eqID_array_second[0]->bucket = temp_storage[0].index_bucket;
	eqID_array_second[0]->eqID = count++;
	simple_storage_second[0][0] = eqID_array_second[0];

	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array_second[0];

		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage_second[0][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}

		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array_second[0];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage_second[0][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);
	/*
	for(j= 0;j<arraySize;j++)
		printf("storage_second[0][%d].eqID = %d \n", j, simple_storage_second[0][j]->eqID);
	printf("**********************************************************************\n");
	*/
	//**************************************deal with des_ip*********************************************
	temp_storage = malloc(sizeof(node) * eqID_size[2] * eqID_size[3]);
	//set phase 1 CHUNK2
	i = j = 0;
	
	for (i = 0, pa = eqID_array[2]; i < eqID_size[2]; i++, pa = pa->next) {
		for (j = 0, pb = eqID_array[3]; j < eqID_size[3]; j++, pb = pb->next) {
			index = i * eqID_size[3] + j;
			temp = intersection(pa->bucket, pa->index_size, pb->bucket, pb->index_size);
			temp_storage[index] = *temp;
			free(temp);
		}
	}

	arraySize = eqID_size[2] * eqID_size[3];
	simple_storage_second[1] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	//assign eqID
	count = 0;
	eqID_array_second[1] = create_listnode();
	eqID_array_second[1]->index_size = temp_storage[0].bucketSize;
	eqID_array_second[1]->bucket = temp_storage[0].index_bucket;
	eqID_array_second[1]->eqID = count++;
	simple_storage_second[1][0] = eqID_array_second[1];

	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array_second[1];

		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage_second[1][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}

		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array_second[1];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage_second[1][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);
	/*
	for (j = 0; j < arraySize; j++)
		printf("storage_second[1][%d].eqID = %d \n", j, simple_storage_second[1][j]->eqID);
	printf("**********************************************************************\n");
	*/
	//**************************************deal with port & protocol *********************************************
	temp_storage = malloc(sizeof(node) * eqID_size[4] * eqID_size[5] * eqID_size[6]);
	//set phase 1 CHUNK2
	i = j = k = 0;
	
	for (i = 0, pa = eqID_array[4]; i < eqID_size[4]; i++, pa = pa->next) {
		for (j = 0, pb = eqID_array[5]; j < eqID_size[5]; j++, pb = pb->next) {
			tempB = intersection(pa->bucket, pa->index_size, pb->bucket, pb->index_size);
			for (k = 0, pc = eqID_array[6]; k < eqID_size[6]; k++, pc = pc->next) {
				index = i * eqID_size[5] * eqID_size[6] + j * eqID_size[6] + k;
				temp = intersection(tempB->index_bucket, tempB->bucketSize, pc->bucket, pc->index_size);
				temp_storage[index] = *temp;
				free(temp);
			}
		}
	}

	arraySize = eqID_size[4] * eqID_size[5] * eqID_size[6];
	simple_storage_second[2] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	//assign eqID
	count = 0;
	eqID_array_second[2] = create_listnode();
	eqID_array_second[2]->index_size = temp_storage[0].bucketSize;
	eqID_array_second[2]->bucket = temp_storage[0].index_bucket;
	eqID_array_second[2]->eqID = count++;
	simple_storage_second[2][0] = eqID_array_second[2];

	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array_second[2];

		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage_second[2][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}

		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array_second[2];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage_second[2][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);
	/*
	for (j = 0; j < arraySize; j++)
		printf("storage_second[2][%d].eqID = %d \n", j, simple_storage_second[2][j]->eqID);
	printf("**********************************************************************\n");
	*/
}
////////////////////////////////////////////////////////////////////////////////////
void create_phase_zero() {
	int i, j;
	struct list_node *pointer_eqID;
	int arraySize;
	int count;
	unsigned int s_index, e_index;
	unsigned int ip_upper, ip_lower;
	node *temp_storage;
	node *temp = NULL;

	//**************************************************deal with src_ip_upper**********************************************************
	arraySize = pow(2, CHUNK0);
	temp_storage = malloc(sizeof(node) * arraySize);
	simple_storage[0] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	for (j = 0; j < arraySize; j++) {
		temp_storage[j].bucketSize = 0;
		temp_storage[j].eqID = -1;
		temp_storage[j].index_bucket = NULL;
		simple_storage[0][j] = NULL;
	}

	//count array size
	for (i = 0; i < num_entry; i++) {
		ip_upper = (table[i].src_ip & (0xFFFF0000)) >> CHUNK0;
		if (table[i].src_len >= CHUNK0)
			temp_storage[ip_upper].bucketSize++;
		else {
			s_index = ip_upper & (0x0000FFFF << (CHUNK0 - table[i].src_len));
			e_index = ip_upper | (0x0000FFFF >> (table[i].src_len));
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].bucketSize++;
		}
	}
	//malloc store index array
	for (j = 0; j < arraySize; j++){
		if (temp_storage[j].bucketSize > 0) {
			temp_storage[j].index_bucket = malloc(sizeof(int) * temp_storage[j].bucketSize);
			temp_storage[j].bucketSize = 0;
		}
	}
	//assign to index bucket
	for (i = 0; i < num_entry; i++) {
		ip_upper = (table[i].src_ip & (0xFFFF0000)) >> CHUNK0;
		if (table[i].src_len >= CHUNK0)
			temp_storage[ip_upper].index_bucket[temp_storage[ip_upper].bucketSize++] = i;
		else {
			s_index = ip_upper & (0x0000FFFF << (CHUNK0 - table[i].src_len));
			e_index = ip_upper | (0x0000FFFF >> (table[i].src_len));
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].index_bucket[temp_storage[j].bucketSize++] = i;
		}
	}
	
	//assign eqID
	count = 0;
	eqID_array[0] = create_listnode();
	eqID_array[0]->index_size = temp_storage[0].bucketSize;
	eqID_array[0]->bucket = temp_storage[0].index_bucket;
	eqID_array[0]->eqID = count++;
	simple_storage[0][0] = eqID_array[0];
	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array[0];
		
		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage[0][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}

		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array[0];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage[0][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);

	
	//**************************************************deal with src_ip_lower**********************************************************
	arraySize = pow(2, CHUNK1);
	temp_storage = malloc(sizeof(node) * arraySize);
	simple_storage[1] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	for (j = 0; j < arraySize; j++) {
		temp_storage[j].bucketSize = 0;
		temp_storage[j].eqID = -1;
		temp_storage[j].index_bucket = NULL;
		simple_storage[1][j] = NULL;
	}
	//count array size
	for (i = 0; i < num_entry; i++) {
		ip_lower = (table[i].src_ip & (0x0000FFFF));

		if (table[i].src_len >= CHUNK0 + CHUNK1)
			temp_storage[ip_lower].bucketSize++;
		else if (table[i].src_len < CHUNK0) {
			s_index = 0x00000000;
			e_index = 0x0000FFFF;
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].bucketSize++;
		}
		else {
			s_index = ip_lower & (0x0000FFFF << (CHUNK0 + CHUNK1 - table[i].src_len));
			e_index = ip_lower | (0x0000FFFF >> (table[i].src_len - CHUNK0));
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].bucketSize++;
		}
	}
	//malloc store index array
	for (j = 0; j < arraySize; j++) {
		if (temp_storage[j].bucketSize > 0) {
			temp_storage[j].index_bucket = malloc(sizeof(int) * temp_storage[j].bucketSize);
			temp_storage[j].bucketSize = 0;
		}
	}
	//assign to index bucket
	for (i = 0; i < num_entry; i++) {
		ip_lower = (table[i].src_ip & (0x0000FFFF));

		if (table[i].src_len >= CHUNK0 + CHUNK1)
			temp_storage[ip_lower].index_bucket[temp_storage[ip_lower].bucketSize++] = i;
		else if (table[i].src_len < CHUNK0) {
			s_index = 0x00000000;
			e_index = 0x0000FFFF;
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].index_bucket[temp_storage[j].bucketSize++] = i;
		}
		else {
			s_index = ip_lower & (0x0000FFFF << (CHUNK0 + CHUNK1 - table[i].src_len));
			e_index = ip_lower | (0x0000FFFF >> (table[i].src_len - CHUNK0));
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].index_bucket[temp_storage[j].bucketSize++] = i;
		}
	}

	//assign eqID
	count = 0;
	eqID_array[1] = create_listnode();
	eqID_array[1]->index_size = temp_storage[0].bucketSize;
	eqID_array[1]->bucket= temp_storage[0].index_bucket;
	eqID_array[1]->eqID = count++;
	simple_storage[1][0] = eqID_array[1];
	for (j = 1; j < arraySize; j++) {
			pointer_eqID = eqID_array[1];

			//have eqID need to compare
			while (pointer_eqID != NULL) {
				//if same need to do intersection compare
				if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
					temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
					//same ruleset
					if (temp->bucketSize == temp_storage[j].bucketSize) {
						simple_storage[1][j] = pointer_eqID;
						break;
					}
					//free temp
					free(temp->index_bucket);
					free(temp);
				}
				pointer_eqID = pointer_eqID->next;	
			}
			//new eqID
			if (pointer_eqID == NULL) {
				pointer_eqID = eqID_array[1];
				while (pointer_eqID->next != NULL)
					pointer_eqID = pointer_eqID->next;
				pointer_eqID->next = create_listnode();
				pointer_eqID->next->eqID = count;
				pointer_eqID->next->index_size = temp_storage[j].bucketSize;
				pointer_eqID->next->bucket = temp_storage[j].index_bucket;
				simple_storage[1][j] = pointer_eqID->next;
				count++;
			}
			//not first element
			else
				free(temp_storage[j].index_bucket);
	}
	free(temp_storage);
	
	//**************************************************deal with des_ip_upper**********************************************************
	arraySize = pow(2, CHUNK2);
	temp_storage = malloc(sizeof(node) * arraySize);
	simple_storage[2] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	for (j = 0; j < arraySize; j++) {
		temp_storage[j].bucketSize = 0;
		temp_storage[j].eqID = -1;
		temp_storage[j].index_bucket = NULL;
		simple_storage[2][j] = NULL;
	}

	//count array size
	for (i = 0; i < num_entry; i++) {
		ip_upper = (table[i].des_ip & (0xFFFF0000)) >> CHUNK2;
		if (table[i].des_len >= CHUNK2)
			temp_storage[ip_upper].bucketSize++;
		else {
			s_index = ip_upper & (0x0000FFFF << (CHUNK2 - table[i].des_len));
			e_index = ip_upper | (0x0000FFFF >> (table[i].des_len));
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].bucketSize++;
		}
	}
	//malloc store index array
	for (j = 0; j < arraySize; j++) {
		if (temp_storage[j].bucketSize > 0) {
			temp_storage[j].index_bucket = malloc(sizeof(int) * temp_storage[j].bucketSize);
			temp_storage[j].bucketSize = 0;
		}
	}
	//assign to index bucket
	for (i = 0; i < num_entry; i++) {
		ip_upper = (table[i].des_ip & (0xFFFF0000)) >> CHUNK2;
		if (table[i].des_len >= CHUNK2)
			temp_storage[ip_upper].index_bucket[temp_storage[ip_upper].bucketSize++] = i;
		else {
			s_index = ip_upper & (0x0000FFFF << (CHUNK2 - table[i].des_len));
			e_index = ip_upper | (0x0000FFFF >> (table[i].des_len));
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].index_bucket[temp_storage[j].bucketSize++] = i;
		}
	}

	//assign eqID
	count = 0;
	eqID_array[2] = create_listnode();
	eqID_array[2]->index_size = temp_storage[0].bucketSize;
	eqID_array[2]->bucket = temp_storage[0].index_bucket;
	eqID_array[2]->eqID = count++;
	simple_storage[2][0] = eqID_array[0];
	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array[2];

		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage[2][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}

		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array[2];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage[2][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);


	//**************************************************deal with des_ip_lower**********************************************************
	arraySize = pow(2, CHUNK3);
	temp_storage = malloc(sizeof(node) * arraySize);
	simple_storage[3] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	for (j = 0; j < arraySize; j++) {
		temp_storage[j].bucketSize = 0;
		temp_storage[j].eqID = -1;
		temp_storage[j].index_bucket = NULL;
		simple_storage[3][j] = NULL;
	}
	//count array size
	for (i = 0; i < num_entry; i++) {
		ip_lower = (table[i].des_ip & (0x0000FFFF));

		if (table[i].des_len >= CHUNK2 + CHUNK3)
			temp_storage[ip_lower].bucketSize++;
		else if (table[i].des_len < CHUNK2) {
			s_index = 0x00000000;
			e_index = 0x0000FFFF;
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].bucketSize++;
		}
		else {
			s_index = ip_lower & (0x0000FFFF << (CHUNK2 + CHUNK3 - table[i].des_len));
			e_index = ip_lower | (0x0000FFFF >> (table[i].des_len - CHUNK2));
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].bucketSize++;
		}
	}
	//malloc store index array
	for (j = 0; j < arraySize; j++) {
		if (temp_storage[j].bucketSize > 0) {
			temp_storage[j].index_bucket = malloc(sizeof(int) * temp_storage[j].bucketSize);
			temp_storage[j].bucketSize = 0;
		}
	}
	//assign to index bucket
	for (i = 0; i < num_entry; i++) {
		ip_lower = (table[i].des_ip & (0x0000FFFF));

		if (table[i].des_len >= CHUNK2 + CHUNK3)
			temp_storage[ip_lower].index_bucket[temp_storage[ip_lower].bucketSize++] = i;
		else if (table[i].des_len < CHUNK2) {
			s_index = 0x00000000;
			e_index = 0x0000FFFF;
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].index_bucket[temp_storage[j].bucketSize++] = i;
		}
		else {
			s_index = ip_lower & (0x0000FFFF << (CHUNK2 + CHUNK3 - table[i].des_len));
			e_index = ip_lower | (0x0000FFFF >> (table[i].des_len - CHUNK2));
			for (j = s_index; j <= e_index; j++)
				temp_storage[j].index_bucket[temp_storage[j].bucketSize++] = i;
		}
	}

	//assign eqID
	count = 0;
	eqID_array[3] = create_listnode();
	eqID_array[3]->index_size = temp_storage[0].bucketSize;
	eqID_array[3]->bucket = temp_storage[0].index_bucket;
	eqID_array[3]->eqID = count++;
	simple_storage[3][0] = eqID_array[1];
	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array[3];

		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage[3][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}
		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array[3];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage[3][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);

	//*************************************************deal with src port**********************************************
	arraySize = pow(2, CHUNK4);
	temp_storage = malloc(sizeof(node) * arraySize);
	simple_storage[4] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	for (j = 0; j < arraySize; j++) {
		temp_storage[j].bucketSize = 0;
		temp_storage[j].eqID = -1;
		temp_storage[j].index_bucket = NULL;
		simple_storage[4][j] = NULL;
	}
	//count array size
	for (i = 0; i < num_entry; i++) {
		s_index = table[i].src_port_start;
		e_index = table[i].src_port_end;
		for (j = s_index; j <= e_index; j++)
			temp_storage[j].bucketSize++;
	}
	//malloc store index array
	for (j = 0; j < arraySize; j++) {
		if (temp_storage[j].bucketSize > 0) {
			temp_storage[j].index_bucket = malloc(sizeof(int) * temp_storage[j].bucketSize);
			temp_storage[j].bucketSize = 0;
		}
	}
	//assign to index bucket
	for (i = 0; i < num_entry; i++) {
		s_index = table[i].src_port_start;
		e_index = table[i].src_port_end;
		for (j = s_index; j <= e_index; j++)
			temp_storage[j].index_bucket[temp_storage[j].bucketSize++] = i;
	}
	//assign eqID
	count = 0;
	eqID_array[4] = create_listnode();
	eqID_array[4]->index_size = temp_storage[0].bucketSize;
	eqID_array[4]->bucket = temp_storage[0].index_bucket;
	eqID_array[4]->eqID = count++;
	simple_storage[4][0] = eqID_array[4];
	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array[4];

		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage[4][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}
		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array[4];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage[4][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);

	//*************************************************deal with des port**********************************************
	arraySize = pow(2, CHUNK5);
	temp_storage = malloc(sizeof(node) * arraySize);
	simple_storage[5] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	for (j = 0; j < arraySize; j++) {
		temp_storage[j].bucketSize = 0;
		temp_storage[j].eqID = -1;
		temp_storage[j].index_bucket = NULL;
		simple_storage[5][j] = NULL;
	}
	//count array size
	for (i = 0; i < num_entry; i++) {
		s_index = table[i].des_port_start;
		e_index = table[i].des_port_end;
		for (j = s_index; j <= e_index; j++)
			temp_storage[j].bucketSize++;
	}
	//malloc store index array
	for (j = 0; j < arraySize; j++) {
		if (temp_storage[j].bucketSize > 0) {
			temp_storage[j].index_bucket = malloc(sizeof(int) * temp_storage[j].bucketSize);
			temp_storage[j].bucketSize = 0;
		}
	}
	//assign to index bucket
	for (i = 0; i < num_entry; i++) {
		s_index = table[i].des_port_start;
		e_index = table[i].des_port_end;
		for (j = s_index; j <= e_index; j++)
			temp_storage[j].index_bucket[temp_storage[j].bucketSize++] = i;
	}
	//assign eqID
	count = 0;
	eqID_array[5] = create_listnode();
	eqID_array[5]->index_size = temp_storage[0].bucketSize;
	eqID_array[5]->bucket = temp_storage[0].index_bucket;
	eqID_array[5]->eqID = count++;
	simple_storage[5][0] = eqID_array[5];
	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array[5];

		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage[5][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}
		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array[5];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage[5][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);

	//**************************************************deal with protocol********************************************
	arraySize = pow(2, CHUNK6);
	temp_storage = malloc(sizeof(node) * arraySize);
	simple_storage[6] = malloc(sizeof(struct list_node*) * arraySize);
	num_pointer += arraySize;
	for (j = 0; j < arraySize; j++) {
		temp_storage[j].bucketSize = 0;
		temp_storage[j].eqID = -1;
		temp_storage[j].index_bucket = NULL;
		simple_storage[6][j] = NULL;
	}
	//count array size
	for (i = 0; i < num_entry; i++) {
		s_index = table[i].protocol;
		temp_storage[s_index].bucketSize++;
	}
	//malloc store index array
	for (j = 0; j < arraySize; j++) {
		if (temp_storage[j].bucketSize > 0) {
			temp_storage[j].index_bucket = malloc(sizeof(int) * temp_storage[j].bucketSize);
			temp_storage[j].bucketSize = 0;
		}
	}
	//assign to index bucket
	for (i = 0; i < num_entry; i++) {
		s_index = table[i].protocol;
		temp_storage[s_index].index_bucket[temp_storage[s_index].bucketSize++] = i;
	}
	//assign eqID
	count = 0;
	eqID_array[6] = create_listnode();
	eqID_array[6]->index_size = temp_storage[0].bucketSize;
	eqID_array[6]->bucket = temp_storage[0].index_bucket;
	eqID_array[6]->eqID = count++;
	simple_storage[6][0] = eqID_array[6];

	for (j = 1; j < arraySize; j++) {
		pointer_eqID = eqID_array[6];

		//have eqID need to compare
		while (pointer_eqID != NULL) {
			//if same need to do intersection compare
			if (pointer_eqID->index_size == temp_storage[j].bucketSize) {
				temp = intersection(pointer_eqID->bucket, pointer_eqID->index_size, temp_storage[j].index_bucket, temp_storage[j].bucketSize);
				//same ruleset
				if (temp->bucketSize == temp_storage[j].bucketSize) {
					simple_storage[6][j] = pointer_eqID;
					break;
				}
				//free temp
				free(temp->index_bucket);
				free(temp);
			}
			pointer_eqID = pointer_eqID->next;
		}
		//new eqID
		if (pointer_eqID == NULL) {
			pointer_eqID = eqID_array[6];
			while (pointer_eqID->next != NULL)
				pointer_eqID = pointer_eqID->next;
			pointer_eqID->next = create_listnode();
			pointer_eqID->next->eqID = count;
			pointer_eqID->next->index_size = temp_storage[j].bucketSize;
			pointer_eqID->next->bucket = temp_storage[j].index_bucket;
			simple_storage[6][j] = pointer_eqID->next;
			count++;
		}
		//not first element
		else
			free(temp_storage[j].index_bucket);
	}
	free(temp_storage);
	/*
	//for test
	for (i = 0; i < 7; i++) {
		for (j = 0; j < 16; j++)
			printf("storage[%d][%d].eqID = %d \n", i, j, simple_storage[i][j]->eqID);
		printf("**********************************************************************\n");
	}
	*/
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
void count_element() {
	int i;
	struct list_node *p;
	for (i = 0; i < 7; i++) {
		p = eqID_array[i];
		while (p != NULL) {
			p = p->next;
			num_element++;
		}
	}
	for (i = 0; i < 3; i++) {
		p = eqID_array_second[i];
		while (p != NULL) {
			p = p->next;
			num_element++;
		}
	}
	for (i = 0; i < 1; i++) {
		p = eqID_array_third[i];
		while (p != NULL) {
			p = p->next;
			num_element++;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
	int i, j;
	char filename[50] = "output_500.txt";
	set_table(argv[1]);
	set_query(argv[2]);
	//phase 0
	create_phase_zero();
	parse_zero();
	parse_first();
	count_element();
	printf("num of eqID element:%d\n", num_element);
	printf("num of pointer:%d\n", num_pointer);
	for (i = 0; i < 7; i++)
		printf("eqID_size[%d]:%d\n", i, eqID_size[i]);
	for (i = 0; i < 3; i++)
		printf("eqID_size_second[%d]:%d\n", i, eqID_size_second[i]);
	printf("eqID_size_third[0]:%d\n", eqID_size_second[0]);
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
	return 0;
}

