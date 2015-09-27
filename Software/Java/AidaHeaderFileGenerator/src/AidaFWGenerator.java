/*
  AidaFWGenerator.java - Aida DSP header file generator
 Copyright (c) 2015 AIDA Team.  All right reserved.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

// VERSION 01.45.2015 10:24 A.M. 

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
	static String position="out";  // sed if i am in register, program or name
	static String strLine;
	static boolean pathWriteSelected=false;
	static String selectAddress="0x6C";
	static File selectedFile;

	/** Create the whole GUI, and set up event listeners */
	public AIDA_header_file_generator(String title) {
		super(title);  // set frame title.
		pathWrite=System.getProperty("java.class.path");

		// Arrange to detect window close events
		this.addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent e) { System.exit(0); }
		});

		// Set a default font
		this.setFont(new Font("SansSerif", Font.PLAIN, 12));
/*
		// Create the menubar.  Tell the frame about it.
		MenuBar menubar = new MenuBar();
		this.setMenuBar(menubar);

		// Create the file menu.  Add to menubar.
		Menu AddressMenu = new Menu("Address Menu");		
		menubar.add(AddressMenu);

		// Create two items for the file menu, setting their label, shortcut,
		// action command and listener.  Add them to File menu.
		// Note that we use the frame itself as the action listener
		MenuItem X68 = new MenuItem("0X68", new MenuShortcut(KeyEvent.VK_8));		
		X68.setActionCommand("0X68");
		X68.addActionListener(this);
		AddressMenu.add(X68);
 
		MenuItem X6A = new MenuItem("0X6A", new MenuShortcut(KeyEvent.VK_A));
		X6A.setActionCommand("0X6A");
		X6A.addActionListener(this);
		AddressMenu.add(X6A);
 
		MenuItem X6C = new MenuItem("0X6C", new MenuShortcut(KeyEvent.VK_C));
		X6C.setActionCommand("0X6C");
		X6C.addActionListener(this);
		AddressMenu.add(X6C);
 
		MenuItem X6E = new MenuItem("0X6E", new MenuShortcut(KeyEvent.VK_E));
		X6E.setActionCommand("0X6E");
		X6E.addActionListener(this);
		AddressMenu.add(X6E);

		// Now that we've done the menu, we can begin work on the contents of
		// the frame.  Assign a BorderLayout manager with margins for this frame.
		this.setLayout(new BorderLayout(10, 10));

		// Create two panels to contain two columns of components.  Use our custom
		// ColumnLayout layout manager for each.  Add them on the west and
		// center of the frame's border layout
		//Panel pannel_text = new Panel();
		// pannel_text.setLayout(new ColumnLayout(5, 10, 2, ColumnLayout.LEFT));
		//this.add(pannel_text, "Center");
 */
		Panel pannel_text = new Panel();
		Choice choice = new Choice();
	    choice.addItem("0x68");
	    choice.addItem("0x6A");
	    choice.addItem("0x6C");
	    choice.addItem("0x6E");
	    pannel_text.add(new Label("Address:"));
	    pannel_text.add(choice);
		//pannel_text.setLayout(new FlowLayout(FlowLayout.TRAILING, 100, 40));
		this.add(pannel_text, "Center");
		
		// Handle events on this choice
	    choice.addItemListener(new ItemListener() {
	      public void itemStateChanged(ItemEvent e) {
	    	  selectAddress=e.getItem().toString();
	        //textarea.append("Your favorite color is: " + e.getItem() + "\n");
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

	static String version="VERSION 01.05.2015 10:24 A.M.";
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
	   
	    fw.write("#ifndef _AIDAFW_H_\n");	    
	    fw.write("#define _AIDAFW_H_\n\n\n"); 
	    fw.write("#include <AidaDSP.h>\n\r");
	    fw.write("/*AIDAFW.h - Main include file for the AIDA audio DSP library Copyright (c) 2015 AIDA Team.  All right reserved."); 
	    fw.write("\n"); 
	    fw.write("This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.");
	    fw.write("\n");
	    fw.write("This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.");
	    fw.write("\n"); 
	    fw.write("You should have received a copy of the GNU Lesser General Public  License along with this library; if not, write to the Free Software  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA  */");
	    fw.write("\n\n\n");
	    fw.write("#define DEVICE_ADDR " + selectAddress);
	    fw.write("\n");    
	    fw.write("#define DEVICE_ADDR_7bit DEVICE_ADDR>>1\n\r");
	    
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
	    		if(name.compareTo("IC1.CoreRegister")==0 && Ic1_count==0){
	    			Ic1_count=1;
	    			name="#define CoreRegisterR0";	    			
	    		}
	    		else if(name.compareTo("IC1.CoreRegister")==0 && Ic1_count==1){
	    			Ic1_count=1;
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
	                	// now I extract data
	            strLine=reader.readLine().trim();   //<Data>0x00, 0x18, </Data>
	            buf=strLine.substring(6,strLine.length()-9);
	                	//now I write #define programdata[programsize]={....}
	            fw.write("const unsigned char " + name.substring(8) + "Data[" + size + "]={" + buf + "};" + "\n\n\r");            
	           
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
	            buf=strLine.substring(6,strLine.length()-9);
	                	//now I write #define programdata[programsize]={....}
	            fw.write("const unsigned char prog" + name + "Data[" + size + "]={" + buf + "};" + "\n\n\r");
	        }
	        if(position=="Cell"){
	        	//System.out.print(strLine);	        	
	            //name=strLine.substring(10,strLine.length()-11);
	            name=CompactName(strLine.substring(10,strLine.length()-11));
	            //if name contains '-' it shall be '_'
	            /*if(name.contains("-")==true)
	            	name.replace('-','_'); //non funziona
	            name.rep*/
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
		            fw.write("const unsigned char "+ name + "Data[]={" + buf + "};");
		            fw.newLine();		            
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
	    reader.close();
	    fw.newLine();
	    fw.write("void program_download() {\n");
	    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, CoreRegisterR0Addr, CoreRegisterR0Size, CoreRegisterR0Data );\n");
	    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, progDataAddr, progDataSize, progDataData );\n");
	    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, regParamAddr, regParamSize, regParamData );\n");	
	    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, HWConFigurationAddr, HWConFigurationSize, HWConFigurationData );\n");	
	    fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data );\n");	
	    fw.write("\n}");
	    fw.write("\n\n\n#endif");
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
