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
		}else if((rtable1[i].mask & dest_ip) == (rtable1[i].prefix & rtable1[i].mask) ){
			if(!best_route)
				best_route = &rtable1[i];
			else if(ntohl(rtable1[i].mask) > ntohl(best_route->mask))
				best_route = &rtable1[i];
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
uint32_t str_to_ip(char *ipstr){
	uint32_t router_ip;	
	struct in_addr adr;				
	inet_aton(ipstr,&adr);
	router_ip = adr.s_addr;
	return router_ip;
}
char *ip_to_str(uint32_t router_ip){
	struct in_addr adr;
	adr.s_addr = router_ip;
	return inet_ntoa(adr);
}
int send_icmp_packet(packet *m , int type){
	struct ether_header *eth_hdr = (struct ether_header *)m->payload;
	struct iphdr *ip_hdr = (struct iphdr *)(m->payload + sizeof(struct ether_header));
	//nu mai e nevoie sa creez eu packetul
	//ma folosesc de send_icmp
	// doar sa modific campurile acum
	struct ether_header *new_eth_hdr = malloc(sizeof(struct ether_header));
	DIE(!new_eth_hdr,"new eth hdr failed\n");
	//fill in eth header info
	memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost,sizeof(uint8_t) * 6);
	memcpy(new_eth_hdr->ether_shost, eth_hdr->ether_dhost,sizeof(uint8_t) * 6);

	struct iphdr *new_ip_hdr = malloc(sizeof(struct iphdr));
	DIE(!new_ip_hdr,"new ip hdr failed\n");
	memcpy(new_ip_hdr,ip_hdr,sizeof(struct iphdr));
	//htonl ca sa am network order de la host order ( l e pentru uint32)
	new_ip_hdr->daddr = (ip_hdr->saddr);
	new_ip_hdr->saddr = (str_to_ip(get_interface_ip(m->interface)));
	new_ip_hdr->protocol = 0x1; // pot sa modific cu macro
	new_ip_hdr->tot_len = htons(sizeof(struct icmphdr) + sizeof(struct iphdr));

	struct icmphdr *icmp_hdr = parse_icmp(m->payload);
	struct icmphdr *new_icmp_hdr = malloc(sizeof(struct icmphdr));
	DIE(!new_icmp_hdr , "icmp header failed\n");
	if(type == 0){
		memcpy(new_icmp_hdr,icmp_hdr,sizeof(struct icmphdr));
	} else if(type == 11){//icmp error
		fprintf(stdout,"icmp error dest unreachable\n");
		send_icmp_error(new_ip_hdr->daddr,new_ip_hdr->saddr,
			  			new_eth_hdr->ether_shost,new_eth_hdr->ether_dhost,
						11,0,m->interface);
	}
	new_icmp_hdr->type = type;//dest unreachable
	new_icmp_hdr->code = 0;
	new_icmp_hdr->checksum = 0;
	new_icmp_hdr->checksum = ip_checksum2(new_icmp_hdr,sizeof(struct icmphdr));

	//la un echo id si un echo seq ce pun??
	struct router_entry *best = get_best_route(new_ip_hdr->daddr);
	
	fprintf(stdout,"sending icmp packet\n");
	fprintf(stdout,"%s is sending to %s\n",
			ip_to_str(new_ip_hdr->saddr),
			ip_to_str(new_ip_hdr->daddr));
	
	send_icmp(new_ip_hdr->daddr,new_ip_hdr->saddr,
			  new_eth_hdr->ether_shost,new_eth_hdr->ether_dhost,
			  new_icmp_hdr->type,new_icmp_hdr->code,
			  m->interface,icmp_hdr->un.echo.id,icmp_hdr->un.echo.sequence);
	return 0;// if succeeded
}

int send_further(packet *m){
	//pentru a modifica campurile pachetului
	//odata ce sunt trimise mai departe
	struct iphdr *ip_hdr = (struct iphdr *)(m->payload + sizeof(struct ether_header));
	if(ip_hdr->ttl <= 1){
		return send_icmp_packet(m, 11);
	}
	if(ip_checksum2(ip_hdr,sizeof(struct iphdr)) != 0)
		return -1;
	//pentru bonus creste checksum cand scade ttl
	memcpy(m->payload + sizeof(struct ether_header),ip_hdr,sizeof(struct iphdr));
	return 0;
}
void send_arp_request(int interface, uint32_t next_hop){
	uint8_t *mac_broadcast = (uint8_t *)malloc(sizeof(uint8_t) * 6);
	DIE(!mac_broadcast,"mac broadcast failed\n");
	//memcpy(mac_broadcast,0xff,6 * sizeof(uint8_t));
	for(int i = 0 ;i < 6; i++)
		mac_broadcast[i] = 0xff;
	
	uint8_t *mac_unknown = (uint8_t *)malloc(sizeof(uint8_t) * 6);
	DIE(!mac_unknown,"mac broadcast failed\n");
	//memcpy(mac_broadcast,0xff,6 * sizeof(uint8_t));
	for(int i = 0 ;i < 6; i++)
		mac_unknown[i] = 0xff;

	uint8_t *mac_interface = (uint8_t *)malloc(sizeof(uint8_t) * 6);
	DIE(!mac_interface,"mac broadcast failed\n");	
	get_interface_mac(interface, mac_interface);

	struct ether_header *new_eth = malloc(sizeof(struct ether_header));
	memcpy(new_eth->ether_dhost,mac_broadcast,sizeof(uint8_t) * 6);
	memcpy(new_eth->ether_shost,mac_interface,sizeof(uint8_t) * 6);
	new_eth->ether_type = htons(ETHERTYPE_ARP);

	struct arphdr *arp_hdr = malloc(sizeof(struct arphdr));
	DIE(!arp_hdr,"arp hdr failed\n");
	arp_hdr->ar_hrd = htons(1);
	arp_hdr->ar_pro = htons(0x8000);
	arp_hdr->ar_hln - 6;
	arp_hdr->ar_pln = 4;
	arp_hdr->ar_op = htons(1);
	
	//ce mai trebuie sa completez de la arp header 
	uint32_t saddr = str_to_ip(get_interface_ip(interface));
	send_arp(next_hop,saddr,new_eth,interface,1);
}

int handle_packet_ip(packet m,queue *q){
	uint32_t daddr;
	struct iphdr *ip_hdr = (struct iphdr*)(m.payload + sizeof(struct ether_header));
	daddr = ip_hdr->daddr;
	if(!strcmp(ip_to_str(daddr),get_interface_ip(m.interface))){
		if(ip_hdr->protocol == 1){
			//ICMP
			fprintf(stdout,"icmp packet\n");
			struct icmphdr *icmp_hdr = parse_icmp(m.payload);
			if(icmp_hdr->type == 8){
				fprintf(stdout,"%s pings to %s\n",
						ip_to_str(ip_hdr->saddr),ip_to_str(ip_hdr->daddr));
				send_icmp_packet(&m, 0);
			}
			return -1;
		}
	}
	struct router_entry *best_route = get_best_route(daddr);
	if(best_route->next_hop == 0){
		return send_icmp_packet(&m, 3);
	}
	
	if(send_further(&m) == -1)
		return -1;
	int iface = best_route->outgoing_interface;
	//daca vreau sa fac cu arp static
	// nu mai folosesc urm alg
	uint8_t *mac_addr = malloc(sizeof(uint8_t) * 6);
	if(arp_table_size <= 0){ // <= 0 pt ca -1 ins ca nu se afla
		packet *p = malloc(sizeof(packet));
		DIE(!p, "packet ! allocated\n");
		memcpy(p,&m, sizeof(packet));
		queue_enq(*q,p);
		send_arp_request(iface,best_route->next_hop);
		return 1;
	}else{
		//ia mac ul pe interfata respectiva
		// shost == mac interface
		// dhost = adresaMac
		struct ether_header *eth_hdr = (struct ether_header*)m.payload;
		uint8_t *mac_addr2 = malloc(sizeof(uint8_t) * 6);
		DIE(!mac_addr2,"mac addr2 failed\n");
		get_interface_mac(iface,mac_addr2);
		struct arp_entry *best_arp = get_arp_entry(best_route->next_hop);
		memcpy(eth_hdr->ether_shost,mac_addr2,sizeof(uint8_t) * 6);
		memcpy(eth_hdr->ether_dhost,best_arp->mac,sizeof(uint8_t) * 6);
		send_packet(best_route->outgoing_interface,&m);
	}
	
	return 0;
}

void handle_packet_arp(packet m,queue *q){
	struct ether_header *eth_hdr = (struct ether_header*)m.payload;
	uint8_t *aux_mac = malloc(sizeof(uint8_t) * 6);
	DIE(!aux_mac,"aux mac failed");
	memcpy(aux_mac, eth_hdr->ether_shost,sizeof(uint8_t) * 6);
	struct arp_header *arp_hdr = parse_arp(m.payload);
	DIE(!arp_hdr,"arp failed");
	if(ntohs(arp_hdr->op) ==1){
		//request
		arp_hdr->op = htons(2); //devine reply
		if(arp_hdr->tpa == str_to_ip(get_interface_ip(m.interface))){
			//change mac  eht
			memcpy(eth_hdr->ether_dhost,eth_hdr->ether_shost,sizeof(uint8_t) * 6);
			uint8_t *mac_src = malloc(sizeof(uint8_t) * 6);
			DIE(!mac_src,"mac src failed");
			get_interface_mac(m.interface,mac_src);
			memcpy(eth_hdr->ether_shost,mac_src,sizeof(uint8_t) * 6);

			memcpy(arp_hdr->tha,arp_hdr->sha,sizeof(uint8_t) * 6);
			get_interface_mac(m.interface,mac_src);
			memcpy(arp_hdr->sha,mac_src,sizeof(uint8_t) * 6);
			
			//change ip 
			uint32_t aux;
			aux = arp_hdr->spa;
			arp_hdr->spa = arp_hdr->tpa;
			arp_hdr->tpa = aux;
		}
		send_arp(arp_hdr->tpa,arp_hdr->spa,eth_hdr,m.interface,ARPOP_REPLY);
	}else{
		struct arp_entry new_arp;
		new_arp.ip = arp_hdr->spa;
		memcpy(new_arp.mac,arp_hdr->sha,sizeof(uint8_t) * 6);
		arp_table[arp_table_size] = new_arp;
		arp_table_size++;

		while(!queue_empty(*q)){
			packet *p = malloc(sizeof(packet));
			DIE(!p,"packet failed");
			p = (packet*)queue_deq(*q);
			uint8_t *mac_addr = malloc(sizeof(uint8_t) * 6);
			uint32_t ip_addr;
			struct iphdr *ip_hdr = (struct iphdr*)(m.payload + sizeof(struct ether_header));
			ip_addr = ip_hdr->daddr;
			struct router_entry *best_route = get_best_route(ip_addr);
			if(get_arp_entry(best_route->next_hop) == NULL){
				break;
			}else{
				struct ether_header *eth_hdr = (struct ether_header*)p->payload;
				memcpy(eth_hdr->ether_dhost, mac_addr,sizeof(uint8_t) * 6);
				memcpy(p->payload,eth_hdr,sizeof(struct ether_header));
				send_packet(best_route->outgoing_interface,p);
			}
		}
		return;
	}
	
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
		int eth_type = ntohs(eth_hdr->ether_type);
		if(eth_type == ETHERTYPE_IP){
			fprintf(stdout,"packet ip\n");
			handle_packet_ip(m,&q);
		}else if(eth_type == ETHERTYPE_ARP){
			handle_packet_arp(m,&q);
		}
		
	}
	
	free(rtable0);
	free(rtable1);
	free(arp_table);
	return 0;
}

