Description:
	This is a C++ that renders .obj files. The user has the option to control the camera and some of the rendering effects.

Controls:
	This program is entirely mouse driven, left-click+drag will edit the
camera in the currently selected manner. Right-clicking will open a menu that
allows you to change your Left-click behavior, view a different model, change
rendering behaviors, and edit lighting.

Usage:
	This program takes command line arguments to know which models to list.
The arguments are ordered as follows: 
<path-to-folder> <rel-path-to-model-1> <rel-path-to-model-2> ...
The models paths will be relative to "<path-to-folder>" The path to the models
should end with the name of the .obj file (excluding ".obj).


Features:
  OpenGL 3D rendering
  Transparency
  Model transformations