Places is compiled and linked against SFML v2.3.
SFML was modified to support Drag & Drop.
I also experienced random crashes in DINPUT.DLL, which is related to joystick input.
This happened after Places was running for several days or weeks.
Therefore I disabled the SFML joystick processing, as it is not used in Places.
The patch files in this directory must be applied to the SFML 2.3 source code.