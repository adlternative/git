#include "cache.h"

#include <pwd.h>
#include <time.h>

#define BLOCKING (1ul << 14)
#define ORIG_OFFSET (40)

/*
 * Leave space at the beginning to insert the tag
 * once we know how big things are.
 *
 * FIXME! Share the code with "write-tree.c"
 */
/* 初始化缓冲区 */
static void init_buffer(char **bufp, unsigned int *sizep)
{
	char *buf = malloc(BLOCKING);
	memset(buf, 0, ORIG_OFFSET);
	*sizep = ORIG_OFFSET;
	*bufp = buf;
}

/* 添加内容到缓冲区 */
static void add_buffer(char **bufp, unsigned int *sizep, const char *fmt, ...)
{
	char one_line[2048];
	va_list args;
	int len;
	unsigned long alloc, size, newsize;
	char *buf;

	va_start(args, fmt);
	len = vsnprintf(one_line, sizeof(one_line), fmt, args);
	va_end(args);
	size = *sizep;
	newsize = size + len;
	alloc = (size + 32767) & ~32767;
	buf = *bufp;
	if (newsize > alloc) {
		alloc = (newsize + 32767) & ~32767;   
		buf = realloc(buf, alloc);
		*bufp = buf;
	}
	*sizep = newsize;
	memcpy(buf + size, one_line, len);
}

/* 将内容的大小放在buffer[i]最前面,这里就是将commit对象的所有信息填好以后，
  将后面的信息放在buffer的开头 */
static int prepend_integer(char *buffer, unsigned val, int i)
{
	buffer[--i] = '\0';
	do {
		buffer[--i] = '0' + (val % 10);
		val /= 10;
	} while (val);
	return i;
}

/* 结束缓冲区并使用prepend_integer去填充Size,tag填充commit */
static void finish_buffer(char *tag, char **bufp, unsigned int *sizep)
{
	int taglen;
	int offset;
	char *buf = *bufp;
	unsigned int size = *sizep;

	offset = prepend_integer(buf, size - ORIG_OFFSET, ORIG_OFFSET);
	taglen = strlen(tag);
	offset -= taglen;
	buf += offset;
	size -= offset;
	memcpy(buf, tag, taglen);

	*bufp = buf;
	*sizep = size;
}

/* 去除特殊字符：\n,<,> */
static void remove_special(char *p)
{
	char c;
	char *dst = p;

	for (;;) {
		c = *p;
		p++;
		switch(c) {
		case '\n': case '<': case '>':
			continue;
		}
		*dst++ = c;
		if (!c)
			break;
	}
}

/*
 * Having more than two parents may be strange, but hey, there's
 * no conceptual reason why the file format couldn't accept multi-way
 * merges. It might be the "union" of several packages, for example.
 *
 * I don't really expect that to happen, but this is here to make
 * it clear that _conceptually_ it's ok..
 */
#define MAXPARENT (16)

int main(int argc, char **argv)
{
	int i, len;
	int parents = 0;
	unsigned char tree_sha1[20];
	unsigned char parent_sha1[MAXPARENT][20];
	char *gecos, *realgecos;
	char *email, realemail[1000];
	char *date, *realdate;
	char comment[1000];
	struct passwd *pw;
	time_t now;
	char *buffer;
	unsigned int size;

	if (argc < 2 || get_sha1_hex(argv[1], tree_sha1) < 0)
		usage("commit-tree <sha1> [-p <sha1>]* < changelog");
  /* 有-p指定父提交 */
	for (i = 2; i < argc; i += 2) {
		char *a, *b;
		a = argv[i]; b = argv[i+1];
		if (!b || strcmp(a, "-p") || get_sha1_hex(b, parent_sha1[parents]))
			usage("commit-tree <sha1> [-p <sha1>]* < changelog");
		parents++;
	}
  /* 没有-p首次提交 */
	if (!parents)
		fprintf(stderr, "Committing initial tree %s\n", argv[1]);
	pw = getpwuid(getuid());
	if (!pw)
		usage("You don't exist. Go away!");
	realgecos = pw->pw_gecos;
	len = strlen(pw->pw_name);
	memcpy(realemail, pw->pw_name, len);
	realemail[len] = '@';
	gethostname(realemail+len+1, sizeof(realemail)-len-1);
	time(&now);
	realdate = ctime(&now);

	gecos = getenv("COMMITTER_NAME") ? : realgecos;
	email = getenv("COMMITTER_EMAIL") ? : realemail;
	date = getenv("COMMITTER_DATE") ? : realdate;

	remove_special(gecos); remove_special(realgecos);
	remove_special(email); remove_special(realemail);
	remove_special(date); remove_special(realdate);

	init_buffer(&buffer, &size);
  /* 添加tree sha1(tree) */
	add_buffer(&buffer, &size, "tree %s\n", sha1_to_hex(tree_sha1));

	/*
	 * NOTE! This ordering means that the same exact tree merged with a
	 * different order of parents will be a _different_ changeset even
	 * if everything else stays the same.
	 */
  /* 添加 parent sha1(parent) */
	for (i = 0; i < parents; i++)
		add_buffer(&buffer, &size, "parent %s\n", sha1_to_hex(parent_sha1[i]));

  /* 添加 author <email> date */
	/* Person/date information */
	add_buffer(&buffer, &size, "author %s <%s> %s\n", gecos, email, date);
	add_buffer(&buffer, &size, "committer %s <%s> %s\n\n", realgecos, realemail, realdate);
  /* 用户输出提交信息 */
	/* And add the comment */
	while (fgets(comment, sizeof(comment), stdin) != NULL)
		add_buffer(&buffer, &size, "%s", comment);
  /* 向buffer前添加commit size 并释放buffer*/
	finish_buffer("commit ", &buffer, &size);
  /*写到文件中 */
	write_sha1_file(buffer, size);
	return 0;
}
