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
 *  https://github.com/emelianov/untarArduino
 *
 */

// Uncomment following definition to enable not flat namespace filesystems
//#define TAR_MKDIR

// Unomment to suppress output extracting process messages to Serial
//#define TAR_SILENT

// Comment to remove callback support
//#define TAR_CALLBACK

#ifdef TAR_CALLBACK
typedef void (*cbTarData)(char* buff, size_t size);
typedef bool (*cbTarProcess)(char* buff);
typedef void (*cbTarEof)();
#endif

template <typename T>
class Tar {
public:
	Tar(T* dst) {
		FSC = dst;
		pathprefix = "";
	}
	void dest(char* path);		// Set directory extract to. tar -C  
	void stream(Stream* src);	// Source stream. Can use (Stream*)File as source
	void extract();			// Extract a tar archive
#ifdef TAR_CALLBACK
	void onFile(cbTarProcess cb);
	void onData(cbTarData cb);
	void onEof(cbTarEof cb);
#endif
private:
	char* pathprefix;
	int parseoct(const char *p, size_t n);		// Parse an octal number, ignoring leading and trailing nonsense.
	int is_end_of_archive(const char *p);		// Returns true if this is 512 zero bytes.
#ifdef TAR_MKDIR
	void create_dir(char *pathname, int mode);	// Create a directory, including parent directories as necessary.
#endif
	File *create_file(char *pathname, int mode);	// Create a file, including parent directory as necessary.
	int verify_checksum(const char *p);		// Verify the tar checksum.
	T* FSC;						// FS object
	Stream* source;					// Source stream
#ifdef TAR_CALLBACK
	cbTarProcess cbProcess = NULL;			// bool cbExclude(filename) calback. Return 'false' means skip file creation then
	cbTarData cbData = NULL;			// cbNull(data, size) callback. Called for each data block if file creation was skipped.
	cbTarEof cbEof = NULL;				// cnEof() callback. Called on end of file if file was skipped or not.
#endif
};
#ifdef TAR_CALLBACK
template <typename T>
void Tar<T>::onFile(cbTarProcess cb){
	cbProcess = cb;
}

template <typename T>
void Tar<T>::onData(cbTarData cb){
	cbData = cb;
}

template <typename T>
void Tar<T>::onEof(cbTarEof cb){
	cbEof = cb;
}
#endif
template <typename T>
void Tar<T>::dest(char* path){
	pathprefix = (char*)malloc (strlen(path) + 1);
		if (pathprefix != NULL) {
			strcpy (pathprefix, path);
		} else {
		#ifndef TAR_SILENT
			Serial.println("Memory allocation error");
		#endif
			pathprefix = "";
     	}
}

template <typename T>
void Tar<T>::stream(Stream* src){
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
	#ifndef TAR_SILENT
		Serial.print("Could not create directory %s");
		Serial.println(pathname);
	#endif
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
void Tar<T>::extract()
{
	char buff[512];
	File *f = NULL;
	size_t bytes_read;
	int filesize;
#ifndef TAR_SILENT
	Serial.println("\nExtracting from source:");
#endif
	for (;;) {
		bytes_read = source->readBytes(buff, 512);
		if (bytes_read < 512) {
		#ifndef TAR_SILENT
			Serial.print(" * Short read: expected 512, got ");
			Serial.println(bytes_read);
		#endif
			return;
		}
		if (is_end_of_archive(buff)) {
		#ifndef TAR_SILENT
			Serial.print("End of source file");
		#endif
			return;
		}
		if (!verify_checksum(buff)) {
		#ifndef TAR_SILENT
			Serial.println("* Checksum failure");
		#endif
			return;
		}
		char* fullpath = (char*)malloc (strlen(pathprefix) + strlen(buff) + 1);
		if (fullpath == NULL) {
		#ifndef TAR_SILENT
			Serial.println("* Memory allocation error. Ignoring entry");
		#endif
     	} else {     	
			filesize = parseoct(buff + 124, 12);
			strcpy (fullpath, pathprefix);
			strcat (fullpath, buff);
			switch (buff[156]) {
			case '1':
			#ifndef TAR_SILENT
				Serial.print("- Ignoring hardlink ");
				Serial.println(buff);
			#endif
				break;
			case '2':
			#ifndef TAR_SILENT
				Serial.print("- Ignoring symlink");
				Serial.println(buff);
			#endif
				break;
			case '3':
			#ifndef TAR_SILENT
				Serial.print("- Ignoring character device");
				Serial.println(buff);
			#endif
				break;
			case '4':
			#ifndef TAR_SILENT
				Serial.print("- Ignoring block device");
				Serial.println(buff);
			#endif
				break;
			case '5':
				filesize = 0;
			#ifdef TAR_MKDIR
			#ifndef TAR_SILENT
				Serial.print("- Extracting dir ");
				Serial.println(buff);
			#endif
				create_dir(buff, parseoct(buff + 100, 8));
			#else
			#ifndef TAR_SILENT
				Serial.print("- Ignoring dir ");
				Serial.println(buff);
			#endif
			#endif
				break;
			case '6':
			#ifndef TAR_SILENT
				Serial.print("- Ignoring FIFO ");
				Serial.println(buff);
			#endif
				break;
			default:
			#ifndef TAR_SILENT
				Serial.print("- Extracting file ");
				Serial.print(buff);
			#endif
			#ifdef TAR_CALLBACK
				if (cbProcess == NULL || cbProcess(buff)) {
			#endif
					f = create_file(fullpath, parseoct(buff + 100, 8));
			#ifdef TAR_CALLBACK
				}
			#endif
				break;
			}
			while (filesize > 0) {
				#ifndef TAR_SILENT
				Serial.print(".");
				#endif
				bytes_read = source->readBytes(buff, 512);
				if (bytes_read < 512) {
				#ifndef TAR_SILENT
					Serial.print("Short read: Expected 512, got ");
					Serial.println(bytes_read);
				#endif
					return;
				}
				if (filesize < 512)
					bytes_read = filesize;
				if (f != NULL) {
					if (f->write((uint8_t*)buff, bytes_read) != bytes_read) {
					#ifndef TAR_SILENT
						Serial.println("Failed write");
					#endif
						f->close();
						f = NULL;
					}
				}
				#ifdef TAR_CALLBACK
				if (cbData != NULL)
					cbData(buff, bytes_read);
				#endif
				filesize -= bytes_read;
			}
			if (f != NULL) {
				#ifndef TAR_SILENT
				Serial.println();
				#endif
				f->close();
				f = NULL;
			}					
			free(fullpath);
		#ifdef TAR_CALLBACK
			if (cbEof != NULL)
				cbEof();
		#endif
		}
	}
}