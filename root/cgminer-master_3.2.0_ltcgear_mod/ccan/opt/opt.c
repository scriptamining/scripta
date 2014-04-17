#include <ccan/opt/opt.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef WIN32
	#include <err.h>
#else
#include <libgen.h>
	#define errx(status, fmt, ...) { \
			fprintf(stderr, fmt, __VA_ARGS__); \
			fprintf(stderr, "\n"); \
			exit(status); }
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include "private.h"

struct opt_table *opt_table;
unsigned int opt_count, opt_num_short, opt_num_short_arg, opt_num_long;
const char *opt_argv0;

/* Returns string after first '-'. */
static const char *first_name(const char *names, unsigned *len)
{
	*len = strcspn(names + 1, "|= ");
	return names + 1;
}

static const char *next_name(const char *names, unsigned *len)
{
	names += *len;
	if (names[0] == ' ' || names[0] == '=' || names[0] == '\0')
		return NULL;
	return first_name(names + 1, len);
}

static const char *first_opt(unsigned *i, unsigned *len)
{
	for (*i = 0; *i < opt_count; (*i)++) {
		if (opt_table[*i].type == OPT_SUBTABLE)
			continue;
		return first_name(opt_table[*i].names, len);
	}
	return NULL;
}

static const char *next_opt(const char *p, unsigned *i, unsigned *len)
{
	for (; *i < opt_count; (*i)++) {
		if (opt_table[*i].type == OPT_SUBTABLE)
			continue;
		if (!p)
			return first_name(opt_table[*i].names, len);
		p = next_name(p, len);
		if (p)
			return p;
	}
	return NULL;
}

const char *first_lopt(unsigned *i, unsigned *len)
{
	const char *p;
	for (p = first_opt(i, len); p; p = next_opt(p, i, len)) {
		if (p[0] == '-') {
			/* Skip leading "-" */
			(*len)--;
			p++;
			break;
		}
	}
	return p;
}

const char *next_lopt(const char *p, unsigned *i, unsigned *len)
{
	for (p = next_opt(p, i, len); p; p = next_opt(p, i, len)) {
		if (p[0] == '-') {
			/* Skip leading "-" */
			(*len)--;
			p++;
			break;
		}
	}
	return p;
}

const char *first_sopt(unsigned *i)
{
	const char *p;
	unsigned int len = 0 /* GCC bogus warning */;

	for (p = first_opt(i, &len); p; p = next_opt(p, i, &len)) {
		if (p[0] != '-')
			break;
	}
	return p;
}

const char *next_sopt(const char *p, unsigned *i)
{
	unsigned int len = 1;
	for (p = next_opt(p, i, &len); p; p = next_opt(p, i, &len)) {
		if (p[0] != '-')
			break;
	}
	return p;
}

static void check_opt(const struct opt_table *entry)
{
	const char *p;
	unsigned len;

	if (entry->type != OPT_HASARG && entry->type != OPT_NOARG)
		errx(1, "Option %s: unknown entry type %u",
		     entry->names, entry->type);

	if (!entry->desc)
		errx(1, "Option %s: description cannot be NULL", entry->names);


	if (entry->names[0] != '-')
		errx(1, "Option %s: does not begin with '-'", entry->names);

	for (p = first_name(entry->names, &len); p; p = next_name(p, &len)) {
		if (*p == '-') {
			if (len == 1)
				errx(1, "Option %s: invalid long option '--'",
				     entry->names);
			opt_num_long++;
		} else {
			if (len != 1)
				errx(1, "Option %s: invalid short option"
				     " '%.*s'", entry->names, len+1, p-1);
			opt_num_short++;
			if (entry->type == OPT_HASARG)
				opt_num_short_arg++;
		}
		/* Don't document args unless there are some. */
		if (entry->type == OPT_NOARG) {
			if (p[len] == ' ' || p[len] == '=')
				errx(1, "Option %s: does not take arguments"
				     " '%s'", entry->names, p+len+1);
		}
	}
}

static void add_opt(const struct opt_table *entry)
{
	opt_table = realloc(opt_table, sizeof(opt_table[0]) * (opt_count+1));
	opt_table[opt_count++] = *entry;
}

void _opt_register(const char *names, enum opt_type type,
		   char *(*cb)(void *arg),
		   char *(*cb_arg)(const char *optarg, void *arg),
		   void (*show)(char buf[OPT_SHOW_LEN], const void *arg),
		   const void *arg, const char *desc)
{
	struct opt_table opt;
	opt.names = names;
	opt.type = type;
	opt.cb = cb;
	opt.cb_arg = cb_arg;
	opt.show = show;
	opt.u.carg = arg;
	opt.desc = desc;
	check_opt(&opt);
	add_opt(&opt);
}

void opt_register_table(const struct opt_table entry[], const char *desc)
{
	unsigned int i, start = opt_count;

	if (desc) {
		struct opt_table heading = OPT_SUBTABLE(NULL, desc);
		add_opt(&heading);
	}
	for (i = 0; entry[i].type != OPT_END; i++) {
		if (entry[i].type == OPT_SUBTABLE)
			opt_register_table(subtable_of(&entry[i]),
					   entry[i].desc);
		else {
			check_opt(&entry[i]);
			add_opt(&entry[i]);
		}
	}
	/* We store the table length in arg ptr. */
	if (desc)
		opt_table[start].u.tlen = (opt_count - start);
}

/* Parse your arguments. */
bool opt_parse(int *argc, char *argv[], void (*errlog)(const char *fmt, ...))
{
	int ret;
	unsigned offset = 0;
	
	#ifdef WIN32
	char *original_argv0 = argv[0];
	argv[0] = (char*)basename(argv[0]);
	#endif

	/* This helps opt_usage. */
	opt_argv0 = argv[0];

	while ((ret = parse_one(argc, argv, &offset, errlog)) == 1);
	
	#ifdef WIN32
	argv[0] = original_argv0;
	#endif

	/* parse_one returns 0 on finish, -1 on error */
	return (ret == 0);
}

void opt_free_table(void)
{
	free(opt_table);
	opt_table=0;
}

void opt_log_stderr(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

void opt_log_stderr_exit(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

char *opt_invalid_argument(const char *arg)
{
	char *str = malloc(sizeof("Invalid argument '%s'") + strlen(arg));
	sprintf(str, "Invalid argument '%s'", arg);
	return str;
}
