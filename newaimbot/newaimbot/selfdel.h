#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC int cppMain();
EXTERNC void newFile(char *argv0);

#undef EXTERNC