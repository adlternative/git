#include "cache.h"

/* 比较缓存项的名字 */
static int cache_name_compare(const char *name1, int len1, const char *name2, int len2)
{
	int len = len1 < len2 ? len1 : len2;
	int cmp;

	cmp = memcmp(name1, name2, len);
	if (cmp)
		return cmp;
	if (len1 < len2)
		return -1;
	if (len1 > len2)
		return 1;
	return 0;
}

/* 在缓存中二分搜索“可插入”的位置 */
static int cache_name_pos(const char *name, int namelen)
{
	int first, last;

	first = 0;
	last = active_nr;
	while (last > first) {
		int next = (last + first) >> 1;
		struct cache_entry *ce = active_cache[next];
		int cmp = cache_name_compare(name, namelen, ce->name, ce->namelen);
		if (!cmp)
			return -next-1;
    /* 如果缓存中有该文件，
    返回一个和next有关的负数*/
		if (cmp < 0) {
			last = next;
			continue;
		}
		first = next+1;
	}
	return first;
}

/* 在缓存中删除某项 */
static int remove_file_from_cache(char *path)
{
  /* 在缓存中找该项的坐标 */
	int pos = cache_name_pos(path, strlen(path));
	if (pos < 0) {
		pos = -pos-1;
		active_nr--;
    /* 从缓存删除 */
		if (pos < active_nr)
			memmove(active_cache + pos, active_cache + pos + 1, (active_nr - pos - 1) * sizeof(struct cache_entry *));
	}
}

/* 在缓存中添加某项 */
static int add_cache_entry(struct cache_entry *ce)
{
	int pos;

	pos = cache_name_pos(ce->name, ce->namelen);
  /* 缓存中有该项，则替换 */
	/* existing match? Just replace it */
	if (pos < 0) {
		active_cache[-pos-1] = ce;
		return 0;
	}

  /* 若容量不够就动态扩容 */
	/* Make sure the array is big enough .. */
	if (active_nr == active_alloc) {
		active_alloc = alloc_nr(active_alloc);
		active_cache = realloc(active_cache, active_alloc * sizeof(struct cache_entry *));
	}
  /* 添加到缓存中 */
	/* Add it in.. */
	active_nr++;
	if (active_nr > pos)
		memmove(active_cache + pos + 1, active_cache + pos, (active_nr - pos - 1) * sizeof(ce));
	active_cache[pos] = ce;
	return 0;
}

/* 将文件数据写入数据库 */
static int index_fd(const char *path, int namelen, struct cache_entry *ce, int fd, struct stat *st)
{
	z_stream stream;
  /*最大输出长度=len(文件名)+len(文件内容)+200 */
	int max_out_bytes = namelen + st->st_size + 200;
	void *out = malloc(max_out_bytes);
	void *metadata = malloc(namelen + 200);
	void *in = mmap(NULL, st->st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	SHA_CTX c;

	close(fd);
	if (!out || (int)(long)in == -1)
		return -1;

	memset(&stream, 0, sizeof(stream));
	deflateInit(&stream, Z_BEST_COMPRESSION);/* 初始化压缩流 */

	/*
	 * ASCII size + nul byte
	 */	
	stream.next_in = metadata;/*blob filesize*/
  /* 需要压缩的数据的大小 */
	stream.avail_in = 1+sprintf(metadata, "blob %lu", (unsigned long) st->st_size);
	stream.next_out = out;/* 输出的内容 */
	stream.avail_out = max_out_bytes;/* 输出的大小 */
	while (deflate(&stream, 0) == Z_OK)/* 进行压缩 */
		/* nothing */;

	/*
	 * File content
	 */
  /* 文件内容 */
	stream.next_in = in;/* 从文件中读取 */
	stream.avail_in = st->st_size;/* 需要压缩的数据大小 */
	while (deflate(&stream, Z_FINISH) == Z_OK)/* 进行压缩 */
		/*nothing */;

	deflateEnd(&stream);
	/* 将压缩后的内容,计算sha1*/
	SHA1_Init(&c);
	SHA1_Update(&c, out, stream.total_out);
	SHA1_Final(ce->sha1, &c);
  /* 写入sha1对应文件  */
	return write_sha1_buffer(ce->sha1, out, stream.total_out);
}

/* 更新文件path到缓存中 */
static int add_file_to_cache(char *path)
{
	int size, namelen;
	struct cache_entry *ce;
	struct stat st;
	int fd;

	fd = open(path, O_RDONLY);/*比如 a.txt */
	if (fd < 0) {
		if (errno == ENOENT)
			return remove_file_from_cache(path);/* 本地没有，则从缓存中删除 */
		return -1;
	}
	if (fstat(fd, &st) < 0) {
		close(fd);
		return -1;
	}
	namelen = strlen(path);/* 文件名长度 */
  /* 貌似这个ce的长度是可变的因为最后的文件名可长可短，
  */
	size = cache_entry_size(namelen);
	ce = malloc(size);
	memset(ce, 0, size);
	memcpy(ce->name, path, namelen);
	ce->ctime.sec = st.st_ctime;
	ce->ctime.nsec = st.st_ctim.tv_nsec;
	ce->mtime.sec = st.st_mtime;
	ce->mtime.nsec = st.st_mtim.tv_nsec;
	ce->st_dev = st.st_dev;
	ce->st_ino = st.st_ino;
	ce->st_mode = st.st_mode;
	ce->st_uid = st.st_uid;
	ce->st_gid = st.st_gid;
	ce->st_size = st.st_size;
	ce->namelen = namelen;
  /* 将文件添加到数据库 */
  /* "a.txt",len("a.txt"),ce of a.txt,fd fd a.txt, stat of a.txt*/
	if (index_fd(path, namelen, ce, fd, &st) < 0)
  	return -1;
  /* 将项添加到缓存 */
	return add_cache_entry(ce);
}

/* 将header和每一项写入到inedx.lock的临时文件中 */
static int write_cache(int newfd, struct cache_entry **cache, int entries)
{
	SHA_CTX c;
	struct cache_header hdr;
	int i;
  /* 头部信息填写 */
	hdr.signature = CACHE_SIGNATURE;/*签名 */
	hdr.version = 1;/* 版本 */
	hdr.entries = entries;/* 项数 */

  /* 计算 (hdr内容(不包括sha1)+所有缓存项)的sha1,作为hdr的sha1 */
	SHA1_Init(&c);
	SHA1_Update(&c, &hdr, offsetof(struct cache_header, sha1));
	for (i = 0; i < entries; i++) {
		struct cache_entry *ce = cache[i];
		int size = ce_size(ce);
		SHA1_Update(&c, ce, size);
	}
	SHA1_Final(hdr.sha1, &c);
  /* 将头部信息写入到index.lock中 */
	if (write(newfd, &hdr, sizeof(hdr)) != sizeof(hdr))
		return -1;
  /* 将每一个缓存项都写入到.index.lock中 */
	for (i = 0; i < entries; i++) {
		struct cache_entry *ce = cache[i];
		int size = ce_size(ce);
		if (write(newfd, ce, size) != size)
			return -1;
	}
	return 0;
}		

/*
 * We fundamentally don't like some paths: we don't want
 * dot or dot-dot anywhere, and in fact, we don't even want
 * any other dot-files (.dircache or anything else). They
 * are hidden, for chist sake.
 *
 * Also, we don't want double slashes or slashes at the
 * end that can make pathnames ambiguous. 
 */
static int verify_path(char *path)
{
	char c;

	goto inside;
	for (;;) {
		if (!c)
			return 1;
		if (c == '/') {
inside:
			c = *path++;
			if (c != '/' && c != '.' && c != '\0')
				continue;
			return 0;
		}
		c = *path++;
	}
}

int main(int argc, char **argv)
{
	int i, newfd, entries;

	entries = read_cache();/* get index  */
	if (entries < 0) {
		perror("cache corrupted");
		return -1;
	}

	newfd = open(".dircache/index.lock", O_RDWR | O_CREAT | O_EXCL, 0600);
	if (newfd < 0) {
		perror("unable to create new cachefile");
		return -1;
	}
	for (i = 1 ; i < argc; i++) {
		char *path = argv[i];
    /*  验证路径的正确*/
		if (!verify_path(path)) {
			fprintf(stderr, "Ignoring path %s\n", argv[i]);
			continue;
		}
    /* 添加文件项进缓存（还会写入到数据库中） */
		if (add_file_to_cache(path)) {
			fprintf(stderr, "Unable to add %s to database\n", path);
			goto out;
		}
	}
  /* 将缓存写入到index.lock文件,
  再用index代替index.lock */
	if (!write_cache(newfd, active_cache, active_nr) && !rename(".dircache/index.lock", ".dircache/index"))
		return 0;
out:
	unlink(".dircache/index.lock");
}
