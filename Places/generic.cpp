
#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include <tchar.h>
#include <shlobj.h>
#include <shlguid.h>
#include <shellapi.h>

#include <SFML/Graphics.hpp>

#include <vector>
using namespace std;

#define ASSERT
#include "hook.h"
#include "RfwString.h"
#include "platform.h"
#include "generic.h"


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
