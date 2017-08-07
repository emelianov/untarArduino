/*
 * "untar" is an extremely simple tar extractor:
 *  * A single C source file, so it should be easy to compile
 *    and run on any system with a C compiler.
 *  * Extremely portable standard C.  The only non-ANSI function
 *    used is mkdir().
 *  * Reads basic ustar tar archives.
 *  * Does not require libarchive or any other special library.
 *
 * To compile: cc -o untar untar.c
 *
 * Usage:  untar <archive>
 *
 * In particular, this program should be sufficient to extract the
 * distribution for libarchive, allowing people to bootstrap
 * libarchive on systems that do not already have a tar program.
 *
 * To unpack libarchive-x.y.z.tar.gz:
 *    * gunzip libarchive-x.y.z.tar.gz
 *    * untar libarchive-x.y.z.tar
 *
 * Written by Tim Kientzle, March 2009.
 *
 * Released into the public domain.
 */

/* These are all highly standard and portable headers. */
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

/* This is for mkdir(); this may need to be changed for some platforms. */
//#include <sys/stat.h>  /* For mkdir() */

template <typename T> class tar {
private:
/* Parse an octal number, ignoring leading and trailing nonsense. */
	static int16_t parseoct(const char *p, size_t n);

/* Returns true if this is 512 zero bytes. */
	static int16_t is_end_of_archive(const char *p);

/* Create a directory, including parent directories as necessary. */
	static void create_dir(char *pathname, int16_t mode);

/* Create a file, including parent directory as necessary. */
	static FILE *create_file(char *pathname, int16_t mode);

/* Verify the tar checksum. */
	static int16_t verify_checksum(const char *p);

	bool fskip(char *pathname);

	<T> FS;

	cbTarFunc cbFile;
	cbTarFunc cbDir;

public:
/* Initialize */
	template <typename T> tar(<T> &dst);

/* Open file.  tar f */
	template <typename S> void f(<S> &src);

/* Extract a tar archive. tar x */
	static void x(const char *path);

/* List a tar archive. tar l */
	static void l(const char *path);

	void onFile(cb);

	void onDir(cb);
}