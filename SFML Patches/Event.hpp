diff U11 include/SFML/Window/Event.hpp include/SFML/Window/Event.hpp
--- include/SFML/Window/Event.hpp	Mon Jan 19 23:26:36 1970
+++ include/SFML/Window/Event.hpp	Mon Jan 19 23:26:36 1970
@@ -24,22 +24,24 @@
 
 #ifndef SFML_EVENT_HPP
 #define SFML_EVENT_HPP
 
 ////////////////////////////////////////////////////////////
 // Headers
 ////////////////////////////////////////////////////////////
 #include <SFML/Config.hpp>
 #include <SFML/Window/Joystick.hpp>
 #include <SFML/Window/Keyboard.hpp>
 #include <SFML/Window/Mouse.hpp>
+#include <tchar.h>
+#include <list>
 
 
 namespace sf
 {
 ////////////////////////////////////////////////////////////
 /// \brief Defines a system event and its parameters
 ///
 ////////////////////////////////////////////////////////////
 class Event
 {
 public :
@@ -133,22 +135,31 @@
     ////////////////////////////////////////////////////////////
     /// \brief Joystick buttons events parameters
     ///        (JoystickButtonPressed, JoystickButtonReleased)
     ///
     ////////////////////////////////////////////////////////////
     struct JoystickButtonEvent
     {
         unsigned int joystickId; ///< Index of the joystick (in range [0 .. Joystick::Count - 1])
         unsigned int button;     ///< Index of the button that has been pressed (in range [0 .. Joystick::ButtonCount - 1])
     };
 
+	////////////////////////////////////////////////////////////
+	/// \brief DropFiles events parameters
+	///
+	////////////////////////////////////////////////////////////
+	struct DropFilesEvent
+	{
+		std::list<TCHAR *> *listDroppedFiles;	///< List of strings with names of dropped files
+	};
+
     ////////////////////////////////////////////////////////////
     /// \brief Enumeration of the different types of events
     ///
     ////////////////////////////////////////////////////////////
     enum EventType
     {
         Closed,                 ///< The window requested to be closed (no data)
         Resized,                ///< The window was resized (data in event.size)
         LostFocus,              ///< The window lost the focus (no data)
         GainedFocus,            ///< The window gained the focus (no data)
         TextEntered,            ///< A character was entered (data in event.text)
@@ -157,42 +168,44 @@
         MouseWheelMoved,        ///< The mouse wheel was scrolled (data in event.mouseWheel)
         MouseButtonPressed,     ///< A mouse button was pressed (data in event.mouseButton)
         MouseButtonReleased,    ///< A mouse button was released (data in event.mouseButton)
         MouseMoved,             ///< The mouse cursor moved (data in event.mouseMove)
         MouseEntered,           ///< The mouse cursor entered the area of the window (no data)
         MouseLeft,              ///< The mouse cursor left the area of the window (no data)
         JoystickButtonPressed,  ///< A joystick button was pressed (data in event.joystickButton)
         JoystickButtonReleased, ///< A joystick button was released (data in event.joystickButton)
         JoystickMoved,          ///< The joystick moved along an axis (data in event.joystickMove)
         JoystickConnected,      ///< A joystick was connected (data in event.joystickConnect)
         JoystickDisconnected,   ///< A joystick was disconnected (data in event.joystickConnect)
+		DropFiles,				///< Files have been dropped via Drag&Drop (data in event.dropFiles)
 
         Count                   ///< Keep last -- the total number of event types
     };
 
     ////////////////////////////////////////////////////////////
     // Member data
     ////////////////////////////////////////////////////////////
     EventType type; ///< Type of the event
 
     union
     {
         SizeEvent            size;            ///< Size event parameters (Event::Resized)
         KeyEvent             key;             ///< Key event parameters (Event::KeyPressed, Event::KeyReleased)
         TextEvent            text;            ///< Text event parameters (Event::TextEntered)
         MouseMoveEvent       mouseMove;       ///< Mouse move event parameters (Event::MouseMoved)
         MouseButtonEvent     mouseButton;     ///< Mouse button event parameters (Event::MouseButtonPressed, Event::MouseButtonReleased)
         MouseWheelEvent      mouseWheel;      ///< Mouse wheel event parameters (Event::MouseWheelMoved)
         JoystickMoveEvent    joystickMove;    ///< Joystick move event parameters (Event::JoystickMoved)
         JoystickButtonEvent  joystickButton;  ///< Joystick button event parameters (Event::JoystickButtonPressed, Event::JoystickButtonReleased)
         JoystickConnectEvent joystickConnect; ///< Joystick (dis)connect event parameters (Event::JoystickConnected, Event::JoystickDisconnected)
+		DropFilesEvent		 dropFiles;       ///< Files have been dropped via Drag&Drop event parameters (Event::DropFiles)
     };
 };
 
 } // namespace sf
 
 
 #endif // SFML_EVENT_HPP
 
 
 ////////////////////////////////////////////////////////////
 /// \class sf::Event
