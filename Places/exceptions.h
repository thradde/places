//
//	Exceptions.h
//	============
//
//	11 / 2003 T. Radde
//


class Exception
{
public:
	enum ExceptionCode
	{
		EXCEPTION_PLAIN_TEXT,		// no standard message
		EXCEPTION_GENERAL_FAILURE,
		EXCEPTION_OUT_OF_MEMORY,

		EXCEPTION_FILE_OPEN,
		EXCEPTION_FILE_READ,
		EXCEPTION_FILE_WRITE,

		EXCEPTION_INI_FILE_CORRUPT,

		EXCEPTION_DB_CORRUPT,
		EXCEPTION_DB_VERSION,
	};

protected:
	ExceptionCode	m_enExceptionCode;
	RString			m_strAdditionalInfo;

public:
	Exception(ExceptionCode code, const RString &additional_info = _T("")) { m_enExceptionCode = code; m_strAdditionalInfo = additional_info; }
	Exception(const RString &plain_text = _T("")) { m_enExceptionCode = EXCEPTION_PLAIN_TEXT; m_strAdditionalInfo = plain_text; }

	ExceptionCode	GetExceptionCode() const { return m_enExceptionCode; }
	const RString	GetExceptionMessage() const;
};


class CriticalException : public Exception
{
public:
	CriticalException(ExceptionCode code)
		:	Exception(code)
	{
	}
};
