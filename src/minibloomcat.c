#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "minibloom.h"
#include "minibloomfile.h"
#define  MAX_LENGTH (1024)
#define  MINIBLOOMDO_CREATE  (1)
#define  MINIBLOOMDO_READ    (2)
#define  MINIBLOOMDO_APPEND  (4)
#define  MINIBLOOMDO_GREP    (8)
#define  MINIBLOOMDO_GREPV   (16)
#define  MINIBLOOMDO_STATS   (32)
#define  MINIBLOOMDO_STATSF  (64)
#define  MINIBLOOMDO_BOOL    (128)
#define  MINIBLOOMDO_CALCE   (256)
#define  MIN(x,y)    ((x)<(y)?(x):(y))

static void usage(const char * progname){
	printf("Usage: <stuff %s -c100 -e0.001 outfile\n"
               "Or:    <stuff %s -c1000 -U500000 -W0.1 outfile\n"
		"Options:\n"
		" -a    Append to an existing bloom filter.\n"
		" -b    Print every line from stdin followed by 1/0.\n"
		" -c    Capacity == approximate number of uniques.\n"
		" -e    Error rate == probability of accepting a random element.\n"
		" -g    Get the lines from stdin that are in the filter.\n"
		" -G    Get the lines from stdin that are not in the filter.\n"
		" -j    Print stats as JSON and exit.\n"
		" -s    Print stats before doing anything else.\n"
		" -S    Print stats when done.\n"
		" -U    Size of universe - use in combination with -c and -W\n"
		" -W    Waste == of accepted entries, what proportion may be wrong?\n"
		,progname);
}

static void die(int code){
	switch(code){
	case MINIERR_OPEN: fprintf(stderr,"ERROR when opening minibloom file.\n");break;
	case MINIERR_WRITE: fprintf(stderr,"ERROR when writing minibloom file.\n");break;
	case MINIERR_CLOSE: fprintf(stderr,"ERROR when closing minibloom file.\n");break;
	case MINIERR_CORRUPT: fprintf(stderr,"ERROR corrupt minibloom file.\n");break;
	case MINIERR_INCONSISTENT: fprintf(stderr,"ERROR inconsistent minibloom file.\n");break;
	case MINIERR_MMAP: fprintf(stderr,"ERROR mmapping minibloom file.\n");break;
	case MINIERR_MUNMAP: fprintf(stderr,"ERROR unmapping minibloom file.\n");break;
	case MINIERR_MAGIC_FILE: fprintf(stderr,"ERROR magic number doesn't match for minibloom file handle.\n");break;
	default: fprintf(stderr,"ERROR %d\n",code);
	}
	exit(code);
}

static void badarg(const char * message){
	fprintf(stderr, "ERROR: %s\n", message);
	exit(2);
}

static void stats(FILE* fp, minibloom_t* bloom, char format){
	uint64_t bits = bloom->nfuncs*bloom->bytesperbloom*8;
	double density = bloom->density/((double)bits);
	double err_rate = 1;
	int i;
	for (i=0; i<bloom->nfuncs; i++) err_rate *= density;
	uint64_t approx_real_uniques;
	char     approx_type;
	// As the real uniques approaches capacity squared we can't distinguish real uniques any more.
	if (density>0.999999){ // Overwhelmed.
		approx_type         = 'o';
		approx_real_uniques = MIN(bloom->entries,bloom->capacity*bloom->capacity+1000);
	} else
	if (density<0.001){ // Underwhelmed.  Observed uniques is probably accurate.
		approx_type         = 'u';
		approx_real_uniques = bloom->uniques;
	} else { // In between.  Use the density.
		approx_type         = 'l';
		approx_real_uniques = (-log(1.0-density) * bits)/bloom->nfuncs;
	}
	double expected_density = 1.0 - exp(-((double)bloom->capacity)/(bloom->bytesperbloom*8));

	switch(format){
	case 'j':
	fprintf(fp,
		"{ \"spec_capacity\":    %u\n"
		", \"spec_err_rate\":    %f\n"
		", \"total_bytes\":      %ld\n"
		", \"nblooms\":          %ld\n"
		", \"bytes_per_bloom\":  %ld\n"
                ", \"expected_density\": %f\n"
		", \"entries\":          %ld\n"
		", \"density\":          %f\n"
		", \"bloom_bits_set\":   %ld\n"
		", \"bloom_bits\":       %ld\n"
		", \"observed_uniques\": %lu\n"
		", \"approx_real_uniq\": %lu\n"
		", \"approx_type\":      \"%c\"\n"
		", \"approx_err_rate\":  %f\n"
		"}\n"
		, bloom->capacity
		, bloom->error_rate
		, bloom->size
		, bloom->nfuncs, bloom->bytesperbloom
		, expected_density
		, bloom->entries
		, density, bloom->density, bits
		, bloom->uniques
		, approx_real_uniques, approx_type
		, err_rate
		);

		break;
	default:
	fprintf(fp,
		"  spec_capacity:    %u\n"
		"  spec_err_rate:    %f\n"
		"  total_size:       %ld  (bytes)\n"
		"  #blooms:          %ld\n"
		"  bloomsize:        %ld  (bytes)\n"
		"  expected_density: %f\n"
		"  entries:          %ld\n"
		"  density:          %f  (%ld/%ld)\n"
		"  observed_uniques: %lu\n"
		"  approx_real_uniq: %lu %c\n"
		"  approx_err_rate:  %f\n"
		, bloom->capacity
		, bloom->error_rate
		, bloom->size
		, bloom->nfuncs, bloom->bytesperbloom
		, expected_density
		, bloom->entries
		, density, bloom->density, bits
		, bloom->uniques
		, approx_real_uniques, approx_type
		, err_rate
		);
	}
}

int main(int argc, char *argv[]){
	// Theign:
	unsigned int	capacity   = 1000;
	double		maxerrprob = 0.001;
	minibloom_t*	bloom;
	minibloomfile_t bloomfile;
	uint64_t	universe = 5000000000;
	double		maxwaste = 0.1;
	// Thrall:
	char*		progname= argv[0];
	char*		filename;
	char		buf[MAX_LENGTH];
	int		ch;
	int		err;
	int		action = 0;
	char		format = 't';

	while ((ch = getopt(argc, argv, "hc:e:U:W:abugvtjsS")) != -1) {
		switch (ch) {
		case 'a':
			action |= MINIBLOOMDO_APPEND;
			break;
		case 'b':
			action |= MINIBLOOMDO_BOOL;
			break;
		case 'c':
			action |= MINIBLOOMDO_CREATE;
			capacity = atoi(optarg);
			if (capacity<1) badarg("Capacity must be greater than 1.");
			break;
		case 'e':
			action |= MINIBLOOMDO_CREATE;
			maxerrprob = atof(optarg);
			if (maxerrprob>1) badarg("Error probability must be less than 1.");
			break;
		case 'g':
			action |= MINIBLOOMDO_GREP;
			break;
		case 'G':
			action |= MINIBLOOMDO_GREPV;
			break;
		case 'j':
			action |= MINIBLOOMDO_STATS;
			format = 'j';
		case 's':
			action |= MINIBLOOMDO_STATS;
			break;
		case 'S':
			action |= MINIBLOOMDO_STATSF;
			break;
		case 'u':
			// Dummy option for compatibility
			break;
		case 'U':
			action |= MINIBLOOMDO_CREATE | MINIBLOOMDO_CALCE;
			universe = atoll(optarg);
			if (universe<1) badarg("Universe must be greater than 1.");
			break;
		case 'W':
			action |= MINIBLOOMDO_CREATE | MINIBLOOMDO_CALCE;
			maxwaste = atof(optarg);
			if (maxwaste>1) badarg("Wastage probability must be less than 1.");
			maxwaste = MIN(maxwaste, 0.95);
			break;
		case 'h':
		default:
			usage(progname);
			exit(0);
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1){
		usage(progname);
		exit(1);
	}
	filename= argv[0];
	// Precisely one of create, read and append should be set:
	if ((action&MINIBLOOMDO_CREATE) && (action&MINIBLOOMDO_APPEND)){
		badarg("Cannot change parameters of an existing bloom filter.");
	}
	if (!((action&MINIBLOOMDO_CREATE) || (action&MINIBLOOMDO_APPEND))){
		action |= MINIBLOOMDO_READ;
	}

	// Calculate parameters?
	if (action & MINIBLOOMDO_CALCE) {
		if (capacity >= universe) maxerrprob = 0.99999;
		else maxerrprob = (maxwaste * capacity)/((1.0-maxwaste) * (universe-capacity));
	}

	// MAKE OR LOAD
	if (action & MINIBLOOMDO_CREATE) {
		// Create bloom filter in memory:
		err = minimake(&bloomfile, filename, capacity, maxerrprob);
	} else {
		// Load from file:
		err = miniload(&bloomfile, filename, action & MINIBLOOMDO_APPEND);
	}
	if (err) die(err);
	bloom = bloomfile.bloom;

	// Print stats
	if (action & MINIBLOOMDO_STATS) {
		if (format=='j') {
			stats(stdout, bloom, format);
			exit(0);
		}
		printf("%s '%s' %s:\n"
			, ((action & MINIBLOOMDO_CREATE)?"Made":"Loaded")
			, filename
			, ((action & (MINIBLOOMDO_CREATE|MINIBLOOMDO_APPEND))?"read/write":"readonly")
			);
		stats(stdout, bloom, format);
	}

	// POPULATE/PROCESS STDIN:
	if (action & (MINIBLOOMDO_CREATE|MINIBLOOMDO_APPEND|MINIBLOOMDO_GREP|MINIBLOOMDO_GREPV|MINIBLOOMDO_BOOL))
	while (fgets(buf, sizeof(buf), stdin)) {
		char *pos = strchr(buf, '\n');
		if (NULL == pos) continue;
		*pos = '\0';
		if (action & (MINIBLOOMDO_GREP | MINIBLOOMDO_GREPV)){
			int extant = miniget(bloom, buf, pos-buf);
			if ((extant?MINIBLOOMDO_GREPV:0) ^ (action & MINIBLOOMDO_GREPV)) {
				printf("%.*s\n", (int)(pos-buf), buf);
			}
		}
		if (action & MINIBLOOMDO_BOOL){
			int extant = miniget(bloom, buf, pos-buf);
			printf("%.*s %d\n", (int)(pos-buf), buf, extant);
		}
		if (action&(MINIBLOOMDO_CREATE | MINIBLOOMDO_APPEND)) {
			miniset(bloom, buf, pos-buf);
		}
	}

	// Print stats
	if (action & MINIBLOOMDO_STATSF) {
		printf("Closing bloom filter '%s':\n", filename);
		stats(stdout, bloom, format);
	}

	// WRITE OR CLOSE:
	miniclose(&bloomfile);

	// FIN
	return 0;
}
