#include "skel.h"

int interfaces[ROUTER_NUM_INTERFACES];
struct route_table_entry *rtable;
int rtable_size;

struct arp_entry *arp_table;
int arp_table_len;

void show_route_entries(struct route_table_entry *rtable,int size){
	printf("IP ADDR   MASK\n");
	for(int i = 0; i < size;i++){
		printf("%d %d\n",rtable[i].prefix,rtable[i].mask);
	}
}
/*
 Returns a pointer (eg. &rtable[i]) to the best matching route
 for the given dest_ip. Or NULL if there is no matching route.
*/
struct route_table_entry *get_best_route(__u32 dest_ip) {
	/* TODO 1: Implement the function */
	int i;
	struct route_table_entry *best_route = NULL;
	for(i = 0 ; i < rtable_size;i++){
		if ((rtable[i].mask & dest_ip) == (rtable[i].prefix & rtable[i].mask) ){
			if(!best_route)
				best_route = &rtable[i];
			else if(ntohl(rtable[i].mask) > ntohl(best_route->mask))
				best_route = &rtable[i];
		}
	}
	
	return best_route;
}

/*
 Returns a pointer (eg. &arp_table[i]) to the best matching ARP entry.
 for the given dest_ip or NULL if there is no matching entry.
*/
struct arp_entry *get_arp_entry(__u32 ip) {
    /* TODO 2: Implement */
	for(int i = 0 ; i < arp_table_len;i++){
		if(ip == arp_table[i].ip)
			return &arp_table[i];
	}
    return NULL;
}
int main(int argc, char *argv[])
{
	msg m;
	int rc;

	init();
	rtable = malloc(sizeof(struct route_table_entry) * 100);
	arp_table = malloc(sizeof(struct  arp_entry) * 100);
	DIE(rtable == NULL, "memory");
	rtable_size = read_rtable(rtable);
	parse_arp_table();
	/* Students will write code here */
	
	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		/* TODO 3: Check the checksum */
		if(ip_checksum(ip_hdr,sizeof(struct iphdr)) != 0)
			continue;
		/* TODO 4: Check TTL >= 1 */
		if(ip_hdr->ttl == 0)
			continue;
		/* TODO 5: Find best matching route (using the function you wrote at TODO 1) */
		struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);
		if(best_route == NULL)
			continue;
		/* TODO 6: Update TTL and recalculate the checksum */
		ip_hdr->ttl--;
		ip_hdr->check = 0; // trebuie facut 0 pentru a verifica daca la urmatorul hop s a corupt pachetul
		ip_hdr->check = ip_checksum(ip_hdr,sizeof(struct iphdr));
		/* TODO 7: Find matching ARP entry and update Ethernet addresses */
		get_interface_mac(best_route->interface,eth_hdr->ether_shost);
		struct arp_entry *best_arp = get_arp_entry(best_route->next_hop);
		if(!best_arp)
			continue;
		memcpy(eth_hdr->ether_dhost,best_arp->mac,sizeof(best_arp->mac));
		/* TODO 8: Forward the pachet to best_route->interface */
		//delete it
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

		//delete until here
		send_packet(best_route->interface,&m);
	}
	//do i have to free elements?
}