/*
 * Builtin "git count-objects".
 *
 * Copyright (c) 2006 Junio C Hamano
 */

#include "cache.h"
#include "config.h"
#include "dir.h"
#include "repository.h"
#include "builtin.h"
#include "parse-options.h"
#include "quote.h"
#include "packfile.h"
#include "object-store.h"

static unsigned long garbage;
static off_t size_garbage;
static int verbose;
static unsigned long loose, packed, packed_loose;
static off_t loose_size;

static const char *bits_to_msg(unsigned seen_bits)
{
	switch (seen_bits) {
	case 0:
		return "no corresponding .idx or .pack";
	case PACKDIR_FILE_GARBAGE:
		return "garbage found";
	case PACKDIR_FILE_PACK:
		return "no corresponding .idx";
	case PACKDIR_FILE_IDX:
		return "no corresponding .pack";
	case PACKDIR_FILE_PACK|PACKDIR_FILE_IDX:
	default:
		return NULL;
	}
}

static void real_report_garbage(unsigned seen_bits, const char *path)
{
	struct stat st;
	const char *desc = bits_to_msg(seen_bits);

	if (!desc)
		return;

	if (!stat(path, &st))
		size_garbage += st.st_size;
	warning("%s: %s", desc, path);
	garbage++;
}

static void loose_garbage(const char *path)
{
	if (verbose)
		report_garbage(PACKDIR_FILE_GARBAGE, path);
}

// 统计松散文件的数据
static int count_loose(const struct object_id *oid, const char *path, void *data)
{
	struct stat st;

	// 文件不存在或者不是 REG，记录垃圾
	if (lstat(path, &st) || !S_ISREG(st.st_mode))
		loose_garbage(path);
	else {
		// 否则记录松散文件的大小
		loose_size += on_disk_bytes(st);
		// 松散对象计数++
		loose++;
		// 如果 PACK 也有这个对象，意味着我们之后可以将这个松散的对象删除
		if (verbose && has_object_pack(oid))
			packed_loose++;
	}
	return 0;
}

// 统计松散目录中的垃圾
static int count_cruft(const char *basename, const char *path, void *data)
{
	loose_garbage(path);
	return 0;
}

static int print_alternate(struct object_directory *odb, void *data)
{
	printf("alternate: ");
	quote_c_style(odb->path, NULL, stdout, 0);
	putchar('\n');
	return 0;
}

static char const * const count_objects_usage[] = {
	"git count-objects [-v] [-H | --human-readable]",
	NULL
};

int cmd_count_objects(int argc, const char **argv, const char *prefix)
{
	int human_readable = 0;
	struct option opts[] = {
		OPT__VERBOSE(&verbose, N_("be verbose")),
		OPT_BOOL('H', "human-readable", &human_readable,
			 N_("print sizes in human readable format")),
		OPT_END(),
	};

	git_config(git_default_config, NULL);

	argc = parse_options(argc, argv, prefix, opts, count_objects_usage, 0);
	/* we do not take arguments other than flags for now */
	if (argc)
		usage_with_options(count_objects_usage, opts);
	if (verbose) {
		report_garbage = real_report_garbage;
		report_linked_checkout_garbage();
	}
	// 统计松散对象目录下面的对象信息和垃圾信息
	for_each_loose_file_in_objdir(get_object_directory(),
				      count_loose, count_cruft, NULL, NULL);

	if (verbose) {
		struct packed_git *p;
		unsigned long num_pack = 0;
		off_t size_pack = 0;
		struct strbuf loose_buf = STRBUF_INIT;
		struct strbuf pack_buf = STRBUF_INIT;
		struct strbuf garbage_buf = STRBUF_INIT;

		for (p = get_all_packs(the_repository); p; p = p->next) {
			if (!p->pack_local)
				continue;
			if (open_pack_index(p))
				continue;
			/* 松散文件中的对象数量 */
			packed += p->num_objects;
			// pack 大小 + index 大小
			size_pack += p->pack_size + p->index_size;
			num_pack++;
		}

		if (human_readable) {
			strbuf_humanise_bytes(&loose_buf, loose_size);
			strbuf_humanise_bytes(&pack_buf, size_pack);
			strbuf_humanise_bytes(&garbage_buf, size_garbage);
		} else {
			strbuf_addf(&loose_buf, "%lu",
				    (unsigned long)(loose_size / 1024));
			strbuf_addf(&pack_buf, "%lu",
				    (unsigned long)(size_pack / 1024));
			strbuf_addf(&garbage_buf, "%lu",
				    (unsigned long)(size_garbage / 1024));
		}

		printf("count: %lu\n", loose);
		printf("size: %s\n", loose_buf.buf);
		printf("in-pack: %lu\n", packed);
		// 包的数量
		printf("packs: %lu\n", num_pack);
		printf("size-pack: %s\n", pack_buf.buf);
		// 在 pack 和 loose 都有 -> 可以删除松散的数量
		printf("prune-packable: %lu\n", packed_loose);
		printf("garbage: %lu\n", garbage);
		printf("size-garbage: %s\n", garbage_buf.buf);
		// 打印多个可选 git 数据库位置
		foreach_alt_odb(print_alternate, NULL);
		strbuf_release(&loose_buf);
		strbuf_release(&pack_buf);
		strbuf_release(&garbage_buf);
	} else {
		// 普通模式，显示松散文件大小之和与数量
		struct strbuf buf = STRBUF_INIT;
		if (human_readable)
			strbuf_humanise_bytes(&buf, loose_size);
		else
			strbuf_addf(&buf, "%lu kilobytes",
				    (unsigned long)(loose_size / 1024));
		printf("%lu objects, %s\n", loose, buf.buf);
		strbuf_release(&buf);
	}
	return 0;
}
