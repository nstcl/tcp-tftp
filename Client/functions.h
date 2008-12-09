#ifndef _FUNCTIONS_H
#define	_FUNCTIONS_H
 void changeDirectory(const char *path);
 void writeToFile();
 void split(char *s,char* delimiter,char*** parameters);
 int file_exists (char * fileName);
#ifdef	__cplusplus
extern "C" {
#endif

// void chagneDirectory(const char *path);


#ifdef	__cplusplus
}
#endif

#endif	/* _FUNCTIONS_H */

