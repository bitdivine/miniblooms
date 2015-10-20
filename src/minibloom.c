#include <math.h>
#include <stdio.h>
#include <string.h>

#include "murmur.h"
#include "minibloom.h"

#define MAGIC  (0xfa1affe1L)
#define VOODOO (0x0009a9d1L)
#define SALT_CONSTANT (0x97c29b3a)

void minihead(minibloom_t*ans, unsigned int capacity, double error_rate){
	// df/dt==-f/M => f=Me^(-t/M)
	// Want max 1/4 bits to be set.  Hence at capacity:
	// f=(3/4)M=Me^(-C/M)=>M=-C/ln(3/4) and nblooms=ceil(ln(err)/ln(1/4))
	size_t bytesperbloom = ceil(capacity/(log(0.75)*-8));
	size_t roundup; for(roundup = 2;bytesperbloom>>=1;roundup<<=1); bytesperbloom = roundup;
	double tabe = 1.0 - exp(-((double)capacity)/(bytesperbloom*8));
	size_t nfuncs = ceil(log(error_rate)/log(tabe));
	memset((void *)ans,0,sizeof(minibloom_t));
	ans->bytesperbloom=bytesperbloom;
	ans->nfuncs=nfuncs;
	ans->size=sizeof(minibloom_t)+nfuncs*bytesperbloom;
	ans->error_rate=error_rate;
	ans->capacity=capacity;
	ans->sanity=VOODOO;
	ans->magic=MAGIC;
}

minibloom_t * minibloom(unsigned int capacity, double error_rate){
	minibloom_t * ans;
	minibloom_t head;
	minihead(&head, capacity, error_rate);
	ans = (minibloom_t *) malloc(head.size);
	if (NULL==ans) {
		fprintf(stderr
			,"ERROR: Could not allocate memory: %ld blooms @ %ld bytes each + a bit = %ld\n"
			 "	   Asked for capacity %ld at max error rate %f\n"
			, (unsigned long)head.nfuncs, (unsigned long)head.bytesperbloom, (unsigned long)head.size
			, (unsigned long)head.capacity, (double)head.error_rate
			);
		exit(2);
	}
	memset((void *)ans,0,head.size);
	memcpy((void *)ans,(void *)(&head),sizeof(minibloom_t));
	return ans;
}

int minicheck(minibloom_t* b){
	return (
	  (b->magic == MAGIC)
	&&(b->sanity == VOODOO)
	&&(b->size == sizeof(minibloom_t)+b->nfuncs*b->bytesperbloom)
	);
}

int miniset(minibloom_t *minibloom, const char *s, size_t len){
	int	i,c=0;
	size_t	bytesperbloom=minibloom->bytesperbloom;
	uint8_t*	bloom= minibloom->blooms;
	uint32_t	checksum[4];
	uint32_t	k;

	MurmurHash3_x64_128(s, len, SALT_CONSTANT, checksum);
	uint32_t h1 = checksum[0];
	uint32_t h2 = checksum[1];
	for (i = 0; i < minibloom->nfuncs; i++) {
		// TODO: sort.
		k = (h1 + i * h2) % (bytesperbloom<<3); // Duh.
		c+= 1&~(bloom[k>>3]>>(k&7));
		bloom[k>>3] |= 1<<(k&7);
		bloom += bytesperbloom;
	}
	minibloom->density+=c;
	if(c) minibloom->uniques++; // Cannot be computed cheaply post factum unlike the other two.
	minibloom->entries++;
	return c;
}

int miniget(minibloom_t *minibloom, const char *s, size_t len){
	int	i;
	size_t	bytesperbloom=minibloom->bytesperbloom;
	uint8_t*	bloom= minibloom->blooms;
	uint32_t	checksum[4];
	uint32_t	k;

	MurmurHash3_x64_128(s, len, SALT_CONSTANT, checksum);
	uint32_t h1 = checksum[0];
	uint32_t h2 = checksum[1];
	for (i = 0; i < minibloom->nfuncs; i++) {
		k = (h1 + i * h2) % (bytesperbloom<<3);
		if (!(bloom[k>>3] & (1<<(k&7)))) return 0;
		bloom += bytesperbloom;
	}
	return 1;
}

