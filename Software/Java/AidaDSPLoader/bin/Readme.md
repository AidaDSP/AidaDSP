### AidaDSPLoader

With this command line utility written in Java, it's possible (for now only with ADAU1701)
to **WRITE** a program contained in an .xml file generated from Sigma Studio into the DSP.
It's also possible to make this utility run at the end of export procedure directly from Sigma Studio:
- Download AidaDSPLoader.jar and copy it in your Sigma Studio project folder (together with **.dspproj** file)
- Program **Arduino** with the _sketch_ provided in _**Software\Examples\Arduino\Advanced\sketch_AidaDSP_Loader\sketch_AidaDSPLoader.ino**_
- in Sigma Studio, go to _**Tools->Settings->System File Export**_
- search for _**Post-export Command**_ and type in the string _AidaDSPLoader.jar Export\Tutorial_2.xml COM10_ (modify it to fit your needs)
- now when you run the command _**Action->Export System Files**_ the command utility also is being called from Sigma Studio at the end
of export file generation procedure

_**Syntax:**_
AidaDSPLoader.jar filepath comport

_**Example:**_
AidaDSPLoader.jar C:\Tutorial_2.xml COM10

_**FAQ:**_
- What's my COM port?
It's a string that identify the USB serial port where your Arduino board is listening to.

_**ASSISTANCE:**_
mail me at maxipenna@libero.it

