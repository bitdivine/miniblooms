#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "minibloom.h"
#include "minibloomfile.h"
#define  MAX_LENGTH (1024)
#define  MINIBLOOMGLUE_CREATE  (1)
#define  MINIBLOOMGLUE_APPEND  (2)
#define  MIN(x,y)    ((x)<(y)?(x):(y))

static void usage(const char * progname){
	printf("Usage: <stuff %s -o outfile infile infile infile infile ...\n"
               "Or:    <stuff %s -a outfile infile infile infile infile ...\n"
		,progname, progname);
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

int main(int argc, char *argv[]){
	// Theign:
	minibloom_t*	bloom;
	minibloomfile_t bloomfile_in;
	minibloomfile_t bloomfile_out;
	minibloomfile_t*bloomfiles;
	// Thrall:
	char*		progname= argv[0];
	char*		outfilename;
	char*		infilename;
	int		ch;
	int		err;
	int		action = 0;
	int		i;

	while ((ch = getopt(argc, argv, "a:o:")) != -1) {
		switch (ch) {
		case 'a':
			action |= MINIBLOOMGLUE_APPEND;
			outfilename = optarg;
			break;
		case 'o':
			action |= MINIBLOOMGLUE_CREATE;
			outfilename = optarg;
			break;
		default:
			usage(progname);
			exit(0);
		}
	}
	argc -= optind;
	argv += optind;
	if (!argc){
		usage(progname);
		exit(1);
	}
	// One of create and append should be set:
	if (!(action & MINIBLOOMGLUE_CREATE) && !(action & MINIBLOOMGLUE_APPEND)){
		badarg("Need an output filename: -o outfile infile infile infile...");
	}
	// We need input files:
	if (0 == argc){
		printf("No input files.  Nothing to do.\n");
		exit (0);
	}
	infilename= argv[0]; // and successive entries.

	// MAKE OR LOAD OUTPUT FILE:
	if (action & MINIBLOOMGLUE_APPEND) {
		// Load existing bloom filter:
		err = miniload(&bloomfile_out, outfilename, action & MINIBLOOMGLUE_APPEND);
			if (err) die(err);
	} else {
		// Create a new bloom filter with the same parameters as the first input:
		err = miniload(&bloomfile_in, infilename, 0);
			if (err) die(err);
		err = miniblankclone(&bloomfile_out, outfilename, bloomfile_in.bloom);
			if (err) die(err);
		err = miniclose(&bloomfile_in);
			if (err) die(err);
	}
	bloom = bloomfile_out.bloom;

	// Load all the input files:
	bloomfiles = (minibloomfile_t *) malloc(sizeof(minibloomfile_t) * argc);
	for(i=0; i<argc; i++){
		err = miniload(&bloomfiles[i], argv[i], 0);
			if (err) {
				fprintf(stderr,"ERROR: failed to load %d/%d: %s\n", i+1, argc, argv[i]);
				die(err);
			}
			if (  (bloomfiles[i].bloom->nfuncs	!= bloom->nfuncs)
			   && (bloomfiles[i].bloom->bytesperbloom!= bloom->bytesperbloom)
			   )  {
				fprintf	(stderr
					, "ERROR: The bloom files don't all have the same parameters:\n"
					  " funcs  bytes/fun  filename\n"
					  "  % 3ld % 10ld %s\n"
					  "  % 3ld % 10ld %s\n"
					  "BAILING OUT...\n"
					, (uint64_t)bloomfiles[i].bloom->nfuncs, (uint64_t)bloomfiles[i].bloom->bytesperbloom, argv[i]
					, (uint64_t)bloomfiles[0].bloom->nfuncs, (uint64_t)bloomfiles[0].bloom->bytesperbloom, argv[0]
					);
			}
	}

	// Glue them all together:
	{
		uint64_t** pointers;
		uint64_t*  pointer;
		uint64_t   chunk;
		uint8_t    dribble;
		int i;
		size_t counter, size, limit;
		uint64_t   weight=0;
		pointer = (uint64_t *)(bloom->blooms);
		pointers = (uint64_t**) malloc(sizeof(uint64_t*)*argc);
		for (i=0; i<argc; i++){
			pointers[i] = (uint64_t*)(bloomfiles[i].bloom->blooms);
		}
		size  = bloom->bytesperbloom * bloom->nfuncs;
		limit = size>>3;
		// Octets:
		for (counter=0; counter<limit; counter++){
			chunk = pointers[0][counter];
			for(i=1; i<argc; i++){
				chunk |= pointers[i][counter];
			}
			pointer[counter] = chunk;
			weight += __builtin_popcountl(chunk);
		}
		// Bytes:
		for (counter <<=3;counter<size;counter++){
			dribble = bloomfiles[0].bloom->blooms[counter];
			for(i=1; i<argc; i++){
				dribble |= bloomfiles[i].bloom->blooms[counter];
			}
			bloom->blooms[counter] = dribble;
			weight += __builtin_popcountl(dribble);
		}
		bloom->density = weight;
		// Other values:
		bloom->entries = bloomfiles[0].bloom->entries;
		for (i=1; i<argc; i++){
			bloom->entries += bloomfiles[i].bloom->entries;
		}
	}

	// WRITE OR CLOSE:
	miniclose(&bloomfile_out);
	for (i=0;i<argc;i++){
		miniclose(&bloomfiles[i]);
	}

	// FIN
	return 0;
}
