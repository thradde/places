
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#ifdef _DEBUG
	#define DEBUG_NEW   new( _NORMAL_BLOCK, __FILE__, __LINE__)
#else
	#define DEBUG_NEW
#endif

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif
