/*
 * This code based on
 * 
 * "untar" is an extremely simple tar extractor
 *
 * Written by Tim Kientzle, March 2009.
 *
 * Released into the public domain.
 *
 * Ported to Arduino library by Alexander Emelainov (a.m.emelianov@gmail.com), August 2017
 *
 */

// Uncomment following definition to enable not flat namespace filesystems
//#define TAR_MKDIR

// Comment to remove output extracting process to Serial
#define TAR_VERBOSE

template <typename T>
class Tar {
public:
	Tar(T* dst) {
		FSC = dst;
		pathprefix = "";
	}
	void C(char* path);				// Set directory extract to. tar -C  
	void f(Stream* src);			// Open file.  tar -f
	void x(const char* path);		// Extract a tar archive. tar -x
	void l(const char* path);		// List a tar archive. tar -l
	//void onFile(cb);
	//void onDir(cb);
private:
	char* pathprefix;
	int parseoct(const char *p, size_t n);		// Parse an octal number, ignoring leading and trailing nonsense.
	int is_end_of_archive(const char *p);		// Returns true if this is 512 zero bytes.
#ifdef TAR_MKDIR
	void create_dir(char *pathname, int mode);	// Create a directory, including parent directories as necessary.
#endif
	File *create_file(char *pathname, int mode);// Create a file, including parent directory as necessary.
	int verify_checksum(const char *p);			// Verify the tar checksum.
	T* FSC;
	Stream* source;
	//bool fskip(char *pathname);
	//cbTarFunc cbFile;
	//cbTarFunc cbDir;
};

template <typename T>
void Tar<T>::C(char* path){
	free(pathprefix);
	pathprefix = (char*)malloc (strlen(path) + 1);
		if (pathprefix != NULL) {
			strcpy (pathprefix, path);
		} else {
			Serial.println("Memory allocation error");
			pathprefix = "";
     	}
}

template <typename T>
void Tar<T>::f(Stream* src){
	source = src;
}

template <typename T>
int Tar<T>::parseoct(const char *p, size_t n)
{
	int i = 0;

	while (*p < '0' || *p > '7') {
		++p;
		--n;
	}
	while (*p >= '0' && *p <= '7' && n > 0) {
		i *= 8;
		i += *p - '0';
		++p;
		--n;
	}
	return (i);
}

template <typename T>
int Tar<T>::is_end_of_archive(const char *p)
{
	int n;
	for (n = 511; n >= 0; --n)
		if (p[n] != '\0')
			return (0);
	return (1);
}

#ifdef TAR_MKDIR
template <typename T>
void Tar<T>::create_dir(char *pathname, int mode)
{
	char *p;
	int r;

	/* Strip trailing '/' */
	if (pathname[strlen(pathname) - 1] == '/')
		pathname[strlen(pathname) - 1] = '\0';

	/* Try creating the directory. */
	r = FSC->mkdir(pathname, mode);

	if (r != 0) {
		/* On failure, try creating parent directory. */
		p = strrchr(pathname, '/');
		if (p != NULL) {
			*p = '\0';
			create_dir(pathname, 0755);
			*p = '/';
			r = FSC->mkdir(pathname, mode);
		}
	}
	if (r != 0)
		Serial.print("Could not create directory %s");
		Serial.println(pathname);
}
#endif

template <typename T>
File* Tar<T>::create_file(char *pathname, int mode)
{
	File* f;
	f = new File();
	*f = FSC->open(pathname, "w+");
#ifdef TAR_MKDIR
	if (f == NULL) {
		/* Try creating parent dir and then creating file. */
		char *p = strrchr(pathname, '/');
		if (p != NULL) {
			*p = '\0';
			create_dir(pathname, 0755);
			*p = '/';
			f = FSC->open(pathname, "w+");
		}
	}
#endif
	return (f);
}

template <typename T>
int Tar<T>::verify_checksum(const char *p)
{
	int n, u = 0;
	for (n = 0; n < 512; ++n) {
		if (n < 148 || n > 155)
			/* Standard tar checksum adds unsigned bytes. */
			u += ((unsigned char *)p)[n];
		else
			u += 0x20;

	}
	return (u == parseoct(p + 148, 8));
}

template <typename T>
void Tar<T>::x(const char *path)
{
	char buff[512];
	File *f = NULL;
	size_t bytes_read;
	int filesize;
	char* fullpath = NULL;
	
	Serial.print("Extracting from ");
	Serial.println(path);
	for (;;) {
		bytes_read = source->readBytes(buff, 512);
		if (bytes_read < 512) {
			Serial.print("Short read on ");
			Serial.print(path);
			Serial.print(": expected 512, got ");
			Serial.println(bytes_read);
			return;
		}
		if (is_end_of_archive(buff)) {
			Serial.print("End of ");
			Serial.println(path);
			return;
		}
		if (!verify_checksum(buff)) {
			Serial.println("Checksum failure\n");
			return;
		}
		filesize = parseoct(buff + 124, 12);
		char* fullpath = (char*)malloc (strlen(pathprefix) + strlen(buff) + 1);
		if (fullpath == NULL) {
			Serial.println("Memory allocation error. Ignoring entry");
     	} else {     	
			strcpy (fullpath, pathprefix);
			strcat (fullpath, buff);
			switch (buff[156]) {
			case '1':
				Serial.print(" Ignoring hardlink ");
				Serial.println(buff);
				break;
			case '2':
				Serial.print(" Ignoring symlink");
				Serial.println(buff);
				break;
			case '3':
				Serial.print(" Ignoring character device");
				Serial.println(buff);
				break;
			case '4':
				Serial.print(" Ignoring block device");
				Serial.println(buff);
				break;
			case '5':
			#ifdef TAR_MKDIR
				Serial.print(" Extracting dir ");
				Serial.println(buff);
				create_dir(buff, parseoct(buff + 100, 8));
				filesize = 0;
			#else
				Serial.print(" Ignoring dir ");
				Serial.println(buff);
			#endif
				break;
			case '6':
				Serial.print(" Ignoring FIFO ");
				Serial.println(buff);
				break;
			default:
				Serial.print(" Extracting file ");
				Serial.println(buff);
				f = create_file(fullpath, parseoct(buff + 100, 8));
				break;
			}
			while (filesize > 0) {
				bytes_read = source->readBytes(buff, 512);
				if (bytes_read < 512) {
					Serial.print("Short read on ");
					Serial.print(path);
					Serial.print(": Expected 512, got ");
					Serial.println(bytes_read);
					return;
				}
				if (filesize < 512)
					bytes_read = filesize;
				if (f != NULL) {
					if (f->write((uint8_t*)buff, bytes_read) != bytes_read) {
						Serial.println("Failed write");
						f->close();
						f = NULL;
					}
				}
				filesize -= bytes_read;
			}
			if (f != NULL) {
				f->close();
				f = NULL;
			}					
			free(fullpath);
		}
	}
}