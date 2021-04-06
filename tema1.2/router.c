#include "include/queue.h"
#include "include/skel.h"
#define ROUTING_LINE_LEN 400
#define MAX_ADDRESSES 200
#define RTABLE_SIZE sizeof(struct router_entry)

struct arp_entry {
	uint32_t ip;
	uint8_t mac[6];
};


struct router_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int outgoing_interface;
	int size;
} __attribute__((packed));

struct router_entry *rtable0;
struct router_entry  *rtable1;
//int rtable0_size, rtable1_size;

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

void parse_router(char *prefix,char *nexth, char *mask,char *interface,int seq,struct router_entry *rtable){
	rtable[seq].prefix = inet_addr(prefix);
	rtable[seq].next_hop = inet_addr(nexth);
	rtable[seq].mask = inet_addr(mask);
	rtable[seq].outgoing_interface = inet_addr(interface);
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
		if(strcmp("filename","rtable0.txt")){
			parse_router(prefix_str, next_hop_str,mask_str,interface_str,i,rtable0);
            rtable0->size = i+1;
        }
		else{
			parse_router(prefix_str, next_hop_str,mask_str,interface_str,i,rtable1);
            rtable1->size = i+1; // am pus i+1 pt ca in lab 4 rtable_size = i la iesire din for
        }
	}  
	fclose(f);
	fprintf(stderr,"Done parsing forwarding table\n");;
}


/*
TODO: implement arp protocol
	 -> send arp packet function
	 -> parse arp header from a packet if its type is 0x0806
ARP -> arp request -> broadcast to find out the mac address of ip destination
	-> arp reply -> message from destination to sender to inform about mac address 
TODO
trebuie sa modific eth_header daca trimit pachetul ?
*/
int check_arp_entry(uint32_t ip){
	for(int i = 0 ; i < arp_table_size; i++){
		if(ip == arp_table[i].ip)
			return i;
	}
	return -1; //if there is no entry for requested ip dest
}
void add_arp_entry(uint32_t ip, uint8_t mac[6]){
	arp_table[arp_table_size].ip = ip ;
	memcpy(arp_table[arp_table_size].mac,mac,sizeof(uint8_t)* 6);
	arp_table_size++;
}
void show_arp_entries(){
	fprintf(stderr,"arp table entries\n");
	for(int i = 0 ;i < arp_table_size; i++){
		fprintf(stderr,"no %d : ip_address %d hw_address %d\n",i,arp_table[i].ip ,arp_table[i].mac);
	}
}
void arp_receive_packet(struct eth_header *eth_hdr,packet *pkt){

	struct arphdr *arp_packet = parse_arp(pkt->payload);
	if(arp_packet){
		//filling in arp_header fields

	}

}

/**
 * @param dha destination MAC
 **/
void arp_broadcast (uint32_t daddr, uint32_t saddr,int interface, struct ether_header *eth_hdr, char *payload){
	/*
	arp opcode pentru a verifica daca e reply / request
	*/
	struct arp_header *arp_h = parse_arp(payload);
	if(arp_h != NULL){
		if(arp_h->op == ARPOP_REQUEST){
			//requestnext_hop_ip
			fprintf(stderr,"Who has %s ?Tell %s\n",daddr, saddr);
			get_interface_mac(interface,arp_h->sha);
			memcpy(eth_hdr + 6,arp_h->sha,6 * sizeof(uint8_t));
			arp_h->spa = htons(saddr);
			memset(arp_h->tha,0xff,sizeof(uint8_t)*6);
			memcpy(eth_hdr,arp_h->tha,6 * sizeof(uint8_t));
			arp_h->tpa = htons(daddr);
			arp_h->op = htons(ARPOP_REQUEST);
			// trebuie sa pun la eth type 0x0806 ?
			eth_hdr->ether_type = ARPOP_REQUEST;
			send_arp(daddr,saddr,eth_hdr,interface,ARPOP_REQUEST);
		}	
	}	
}
/*
daca nu am tabela arp si trebuie umpluta
cand verific pentru fiecare host daca e cel cautat
cum l verific
*/
void arp_unicast(uint32_t daddr, uint32_t saddr,int interface, struct ether_header *eth_hdr, char *payload){
	struct arp_header *arp_h = parse_arp(payload);
	if(arp_h != NULL){
		if(arp_h->op == ARPOP_REPLY){
			fprintf(stderr,"I am %s\n",saddr);
			get_interface_mac(interface, arp_h->sha);
			arp_h->spa = saddr;
			arp_h->tpa = daddr;
			//arp_h->tha = get_interface_mac()
			arp_h->op = htons(ARPOP_REPLY);
			send_arp(daddr,saddr,eth_hdr,interface,ARPOP_REPLY);
		}
	}
}

//functie lab 4 pentru test arp_table = static

void parse_arp_table() 
{
	FILE *f;
	fprintf(stderr, "Parsing ARP table\n");
	f = fopen("arp_table.txt", "r");
	DIE(f == NULL, "Failed to open arp_table.txt");
	char line[100];
	int i = 0;
	for(i = 0; fgets(line, sizeof(line), f); i++) {
		char ip_str[50], mac_str[50];
		sscanf(line, "%s %s", ip_str, mac_str);
		fprintf(stderr, "IP: %s MAC: %s\n", ip_str, mac_str);
		arp_table[i].ip = inet_addr(ip_str);
		int rc = hwaddr_aton(mac_str, arp_table[i].mac);
		DIE(rc < 0, "invalid MAC");
	}
	arp_table_size = i;
	fclose(f);
	fprintf(stderr, "Done parsing ARP table.\n");
}

int main(int argc, char *argv[])
{
	packet m;
	int rc;

	init(argc - 2, argv + 2);
	rtable0 = malloc( RTABLE_SIZE * MAX_ADDRESSES);
	arp_table = malloc(sizeof(struct arp_entry) * MAX_ADDRESSES);
	parse_router_table("rtable0.txt");
	parse_arp_table();
	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		/* Students will write code here */
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		
	}
	
	free(rtable0);
	free(arp_table);
	return 0;
}
