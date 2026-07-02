#define _POSIX_C_SOURCE 200809L
#define NOB_IMPLEMENTATION
#define NOB_UNSTRIP_PREFIX
#define NOB_NO_ECHO
#include "nob.h"

#include <ctype.h>
#include <string.h>

/* --- settings --- */

#define NAME      "varint"
#define STD       "c11"
#define CC        "gcc"
#define BUILD_DIR ".build"
#define SRC_DIR   "src"
#define INC_DIR   "include"
#define LIB_DIR   "lib"
#define TEST_DIR  "tests"

#define WARN_FLAGS \
	"-Wall", "-Wextra", "-Wfloat-equal", "-Wundef", "-Wshadow", \
	"-Wpointer-arith", "-Wwrite-strings", "-Wswitch-default", \
	"-Wconversion", "-Wunreachable-code", "-pedantic"

#define BASE_FLAGS "-std=" STD, WARN_FLAGS, "-I" INC_DIR
#define SAN_FLAGS  "-fsanitize=address,undefined"
#define LNK_FLAGS  "-L" LIB_DIR "/static", "-L" LIB_DIR "/dynamic", "-lm"

/* to expose POSIX/GNU APIs: add "-D_POSIX_C_SOURCE=200809L" or "-D_GNU_SOURCE" to BASE_FLAGS */

/* --- file collection helpers --- */

static bool is_c_file(const char *name) {
	size_t n = strlen(name);
	return n >= 3 && strcmp(name + n - 2, ".c") == 0;
}

static bool is_test_c_file(const char *name) {
	return is_c_file(name) && strncmp(name, "test_", 5) == 0;
}

static bool is_ch_file(const char *name) {
	size_t n = strlen(name);
	if (n < 3) return false;
	return strcmp(name + n - 2, ".c") == 0 || strcmp(name + n - 2, ".h") == 0;
}

typedef bool (*FileFilter)(const char *name);

static bool collect_files(const char *dir, FileFilter filter, Nob_File_Paths *paths) {
	Nob_Dir_Entry de = {0};

	if (!nob_dir_entry_open(dir, &de))
		return false;

	while (nob_dir_entry_next(&de)) {
		if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
			continue;

		if (filter && !filter(de.name))
			continue;

		nob_da_append(paths, nob_temp_sprintf("%s/%s", dir, de.name));
	}

	bool ok = !de.error;
	nob_dir_entry_close(de);
	return ok;
}

/* --- helpers --- */

static const char *git_version(void) {
	FILE *f = popen("git describe --tags --always --dirty 2>/dev/null", "r");
	if (!f) return "unknown";
	char buf[128] = {0};
	if (!fgets(buf, sizeof(buf), f)) { pclose(f); return "unknown"; }
	pclose(f);
	size_t n = strlen(buf);
	if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';
	return nob_temp_sprintf("%s", buf);
}

/* --- semver --- */

/* Validates MAJOR.MINOR.PATCH[-prerelease][+build] */
static bool is_valid_semver(const char *s) {
	if (!s || !*s || !isdigit((unsigned char)*s)) return false;
	int dots = 0;
	bool in_digits = false;
	for (; *s; s++) {
		if (isdigit((unsigned char)*s)) {
			in_digits = true;
		} else if (*s == '.' && dots < 2) {
			if (!in_digits) return false;
			dots++;
			in_digits = false;
		} else if ((*s == '-' || *s == '+') && dots == 2 && in_digits) {
			s++;
			while (*s && (isalnum((unsigned char)*s) || *s == '.' || *s == '-' || *s == '+'))
				s++;
			return *s == '\0';
		} else {
			return false;
		}
	}
	return dots == 2 && in_digits;
}

/* --- commands --- */

static bool build_objects(Nob_File_Paths *srcs, Nob_File_Paths *objs) {
	if (!nob_mkdir_if_not_exists(BUILD_DIR)) return false;
	for (size_t i = 0; i < srcs->count; i++) {
		const char *src = srcs->items[i];
		const char *obj = nob_temp_sprintf(BUILD_DIR "/%s.o", nob_path_name(src));
		/* strip ".c", add ".o" */
		size_t len = strlen(obj);
		char *o = nob_temp_strdup(obj);
		o[len - 2] = '.'; o[len - 1] = 'o';

		Nob_Cmd cmd = {0};
		nob_cmd_append(&cmd, CC, BASE_FLAGS, "-O2", "-DNDEBUG", "-fPIC",
			nob_temp_sprintf("-DVERSION=\"%s\"", git_version()),
			"-c", src, "-o", o);
		if (!nob_cmd_run(&cmd)) return false;
		nob_da_append(objs, o);
	}
	return true;
}

static bool cmd_static(void) {
	Nob_File_Paths srcs = {0};
	if (!collect_files(SRC_DIR, is_c_file, &srcs)) return false;

	Nob_File_Paths objs = {0};
	if (!build_objects(&srcs, &objs)) return false;

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "ar", "rcs", BUILD_DIR "/lib" NAME ".a");
	nob_da_append_many(&cmd, objs.items, objs.count);
	if (!nob_cmd_run(&cmd)) return false;

	nob_log(NOB_INFO, "Built: " BUILD_DIR "/lib" NAME ".a");
	return true;
}

static bool cmd_dynamic(void) {
	Nob_File_Paths srcs = {0};
	if (!collect_files(SRC_DIR, is_c_file, &srcs)) return false;

	Nob_File_Paths objs = {0};
	if (!build_objects(&srcs, &objs)) return false;

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, CC, "-shared", "-o", BUILD_DIR "/lib" NAME ".so");
	nob_da_append_many(&cmd, objs.items, objs.count);
	nob_cmd_append(&cmd, LNK_FLAGS);
	if (!nob_cmd_run(&cmd)) return false;

	nob_log(NOB_INFO, "Built: " BUILD_DIR "/lib" NAME ".so");
	return true;
}

static bool cmd_build(void) {
	return cmd_static() && cmd_dynamic();
}

static bool cmd_test(void) {
	if (!nob_mkdir_if_not_exists(BUILD_DIR "/tests")) return false;

	Nob_File_Paths srcs = {0};
	if (!collect_files(SRC_DIR, is_c_file, &srcs)) return false;

	Nob_File_Paths test_files = {0};
	if (!collect_files(TEST_DIR, is_test_c_file, &test_files)) return false;

	int failed = 0;
	for (size_t i = 0; i < test_files.count; i++) {
		const char *tf   = test_files.items[i];
		const char *base = nob_path_name(tf);
		size_t      blen = strlen(base);
		char tname[256];
		snprintf(tname, sizeof(tname), "%.*s", (int)(blen - 2), base);

		printf("  %-30s", tname);
		fflush(stdout);

		const char *out = nob_temp_sprintf(BUILD_DIR "/tests/%s", tname);

		Nob_Cmd cmd = {0};
		nob_cmd_append(&cmd, CC, BASE_FLAGS, "-I" TEST_DIR, "-g", "-O1", SAN_FLAGS);
		nob_cmd_append(&cmd, tf);
		nob_da_append_many(&cmd, srcs.items, srcs.count);
		nob_cmd_append(&cmd, LNK_FLAGS, "-o", out);
		if (!nob_cmd_run(&cmd)) { printf("FAILED (compile)\n"); failed++; continue; }

		Nob_Cmd run = {0};
		nob_cmd_append(&run, out);
		if (nob_cmd_run(&run)) printf("ok\n");
		else { printf("FAILED\n"); failed++; }
	}

	if (failed > 0) {
		nob_log(NOB_ERROR, "%d test(s) failed", failed);
		return false;
	}
	return true;
}

static bool cmd_valgrind(void) {
	if (!nob_mkdir_if_not_exists(BUILD_DIR "/tests")) return false;

	Nob_File_Paths srcs = {0};
	if (!collect_files(SRC_DIR, is_c_file, &srcs)) return false;

	Nob_File_Paths test_files = {0};
	if (!collect_files(TEST_DIR, is_test_c_file, &test_files)) return false;

	for (size_t i = 0; i < test_files.count; i++) {
		const char *tf   = test_files.items[i];
		const char *base = nob_path_name(tf);
		size_t      blen = strlen(base);
		char tname[256];
		snprintf(tname, sizeof(tname), "%.*s-vg", (int)(blen - 2), base);

		const char *out = nob_temp_sprintf(BUILD_DIR "/tests/%s", tname);

		Nob_Cmd cmd = {0};
		nob_cmd_append(&cmd, CC, BASE_FLAGS, "-I" TEST_DIR, "-g", "-O1");
		nob_cmd_append(&cmd, tf);
		nob_da_append_many(&cmd, srcs.items, srcs.count);
		nob_cmd_append(&cmd, LNK_FLAGS, "-o", out);
		if (!nob_cmd_run(&cmd)) return false;

		nob_log(NOB_INFO, "==> valgrind: %s", tname);
		Nob_Cmd vg = {0};
		nob_cmd_append(&vg, "valgrind",
			"--leak-check=full", "--show-leak-kinds=all",
			"--track-origins=yes", "--error-exitcode=1",
			out);
		if (!nob_cmd_run(&vg)) return false;
	}
	return true;
}

static bool cmd_cppcheck(void) {
	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "cppcheck",
		"--enable=all", "--std=" STD, "--inconclusive", "--error-exitcode=1",
		"-I" INC_DIR, SRC_DIR "/");
	return nob_cmd_run(&cmd);
}

static bool cmd_fmt(bool check_only) {
	Nob_File_Paths files = {0};
	if (!collect_files(SRC_DIR, is_ch_file, &files)) return false;
	if (!collect_files(INC_DIR, is_ch_file, &files)) return false;

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "clang-format");
	if (check_only) nob_cmd_append(&cmd, "--dry-run", "--Werror");
	else            nob_cmd_append(&cmd, "-i");
	nob_da_append_many(&cmd, files.items, files.count);
	return nob_cmd_run(&cmd);
}

static bool cmd_compile_commands(void) {
	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "bear", "--", "./nob", "build");
	return nob_cmd_run(&cmd);
}

static bool cmd_docs(void) {
	if (!nob_mkdir_if_not_exists(BUILD_DIR "/docs")) return false;

	const char *doxyfile = BUILD_DIR "/docs/doxyfile";
	const char *content =
		"PROJECT_NAME     = " NAME "\n"
		"INPUT            = " SRC_DIR " " INC_DIR "\n"
		"OUTPUT_DIRECTORY = " BUILD_DIR "/docs\n"
		"GENERATE_HTML    = YES\n"
		"GENERATE_LATEX   = NO\n"
		"RECURSIVE        = YES\n"
		"EXTRACT_ALL      = YES\n"
		"QUIET            = YES\n";
	if (!nob_write_entire_file(doxyfile, content, strlen(content))) return false;

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "doxygen", doxyfile);
	if (!nob_cmd_run(&cmd)) return false;

	nob_log(NOB_INFO, "docs: " BUILD_DIR "/docs/html/index.html");
	return true;
}

static bool cmd_pack(const char *version, const char *prefix) {
	if (!cmd_static()) return false;
	if (!cmd_dynamic()) return false;

	char out[512];
	if (version && *version) {
		if (!is_valid_semver(version)) {
			nob_log(NOB_ERROR, "error: '%s' is not valid semver (e.g. 1.2.3)", version);
			return false;
		}
		snprintf(out, sizeof(out), NAME "-%s.tar.gz", version);
	} else {
		snprintf(out, sizeof(out), NAME ".tar.gz");
	}

	/* create staging directory */
	char stage[] = "/tmp/nob_pack_XXXXXX";
	if (!mkdtemp(stage)) {
		nob_log(NOB_ERROR, "mkdtemp failed: %s", strerror(errno));
		return false;
	}

	const char *root = (prefix && *prefix)
		? nob_temp_sprintf("%s/%s", stage, prefix)
		: stage;

	if (!nob_mkdir_if_not_exists(nob_temp_sprintf("%s/include",     root))) goto fail;
	if (!nob_mkdir_if_not_exists(nob_temp_sprintf("%s/lib",         root))) goto fail;
	if (!nob_mkdir_if_not_exists(nob_temp_sprintf("%s/lib/static",  root))) goto fail;
	if (!nob_mkdir_if_not_exists(nob_temp_sprintf("%s/lib/dynamic", root))) goto fail;

	if (!nob_copy_directory_recursively(INC_DIR, nob_temp_sprintf("%s/include", root))) goto fail;

	const char *a_name  = (version && *version)
		? nob_temp_sprintf("lib" NAME "-%s.a",  version)
		: "lib" NAME ".a";
	const char *so_name = (version && *version)
		? nob_temp_sprintf("lib" NAME "-%s.so", version)
		: "lib" NAME ".so";

	if (nob_file_exists(BUILD_DIR "/lib" NAME ".a"))
		if (!nob_copy_file(BUILD_DIR "/lib" NAME ".a",
		                   nob_temp_sprintf("%s/lib/static/%s", root, a_name))) goto fail;

	if (nob_file_exists(BUILD_DIR "/lib" NAME ".so"))
		if (!nob_copy_file(BUILD_DIR "/lib" NAME ".so",
		                   nob_temp_sprintf("%s/lib/dynamic/%s", root, so_name))) goto fail;

	{
		const char *tar_dir = (prefix && *prefix) ? prefix : ".";
		Nob_Cmd cmd = {0};
		nob_cmd_append(&cmd, "tar", "-czf", out, "-C", stage, tar_dir);
		if (!nob_cmd_run(&cmd)) goto fail;
	}

	{
		Nob_Cmd cmd = {0};
		nob_cmd_append(&cmd, "rm", "-rf", stage);
		nob_cmd_run(&cmd); /* best effort cleanup */
	}

	nob_log(NOB_INFO, "packed: %s", out);
	return true;

fail:
	{
		Nob_Cmd cmd = {0};
		nob_cmd_append(&cmd, "rm", "-rf", stage);
		nob_cmd_run(&cmd);
	}
	return false;
}

static bool cmd_clean(void) {
	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "rm", "-rf", BUILD_DIR);
	return nob_cmd_run(&cmd);
}

static bool cmd_strace(void) {
	/* build each test without sanitizers — cleaner strace output */
	if (!nob_mkdir_if_not_exists(BUILD_DIR "/tests")) return false;

	Nob_File_Paths srcs = {0};
	if (!collect_files(SRC_DIR, is_c_file, &srcs)) return false;

	Nob_File_Paths test_files = {0};
	if (!collect_files(TEST_DIR, is_test_c_file, &test_files)) return false;

	for (size_t i = 0; i < test_files.count; i++) {
		const char *tf   = test_files.items[i];
		const char *base = nob_path_name(tf);
		size_t      blen = strlen(base);
		char tname[256];
		snprintf(tname, sizeof(tname), "%.*s-st", (int)(blen - 2), base);

		const char *out = nob_temp_sprintf(BUILD_DIR "/tests/%s", tname);

		Nob_Cmd cmd = {0};
		nob_cmd_append(&cmd, CC, BASE_FLAGS, "-I" TEST_DIR, "-g", "-O1");
		nob_cmd_append(&cmd, tf);
		nob_da_append_many(&cmd, srcs.items, srcs.count);
		nob_cmd_append(&cmd, LNK_FLAGS, "-o", out);
		if (!nob_cmd_run(&cmd)) return false;

		nob_log(NOB_INFO, "==> strace: %s", tname);
		Nob_Cmd st = {0};
		nob_cmd_append(&st, "strace", out);
		if (!nob_cmd_run(&st)) return false;
	}
	return true;
}

static bool cmd_ctags(void) {
	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "ctags", "-R", SRC_DIR, INC_DIR);
	return nob_cmd_run(&cmd);
}

/* --- usage --- */

static void usage(const char *program) {
	fprintf(stderr, "Usage: %s [command]\n", program);
	fprintf(stderr, "\n");
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "  build            Build static and dynamic libraries (default)\n");
	fprintf(stderr, "  static           Build " BUILD_DIR "/lib" NAME ".a\n");
	fprintf(stderr, "  dynamic          Build " BUILD_DIR "/lib" NAME ".so\n");
	fprintf(stderr, "  test             Compile and run all tests/test_*.c\n");
	fprintf(stderr, "  valgrind         Per-test Valgrind run (no sanitizers)\n");
	fprintf(stderr, "  strace           Per-test strace (no sanitizers)\n");
	fprintf(stderr, "  cppcheck         cppcheck static analysis\n");
	fprintf(stderr, "  ctags            Generate tags file for editor navigation\n");
	fprintf(stderr, "  fmt              clang-format in-place\n");
	fprintf(stderr, "  fmt-check        Check formatting (for CI)\n");
	fprintf(stderr, "  compile-commands Generate compile_commands.json via bear\n");
	fprintf(stderr, "  docs             Generate HTML docs in " BUILD_DIR "/docs/html/\n");
	fprintf(stderr, "  pack [ver] [pfx] Pack library as tar.gz (semver optional)\n");
	fprintf(stderr, "  clean            Remove " BUILD_DIR "/\n");
}

/* --- main --- */

int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	const char *program = nob_shift(argv, argc);
	const char *cmd     = argc > 0 ? nob_shift(argv, argc) : "build";

	if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "help") == 0) {
		usage(program);
		return 0;
	} else if (strcmp(cmd, "build") == 0) {
		return cmd_build() ? 0 : 1;
	} else if (strcmp(cmd, "static") == 0) {
		return cmd_static() ? 0 : 1;
	} else if (strcmp(cmd, "dynamic") == 0) {
		return cmd_dynamic() ? 0 : 1;
	} else if (strcmp(cmd, "test") == 0) {
		return cmd_test() ? 0 : 1;
	} else if (strcmp(cmd, "valgrind") == 0) {
		return cmd_valgrind() ? 0 : 1;
	} else if (strcmp(cmd, "strace") == 0) {
		return cmd_strace() ? 0 : 1;
	} else if (strcmp(cmd, "cppcheck") == 0) {
		return cmd_cppcheck() ? 0 : 1;
	} else if (strcmp(cmd, "ctags") == 0) {
		return cmd_ctags() ? 0 : 1;
	} else if (strcmp(cmd, "fmt") == 0) {
		return cmd_fmt(false) ? 0 : 1;
	} else if (strcmp(cmd, "fmt-check") == 0) {
		return cmd_fmt(true) ? 0 : 1;
	} else if (strcmp(cmd, "compile-commands") == 0) {
		return cmd_compile_commands() ? 0 : 1;
	} else if (strcmp(cmd, "docs") == 0) {
		return cmd_docs() ? 0 : 1;
	} else if (strcmp(cmd, "pack") == 0) {
		const char *ver = argc > 0 ? nob_shift(argv, argc) : "";
		const char *pfx = argc > 0 ? nob_shift(argv, argc) : "";
		return cmd_pack(ver, pfx) ? 0 : 1;
	} else if (strcmp(cmd, "clean") == 0) {
		return cmd_clean() ? 0 : 1;
	} else {
		nob_log(NOB_ERROR, "unknown command: %s", cmd);
		usage(program);
		return 1;
	}
}
