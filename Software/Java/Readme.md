# Java utilities for Aida DSP 

* __AidaHeaderFileGenerator.jar__
with this program you can create a c header file 
from .xml file you generate with Sigma Studio 
with compile and export menu commands [AidaHeaderFileGenerator.jar](../Java/AidaHeaderFileGenerator/bin) 

* __AidaDSPLoader.jar__
this is a command line utility for Windows and Linux which
you can use to "send" a Sigma Studio .xml exported file 
to an Arduino board listening on COM port [AidaDSPLoader.jar](../Java/AidaDSPLoader/bin) 
  * Sintax: AidaDSPLoader [dir] [filename] [comport]
  * Example: AidaDSPLoader C:\AidaDSP myfile COM2
  * Sketch to use with: [sketch.ino](../Examples) 


