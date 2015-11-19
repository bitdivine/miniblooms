#include <stdlib.h>
#include <stdio.h>
// man 3 open:
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// man 3 mmap:
#include <sys/mman.h>
// man 2 stat:
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
// man memset
#include <string.h>
// local:
#include "minibloomfile.h"

// Take existing memory and dump it to a new file descriptor.  Not memory mapped or any of that jazz.
int minidump(const char* filename, minibloom_t*mb){
	FILE * fd;
	// Seriously??  I thought I could make a new mmapped file with a pointer to a bunch of mem.  Am I misremembering?
	// Do I actually have to copy??
	if (NULL == (fd = fopen(filename,"wb"))) return MINIERR_OPEN;
	return minidumpf(fd, mb);
}
int minidumpf(FILE* fp, minibloom_t*mb){
	if (fwrite(mb, mb->size, 1, fp) != 1) return MINIERR_WRITE;
	if (fclose(fp)) return MINIERR_CLOSE;
	return MINIERR_OK;
}
int minidumpfd(int fd, minibloom_t*mb){
	if (mb->size != write(fd,(void*)mb, mb->size)) return MINIERR_WRITE;
	return MINIERR_OK;
}

// Create a new memory mapped bloom filter
int minimake(minibloomfile_t* ans, const char* filename, unsigned int capacity, double error_rate){
	int fd = open(filename, O_RDWR|O_CREAT, 0666);
	return minimakefd(ans, fd, capacity, error_rate);
}
int minimakef(minibloomfile_t* ans, FILE* fp, unsigned int capacity, double error_rate){
	return minimakefd(ans, fileno(fp), capacity, error_rate);
}
int minimakefd(minibloomfile_t* ans, int fd, unsigned int capacity, double error_rate){
	minibloom_t head;
	minihead(&head, capacity, error_rate);
	return miniblankclonefd(ans, fd, &head);
}

// Create a new bloom filter using all the parameters (but none of the data) of an existing one:
int miniblankclone(minibloomfile_t* ans, const char* filename, minibloom_t* head){
	int fd = open(filename, O_RDWR|O_CREAT, 0666);
	return miniblankclonefd(ans, fd, head);
}
int miniblankclonef(minibloomfile_t* ans, FILE* fp, minibloom_t* head){
	return miniblankclonefd(ans, fileno(fp), head);
}
int miniblankclonefd(minibloomfile_t* ans, int fd, minibloom_t* head){
	minibloom_t* bloom;
	size_t page_size = getpagesize();
	size_t total_size = head->size + ((page_size-1)&(page_size-head->size));
	if (ftruncate(fd,(off_t)total_size)) return MINIERR_WRITE;
	bloom = (minibloom_t*) mmap(NULL, total_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (bloom == MAP_FAILED) return MINIERR_MMAP;
	memset((void*)bloom,0,total_size);
	minihead_init(bloom);
	minihead_clone_params(bloom,head);
	minihead_fin(bloom);
	ans->fd = fd;
	ans->bloom = bloom;
	ans->writable=1;
	ans->magic=MINIBLOOMFILE_MAGIC;
	return MINIERR_OK;
}

// Load an existing bloom filter from a file.
int miniload(minibloomfile_t* ans, const char*filename, int append){
	int fd = open (filename, O_RDWR, S_IRUSR | (append?S_IWUSR:0));
	if (-1 == fd) return MINIERR_OPEN;
	return miniloadfd(ans, fd, append);
}
int miniloadf(minibloomfile_t* ans, FILE* fp, int append){
	return miniloadfd(ans, fileno(fp), append);
}
int miniloadfd(minibloomfile_t* ans, int fd, int append){
	size_t filesize;
	struct stat filestats;
	minibloom_t* bloom;

	// Get size of file:
	fstat(fd, &filestats);
	filesize = filestats.st_size;

	// Check:
	if (filesize < sizeof(minibloom_t)){
		close(fd);
		return MINIERR_CORRUPT;
	}

	// Map:
	bloom = (minibloom_t*) mmap (0, filesize, PROT_READ | (append?PROT_WRITE:0),MAP_SHARED, fd, 0);

	// Check:
	if (MAP_FAILED == ans){
		close(fd);
		return MINIERR_MMAP;
	}
	if (!minicheck(bloom)){
		munmap (bloom, filesize);
		close(fd);
		return MINIERR_INCONSISTENT;
	}
	if (filesize < bloom->size){
		munmap (bloom, filesize);
		close(fd);
		return MINIERR_CORRUPT;
	}
	ans->fd = fd;
	ans->bloom = bloom;
	ans->writable = append;
	ans->magic = MINIBLOOMFILE_MAGIC;
	return MINIERR_OK;
}

int miniclose(minibloomfile_t*b){
	if (0 != munmap(b->bloom, b->bloom->size)) return MINIERR_MUNMAP;
	if (0 != close(b->fd)) return MINIERR_CLOSE;
	return MINIERR_OK;
}

int minicheckfilehandle(minibloomfile_t*bf){
	if (MINIBLOOMFILE_MAGIC != bf->magic) return MINIERR_MAGIC_FILE;
	return MINIERR_OK;
}

