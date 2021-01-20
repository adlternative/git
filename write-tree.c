#include "cache.h"

/* 查看数据库是否有相应文件 */
static int check_valid_sha1(unsigned char *sha1)
{
	char *filename = sha1_file_name(sha1);
	int ret;

	/* If we were anal, we'd check that the sha1 of the contents actually matches */
	ret = access(filename, R_OK);
	if (ret)
		perror(filename);
	return ret;
}
/* 将数字转换成字符串添加到buffer中（注意这里是向前添加）*/
static int prepend_integer(char *buffer, unsigned val, int i)
{
	buffer[--i] = '\0';
	do {
		buffer[--i] = '0' + (val % 10);
		val /= 10;
	} while (val);
	return i;
}

#define ORIG_OFFSET (40)	/* Enough space to add the header of "tree <size>\0" */

int main(int argc, char **argv)
{
	unsigned long size, offset, val;
	int i, entries = read_cache();/* read index to active_cache */
	char *buffer;

	if (entries <= 0) {
		fprintf(stderr, "No file-cache to create a tree of\n");
		exit(1);
	}

	/* Guess at an initial size */
	size = entries * 40 + 400;
	buffer = malloc(size);
	offset = ORIG_OFFSET;

	for (i = 0; i < entries; i++) {
		struct cache_entry *ce = active_cache[i];
		if (check_valid_sha1(ce->sha1) < 0)
			exit(1);
		if (offset + ce->namelen + 60 > size) {
			size = alloc_nr(offset + ce->namelen + 60);
			buffer = realloc(buffer, size);
		}
    /* 100644 a.c\0sha1*/
		offset += sprintf(buffer + offset, "%o %s", ce->st_mode, ce->name);
		buffer[offset++] = 0;
		memcpy(buffer + offset, ce->sha1, 20);
		offset += 20;
	}
  /*
  size = sizeof(ce ce ce ce)
  --tree---size[ORIG_OFFSET](ce ce ce ce)
 */
	i = prepend_integer(buffer, offset - ORIG_OFFSET, ORIG_OFFSET);
	i -= 5;
	memcpy(buffer+i, "tree ", 5);

	buffer += i;
	offset -= i;
  /* 写到数据库中（压缩+计算sha1)*/
	write_sha1_file(buffer, offset);
	return 0;
}
