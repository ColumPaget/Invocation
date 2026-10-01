#ifdef HAVE_LIBUSEFUL
#include "libUseful-5/libUseful.h"
#else
#include "libUseful-bundled/libUseful.h"
#endif

#define VERSION "2.0"

int IsItemInList(const char *Item, const char *StrList);
