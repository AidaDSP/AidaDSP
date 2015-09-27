//VERSION 10.09.2015 05:00 P.M.

package AIDA;

import java.awt.*;
import java.awt.event.*;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.StringTokenizer;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.UIManager;
import javax.swing.filechooser.FileFilter;


/** A program that uses all the standard AWT components */
public class AIDA_header_file_generator extends Frame implements ActionListener {
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	TextArea textarea;  // Events messages will be displayed here.
	static String pathRead=null; 
	static String pathWrite=null;
	static int Ic1_count=0;
	static int pulse_count=0;
	static int djitter_count=0;
	static int Run_Bit_count=0;
	static String position="out";  // sed if i am in register, program or name
	static String strLine;
	static boolean pathWriteSelected=false;
	static String selectAddress="0x6C";
	static File selectedFile;
	public static String selectedDSP;
	private String OSName, OSversion,OSarchitecture, JVruntypeVersion;
	public static boolean okButtonPressed=false;
	private String[] addresses_vector =new String[4];
	private static String version="VERSION 10.09.2015 05:00 P.M.";

	/** Create the whole GUI, and set up event listeners */
	public AIDA_header_file_generator(String title) {
		
		super(title);  // set frame title.
		
		//information abouth OS
		OSName = System.getProperty("os.name").toLowerCase();
		OSversion = System.getProperty("os.version").toLowerCase();
		OSarchitecture = System.getProperty("os.arch").toLowerCase();	
		JVruntypeVersion = System.getProperty("java.runtime.version").toLowerCase();	
		
		/*JOptionPane.showMessageDialog(AIDA_header_file_generator.this,
				"OS name: " + OSName + "\n" + "OS version: " + OSversion + "\n" + "OS architecture: " + OSarchitecture + "\n" + "Java runtime version: " + JVruntypeVersion);*/
		
		//infoBox("Software report","OS name: " + OSName + "\n" + "OS version: " + OSversion + "\n" + "OS architecture: " + OSarchitecture + "\n" + "Java runtime version: " + JVruntypeVersion);
		
		
		System.out.println("OS name: ");
		System.out.println("OS version: ");
		System.out.println("OS architecture: ");
		System.out.println("Java runtime version: ");
		
		System.out.println("\n");
		
		//I read absolute path (where is the exe java file e.g. c:\micro\aida\file\)
		pathWrite=System.getProperty("java.class.path");				
		
		//first combo
		ComboBox conmbo=new ComboBox();			
		while(okButtonPressed==false){
			//I whait until you press OK button and select an micro 			
			//Do NOT erase next four code row!!!!!!
			if(okButtonPressed==false){
				try {
					Thread.sleep(2000);
				} catch (InterruptedException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
				System.out.println("Please, press button");
			}
			else
				System.out.println("Button pressed");
		}
		System.out.println("\nSelected DSP: " + selectedDSP);
		
		//now I insert addresses agree with selected DSP
		if(selectedDSP.compareTo("ADAU144x")==0){
			addresses_vector[0]="0x70";
			addresses_vector[1]="0x74";
			addresses_vector[2]="0x78";
			addresses_vector[3]="0x7C";
		}
		else if(selectedDSP.compareTo("ADAU170x")==0){
			addresses_vector[0]="0x68";
			addresses_vector[1]="0x6A";
			addresses_vector[2]="0x6C";
			addresses_vector[3]="0x6E";
		}		

		// Arrange to detect window close events
		this.addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent e) { System.exit(0); }
		});			
		
		// Set a default font
		this.setFont(new Font("SansSerif", Font.PLAIN, 12));

		Panel pannel_text = new Panel();
		Choice choice = new Choice();
		
		for(int i=0;i<4;i++)			
			choice.addItem(addresses_vector[i]);
		
		choice.select(3);
		selectAddress=choice.getSelectedItem();
	    
	    pannel_text.add(new Label("Address:"));
	    pannel_text.add(choice);	    
	    
		//pannel_text.setLayout(new FlowLayout(FlowLayout.TRAILING, 100, 40));
		this.add(pannel_text, "Center");
		
		// Handle events on this choice
	    choice.addItemListener(new ItemListener() {
	      public void itemStateChanged(ItemEvent e) {
	    	  selectAddress=e.getItem().toString();	    	  
	      }
	    });
	
		// Create a panel to contain the buttons at the bottom of the window
		// Give it a FlowLayout layout manager, and add it along the south border
		Panel buttonbox = new Panel();
		buttonbox.setLayout(new FlowLayout(FlowLayout.CENTER, 100, 10));
		this.add(buttonbox, "South");

		// Create pushbuttons and add them to the buttonbox
		JButton FileinFileChooser = new JButton("File IN");
		JButton generaFileChooser = new JButton("MAKE");
		JButton escFileChooser = new JButton("ESC");
		buttonbox.add(FileinFileChooser);
		buttonbox.add(generaFileChooser);
		buttonbox.add(escFileChooser);

		// Handle events on the buttons
		ActionListener buttonlistener = new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				if(((JButton)e.getSource()).getLabel().compareTo("File IN")==0){
					try {						
						textarea.setText("");						
						UIManager.put("FileChooser.cancelButtonText", "Cancel");
						JFileChooser fileChooser = new JFileChooser(System.getProperty("java.class.path")); 
						fileChooser.setFileFilter(new TxtFileFilter());
						fileChooser.setDialogTitle("FILE di INPUT");
						fileChooser.setApproveButtonText("OK"); 
						int n = fileChooser.showOpenDialog(AIDA_header_file_generator.this);  
						//FileFilter type = new ExtensionFilter("XML", ".xml");
						// fileChooser.addChoosableFileFilter(type);
						// fileChooser.setFileFilter(type); 

						if (n == JFileChooser.APPROVE_OPTION) {							
							textarea.append(" " );
							File f = fileChooser.getSelectedFile(); 
							pathRead=f.getPath();					
							selectedFile=new File(pathRead);							
							if(selectedFile.exists()==true)
								textarea.append("File di input:  " + f.getPath() + "\n");  
							else
								textarea.append("File  " + f.getPath() + "  IS NOT A VALID FILE" + "\n"); 
							         
						}
					} catch (Exception ex) {}    		  
				}
				else if(((JButton)e.getSource()).getLabel().compareTo("MAKE")==0){
					if(pathRead==null)
						pathRead=System.getProperty("user.dir");					
					
					selectedFile=new File(pathRead);
					if(selectedFile.isFile()==true){
						try {							
							//pathWrite=System.getProperty("user.dir");
							textarea.setText(""); 	  	        
							Estrapola();			
							textarea.append("\n\n"  );
							textarea.append("Created file: " + pathWrite + "\\AidaFW.h" );
							textarea.append("\n"  );
							textarea.append("With Address: " + selectAddress );
						} catch (IOException e1) {
							// TODO Auto-generated catch block
							e1.printStackTrace();
						}
					}
					else{
						textarea.setText("");
						textarea.append("\n\n "  );
						textarea.append("PLEASE, SELECT A VALID INPUT FILE"  );
					} 		  
				}
				else if(((JButton)e.getSource()).getLabel().compareTo("ESC")==0){
					System.exit(0);  
				}
				//textarea.append("You clicked: " +((JButton)e.getSource()).getLabel() + "\n");
                     
			}
		};
		FileinFileChooser.addActionListener(buttonlistener);
		generaFileChooser.addActionListener(buttonlistener);
		escFileChooser.addActionListener(buttonlistener);
		
		// Now start filling the left column.
		// Create a 1-line text field and add to left column, with a label
		textarea = new TextArea(6, 80);
		textarea.setEditable(false);
		pannel_text.add(textarea);
	}

	/** This is the action listener method that the menu items invoke */
	public void actionPerformed(ActionEvent e) {
	/*	String command = e.getActionCommand();
		if (command.equals("0X68")) {
			selectAddress="0X68";    
		}
		else if (command.equals("0X6A")) {
			selectAddress="0X6A";    
		}
		else if (command.equals("0X6C")) {
			selectAddress="0X6C"; 
		}
		else if (command.equals("0X6E")) {
			selectAddress="0X6E"; 	    
		}
		textarea.append("\nSelected Address: " + selectAddress );*/
	}

	
	public static void main(String[] args) {
		Frame f = new AIDA_header_file_generator(version);
		f.pack();
		f.show();
	}

	public static void Estrapola() throws IOException {
		// TODO Auto-generated method stub
		
		
		String buf,name,address,size;		
	    //String pathRead= new String("C:\\Users\\Gaetano\\Documents\\Tutorial_6.xml"); 
	    //String pathWrite = new String("C:\\Users\\Gaetano\\Documents\\out_Tutorial_6.txt"); 
	            //open file for read
	    BufferedReader reader =new BufferedReader(new FileReader(pathRead));
	    
	            //open file for write
	    FileWriter w;
	    w=new FileWriter(System.getProperty("user.dir") + "\\AidaFW.h");
	    //w=new FileWriter(pathWrite + "\\AidaFWh.h");
	    BufferedWriter fw;
	    fw=new BufferedWriter(w); 
	    fw.write("# Aida header file generator version: "+ version);
	    fw.newLine();
	    fw.newLine();
	    System.out.println("Selected address: " + selectAddress);
	    if(selectedDSP.compareTo("ADAU170x")==0){
	    	
	    	fw.write("#ifndef _AIDAFW_H_\n");	    
		    fw.write("#define _AIDAFW_H_\n\n\n"); 
		    fw.write("#include <AidaDSP.h>\n\r");
		    fw.write("/*AIDAFW.h - Main include file for the AIDA audio DSP library Copyright (c) 2015 AIDA Team.  All right reserved."); 
		    fw.newLine(); 
		    fw.write("This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General ");
		    fw.newLine();
		    fw.write("Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.");
		    fw.newLine(); 
		    fw.write("This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; ");
		    fw.newLine();
		    fw.write("without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");
		    fw.newLine();
		    fw.write("  See the GNU Lesser General Public License for more details."); 
		    fw.newLine();  
		    fw.write("You should have received a copy of the GNU Lesser General Public  License along with this");
		    fw.newLine();
		    fw.write("library; if not, write to the Free Software  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA  */");
		    fw.newLine(); 
		    fw.newLine(); 
		    fw.newLine(); 
		    fw.write("#define DEVICE_ADDR " + selectAddress);
		    fw.newLine();   
		    fw.write("#define DEVICE_ADDR_7bit DEVICE_ADDR>>1\n\r");
		    
		    fw.flush();
		    
		    while ((strLine = reader.readLine()) != null){
		    	strLine=strLine.trim();
		    	WhereIAm(strLine);  //am I in register, program or other?
		        if(position=="Register"){
		        	//System.out.print(strLine);
		        	strLine = reader.readLine().trim(); //now i read <Name>IC 1.CoreRegister</Name>
		            name=strLine.substring(6,strLine.length()-7);		            
		            //compatto il nome(eliminogli spazi)
		            StringTokenizer Tok=new StringTokenizer(name);
		    		String word = "";
		    		while(Tok.hasMoreTokens()) { 
		    			word=word+Tok.nextToken();			 
		    		}
		    		name=word;
		    		//System.out.println("NAME: " + name);
		    		if(name.compareTo("IC1.CoreRegister")==0 && Ic1_count==0){
		    			Ic1_count=1;
		    			name="#define CoreRegisterR0";	    			
		    		}
		    		else if(name.compareTo("IC1.CoreRegister")==0 && Ic1_count==1){
		    			name="#define CoreRegisterR4";	    			
		    		}
		    		else if(name.contains("HWConFiguration")){
		    			//Elimino IC1.
		    			StringTokenizer Tok2=new StringTokenizer(name,".");
			    		String word2 = "";
			    		
			    		word2=Tok2.nextToken();	
			    		word2=Tok2.nextToken();  //ora word 2 ha solo HWConf....
			    		
		    			name="#define " + word2;	    			
		    		}
		    		else
		    			name="#define reg"+name;
		    		
		            strLine = reader.readLine(); // now I read <Address>2076</Address>
		            strLine=strLine.trim();
		            address=strLine.substring(9,strLine.length()-10);
		            strLine = reader.readLine(); // now I read <Size>2</Size>
		            strLine=strLine.trim();
		            size=strLine.substring(6,strLine.length()-7);
		              //now I write #define regnameaddr address			
		            fw.write(name + "Addr " + "\t" + address + "\n");	 
		                	//now I write #define regnamesize size
		            fw.write(name + "Size " + "\t" + size+ "\n");
		            fw.flush();
		                	// now I extract data
		            strLine=reader.readLine().trim();   //<Data>0x00, 0x18, </Data>
		            buf=strLine.substring(6,strLine.length()-9);  //buf contain only data
		            
		            if(name.compareTo("Param")==0){		            	
		            	buf=buf+", ";		            	
		            	fw.write("const PROGMEM unsigned char " + name.substring(8) + "Data[" + size + "]={");
		            	fw.newLine();
		            	//+ buf + "};" + "\n\n\r"); 
		            	for(int j=0; j<Integer.parseInt(size)/4-1; j++){
		            		fw.write(buf.substring(j*24, (j+1)*24));
		            		fw.flush();
		            		fw.newLine();
		            	}		            	
		            	fw.write(buf.substring((buf.length()-24), buf.length()-2)+ "};" + "\n\n\r");		            			            	
		            }
		            else if(name.compareTo("Data")==0){		            	
		            	buf=buf+", ";		            	
		            	fw.write("const PROGMEM unsigned char " + name + "Data[" + size + "]={");
		            	//+ buf + "};" + "\n\n\r"); 
		            	fw.newLine();
		            	for(int j=0; j<Integer.parseInt(size)/5-1; j++){
		            		fw.write(buf.substring(j*30, (j+1)*30));
		            		fw.flush();
		            		fw.newLine();
		            	}		
		            	//now I write #define programdata[programsize]={....}
		            	fw.write(buf.substring((buf.length()-30 ), buf.length()-2)+ "};" + "\n\n\r");		            			            	
		            }
		            else
		                	//now I write #define programdata[programsize]={....}
		            	fw.write("const PROGMEM unsigned char " + name.substring(8) + "Data[" + size + "]={" + buf + "};" + "\n\n\r");            
		           
		        }
		        else if(position=="Program"){
		            strLine = reader.readLine().trim(); //now i read <Name>IC 1.CoreRegister</Name>
		            name=strLine.substring(14,strLine.length()-7);
		            strLine = reader.readLine(); // now I read <Address>2076</Address>
		            strLine=strLine.trim();
		            address=strLine.substring(9,strLine.length()-10);
		            strLine = reader.readLine(); // now I read <Size>2</Size>
		            strLine=strLine.trim();
		            size=strLine.substring(6,strLine.length()-7);
		                	//now I write #define regnameaddr address			
		            fw.write("#define prog" + name + "Addr" +  "\t" + address + "\n");	            
		                	//now I write #define regnamesize size
		            fw.write("#define prog" + name + "Size" + "\t\t" + size + "\n");
		            //System.out.println("Name: " + name);
		             //unsigned char progrDataData[progDataSize]={
		                	// now I extract data
		            strLine=reader.readLine();   //<Data>0x00, 0x18, </Data>
		            strLine=strLine.trim();	            
		           // byte c=Byte.valueOf("0x55");
		            buf=strLine.substring(6,strLine.length()-9);
		            
		            if(name.compareTo("Param")==0){		            	
		            	buf=buf+", ";		            	
		            	fw.write("const PROGMEM unsigned char " + name.substring(8) + "Data[" + size + "]={");
		            	//+ buf + "};" + "\n\n\r"); 
		            	fw.newLine();
		            	for(int j=0; j<Integer.parseInt(size)/4-1; j++){
		            		fw.write(buf.substring(j*24, (j+1)*24));
		            		fw.flush();
		            		fw.newLine();
		            	}		            	
		            	fw.write(buf.substring((buf.length()-24), buf.length()-2)+ "};" + "\n\n\r");		            			            	
		            }
		            else if(name.compareTo("Data")==0){		            	
		            	buf=buf+", ";		            	
		            	fw.write("const PROGMEM unsigned char " + name + "Data[" + size + "]={");
		            	//+ buf + "};" + "\n\n\r"); 
		            	fw.newLine();
		            	for(int j=0; j<Integer.parseInt(size)/5-1; j++){
		            		fw.write(buf.substring(j*30, (j+1)*30));
		            		fw.flush();
		            		fw.newLine();
		            	}		
		            	//now I write #define programdata[programsize]={....}
		            	fw.write(buf.substring((buf.length()-30 ), buf.length()-2)+ "};" + "\n\n\r");	
		            	fw.flush();
		            }
		            else{
		                	//now I write #define programdata[programsize]={....}
		            	fw.write("const PROGMEM unsigned char " + name.substring(8) + "Data[" + size + "]={" + buf + "};" + "\n\n\r");      
		            	fw.flush();
		            }
		        }
		        if(position=="Cell"){
		        	//System.out.print(strLine);	        	
		            //name=strLine.substring(10,strLine.length()-11);
		            name=CompactName(strLine.substring(10,strLine.length()-11));
		            //if name contains '-' it shall be '_'
		            if(name.contains("-")==true){	            	
		            	name=name.replace('-','_'); 
		            	/*for(int j=0; j<name.length(); j++){ //if I have more "-" I have to replace all it
		            		int occurrence=name.indexOf('-');
		            		name = name.substring(0,occurrence) + "_" + name.substring(occurrence+1);	
		            	}*/
		            }
		            
		            if(name.contains("ReadBack")==true){
		            	for(int i=0;i<5; i++){
			            	strLine = reader.readLine().trim();
			            }
		            	name=strLine; // <Name>ReadBackAlg1</Name>
		            	name=CompactName(strLine.substring(6,strLine.length()-7));
		            	for(int i=0;i<2; i++){
			            	strLine = reader.readLine().trim();
			            }	            
			            address=strLine.substring(9,strLine.length()-10);	            
			              //now I write #define regnameaddr address	
			            if(name.length()<9)
			            	fw.write("#define " + name + "\t\t" + address);
			            else
			            	fw.write("#define " + name + "\t" + address);
			            
			            fw.newLine();
			            //System.out.println("Name: " + name);
	                	// now I extract data
			            strLine=reader.readLine();
			            strLine=reader.readLine().trim();   //<Data>0x00, 0x18, </Data>
			            buf=strLine.substring(6,strLine.length()-9);		            
			            fw.newLine();
			            fw.write("const PROGMEM unsigned char "+ name + "Data[]={" + buf + "};");
			            fw.newLine();	
			            fw.flush();
		            }
		            else{
		            	for(int i=0;i<7; i++){
			            	strLine = reader.readLine().trim();
			            }	            
			            address=strLine.substring(9,strLine.length()-10);	            
			              //now I write #define regnameaddr address	
			            if(name.length()<9)
			            	fw.write("#define " + name + "\t\t" + address);
			            else
			            	fw.write("#define " + name + "\t" + address);
			            fw.newLine();
			            fw.flush();
		            }
		        }
		        position="out";
		 }
		    fw.newLine();
		    fw.write("void program_download() {\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, CoreRegisterR0Addr, CoreRegisterR0Size, CoreRegisterR0Data );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, progDataAddr, progDataSize, progDataData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, regParamAddr, regParamSize, regParamData );\n");	
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, HWConFigurationAddr, HWConFigurationSize, HWConFigurationData );\n");	
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data );\n");	
		    fw.write("\n}");
		    fw.write("\n\n\n#endif");
	    }	    
		else if(selectedDSP.compareTo("ADAU144x")==0){
			fw.write("#ifndef _AIDAFW_H_\n");	    
		    fw.write("#define _AIDAFW_H_\n\n\n"); 
		    fw.write("#include <AidaDSP.h>\n\r");    
		    //fw.write("#include <avr/pgmspace.h>\n\r");
		    fw.write("/*AIDAFW.h - Main include file for the AIDA audio DSP library Copyright (c) 2015 AIDA Team.  All right reserved."); 
		    fw.write("\n"); 
		    fw.write("This library is free software; you can redistribute it and/or modify it under the terms of the");
		    fw.newLine();
		    fw.write("GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License,");
		    fw.newLine();
		    fw.write("or (at your option) any later version.");
		    fw.newLine();
		    fw.write("This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied");
		    fw.newLine();
		    fw.write("warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.");
		    fw.newLine();
		    fw.write("You should have received a copy of the GNU Lesser General Public  License along with this library; if not,");
		    fw.newLine();
		    fw.write("write to the Free Software  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA  */");
		    fw.write("\n\n\n");		   
		    fw.write("#define DEVICE_ADDR " + selectAddress + "  // Here insert 8bit I2C address (write mode) for ADAU144x (0x70, 0x74, 0x78, 0x7C)");
		    fw.write("\n");    
		    fw.write("#define DEVICE_ADDR_7bit DEVICE_ADDR>>1\n\r");
		    
		    while ((strLine = reader.readLine()) != null){
		    	strLine=strLine.trim();
		    	WhereIAm(strLine);  //am I in register, program or other?
		        if(position=="Register"){
		        	//System.out.print("sre " + strLine);
		        	strLine = reader.readLine().trim(); //now i read <Name>IC 1.CoreRegister</Name>
		            name=strLine.substring(6,strLine.length()-7);
		            //compatto il nome(elimino gli spazi)
		            StringTokenizer Tok=new StringTokenizer(name);
		    		String word = "";
		    		while(Tok.hasMoreTokens()) { 
		    			word=word+Tok.nextToken();			 
		    		}
		    		name=word;		            
		            //System.out.println("name " + name);
		    		if(name.compareTo("NonModRamAlloc")==0){
		    			name="#define NonModRamAlloc";	    			
		    		}
		    		else if(name.compareTo("IC1.Start_Pulse_Select")==0){
		    			name="#define Start_Pulse_Select";
		    			pulse_count++;
		    		}		    			
		    		else if(name.compareTo("IC1.Core_Run_Bit")==0){
		    			name="#define Core_Run_Bit";
		    			Run_Bit_count++;
		    		}		    			    		
		    		else if(name.compareTo("IC1.Serial Input Modes")==0){
		    			name="#define SerialInputModes";		    					
		    		}
		    		else if(name.compareTo("IC1.Serial Output Modes")==0){
		    			name="#define SerialOutputModes";	    			
		    		}
		    		else if(name.compareTo("IC1.High-Speed Slave Interface Mode")==0){
		    			name="#define HighSpeedSlaveInterfaceMode";	    			
		    		}
		    		else if(name.compareTo("IC1.Routing Matrix ASRC Input Data Selector")==0){
		    			name="#define RoutingMatrixASRCInputDataSelector";	    			
		    		}
		    		else if(name.compareTo("IC1.Routing Matrix ASRC Output Rate Selector")==0){
		    			name="#define RoutingMatrixASRCOutputRateSelector";	 	    			
		    		}
		    		else if(name.compareTo("IC1.Routing Matrix Serial Output Data Selector")==0){
		    			name="#define RoutingMatrixSerialOutputDataSelector";	    			
		    		}
		    		else if(name.compareTo("IC1.S/PDIF Regs")==0){
		    			name="#define SPDIFRegs";	    			
		    		}
		    		else if(name.compareTo("IC1.ASRC1 (0-3)")==0){
		    			name="#define ASRC1";	    			
		    		}
		    		else if(name.compareTo("IC1.ASRC2 (4-7)")==0){
		    			name="#define ASRC2";		    			
		    		}
		    		else if(name.compareTo("IC1.Serial In Flex Modes")==0){
		    			name="#define SerialInFlexModes";	    			
		    		}
		    		else if(name.compareTo("IC1.Serial Out Flex Modes")==0){
		    			name="#define SerialOutFlexModes";	    			
		    		}
		    		else if(name.compareTo("IC1.DSP Regs")==0){
		    			name="#define DSPRegs";	    			
		    		}
		    		else if(name.compareTo("IC1.Other DSP Modes 1")==0){
		    			name="#define OtherDSPModes1";	    			
		    		}
		    		else if(name.compareTo("IC1.Other DSP Modes 2")==0){
		    			name="#define OtherDSPModes2";	    			
		    		}
		    		else if(name.compareTo("IC1.Enable Register")==0){
		    			name="#define EnableRegister";	    			
		    		}		    		
		    		else if(name.compareTo("IC1.Dejitter_Window")==0){
		    			name="#define " + "R" + (23 + djitter_count) + "_Dejitter_Window";
		    			djitter_count++;
		    		}
		    		else if(name.compareTo("Non Modulo RAM")==0){
		    			name="#define NonModuloRAM";	    			
		    		}
		    		else if(name.compareTo("Modulo Data Memory Register")==0){
		    			name="#define ModuloDataMemoryRegister";	    			
		    		}
		    		else if(name.compareTo("Param")==0){
		    			name="#define Param";	    			
		    		}		    		
		    		else
		    			name="#define reg" + name;
		    		
		            strLine = reader.readLine(); // now I read <Address>2076</Address>
		            strLine=strLine.trim();
		            address=strLine.substring(9,strLine.length()-10);
		            strLine = reader.readLine(); // now I read <Size>2</Size>
		            strLine=strLine.trim();
		            size=strLine.substring(6,strLine.length()-7);
		            //System.out.println("name: " + name);
		            	// now I extract data
		            strLine=reader.readLine().trim();   //<Data>0x00, 0x18, </Data>
		            buf=strLine.substring(6,strLine.length()-9);  //buf contain only data
		            //System.out.println("Name: " + name);
		            if(name.compareTo("#define Start_Pulse_Select")==0){
		            			//now I write 			
			            fw.write(name + "Addr" + pulse_count +"\t" + address + "\n");	            
		            			//now I write 
			            fw.write(name + "Size" + pulse_count +"\t" + size+ "\n");
			        	//now I write #define programdata[programsize]={....}
			            fw.write("const PROGMEM unsigned char " + name.substring(8) + "Data"+ pulse_count +"[" + size + "]={" + buf + "};" + "\n\n\r");
			            fw.flush();
		            }
		            else if(name.compareTo("#define Core_Run_Bit")==0){
            			//now I write #define regnameaddr address			
		            	fw.write(name + "Addr" + Run_Bit_count +"\t" + address + "\n");	            
            			//now I write #define regnamesize size
		            	fw.write(name + "Size" + Run_Bit_count +"\t" + size+ "\n");
		            	//now I write #define programdata[programsize]={....}
			            fw.write("const PROGMEM unsigned char " + name.substring(8) + "Data"+ Run_Bit_count +"[" + size + "]={" + buf + "};" + "\n\n\r");
			            fw.flush();
		            } 		            
		            else{
		            	//now I write #define regnameaddr address			
			            fw.write(name + "Addr " + "\t" + address + "\n");	            
			                	//now I write #define regnamesize size
			            fw.write(name + "Size " + "\t" + size+ "\n");
			            
			            if(name.compareTo("Param")==0){		            	
			            	buf=buf+", ";		            	
			            	fw.write("const PROGMEM unsigned char " + name.substring(8) + "Data[" + size + "]={");
			            	//+ buf + "};" + "\n\n\r"); 
			            	fw.newLine();
			            	for(int j=0; j<Integer.parseInt(size)/4-1; j++){
			            		fw.write(buf.substring(j*24, (j+1)*24));
			            		fw.newLine();
			            		fw.flush();
			            	}
			            	fw.write(buf.substring((buf.length()-24), buf.length()-2)+ "};" + "\n\n\r");
			            	fw.flush();
			            }
			            else if(name.compareTo("Data")==0){		            	
			            	buf=buf+", ";		            	
			            	fw.write("const PROGMEM unsigned char " + name + "Data[" + size + "]={");
			            	//+ buf + "};" + "\n\n\r"); 
			            	fw.newLine();
			            	for(int j=0; j<Integer.parseInt(size)/6-1; j++){
			            		fw.write(buf.substring(j*36, (j+1)*36));
			            		fw.newLine();
			            		fw.flush();
			            	}
			            	//System.out.println("buf: " + buf);
			            	fw.write(buf.substring((buf.length()-36), buf.length()-2)+ "};" + "\n\n\r");
			            	fw.flush();
			            }
			            else{
			                  	//now I write #define programdata[programsize]={....}
			            	fw.write("const PROGMEM unsigned char " + name.substring(8) + "Data[" + size + "]={" + buf + "};" + "\n\n\r");
			            	fw.flush();
			            }
		            }      		
		        }

		        else if(position=="Program"){
		            strLine = reader.readLine().trim(); //now i read <Name>IC 1.CoreRegister</Name>
		            name=strLine.substring(14,strLine.length()-7);
		            strLine = reader.readLine(); // now I read <Address>2076</Address>
		            strLine=strLine.trim();
		            address=strLine.substring(9,strLine.length()-10);
		            strLine = reader.readLine(); // now I read <Size>2</Size>
		            strLine=strLine.trim();
		            size=strLine.substring(6,strLine.length()-7);
		                	//now I write #define regnameaddr address			
		            fw.write("#define prog" + name + "Addr" +  "\t" + address + "\n");	            
		                	//now I write #define regnamesize size
		            fw.write("#define prog" + name + "Size" + "\t\t" + size + "\n");
		             //unsigned char progrDataData[progDataSize]={
		                	// now I extract data
		            strLine=reader.readLine();   //<Data>0x00, 0x18, </Data>
		            strLine=strLine.trim();	            
		           // byte c=Byte.valueOf("0x55");
		            buf=strLine.substring(6,strLine.length()-9);
		            
		            //System.out.println("name: " + name);
		            if(name.compareTo("Param")==0){		            	
		            	buf=buf+", ";		            	
		            	fw.write("const PROGMEM unsigned char " + name + "Data[" + size + "]={");
		            	//+ buf + "};" + "\n\n\r"); 
		            	for(int j=0; j<Integer.parseInt(size)/4-1; j++){
		            		fw.write(buf.substring(j*24, (j+1)*24));
		            		fw.newLine();
		            		fw.flush();
		            	}		
		            	//now I write #define programdata[programsize]={....}
		            	fw.write(buf.substring((buf.length()-24), buf.length()-2)+ "};" + "\n\n\r");		            			            	
		            }
		            else if(name.compareTo("Data")==0){		            	
		            	buf=buf+", ";		            	
		            	fw.write("const PROGMEM unsigned char " + name + "Data[" + size + "]={");
		            	//+ buf + "};" + "\n\n\r"); 
		            	fw.newLine();
		            	for(int j=0; j<Integer.parseInt(size)/6-1; j++){
		            		fw.write(buf.substring(j*36, (j+1)*36));
		            		fw.newLine();
		            		fw.flush();
		            	}		
		            	//now I write #define programdata[programsize]={....}
		            	fw.write(buf.substring((buf.length()-36), buf.length()-2)+ "};" + "\n\n\r");		            			            	
		            }
		            else
		                	//now I write #define programdata[programsize]={....}
		            	fw.write("const PROGMEM unsigned char prog" + name + "Data[" + size + "]={" + buf + "};" + "\n\n\r");
		        
		        }
		        if(position=="Cell"){
		        	//System.out.print(strLine);	        	
		            //name=strLine.substring(10,strLine.length()-11);
		            name=CompactName(strLine.substring(10,strLine.length()-11));
		            //if name contains '-' it shall be '_'
		            if(name.contains("-")==true){	            	
		            	name=name.replace('-','_'); 
		            	/*for(int j=0; j<name.length(); j++){ //if I have more "-" I have to replace all it
		            		int occurrence=name.indexOf('-');
		            		name = name.substring(0,occurrence) + "_" + name.substring(occurrence+1);	
		            	}*/
		            }
		            
		            if(name.contains("ReadBack")==true){
		            	for(int i=0;i<5; i++){
			            	strLine = reader.readLine().trim();
			            }
		            	name=strLine; // <Name>ReadBackAlg1</Name>
		            	name=CompactName(strLine.substring(6,strLine.length()-7));
		            	for(int i=0;i<2; i++){
			            	strLine = reader.readLine().trim();
			            }	            
			            address=strLine.substring(9,strLine.length()-10);	            
			              //now I write #define regnameaddr address	
			            if(name.length()<9)
			            	fw.write("#define " + name + "\t\t" + address);
			            else
			            	fw.write("#define " + name + "\t" + address);
			            
			            fw.newLine();
	                	// now I extract data
			            strLine=reader.readLine();
			            strLine=reader.readLine().trim();   //<Data>0x00, 0x18, </Data>
			            buf=strLine.substring(6,strLine.length()-9);		            
			            fw.newLine();
			            fw.write("const PROGMEM unsigned char "+ name + "Data[]={" + buf + "};");
			            fw.newLine();
			            fw.flush();
		            }
		            else{
		            	for(int i=0;i<7; i++){
			            	strLine = reader.readLine().trim();
			            }	            
			            address=strLine.substring(9,strLine.length()-10);	            
			              //now I write #define regnameaddr address	
			            if(name.length()<9)
			            	fw.write("#define " + name + "\t\t" + address);
			            else
			            	fw.write("#define " + name + "\t" + address);
			            fw.newLine();
		            }
		        }
		        position="out";
		 }
		    fw.newLine();
		    fw.write("void program_download() {\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, NonModRamAllocAddr, NonModRamAllocSize, NonModRamAllocData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, Start_Pulse_SelectAddr1, Start_Pulse_SelectSize1, Start_Pulse_SelectData1 );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, Core_Run_BitAddr1, Core_Run_BitSize1, Core_Run_BitData1 );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, progDataAddr, progDataSize, progDataData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, SerialInputModesAddr, SerialInputModesSize, SerialInputModesData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, SerialOutputModesAddr, SerialOutputModesSize, SerialOutputModesData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, HighSpeedSlaveInterfaceModeAddr, HighSpeedSlaveInterfaceModeSize, HighSpeedSlaveInterfaceModeData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, RoutingMatrixASRCInputDataSelectorAddr, RoutingMatrixASRCInputDataSelectorSize, RoutingMatrixASRCInputDataSelectorData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, RoutingMatrixASRCOutputRateSelectorAddr, RoutingMatrixASRCOutputRateSelectorSize, RoutingMatrixASRCOutputRateSelectorData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, RoutingMatrixSerialOutputDataSelectorAddr, RoutingMatrixSerialOutputDataSelectorSize, RoutingMatrixSerialOutputDataSelectorData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, SPDIFRegsAddr, SPDIFRegsSize, SPDIFRegsData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, ASRC1Addr, ASRC1Size, ASRC1Data );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, ASRC2Addr, ASRC2Size, ASRC2Data );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, SerialInFlexModesAddr, SerialInFlexModesSize, SerialInFlexModesData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, SerialOutFlexModesAddr, SerialOutFlexModesSize, SerialOutFlexModesData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, DSPRegsAddr, DSPRegsSize, DSPRegsData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, OtherDSPModes1Addr, OtherDSPModes1Size, OtherDSPModes1Data );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, OtherDSPModes2Addr, OtherDSPModes2Size, OtherDSPModes2Data );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, EnableRegisterAddr, EnableRegisterSize, EnableRegisterData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, ParamAddr, ParamSize, ParamData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, ModuloDataMemoryRegisterAddr, ModuloDataMemoryRegisterSize, ModuloDataMemoryRegisterData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, NonModuloRAMAddr, NonModuloRAMSize, NonModuloRAMData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, Start_Pulse_SelectAddr2, Start_Pulse_SelectSize2, Start_Pulse_SelectData2 );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, Core_Run_BitAddr2, Core_Run_BitSize2, Core_Run_BitData2 );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, R23_Dejitter_WindowAddr, R23_Dejitter_WindowSize, R23_Dejitter_WindowData );\n");
		    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, R24_Dejitter_WindowAddr, R24_Dejitter_WindowSize, R24_Dejitter_WindowData );\n");
		    fw.write("\n}");
		    fw.write("\n\n\n#endif");
	    }
	    
	    reader.close();	    
	    fw.close();
     //System.out.print("Processo finito");
	}	
	
	
	static String WhereIAm(String Element){
		//System.out.print(Element);
		//System.out.println();		
		if(Element.compareTo("<Register>")==0)  
			position="Register";
		else if(Element.compareTo("<Program>")==0)  
			position="Program";		
		else if(Element.compareTo("</Register>")==0||Element.compareTo("</Program>")==0)  
			position="out"; // neither in Program neither in Register
		if(Element.length()>9){
			if(Element.substring(0,10).compareTo("<CellName>")==0){  
				position="Cell";
				//System.out.print(Element);
			}
		}
		return position;
	}
	
	static String CompactName(String Element){
		//now I have to delete white space from CellName
		StringTokenizer Token=new StringTokenizer(Element);
		String word = "";
		while(Token.hasMoreTokens()) { 
			word=word+Token.nextToken();			 
		} 
		return word;
	}
	
	public static void infoBox(String infoMessage, String titleBar)
    {
         
    }	
	
	private class TxtFileFilter extends FileFilter {

	    public boolean accept(File file) {
	      if (file.isDirectory()) return true;
	        String fname = file.getName().toLowerCase();
	        return fname.endsWith("xml");
	    }

	    public String getDescription() {
	      return "File XML";
	    }
	  }
	
}



