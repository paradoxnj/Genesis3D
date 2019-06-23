Welcome to the Open Source release of the Genesis3D engine, version 1.1.  

This version is based on the 1.0 release - with some new features and a 
collection of bug fixes and repairs.

This release contains the source code to the Genesis3D engine, plus source 
code to all the tools that were used to build the Genesis3D demo GTest.  
This includes a world editor, texture packer and 3D Actor tools.  The file 
BUILD.TXT describes the layout of the source code, including where to find 
the individual project files, and information about which external tools 
you will have to obtain to build the engine and support code.

By installing the contents of this Open Source Distribution, you have 
agreed to be bound by the terms found in LICENSE.TXT.  Please refer to this 
file for licensing and contact information regarding this source.


New features for version 1.1:
· Distance Fog. (supported in the D3D driver and the Glide driver) See the 
  function geEngine_SetFogEnable( ) with comments in the Genesis.h header file. 
· Far clipping plane support. See the functions geCamera_SetFarClipPlane( ) 
  and geCamera_GetFarClipPlane( ) with comments in the Camera.h header file. 
· Texture lock feature in the gedit world editor. 
· 3D Studio Max R3 support with specific exporters built for R3. Find these 
  new exporters (keyexp.dle, nfoexp.dle) in /R3. Use these only for R3. 
  For R2.5, use the previous exporters with the same name in the root directory. 
· Texture pack tool (Tpack.exe) enhanced to also export textures from 
  previously packed data sets. 
· MIP-Mapping now works on actors, and user polygons if they have MIP maps. 
· An additional sound API is provided. geSound3D_GetConfigIgnoreObstructions( ) 
  behaves similarly to geSound3D_GetConfig( ), but it doesn't diminish the 
  sound if there are visible obstructions between the sound and the camera. 
  This essentially allows the sound to go through objects and world geometry.

Bugs fixed for version 1.1:
· Hint brushes work like they are designed to. Use them to create extra splits 
  in the BSP (a 'hint' to the level compiler) where visibility is otherwise 
  leaking around corners non ideally. 
· The D3D driver has extended ability to tailor the number of texture handles 
  to specific 3D cards. Cards that can handle more will be allowed more, and 
  cards that can't will be limited. The result is a performance gain on cards 
  that can support more handles, and no impact to those that can't. 
· The Software driver properly supports more screen pixel formats, and also 
  should work on NT and Windows 2000. 
· The drivers should always choose a reasonable MIP-Map level to render 
  textured polygons. 
· The D3D driver is not enabled if a Glide driver is present. This may cause 
  undesirable problems with secondary video cards being ignored, but avoids 
  an otherwise unavoidable black-screen lockup that happens when both Glide 
  and D3D are accessed during a single session. Deleting or removing the 
  glide.dll will result in D3D only, which should also now work properly on 
  3DFX chipsets. 
· D3D driver identification for some 3D hardware cards with very long 
  identification strings sometimes caused crashes or instabilities. This is 
  repaired.
· If the D3D log file existed and was read-only, the D3D driver improperly 
  reported a startup error. Now, if this condition occurs, D3D will attempt 
  to run, but the log file will not be written. 
· The D3D driver has an additional flush after shaded polygons to compensate 
  for drivers that cannot properly switch between shaded polygons and textured 
  polygons if they are both in the D3D drawing cache. 
· A texture alignment problem with the Glide driver was repaired. 
· Fixed a crash problem that happened when converting 24 bit bitmaps to 
  8 bit bitmaps with palettes. 
· Fixed a problem with geVFile_GetS( ) when used on in-memory VFiles.
· The startup logo is properly centered.

Changes to GTest for version 1.1:
· A new command line to demonstrate fog in action.  -fog enables this 
  feature demo.  Remember that via the API you have control over the 
  depth of the fog and it's color.
· A new command line to demonstrate the far clipping plane working.  
  -farclip enables this feature demo.  The API gives control over how far 
  out the far clipping plane is.  Combined with fog, this can be used to 
  increase the performance of levels with large expansive areas.

Please review the release notes in the 
HTML document installed to your Genesis3D folder as "Release Notes". 

We hope that you will use the source to this engine to build things that 
are new, exciting and fun.  We have provided the source in the interest of 
sharing ideas and promoting easier development of 3D applications in 
today's world of ever improving hardware.  Please enjoy yourselves, and 
help the industry where ever you can by donating your improvements back 
into the main source branch of Genesis3D.  Good luck, and we hope to hear 
from you.

November, 1999
The Genesis3D Team


