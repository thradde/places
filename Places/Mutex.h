
// Copyright (c) 2020 IDEAL Software GmbH, Neuss, Germany.
// www.idealsoftware.com
// Author: Thorsten Radde
//
// This program is free software; you can redistribute it and/or modify it under the terms of the GNU 
// General Public License as published by the Free Software Foundation; either version 2 of the License, 
// or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General Public 
// License for more details.
//
// You should have received a copy of the GNU General Public License along with this program; if not, 
// write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
//

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
