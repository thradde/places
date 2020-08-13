
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

#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include <tchar.h>
#include <shlobj.h>
#include <shlguid.h>
#include <shellapi.h>

#include "memdebug.h"

#include <SFML/Graphics.hpp>

#include <vector>
using namespace std;

#define ASSERT
#include "hook.h"
#include "RfwString.h"
#include "platform.h"
#include "generic.h"


// --------------------------------------------------------------------------------------------------------------------------------------------
//													ExtractFileName()
// --------------------------------------------------------------------------------------------------------------------------------------------
RString ExtractFileName(const RString &path)
{
	RString file_name;

	int n = path.reverse_find(_T('\\'));
	if (n >= 0)
	{
		file_name = path.substr(n + 1);
		n = file_name.reverse_find(_T('.'));
		if (n >= 0)
			file_name = file_name.substr(0, n);
	}

	return file_name;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//													CSfmlApp::HandleEvents()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CSfmlApp::HandleEvents()
{
	sf::Event event;
	while (m_Window.pollEvent(event))
	{
		switch (event.type)
		{
		case sf::Event::Closed:
			if (OnClose())
				m_Window.close();
			break;

		case sf::Event::MouseButtonPressed:
			OnMouseDown(event);
			break;

		case sf::Event::MouseButtonReleased:
			OnMouseUp(event);
			break;

		case sf::Event::MouseMoved:
			OnMouseMove(event);
			break;

		case sf::Event::MouseWheelMoved:
			OnMouseWheel(event);
			break;

		case sf::Event::KeyPressed:
			OnKeyPressed(event);
			break;

		case sf::Event::DropFiles:
			if (event.dropFiles.listDroppedFiles)
			{
				OnDroppedFiles(event);
				delete event.dropFiles.listDroppedFiles;
			}
			break;
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//													CSfmlApp::Run()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CSfmlApp::Run()
{
	do
	{
		CreateMainWindow();
		m_bRecreateWindow = false;

		// event loop
		while (m_Window.isOpen())
		{
			Sleep(5);	// 10
			if (m_bRequestClose)
			{
				if (OnClose())
					m_Window.close();
			}
			HandleEvents();
			Work();

			if (IsWindowVisible(m_Window.getSystemHandle()))
			{
				Draw();
				m_Window.display();
			}
		}
	}
	while (m_bRecreateWindow);
}
