package AIDA;

import java.awt.Button;
import java.awt.Color;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.Frame;
import java.awt.Panel;
import java.awt.TextArea;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.Locale;

import javax.swing.DefaultComboBoxModel;
import javax.swing.Icon;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;

/**
 * A Swing program that demonstrates how to create and use JComboBox component.
 * 
 * @author www.codejava.net
 * 
 */
public class ComboBox extends JFrame {

	private JButton buttonOK = new JButton("OK");
	private JButton buttonESC = new JButton("ESC");
	private String OSName, OSversion,OSarchitecture, JVruntypeVersion;
	private Panel pannel_text = new Panel();
	private TextArea textarea = new TextArea(6,40);
	private Calendar calendar=new GregorianCalendar();

	public ComboBox() {
		super("Select DSP please");
		
		setLayout(new FlowLayout(FlowLayout.LEFT, 10, 10));

		String[] microTitle = new String[] { "ADAU144x","ADAU170x" };

		// create a combo box with items specified in the String array:
		final JComboBox<String> microList = new JComboBox<String>(microTitle);		

		// customize some appearance:
		microList.setForeground(Color.BLUE);
		microList.setFont(new Font("Arial", Font.BOLD, 14));
		microList.setMaximumRowCount(10);
		
		// make the combo box not editable so we can add new item when needed
		microList.setEditable(false);

		// add an event listener for the combo box
		microList.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent event) {
				textarea.setText("SELECTED DSP: \n\n\t" + microList.getSelectedItem().toString() + 
						"\n\nPress OK button please" );				
			}
		});

		// add event listener for the button OK 
		buttonOK.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent event) {
				//String selectedMicro = (String) microList.getSelectedItem();
				AIDA_header_file_generator.selectedDSP = (String) microList.getSelectedItem();	
				
				Icon icon = new ImageIcon("chip.jpg");
				Object[] options = {"OK"};
			    int n = JOptionPane.showOptionDialog(ComboBox.this,
			    				"You selected DSP " + 
								AIDA_header_file_generator.selectedDSP + "." 
			    				+ "\nPush button to continue.",
			    				"SELECTED DSP",
			                   JOptionPane.PLAIN_MESSAGE,
			                   JOptionPane.QUESTION_MESSAGE,
			                   icon,
			                   options,
			                   options[0]);
			   
				/*Icon icon = new ImageIcon("images.jpg");  
				JOptionPane jp = new JOptionPane("You selected micro: " + 
						  AIDA_header_file_generator.selectedDSP + "." + "\nPush button to continue.",   
				  JOptionPane.INFORMATION_MESSAGE,   
				  JOptionPane.WARNING_MESSAGE,   
				  icon);  
				JDialog dialog = jp.createDialog(null, "");				
				dialog.setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
				((Frame)dialog.getParent()).setIconImage(((ImageIcon)icon).getImage());  
				//dialog.setResizable(true);  
				dialog.setVisible(true); */
				
				AIDA_header_file_generator.okButtonPressed=true;
				setVisible(false);
			}
		});

		// add event listener for the button Remove
		buttonESC.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent event) {
				System.exit(0);
			}
		});

		// add components to this frame
		add(microList);
		add(buttonOK);
		add(buttonESC);
		
		//information abouth OS
		OSName = System.getProperty("os.name").toLowerCase();
		OSversion = System.getProperty("os.version").toLowerCase();
		OSarchitecture = System.getProperty("os.arch").toLowerCase();	
		JVruntypeVersion = System.getProperty("java.runtime.version").toLowerCase();
			
		//SW report area
		pannel_text.add(textarea);
		textarea.setEditable(false);
		add(pannel_text);
		textarea.append("***SOFTWARE REPORT***   " + String.valueOf(calendar.get(Calendar.HOUR))+
				":" + String.valueOf(calendar.get(Calendar.MINUTE))+ ":" + 
				String.valueOf(calendar.get(Calendar.SECOND))+
				"\n\n"+"OS name: " + OSName + "\n" + "OS version: " + 
				OSversion + "\n" + "OS architecture: " + OSarchitecture + "\n" + 
				"Java runtime version: " + JVruntypeVersion );	
				
		pack();
	    setVisible(true);
		setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
		setLocationRelativeTo(null);
	}

	/*public static void main(String[] args) {
		
		//new ComboBox().setVisible(true);
		/*try {
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		} catch (Exception ex) {
			ex.printStackTrace();
		}

		SwingUtilities.invokeLater(new Runnable() {

			@Override
			public void run() {
				new ComboBox().setVisible(true);
			}
		});
	}*/
}