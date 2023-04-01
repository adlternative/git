// SEEN
/*
 * csum-file.c
 *
 * Copyright (C) 2005 Linus Torvalds
 *
 * Simple file write infrastructure for writing SHA1-summed
 * files. Useful when you write a file that you want to be
 * able to verify hasn't been messed with afterwards.
 */
#include "cache.h"
#include "progress.h"
#include "csum-file.h"

/* 检查 hashfile 的 check_fd 读出来内容是否和 buf 相同 */
static void verify_buffer_or_die(struct hashfile *f,
				 const void *buf,
				 unsigned int count)
{
	ssize_t ret = read_in_full(f->check_fd, f->check_buffer, count);

	if (ret < 0)
		die_errno("%s: sha1 file read error", f->name);
	if (ret != count)
		die("%s: sha1 file truncated", f->name);
	if (memcmp(buf, f->check_buffer, count))
		die("sha1 file '%s' validation error", f->name);
}

/* 写数据 buf。需要校验则校验 */
static void flush(struct hashfile *f, const void *buf, unsigned int count)
{
	/* 先校验 check_fd 和 buf */
	if (0 <= f->check_fd && count)
		verify_buffer_or_die(f, buf, count);

	/* 然后写 buf */
	if (write_in_full(f->fd, buf, count) < 0) {
		if (errno == ENOSPC)
			die("sha1 file '%s' write error. Out of diskspace", f->name);
		die_errno("sha1 file '%s' write error", f->name);
	}

	f->total += count;
	display_throughput(f->tp, f->total);
}

/* 写数据 + 算哈希 f->buffer[:offset] */
void hashflush(struct hashfile *f)
{
	// offset 是 f->buffer 中数据的大小
	unsigned offset = f->offset;

	if (offset) {
		/* 一边算 hash 一边写 buffer[:offset] */
		the_hash_algo->update_fn(&f->ctx, f->buffer, offset);
		flush(f, f->buffer, offset);
		f->offset = 0;
	}
}

static void free_hashfile(struct hashfile *f)
{
	free(f->buffer);
	free(f->check_buffer);
	free(f);
}

/* 结束工作 1. 写完剩下的数据 2. 算文件 hash 并写入文件尾
3. fsync close fd close check_fd */
int finalize_hashfile(struct hashfile *f, unsigned char *result,
		      enum fsync_component component, unsigned int flags)
{
	int fd;

	/* 一边算 hash 一边写数据 data=f->buffer[:f->offset] */
	hashflush(f);
	/* hash 拿出来放在 f->buffer */
	the_hash_algo->final_fn(f->buffer, &f->ctx);
	/* hash 拷贝到 result */
	if (result)
		hashcpy(result, f->buffer);
	/* 写数据 data=hash */
	if (flags & CSUM_HASH_IN_STREAM)
		flush(f, f->buffer, the_hash_algo->rawsz);
	/* fsync */
	if (flags & CSUM_FSYNC)
		fsync_component_or_die(component, f->fd, f->name);
	/* close fd */
	if (flags & CSUM_CLOSE) {
		if (close(f->fd))
			die_errno("%s: sha1 file error on close", f->name);
		fd = 0;
	} else
		fd = f->fd;
	/* close check_fd */
	if (0 <= f->check_fd) {
		char discard;
		int cnt = read_in_full(f->check_fd, &discard, 1);
		if (cnt < 0)
			die_errno("%s: error when reading the tail of sha1 file",
				  f->name);
		if (cnt)
			die("%s: sha1 file has trailing garbage", f->name);
		if (close(f->check_fd))
			die_errno("%s: sha1 file error on close", f->name);
	}
	free_hashfile(f);
	return fd;
}

/* 写 buf 到 hashfile 更新 hash */
void hashwrite(struct hashfile *f, const void *buf, unsigned int count)
{
	while (count) {
		unsigned left = f->buffer_len - f->offset;
		unsigned nr = count > left ? left : count;

		/* 更新 crc32 */
		if (f->do_crc)
			f->crc32 = crc32(f->crc32, buf, nr);

		/* 说明这个 buf 足够大就不拷贝了，算个 hash 直接 flush 写数据 */
		if (nr == f->buffer_len) {
			/*
			 * Flush a full batch worth of data directly
			 * from the input, skipping the memcpy() to
			 * the hashfile's buffer. In this block,
			 * f->offset is necessarily zero.
			 */
			the_hash_algo->update_fn(&f->ctx, buf, nr);
			flush(f, buf, nr);
		} else {
			/*
			 * Copy to the hashfile's buffer, flushing only
			 * if it became full.
			 */
			/* 否则拷贝到 f->buffer 中，等到满了，再写数据 */
			memcpy(f->buffer + f->offset, buf, nr);
			f->offset += nr;
			left -= nr;
			if (!left)
				hashflush(f);
		}

		count -= nr;
		buf = (char *) buf + nr;
	}
}


/* 初始化 128k buffer 的 hashfile +
check_buffer + check_fd=name
fd = /dev/null */

// 就比如 Name = xxx.idx 则 checkfd 就是它
// 之后在做校验的时候会把数据写到 /dev/null
struct hashfile *hashfd_check(const char *name)
{
	int sink, check;
	struct hashfile *f;

	sink = xopen("/dev/null", O_WRONLY);
	check = xopen(name, O_RDONLY);
	f = hashfd(sink, name);
	f->check_fd = check;
	f->check_buffer = xmalloc(f->buffer_len);

	return f;
}

/* 初始化了一个 hashfile */
static struct hashfile *hashfd_internal(int fd, const char *name,
					struct progress *tp,
					size_t buffer_len)
{
	struct hashfile *f = xmalloc(sizeof(*f));
	f->fd = fd;
	f->check_fd = -1;
	f->offset = 0;
	f->total = 0;
	f->tp = tp;
	f->name = name;
	f->do_crc = 0;
	the_hash_algo->init_fn(&f->ctx);

	f->buffer_len = buffer_len;
	f->buffer = xmalloc(buffer_len);
	f->check_buffer = NULL;

	return f;
}

/* 初始化 128k buffer 的 hashfile */
struct hashfile *hashfd(int fd, const char *name)
{
	/*
	 * Since we are not going to use a progress meter to
	 * measure the rate of data passing through this hashfile,
	 * use a larger buffer size to reduce fsync() calls.
	 */
	/*
	 * 由于我们不打算用一个进度表来
	 * 通过这个哈希文件的速度测量数据。
	 * 使用更大的缓冲区来减少fsync()的调用。
	 */
	/* 这里用 128k */
	return hashfd_internal(fd, name, NULL, 128 * 1024);
}

/* 初始化 8k buffer 的 hashfile 带进度条  */
struct hashfile *hashfd_throughput(int fd, const char *name, struct progress *tp)
{
	/*
	 * Since we are expecting to report progress of the
	 * write into this hashfile, use a smaller buffer
	 * size so the progress indicators arrive at a more
	 * frequent rate.
	 */
	/*
	 * 因为我们期望报告的是
	 * 写入这个哈希文件，使用一个较小的缓冲区
	 * 这样进度指示器就会以更快的速度到达
	 */
	return hashfd_internal(fd, name, tp, 8 * 1024);
}

/*  在 checkpoint 记录当前 f 中已经写入的大小和哈希 */
void hashfile_checkpoint(struct hashfile *f, struct hashfile_checkpoint *checkpoint)
{
	/* 写数据 + 算哈希 */
	hashflush(f);
	/* 在 checkpoint 记录当前 f 中已经写入的大小 */
	checkpoint->offset = f->total;
	/* 在 checkpoint 记录当前 f 已经计算的哈希 */
	the_hash_algo->clone_fn(&checkpoint->ctx, &f->ctx);
}

/* 截断 hashfile 到 checkpoint->offset 长度 */
int hashfile_truncate(struct hashfile *f, struct hashfile_checkpoint *checkpoint)
{
	off_t offset = checkpoint->offset;

	if (ftruncate(f->fd, offset) ||
	    lseek(f->fd, offset, SEEK_SET) != offset)
		return -1;
	f->total = offset;
	f->ctx = checkpoint->ctx;
	f->offset = 0; /* hashflush() was called in checkpoint */
	return 0;
}

/* 表示启用 crc32 计算 */
void crc32_begin(struct hashfile *f)
{
	f->crc32 = crc32(0, NULL, 0);
	f->do_crc = 1;
}

/* 表示关闭 crc32 计算，并返回 crc32 结果 */
uint32_t crc32_end(struct hashfile *f)
{
	f->do_crc = 0;
	return f->crc32;
}

/* HASHFILE 最后会存它的 checksum，
因此我们可以通过整个文件计算 checksum 和
最后的 checksum 来进行校验 */
int hashfile_checksum_valid(const unsigned char *data, size_t total_len)
{
	unsigned char got[GIT_MAX_RAWSZ];
	git_hash_ctx ctx;
	size_t data_len = total_len - the_hash_algo->rawsz;

	if (total_len < the_hash_algo->rawsz)
		return 0; /* say "too short"? */

	the_hash_algo->init_fn(&ctx);
	the_hash_algo->update_fn(&ctx, data, data_len);
	the_hash_algo->final_fn(got, &ctx);

	return hasheq(got, data + data_len);
}
