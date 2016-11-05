
    -------------------
    Places Design Goals
    -------------------

-   Starting applications or opening folder-/ file links as quick as possible with a minimum
    number of user interactions (i.e. mouse clicks, keys presses).

-   Everything stays in its place: icons will always stay in the same position, where you once placed them.
    No sorting, no re-ordering, no changes if the resolution of the screen changes. Instead, the icons 
    are scaled accordingly to the screen resolution. You will always find your icons at the same position 
    with a quick mouse click. Therefore the name "Places".

-   Eye-Candy and Smooth
    - animated icons
    - animated smooth scrolling between pages
    - bitmaps for icons can be loaded from external files
    - use shaders, patterns or images as background wallpaper
    
    - Intuitive and fast user interface
    - Places is moved to the front of the screen and is not hidden by other windows
    - unlimited undo and redo (ctrl + z, ctrl + y)
    - Lasso to select and de-select groups of items (groups of icons can be arranged or moved to other pages)
    - provides a total of 10 pages
    - special windowed mode, also useful to drag apps and folders into "Places"
    - wipe to scroll between pages
    
    - activate / hide Places with a definable special mouse-button, if you have more than 2 mouse-buttons
    - activation can be suspended, when a full-screen app is running in foreground, e.g. a game
    - activate / hide Places with a definable special key, or key combination, eg F12
    - activate Places with a double-click into the free desktop space of the Windows Program Manager
    - activate Places by moving the mouse into the upper-right corner of the screen


    System Requirements
    -------------------
    - Windows Vista ore higher
    - Graphics Card with OpenGL driver


    User Interface
    --------------
    Keyboard
    - ctrl + z  : undo
    - ctrl + y  : redo
    - ctrl + a  : select all icons on the current page
    - F2        : if a single icon is selected, show its properties
    - page up   : one page to the left
    - page down : one page to the right
    - home      : first page
    - end       : last page
    - arrow keys: navigate within the current page from one icon to the next
    - delete    : delete selected icons
    - digit 1-0 : jump to n-th page (also works with numpad)
    - Enter     : open selected link (if a single icon is selected)
    - ESC       : hide Places
    
    Mouse
    Icon operations
    - short left-click onto icon: open / execute
    - long left-click onto icon: select
    - ctrl + left-click onto icon: select / deselect; other selected icons stay selected
    - right-click onto icon to choose between "properties" or "remove"
    - left-click onto selected icon(s) and drag the mouse: move selected icon(s)
    
    Lasso
    - alt + left-click: start lasso mode (all selected icons are deselected)
    - shift + alt + left-click: start lasso mode (additive, all selected icons stay selected)
    - ctrl + alt + left-click: start lasso mode (subtractive, covered selected icons are deselected)
    
    Scrolling
    - left-click into free space and drag mouse left / right to scroll to another page
    - use the scroll-wheel of the mouse
    
    Other operations
    - right-click onto free space for global actions
    - left-click onto a page indicator (the white circles at the bottom) to jump to a selected page
    - left-click into free space to hide Places

    
    Global Actions Menu (right-click into free space):
    --------------------------------------------------
    - Toggle between full-screen and windowed mode
    - Undo / Redo
    - Save
      Save the database. If there are any relevant changes, Places always saves the database, when it is terminated.
    - Reload Icons
      All icons are checked, whether their target has changed its icon. If so, it is reloaded. This is done in background.
    - Settings
      Dialog with global program settings
    - About: program info
    - Exit: terminate Places

    
    Settings Dialog
    ---------------
    - activate / hide with a definable special key, or key combination, eg F12
    - activate / hide with a definable special mouse-button, if you have more than 2 mouse-buttons
    - activation can be suspended, when a full-screen app is running in foreground, e.g. a game
    - activate by moving the mouse into the upper-right corner [a definable corner] of the screen
	  the mouse needs to hover still-standing for at least 1.25 sec


Run Programs, Open Documents, Folders and Websites, or Write e-Mails
--------------------------------------------------------------------
- For quick access, drag programs, documents or folders into Places.
  Documents will be opened in their associated application, and folders will open in Windows Explorer.
- To place a link to a website, right-click into a free space of Places and choose "New Icon".
  Edit the icon's file path and set it to, e.g.: http://www.something.com
  Assign a nice image to the icon using the "Icon" edit field.
  A click onto the icon will open the URL in the default web browser.
- For e-Mails, right-click into a free space of Places and choose "New Icon".
  Edit the icon's file path and set it to, e.g.: mailto:someone@somewhere.com
  Assign a nice image to the icon using the "Icon" edit field.
  A click onto the icon will open your default mail client.
- A great set of free and beautiful icons can be found in the "Open Icon Library". Use the 128 x 128 pixel size.
- When you are looking for a special icon for an application, use Googles image search. Either enter the name of
  the application, or enter its type, e.g. the search term "text editor icon" will present you lots of beautiful
  icons for text editors. The recommended image size is 128 x 128 pixels in PNG or JPG format.


Stuttering Scrolling
--------------------
If the scrolling should stutter, you can try the following:
nVidia:  try setting "Threaded Optimization = Off" in the nVidia driver panel
ATI/AMD: try setting "Wait for vertical refresh = Always Off" in the ATI/AMD driver panel


Shaders
-------
Shaders require a graphics card, which can run shaders. They won't work on older
graphics cards and an error message is shown ("Couldn't load shader."). 
The shaders have been taken with their original names from https://www.shadertoy.com/
They have been slightly modified to work with Places/SFML (see below).
The author of each shader can be seen, when you open a shader file with notepad.exe.


SFML
----
Places uses SFML, which is a very good multi-media library for C++.


Backup
------
If you wish to backup the configuration of Places, you need to backup the folder "%appdata%\Places"

Note: %appdata% usually resolves to C:\Users\<user name>\AppData\Roaming, but you can
      also enter "%appdata%" in Explorer.


Tips:
-----
- You can activate Places with a double-click into the free desktop space of the Windows program manager
- The background patterns shipped with Places are taken from http://subtlepatterns.com/, you can find there
  many other nice patterns.


You can create new icons, which point to special Windows folders, for example the "Libraries" folder in Windows 7:
- Right-click into the desktop of Places and select "New Icon".
- Enter a title.
- For "File", enter "<explorer>" (all lower-case without quotes).
- For "Parameters", enter one of the commands listed below, e.g. "shell:Libraries".
- Choose a nice icon file.


Commands in Windows 7
---------------------
source: http://www.winhelponline.com/blog/shell-commands-to-access-the-special-folders/

In addition to most of the shell commands in Windows Vista:

shell:Libraries
shell:MusicLibrary
shell:VideosLibrary
shell:OtherUsersFolder
shell:Device Metadata Store
shell:PublicSuggestedLocations
shell:SuggestedLocations
shell:RecordedTVLibrary
shell:UserProgramFiles
shell:DocumentsLibrary
shell:User Pinned
shell:UsersLibrariesFolder
shell:PicturesLibrary
shell:ImplicitAppShortcuts
shell:UserProgramFilesCommon
shell:Ringtones
shell:CommonRingtones


Commands in Windows Vista
-------------------------
shell:Common Programs
shell:GameTasks
shell:UserProfiles
shell:MyComputerFolder
shell:SyncSetupFolder
shell:DpapiKeys
shell:SamplePlaylists
shell:Favorites
shell:My Video
shell:SearchHomeFolder
shell:System
shell:CommonVideo
shell:SyncResultsFolder
shell:LocalizedResourcesDir
shell:Cookies
shell:Original Images
shell:CommonMusic
shell:My Pictures
shell:Cache
shell:Downloads
shell:CommonDownloads
shell:AppData
shell:SyncCenterFolder
shell:My Music
shell:ConflictFolder
shell:SavedGames
shell:InternetFolder
shell:Quick Launch
shell:SystemCertificates
shell:Contacts
shell:TreePropertiesFolder
shell:Profile
shell:Start Menu
shell:Common AppData
shell:PhotoAlbums
shell:ConnectionsFolder
shell:Administrative Tools
shell:PrintersFolder
shell:Default Gadgets
shell:ProgramFilesX86
shell:Searches
shell:Common Startup
shell:ControlPanelFolder
shell:SampleVideos
shell:SendTo
shell:ResourceDir
shell:ProgramFiles
shell:CredentialManager
shell:PrintHood
shell:MAPIFolder
shell:CD Burning
shell:AppUpdatesFolder
shell:Common Start Menu
shell:LocalAppDataLow
shell:Templates
shell:Gadgets
shell:Programs
shell:Recent
shell:SampleMusic
shell:Desktop
shell:CommonPictures
shell:RecycleBinFolder
shell:CryptoKeys
shell:Common Templates
shell:Startup
shell:Links
shell:OEM Links
shell:SamplePictures
shell:Common Desktop
shell:NetHood
shell:Games
shell:Common Administrative Tools
shell:NetworkPlacesFolder
shell:SystemX86
shell:History
shell:AddNewProgramsFolder
shell:Playlists
shell:ProgramFilesCommonX86
shell:PublicGameTasks
shell:ChangeRemoveProgramsFolder
shell:Public
shell:Common Documents
shell:CSCFolder
shell:Local AppData
shell:Windows
shell:UsersFilesFolder
shell:ProgramFilesCommon
shell:Fonts
shell:Personal


Commands in Windows XP
----------------------
shell:Common Programs
shell:Favorites
shell:My Video
shell:System
shell:CommonVideo
shell:LocalizedResourcesDir
shell:Cookies
shell:My Pictures
shell:Cache
shell:AppData
shell:My Music
shell:InternetFolder
shell:Profile
shell:Start Menu
shell:Common AppData
shell:ConnectionsFolder
shell:Administrative Tools
shell:PrintersFolder
shell:ProgramFiles
shell:Common Startup
shell:ControlPanelFolder
shell:SendTo
shell:ResourceDir
shell:ProgramFiles
shell:PrintHood
shell:CD Burning
shell:Common Start Menu
shell:Templates
shell:Programs
shell:Recent
shell:Desktop
shell:CommonPictures
shell:RecycleBinFolder
shell:Common Templates
shell:Startup
shell:Common Desktop
shell:NetHood
shell:Common Administrative Tools
shell:SystemX86
shell:History
shell:Common Documents
shell:Local AppData
shell:Windows
shell:Fonts
shell:Personal

            
Other useful locations:
-----------------------
    - My Computer:          File: <explorer>      Parameters: =
    - A hard drive:         File: <explorer>      Parameters: <drive letter>: (eg "c:")
    - Network Computers:    File: <explorer>      Parameters: ::{208d2c60-3aea-1069-a2d7-08002b30309d}
    - Add/Remove Programs:  File: <control>       Parameters: appwiz.cpl
    - Printers:             File: <control>       Parameters: printers.cpl
    - Sound Properties:     File: <control>       Parameters: mmsys.cpl sounds
    - Scanners and Cameras: File: <control>       Parameters: sticpl.cpl
    - Fonts:                File: <control>       Parameters: fonts.cpl
    - Device Manager:       File: <mmc>           Parameters: devmgmt.msc
    - Computer Management:  File: <mmc>           Parameters: compmgmt.msc
    - Disk Management:      File: <mmc>           Parameters: diskmgmt.msc


Enjoy,
Thorsten Radde
