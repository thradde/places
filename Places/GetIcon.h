
typedef unsigned int uint32;
typedef uint32 *DIBitmap;

DIBitmap GetIconBitmap(LPCTSTR filePath, int &width, int &height);
DIBitmap GetIconBitmap(LPCTSTR filePath, int &pixels, bool thumbnail);
