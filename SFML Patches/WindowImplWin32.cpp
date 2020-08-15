diff U11 src/SFML/Window/Win32/WindowImplWin32.cpp src/SFML/Window/Win32/WindowImplWin32.cpp
--- src/SFML/Window/Win32/WindowImplWin32.cpp	Mon Jan 19 23:26:36 1970
+++ src/SFML/Window/Win32/WindowImplWin32.cpp	Mon Jan 19 23:26:36 1970
@@ -741,25 +741,51 @@
                     Event event;
                     event.type = Event::MouseEntered;
                     pushEvent(event);
                 }
 
                 // Generate a MouseMove event
                 Event event;
                 event.type        = Event::MouseMoved;
                 event.mouseMove.x = x;
                 event.mouseMove.y = y;
                 pushEvent(event);
-                break;
             }
         }
+		break;
+
+		case WM_DROPFILES:
+		{
+			TCHAR *buffer;
+			Event event;
+			event.type = Event::DropFiles;
+			event.dropFiles.listDroppedFiles = new std::list<TCHAR *>;
+			UINT nFileCount = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
+			for (unsigned int n = 0; n < nFileCount; n++)
+			{
+				UINT size = DragQueryFile((HDROP)wParam, n, 0, 0);
+				if (size > 0)
+				{
+					buffer = new TCHAR[size + 1];
+					UINT result_size = DragQueryFile((HDROP)wParam, n, buffer, size + 1);
+					if (result_size == 0)
+						*buffer = 0;	// something is wrong...
+				}
+				else
+					buffer = NULL;		// something is wrong...
+				event.dropFiles.listDroppedFiles->push_back(buffer);
+			}
+			DragFinish((HDROP)wParam); 
+			pushEvent(event);
+		}
+		break; 
     }
 }
 
 
 ////////////////////////////////////////////////////////////
 Keyboard::Key WindowImplWin32::virtualKeyCodeToSF(WPARAM key, LPARAM flags)
 {
     switch (key)
     {
         // Check the scancode to distinguish between left and right shift
         case VK_SHIFT :
