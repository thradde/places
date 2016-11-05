//
//	Stream.h
//	========
//
//	11 / 2003 T. Radde
//

// =================================================================================
//							types and defines
// =================================================================================
#ifndef _MAX_PATH
	#define _MAX_PATH	4096
#endif

#ifdef _WINDOWS
	#define PATH_SEPARATOR			_T('\\')	// Windows
	#define PATH_SEPARATOR_STRING	_T("\\")
	#define CURRENT_PATH			_T(".\\")
	#define PREV_PATH				_T("..\\")
#else
	#define PATH_SEPARATOR			_T('/')		// UNIX
	#define PATH_SEPARATOR_STRING	_T("/")
	#define CURRENT_PATH			_T("./")
	#define PREV_PATH				_T("../")
#endif


// =================================================================================
//									Prototypes
// =================================================================================
bool TestWriteAccess(const RString &fname);


// =================================================================================
//								class Stream
// =================================================================================
class Stream
{
protected:
	FILE		*fh;
	RString		m_FileName;
	void		*m_pExtra;		// pointer that can be used freely by the caller

public:
	Stream(const RString &file_name, const RString &mode);
	virtual ~Stream();

	virtual bool Eof();

	virtual void Write(void const *buf, size_t count);
	virtual void WriteByte(BYTE buf);
	virtual void WriteBool(bool buf);
	virtual void WriteWord(WORD buf);
	virtual void WriteInt(int buf);
	virtual void WriteUInt(unsigned int buf);
	virtual void WriteInt64(__int64 buf);
	virtual void WriteString(const RString &s);	// size of string is written as int in advance

	virtual void	Read(void *buf, size_t count);
	virtual BYTE	ReadByte();
	virtual bool	ReadBool();
	virtual WORD	ReadWord();
	virtual int		ReadInt();
	virtual unsigned int ReadUInt();
	virtual __int64	ReadInt64();
	virtual RString ReadString();						// returns malloc'ed string, caller must free() it

	void *GetExtra() const { return m_pExtra; }
	void SetExtra(void *val) { m_pExtra = val; }
};
