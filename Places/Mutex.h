
// --------------------------------------------------------------------------------------------------------------------------------------------
//																class CMutex
// --------------------------------------------------------------------------------------------------------------------------------------------
class CMutex
{
protected:
	CRITICAL_SECTION m_CS;

public:
	CMutex() { ::InitializeCriticalSection(&m_CS); }

	~CMutex() { ::DeleteCriticalSection(&m_CS); }

	void Enter() { ::EnterCriticalSection(&m_CS); }
	void Leave() {  ::LeaveCriticalSection(&m_CS); }
};


class CMutexLock
{
protected:
	CMutex	&m_Mutex;

public:
	CMutexLock(CMutex &mutex)
		:	m_Mutex(mutex)
	{
		m_Mutex.Enter();
	}

	~CMutexLock()
	{
		m_Mutex.Leave();
	}
};


#if 0
unsigned __stdcall ResyncIcons(void *param)
{
	CBitmapCache *pCache = (CBitmapCache *)param;
	param->ResyncIcons();
	return 1;
}


void CBitmapCache::ResyncThreaded()
{
	unsigned aID;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ResyncIcons, this, 0, &aID);
}



class CWriteToFileThreadParams
{
public:
	CIconManager	*m_pIconManager;
	RString			&m_strFileName;
	CSettings		&m_Settings;

public:
	CWriteToFileThreadParams(CIconManager *pIconManager, const RString &file_name, const CSettings &settings)
		:	m_pIconManager(pIconManager),
			m_strFileName(file_name),
			m_Settings(settings)
	{
	}
};


unsigned __stdcall WriteToFileThread(void *param)
{
	CWriteToFileThreadParams *wtf = (CWriteToFileThreadParams *)param;
	wtf->m_pIconManager->SetIsBusyOnFile(true);
	wtf->m_pIconManager->WriteToFile(wtf->m_strFileName, wtf->m_Settings);
	wtf->m_pIconManager->SetIsBusyOnFile(false);
	return 1;
}


void CIconManager::WriteToFileThreaded(const RString &file_name, const CSettings &settings) const
{
	unsigned aID;
	CWriteToFileThreadParams params(this, file_name, settings);
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, WriteToFileThread, &params, 0, &aID);
}
#endif
