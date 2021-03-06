#ifndef __BLOOM_H__
#define __BLOOM_H__
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint64_t magic;  // Set when making a bloom filter: 0xBABAC05E
    uint64_t sanity; // Reserved for a CRC.
    // External parameters:
    unsigned int capacity;   // Desired capacity.
    double       error_rate; // Desired error rate.
    // Internal parameters:
    size_t size;
    size_t nfuncs;
    size_t bytesperbloom;
    // Data:
    uint64_t entries;  // Insertions.
    uint64_t uniques;  // Entries that required setting at least one bit.
    uint64_t density;  // Bits set.
    // Memaligned.
    uint8_t blooms[0]; // Blooms proper.  Actual length == nfuncs * bytesperbloom.
} minibloom_t;


// Make a bloom filter:
minibloom_t *minibloom(unsigned int capacity, double error_rate); // Easy version.
minibloom_t *minibloom_make(minibloom_t*head);                    // Hard version.  You do the maths.


// Discard a bloom filter:
// ... a bloom filter is just a chunk of memory so use: free(thebloom);


// Work with a bloom filter:
int miniset(minibloom_t *minibloom, const char *s, size_t len);
int miniget(minibloom_t *minibloom, const char *s, size_t len);
int minicheck(minibloom_t *minibloom);
void minibloom_clear(minibloom_t* b);


// Work on a boom filter header:
void minihead(minibloom_t *ans, unsigned int capacity, double error_rate);
void minihead_init(minibloom_t *ans);
void minihead_fin(minibloom_t *ans);
void minihead_clone_params(minibloom_t*target, minibloom_t* src);
void minihead_clear(minibloom_t*ans);


#define MINIMAGIC  (0xfa1affe1L)
#define MINIVOODOO (0x97c29b3aL)

#endif
