//VERSION 16.11.2015 14:12 P.M.
/*
 AIDA_header_file_generator.java - Aida DSP Java Class
 Copyright (c) 2015 Gaetano Grano.  All right reserved.

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
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.UIManager;
import javax.swing.filechooser.FileFilter;

/** A program that uses all the standard AWT components */
public class AIDA_header_file_generator extends Frame implements ActionListener {

	private static final long serialVersionUID = 1L;
	TextArea textarea; // Events messages will be displayed here.
	static String pathRead = null;
	static String pathWrite = null;
	static String position = "out"; // sed if i am in register, program or name
	static boolean pathWriteSelected = false;
	static String selectAddress = "0x6C";
	static File selectedFile;
	public static String selectedDSP;
	private String OSName, OSversion, OSarchitecture, JVruntypeVersion;
	public static boolean okButtonPressed = false;
	private String[] addresses_vector = new String[4];
	private static String version = "VERSION 16.11.2015 14:12 P.M.";
	static String blackListChars = "[-. /|><()=%&?!*+]";

	// static String strLine;

	/** Create the whole GUI, and set up event listeners */
	public AIDA_header_file_generator(String title) {

		super(title); // set frame title.

		// information abouth OS
		OSName = System.getProperty("os.name").toLowerCase();
		OSversion = System.getProperty("os.version").toLowerCase();
		OSarchitecture = System.getProperty("os.arch").toLowerCase();
		JVruntypeVersion = System.getProperty("java.runtime.version")
				.toLowerCase();

		/*
		 * JOptionPane.showMessageDialog(AIDA_header_file_generator.this,
		 * "OS name: " + OSName + "\n" + "OS version: " + OSversion + "\n" +
		 * "OS architecture: " + OSarchitecture + "\n" +
		 * "Java runtime version: " + JVruntypeVersion);
		 * 
		 * infoBox("Software report","OS name: " + OSName + "\n" +
		 * "OS version: " + OSversion + "\n" + "OS architecture: " +
		 * OSarchitecture + "\n" + "Java runtime version: " + JVruntypeVersion);
		 */

		System.out.println("OS name: ");
		System.out.println("OS version: ");
		System.out.println("OS architecture: ");
		System.out.println("Java runtime version: ");

		System.out.println("\n");

		// I read absolute path (where is the exe java file e.g.
		// c:\micro\aida\file\)
		pathWrite = System.getProperty("java.class.path");
		pathWrite = System.getProperty("user.dir");

		// first combo
		ComboBox conmbo = new ComboBox();
		while (okButtonPressed == false) {
			// I whait until you press OK button and select an micro
			// Do NOT erase next four code row!!!!!!
			if (okButtonPressed == false) {
				try {
					Thread.sleep(2000);
				} catch (InterruptedException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
				System.out.println("Please, press button");
			} else
				System.out.println("Button pressed");
		}
		System.out.println("\nSelected DSP: " + selectedDSP);

		// now I insert addresses agree with selected DSP
		if (selectedDSP.compareTo("ADAU144x") == 0) {
			addresses_vector[0] = "0x70";
			addresses_vector[1] = "0x74";
			addresses_vector[2] = "0x78";
			addresses_vector[3] = "0x7C";
		} else if (selectedDSP.compareTo("ADAU170x") == 0) {
			addresses_vector[0] = "0x68";
			addresses_vector[1] = "0x6A";
			addresses_vector[2] = "0x6C";
			addresses_vector[3] = "0x6E";
		}

		// Arrange to detect window close events
		this.addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent e) {
				System.exit(0);
			}
		});

		// Set a default font
		this.setFont(new Font("SansSerif", Font.PLAIN, 12));

		Panel pannel_text = new Panel();
		Choice choice = new Choice();

		for (int i = 0; i < 4; i++)
			choice.addItem(addresses_vector[i]);

		choice.select(3);
		selectAddress = choice.getSelectedItem();

		pannel_text.add(new Label("Address:"));
		pannel_text.add(choice);

		// pannel_text.setLayout(new FlowLayout(FlowLayout.TRAILING, 100, 40));
		this.add(pannel_text, "Center");

		// Handle events on this choice
		choice.addItemListener(new ItemListener() {
			public void itemStateChanged(ItemEvent e) {
				selectAddress = e.getItem().toString();
			}
		});

		// Create a panel to contain the buttons at the bottom of the window
		// Give it a FlowLayout layout manager, and add it along the south
		// border
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
				if (((JButton) e.getSource()).getLabel().compareTo("File IN") == 0) {
					try {
						// textarea.setText("");
						UIManager.put("FileChooser.cancelButtonText", "Cancel");
						JFileChooser fileChooser = new JFileChooser(
								System.getProperty("java.class.path"));
						fileChooser.setFileFilter(new TxtFileFilter());
						fileChooser.setDialogTitle("FILE di INPUT");
						fileChooser.setApproveButtonText("OK");
						int n = fileChooser
								.showOpenDialog(AIDA_header_file_generator.this);
						// FileFilter type = new ExtensionFilter("XML", ".xml");
						// fileChooser.addChoosableFileFilter(type);
						// fileChooser.setFileFilter(type);

						if (n == JFileChooser.APPROVE_OPTION) {
							// textarea.append(" " );
							File f = fileChooser.getSelectedFile();
							pathRead = f.getPath();
							selectedFile = new File(pathRead);
							if (selectedFile.exists() == true)
								textarea.append("File di input:  "
										+ f.getPath() + "\n");
							else
								textarea.append("File  " + f.getPath()
										+ "  IS NOT A VALID FILE" + "\n");

						}
					} catch (Exception ex) {
					}
				} else if (((JButton) e.getSource()).getLabel().compareTo(
						"MAKE") == 0) {
					if (pathRead == null)
						pathRead = System.getProperty("user.dir");

					selectedFile = new File(pathRead);
					if (selectedFile.isFile() == true) {
						try {
							// pathWrite=System.getProperty("user.dir");
							// textarea.setText("");
							Estrapola();
							textarea.append("\n\n");
							textarea.append("Created file: " + pathWrite
									+ "\\AidaFW.h");
							textarea.append("\n");
							textarea.append("With Address: " + selectAddress);
						} catch (IOException e1) {
							// TODO Auto-generated catch block
							e1.printStackTrace();
						}
					} else {
						// textarea.setText("");
						textarea.append("\n\n ");
						textarea.append("PLEASE, SELECT A VALID INPUT FILE");
					}
				} else if (((JButton) e.getSource()).getLabel()
						.compareTo("ESC") == 0) {
					System.exit(0);
				}
				// textarea.append("You clicked: "
				// +((JButton)e.getSource()).getLabel() + "\n");

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
		textarea.append("DSP: " + selectedDSP + "\n\n\n");
	}

	public static void Estrapola() throws IOException {
		// TODO Auto-generated method stub

		int Ic1_count = -4;
		int pulse_count = 0;
		int djitter_count = 0;
		int Run_Bit_count = 0;
		boolean in_ModuleParameter = false;
		boolean in_Cell = false;
		String buf, prefix, suffix, name = null, address, size;
		String strLine;
		BufferedReader reader = new BufferedReader(new FileReader(pathRead));
		Vector name_vector = new Vector();

		// open file for write
		FileWriter w;
		w = new FileWriter(System.getProperty("user.dir") + "\\AidaFW.h");
		// w=new FileWriter(pathWrite + "\\AidaFWh.h");
		BufferedWriter fw;
		fw = new BufferedWriter(w);
		fw.write("// Aida header file generator version: " + version + "\n");
		fw.write("// Selected DSP: " + selectedDSP + "\n");
		fw.write("/*\n"
				+ "  AidaFW.h - Aida DSP library\n"
				+ " Copyright (c) 2015 Aida DSP Team.  All right reserved.\n\n"
				+ " This library is free software; you can redistribute it and/or\n"
				+ " modify it under the terms of the GNU Lesser General Public\n"
				+ " License as published by the Free Software Foundation; either\n"
				+ " version 2.1 of the License, or (at your option) any later version.\n\n"
				+ " This library is distributed in the hope that it will be useful,\n"
				+ " but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
				+ " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
				+ " Lesser General Public License for more details.\n\n"
				+ " You should have received a copy of the GNU Lesser General Public\n"
				+ " License along with this library; if not, write to the Free Software\n"
				+ " Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA\n"
				+ " */\n");

		fw.write("#ifndef _AIDAFW_H_\n");
		fw.write("#define _AIDAFW_H_\n\n");
		fw.write("#include <AidaDSP.h>\n\n");
		fw.write("#define DEVICE_ADDR " + selectAddress + "\n");
		fw.write("#define DEVICE_ADDR_7bit DEVICE_ADDR>>1\n");
		fw.write("#define " + selectedDSP + "\n\n\n");
		fw.flush();

		do {
			strLine = reader.readLine();
		} while (strLine.compareTo("<Schematic>") != 0);

		/*
		 * Now I read:
		 * 
		 * <IC> <Name>IC 1</Name>
		 */

		strLine = reader.readLine(); // <IC>
		strLine = reader.readLine(); // <Name>IC 1</Name>

		while ((strLine = reader.readLine()) != null) {

			if (strLine.contains("<ModuleParameter>") == true) // I am in
																// ModuleParameter
																// so I will
																// ignore name
																// next field
				in_ModuleParameter = true;
			else if (strLine.contains("</ModuleParameter>") == true) // I am not
																		// in
																		// ModuleParameter
																		// so I
																		// will
																		// not
																		// ignore
																		// name
																		// next
																		// field
				in_ModuleParameter = false;
			else if (strLine.contains("<CellName>") == true)
				in_Cell = true;

			prefix = suffix = "";
			if ((strLine.contains("Name>") == true)
					&& (in_ModuleParameter == false)) { // if I read <Name>IC
														// 1.CoreRegister</Name>.
														// Warning: I use Name>
														// and not <Name>
				name = strLine.trim();
				name = name.substring(1, name.length() - 1); // now I have
																// Name>IC
																// 1.CoreRegister</Name.
																// I do this in
																// order to
																// erase first
																// <, in order
																// to use next
																// code line
				name = name.substring(name.indexOf('>') + 1, name.indexOf('<')); // IC
																					// 1.CoreRegister
				System.out.println(strLine.trim());

				System.out.println(name);
				if (name.compareTo("IC 1.CoreRegister") == 0) {
					Ic1_count = Ic1_count + 4;
					suffix = "R" + String.valueOf((Ic1_count));
				} else if (name.compareTo("IC 1.Start_Pulse_Select") == 0) {
					pulse_count++;
					suffix = String.valueOf((pulse_count));
				} else if (name.compareTo("IC 1.Dejitter_Window") == 0) {
					prefix = "R" + String.valueOf(23 + djitter_count);
					suffix = String.valueOf((djitter_count));
					djitter_count++;
				} else if (name.compareTo("IC 1.Core_Run_Bit") == 0) {
					Run_Bit_count++;
					suffix = String.valueOf((Run_Bit_count));
				} else if (name.compareTo("Param") == 0) {
					prefix = "reg";
				}
				// If I found ReadBack I have to take <Name>ReadBackAlg1</Name>
				else if (name.contains("ReadBack") == true) {
					do {
						strLine = reader.readLine();
					} while (strLine.contains("<Name>") == false); // Now I have
																	// <Name>ReadBackAlg1</Name>
					name = strLine.trim();
					name = name.substring(1, name.length() - 1); // now I have
																	// Name>ReadBackAlg1</Name.
																	// I do this
																	// in order
																	// to erase
																	// first <,
																	// in order
																	// to use
																	// next code
																	// line
					name = name.substring(name.indexOf('>') + 1,
							name.indexOf('<')); // ReadBackAlg1
				}

				// Erase IC 1. and blank space
				name = name.replace("IC 1.", "");
				name = name.replace(" ", "");
				// now I have to erase not allowed parameters in name
				name = name.replaceAll(blackListChars, "");

				// now I go on until I get <Address>
				do {
					strLine = reader.readLine();
				} while (strLine.contains("</Address>") == false);

				// now I have read <Address>2076</Address>
				strLine = strLine.trim(); // erase eventually blank space
				address = strLine.substring(9, strLine.length() - 10);

				name_vector.add(prefix + name + suffix); // I load all name in a
															// vector

				// now I write on file
				fw.write("#define " + prefix + name + suffix + "Addr " + "\t"
						+ address + "\n");
				fw.flush();
				// now I write #define regnamesize size but only if I am not in
				// cell
				// If I am not in cellName i print size also and i go on until I
				// get data
				if (in_Cell == false) {
					// now I go on until i get <Size>
					do {
						strLine = reader.readLine();
					} while (strLine.contains("</Size>") == false);

					// now I have read <Size>2<Size>
					strLine = strLine.trim(); // erase eventually blank space
					size = strLine.substring(6, strLine.length() - 7);
					fw.write("#define " + prefix + name + suffix + "Size "
							+ "\t" + size + "\n");
					fw.flush();
					// now I go on until i get <Data>
					do {
						strLine = reader.readLine();
					} while (strLine.contains("</Data>") == false);
					// now I extract data
					strLine = strLine.trim(); // <Data>0x00,0x18,</Data>
					buf = strLine.substring(6, strLine.length() - 9); // buf
																		// contain
																		// only
																		// data
																		// 0x00,0x18,

					fw.write("const PROGMEM unsigned char " + prefix + name
							+ suffix + "Data[" + size + "]={");

					if (prefix.contains("reg") == true) {
						fw.newLine();
						for (int j = 0; j < Integer.parseInt(size) / 4 - 1; j++) {
							fw.write(buf.substring(j * 24, (j + 1) * 24));
							fw.flush();
							fw.newLine();
						}
						fw.write(buf.substring((buf.length() - 22),
								buf.length())
								+ "};" + "\n\n\r");
					} else if (name.compareTo("ProgramData") == 0) {
						fw.newLine();
						for (int j = 0; j < Integer.parseInt(size) / 5 - 1; j++) {
							fw.write(buf.substring(j * 30, (j + 1) * 30));
							fw.flush();
							fw.newLine();
						}
						fw.write(buf.substring((buf.length() - 28),
								buf.length())
								+ "};" + "\n\n\r");
						fw.flush();
					} else {
						fw.write(buf + "};" + "\n\n\r");
						fw.flush();
					}
				} else {
					if (name.contains("ReadBack") == true) {
						fw.write("const PROGMEM unsigned char " + prefix + name
								+ suffix + "Data[" + "]={");
						do {
							strLine = reader.readLine();
						} while (strLine.contains("<Data>") == false);
						// now I extract data
						strLine = strLine.trim(); // <Data>0x00,0x18,</Data>
						buf = strLine.substring(6, strLine.length() - 9); // buf
																			// contain
																			// only
																			// data
																			// 0x00,0x18,
						fw.write(buf);
						fw.write("};");
						fw.write("\n");
					}
					//fw.write("\n");
					// if I am in cellName I go on until I found </Module>
					do {
						strLine = reader.readLine();
					} while (strLine.contains("</Module>") == false);
					in_Cell = false;
				}
			}
		}

		if (selectedDSP.compareTo("ADAU170x") == 0) {
			fw.newLine();
			fw.write("void program_download() {\n");
			fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, CoreRegisterR0Addr, CoreRegisterR0Size, CoreRegisterR0Data );\n");
			fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, ProgramDataAddr, ProgramDataSize, ProgramDataData );\n");
			fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, regParamAddr, regParamSize, regParamData );\n");
			fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, HWConFigurationAddr, HWConFigurationSize, HWConFigurationData );\n");
			fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data );\n");
			fw.write("}");
			fw.write("\n\n\n#endif");
			fw.flush();
		} else if (selectedDSP.compareTo("ADAU144x") == 0) {
			fw.newLine();
			fw.write("void program_download() {\n");

			for (int j = 0; j < name_vector.size(); j++) {
				fw.write("\tAIDA_WRITE_REGISTER_BLOCK( DEVICE_ADDR_7bit, "
						+ name_vector.elementAt(j) + "Addr, "
						+ name_vector.elementAt(j) + "Size, "
						+ name_vector.elementAt(j) + "Data );\n");
				fw.flush();
			}
			fw.write("}");
			fw.write("\n\n\n#endif");
		}

		reader.close();
		fw.close();
	}

	public static void infoBox(String infoMessage, String titleBar) {

	}

	private class TxtFileFilter extends FileFilter {

		public boolean accept(File file) {
			if (file.isDirectory())
				return true;
			String fname = file.getName().toLowerCase();
			return fname.endsWith("xml");
		}

		public String getDescription() {
			return "File XML";
		}
	}

	public static void main(String[] args) {
		Frame f = new AIDA_header_file_generator(version);
		f.pack();
		f.show();
	}

	@Override
	public void actionPerformed(ActionEvent arg0) {
		// TODO Auto-generated method stub

	}

}
