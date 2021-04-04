#include "include/queue.h"
#include "include/skel.h"

#define ROUTING_LINE_LEN 400

struct router_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int outgoing_interface;
} __attribute__((packed));

struct arp_entry {
	__u32 ip;
	uint8_t mac[6];
};

struct router_entry *router_table;
int rtable_size;

struct arp_entry *arp_table;
int arp_table_size;
/*
un fel de inet_addr

*/
uint32_t strToAddress (char *str){
	uint32_t value;
	uint32_t bytes[5];
	char *split = strtok(str,".");
	int i = 0;
	while(split){
		value = atoi(split);
		bytes[i++] = value;
		split = strtok(NULL, ".");
	}
	value = 0 ;
	for(i = 0; i < 4; i++){
		value = value | (bytes[i] << (32 - (i+1) * 8)); 
	}
	return value;
}
void parse_router_table(char *filename){
	FILE *f;
	fprintf(stderr, "Parsing Routing table\n");
	f = fopen(filename, "r");
	DIE(f == NULL, "failed to open routing table.");
	char line[ROUTING_LINE_LEN];
	int i ;
	for( i = 0 ;fgets(line,sizeof(line),f); i++){
		char prefix_str[50],next_hop_str[50],
			 mask_str[50], interface_str[2];
		sscanf(line,"%s %s %s %s",prefix_str, next_hop_str, mask_str, interface_str);
		fprintf(stderr,"PREFIX: %s NEXT_HOP: %s MASK: %s INTERFACE: %s\n",
				        prefix_str, next_hop_str, mask_str,interface_str);
		router_table[i].prefix = inet_addr(prefix_str);
		router_table[i].next_hop = inet_addr(next_hop_str);
		router_table[i].mask = inet_addr(mask_str);
		router_table[i].outgoing_interface = inet_addr(interface_str);
	}  
	rtable_size = i;
	fclose(f);
	fprintf(stderr,"Done parsing forwarding table\n");;
}


int main(int argc, char *argv[])
{
	packet m;
	int rc;

	init(argc - 2, argv + 2);

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		/* Students will write code here */
		
	}
}
