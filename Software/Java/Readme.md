# Java utilities for Aida DSP 

* __AidaHeaderFileGenerator.jar__
In Sigma Studio you can generate an .xml export file (hit Action->Link Compile Download and then hit Action->Export System Files)
which contains DSP firmware and the names/addresses of the cells (algorithm blocks) you've used in your project.
With the informations contained in this file you can download the program inside the DSP and control it in real-time 
unfortunately you can't compile this .xml file with Arduino. 
Our program parse this .xml file and generates a C header file (.h) and write in it all the precious informations you need to 
run your audio project. You can download a binary (.jar) [here](../Java/AidaHeaderFileGenerator/bin) 

* __AidaDSPLoader.jar (This program has not been fully tested please be patient...)__
This is a command line utility for Windows and Linux which
you can use to "send" a Sigma Studio .xml exported file 
to an Arduino board listening on COM port 
  * Sintax: AidaDSPLoader [dir] [filename] [comport]
  * Example: AidaDSPLoader C:\AidaDSP myfile COM2
  * Sketch to use with: [sketch.ino](../Examples) 
You can download a binary (.jar) [here](../Java/AidaDSPLoader/bin) 

