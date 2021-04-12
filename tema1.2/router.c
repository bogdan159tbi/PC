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
	rtable[seq].outgoing_interface = atoi(interface);
}

void parse_router_table(char *filename){
	FILE *f;
	fprintf(stderr, "Parsing Routing table\n");
	f = fopen(filename, "r");
	DIE(f == NULL, "failed to open routing table.");
	char line[ROUTING_LINE_LEN * 10];
	int i ;
	for( i = 0 ;fgets(line,sizeof(line),f); i++){
		char prefix_str[200],next_hop_str[200],
			 mask_str[200], interface_str[2];
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
		struct in_addr a;
		a.s_addr = arp_table[i].ip; 
		fprintf(stderr,"nr %d : ip_address %s hw_address %d\n",i,inet_ntoa(a),arp_table[i].mac);
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
			fprintf(stderr,"Who has %d ?Tell %d\n",daddr, saddr);
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
			fprintf(stderr,"I am %d\n",saddr);
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

struct router_entry *get_best_route(__u32 dest_ip) {
	/* TODO 1: Implement the function */
	int i;
	struct router_entry *best_route = NULL;
	for(i = 0 ; i < rtable0->size;i++){
		if ((rtable0[i].mask & dest_ip) == (rtable0[i].prefix & rtable0[i].mask) ){
			if(!best_route)
				best_route = &rtable0[i];
			else if(ntohl(rtable0[i].mask) > ntohl(best_route->mask))
				best_route = &rtable0[i];
		}
	}
	return best_route;
}
struct arp_entry *get_arp_entry(__u32 ip) {
    /* TODO 2: Implement */
	for(int i = 0 ; i < arp_table_size;i++){
		if(ip == arp_table[i].ip)
			return &arp_table[i];
	}
    return NULL;
}
// Incerc rezolvarea algoritmului pentru router
//functie luata din router.c lab 4
uint16_t ip_checksum2(void* vdata,size_t length) {
	// Cast the data pointer to one that can be indexed.
	char* data=(char*)vdata;

	// Initialise the accumulator.
	uint64_t acc=0xffff;

	// Handle any partial block at the start of the data.
	unsigned int offset=((uintptr_t)data)&3;
	if (offset) {
		size_t count=4-offset;
		if (count>length) count=length;
		uint32_t word=0;
		memcpy(offset+(char*)&word,data,count);
		acc+=ntohl(word);
		data+=count;
		length-=count;
	}

	// Handle any complete 32-bit blocks.
	char* data_end=data+(length&~3);
	while (data!=data_end) {
		uint32_t word;
		memcpy(&word,data,4);
		acc+=ntohl(word);
		data+=4;
	}
	length&=3;

	// Handle any partial block at the end of the data.
	if (length) {
		uint32_t word=0;
		memcpy(&word,data,length);
		acc+=ntohl(word);
	}

	// Handle deferred carries.
	acc=(acc&0xffffffff)+(acc>>32);
	while (acc>>16) {
		acc=(acc&0xffff)+(acc>>16);
	}

	// If the data began at an odd byte address
	// then reverse the byte order to compensate.
	if (offset&1) {
		acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
	}

	// Return the checksum in network byte order.
	return htons(~acc);
}

int main(int argc, char *argv[])
{
	packet m;
	int rc;

	init(argc - 2, argv + 2);
	rtable0 = malloc( RTABLE_SIZE * MAX_ADDRESSES * 1000);
	DIE(rtable0 == NULL, "rtable0 memory allocation\n");

	rtable1 = malloc( RTABLE_SIZE * MAX_ADDRESSES * 1000);
	DIE(rtable0 == NULL, "rtable1 memory allocation\n");

	arp_table = malloc(sizeof(struct arp_entry) * MAX_ADDRESSES);
	DIE(rtable0 == NULL, "arp_table memory allocation\n");

	parse_router_table("rtable0.txt");
	parse_router_table("rtable1.txt");
	parse_arp_table();
	queue q = queue_create();
	//functionalitati router
	while (1) {
		// receive packet
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		/* Students will write code here */
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		//	daca router = destinatie si packet = icmp request => answer
		// otherwise drop the packet
		struct icmphdr *icmp_hdr = parse_icmp(m.payload);
		if(icmp_hdr){
			if(icmp_hdr->type == ICMP_ECHO){ //PT REQUEST
				//WARNING nu stiu daca ia adresa buna functia asta
				struct router_entry *route = get_best_route(ip_hdr->daddr);
				struct in_addr adr,adr2,adr3;
				adr2.s_addr = ip_hdr->saddr;
				adr.s_addr = ip_hdr->daddr;
				fprintf(stdout,"icmp request to ip = %s from %s\n",(inet_ntoa(adr)),inet_ntoa(adr2));
				if(!route){
					fprintf(stdout,"route e null\n");
					continue;
				}else{
					adr.s_addr = route->next_hop;
					fprintf(stdout,"interface = %d next_hop %s\n",route->outgoing_interface,inet_ntoa(adr));
				}
				uint32_t router_ip;
			
				
				char *str = malloc(50);// sa i dau free daca l las cu malloc
				str = get_interface_ip(route->outgoing_interface);
				inet_aton(str,&adr3);
				router_ip = adr3.s_addr;
				fprintf(stdout,"router ip addr %s\n",inet_ntoa(adr3));

				if(ip_hdr->daddr == router_ip){
					//adauga icmp reply
					fprintf(stdout,"icmp reply from router\n");
					send_icmp(ip_hdr->daddr,ip_hdr->saddr,eth_hdr->ether_shost,eth_hdr->ether_dhost,
							 0,0,m.interface,icmp_hdr->un.echo.id,icmp_hdr->un.echo.sequence);
				}else if(/*verifica ttl */ip_hdr->ttl <= 1 ){
					//time limit exceeded
					struct in_addr a,b;
					a.s_addr = ip_hdr->saddr;
					b.s_addr = ip_hdr->daddr;
					fprintf(stdout,"ICMP: time exceeded ttl sent %s (dest was %s)\n",inet_ntoa(a),inet_ntoa(b));
					//ce fel de mesaj icmp trebuie trimis?
					send_icmp_error(ip_hdr->saddr,ip_hdr->daddr,eth_hdr->ether_dhost,
									eth_hdr->ether_shost,11,0,m.interface);
					continue;
				}else{
					//destination host unreachable daca nu e pt router 
					struct in_addr a,b;
					a.s_addr = ip_hdr->saddr;
					b.s_addr = ip_hdr->daddr;
					fprintf(stdout,"icmp destination unreachable failed from %s to %s\n",inet_ntoa(b), inet_ntoa(a));
					//send_icmp(ip_hdr->daddr,ip_hdr->saddr,eth_hdr->ether_shost,eth_hdr->ether_dhost,
					//		 3,0,m.interface,icmp_hdr->un.echo.id,icmp_hdr->un.echo.sequence);
					send_icmp_error(ip_hdr->saddr,ip_hdr->daddr,eth_hdr->ether_dhost,
									eth_hdr->ether_shost,3,0,m.interface);
					continue;
				}
			}
		}else
			continue;
		fprintf(stdout,"icmp succeeded\n");
		//packet = arp request to one of router s ip addresses =>
		// answer = arp reply with ip mac address = get_interface_mac()
		struct arp_header *arp_hdr = parse_arp(m.payload);
		struct router_entry *route;
		// Check if the header has been extracted succesfully
		if ( arp_hdr != NULL ) {
			if(arp_hdr->op == ARPOP_REQUEST){
				route = get_best_route(ip_hdr->daddr);
				uint32_t rtr_ip;
				char *s = get_interface_ip(route->outgoing_interface);
				struct in_addr rp;
				inet_aton(s,&rp);
				rtr_ip = rp.s_addr;
				fprintf(stdout,"arprequest \n");
				if(arp_hdr->tpa == rtr_ip){
					fprintf(stdout,"arp request to dest ip = %s\n",inet_ntoa(rp));
					//modifica pt arp reply cu adresele mac potrivite
					//adresa mac sursa = adresa mac ip
					//adr mac dest = adr mac sursa initiala
					memcpy(eth_hdr->ether_dhost,eth_hdr->ether_shost, 6 * sizeof(uint8_t));
					get_interface_mac(route->outgoing_interface,eth_hdr->ether_shost);
					//send arp reply from router
					send_arp(ip_hdr->saddr,ip_hdr->daddr,eth_hdr,route->outgoing_interface,
							ARPOP_REPLY);
				}
			}else if(arp_hdr->op == ARPOP_REPLY){
				//packet = arp reply => update arp_table 

				// ia adresa ip sursa si mapeaz o la adresa mac sursa
				memcpy(eth_hdr->ether_dhost,eth_hdr->ether_shost, 6 * sizeof(uint8_t));
				// unde le transmit mai departe?
			}
		}
		// daca ttl <= 1 => send correct icmp mesage to src 
		if(ip_hdr->ttl <= 1){
			// nu tratez deja mai sus cazul de icmp ttl exceeded ? 
			// mai e nevoie
			
			continue;
		}
		//daca checksum e gresit => continue;
		if(ip_checksum2(ip_hdr,sizeof(struct iphdr)) != 0)
			continue;
		//ttl--
		ip_hdr->ttl--;
		//update checksum
		ip_hdr->check = 0;
		ip_hdr->check = ip_checksum2(ip_hdr,sizeof(struct iphdr));
		//find best router entry
		//if entry not found => send icmp message to src and continue
		struct router_entry *best_route = get_best_route(ip_hdr->daddr);
		if(best_route == NULL){
			//WARNING nu sunt sigur ca aici e bine
			//destination host unreachable daca nu e pt router 
			fprintf(stderr,"best router entry not found  algorithm point 7\n");
			send_icmp(ip_hdr->saddr,ip_hdr->daddr,eth_hdr->ether_shost,eth_hdr->ether_dhost,
					 3,0,m.interface,icmp_hdr->un.echo.id,icmp_hdr->un.echo.sequence);
			continue;
		}
		//modify src / dest mac addr
		get_interface_mac(best_route->outgoing_interface,eth_hdr->ether_shost);
		struct arp_entry *best_arp = get_arp_entry(best_route->next_hop);
		//if dest mac unknown => send arp request on destination interface
		if(!best_arp){
			//trebuie sa modific eth_hdr inainte
			// request = broadcast
			fprintf(stderr,"Who has %d ?Tell %d\n",best_route->next_hop, ip_hdr->daddr);
			uint8_t *mac_r = malloc(sizeof(uint8_t) * 6) ;
			if(!mac_r){
				fprintf(stderr,"failed to allocate mac router address\n");
				DIE(!mac_r,"mac_r failed to malloc\n");
			}
			get_interface_mac(best_route->outgoing_interface,mac_r);
			memcpy(eth_hdr+6, mac_r,6 * sizeof(uint8_t));
			
			struct in_addr adr2 ;
			inet_aton(get_interface_ip(best_route->outgoing_interface),&adr2);		
			arp_hdr->spa = htons(adr2.s_addr);

			memset(arp_hdr->tha, 0xff, sizeof(uint8_t)*6);

			memcpy(eth_hdr,arp_hdr->tha, sizeof(uint8_t)*6);

			arp_hdr->tpa = htons(best_route->next_hop);
			arp_hdr->op = ARPOP_REQUEST;
			eth_hdr->ether_type = 0x0806; // sau pun tot arpop_request ??
			
			send_arp(best_route->next_hop,ip_hdr->daddr,eth_hdr,
			best_route->outgoing_interface,ARPOP_REQUEST);
			//TODO: cum bag pachetul in coada stiind ca inca nu am 
			// aflat adresa mac dest
			continue;
		}
		//and then save the packet in the queue for sending further when
		// dest mac addr is found
		//queue_enq(q,m);
		// send_packet() further
		send_packet(best_route->outgoing_interface,&m);
	}
	
	free(rtable0);
	free(rtable1);
	free(arp_table);
	return 0;
}
