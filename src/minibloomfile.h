#ifndef __MINIBLOOMFILE_H__
#define __MINIBLOOMFILE_H__

#include <stdio.h>
#include "minibloom.h"

#define MINIBLOOMFILE_MAGIC (0xf1940529)

typedef struct {
	int 		fd;
	minibloom_t *	bloom;
	int		writable;
	int		magic;
} minibloomfile_t;

int	minidump(const char*filename, minibloom_t*mb);
int	minimake(minibloomfile_t* ans, const char*filename, unsigned int capacity, double error_rate);
int	miniload(minibloomfile_t* ans, const char*filename, int append);
int	miniclose(minibloomfile_t*);

int	minidumpf(FILE* fp, minibloom_t*mb);
int	minimakef(minibloomfile_t* ans, FILE* fp, unsigned int capacity, double error_rate);
int	miniloadf(minibloomfile_t* ans, FILE* fp, int append);

int	minidumpfd(int fd, minibloom_t*mb);
int	minimakefd(minibloomfile_t* ans, int fd, unsigned int capacity, double error_rate);
int	miniloadfd(minibloomfile_t* ans, int fd, int append);

int	minicheckfilehandle(minibloomfile_t*bf);

#define MINIERR_OK (0)
#define MINIERR_OPEN (1)
#define MINIERR_WRITE (2)
#define MINIERR_CLOSE (3)
#define MINIERR_CORRUPT (4)
#define MINIERR_INCONSISTENT (5)
#define MINIERR_MMAP (6)
#define MINIERR_MUNMAP (7)
#define MINIERR_MAGIC_FILE (8)

#endif
