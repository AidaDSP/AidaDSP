// Version: 22/07/2015  21:59  
// STX NDATA MSB LSB DATA1...N CHKSUM ETX 

package AIDA;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;

import gnu.io.CommPortIdentifier; 
import gnu.io.ParallelPort;
import gnu.io.SerialPort;
import gnu.io.SerialPortEvent; 
import gnu.io.SerialPortEventListener; 

import java.util.Enumeration;
import java.util.HashMap;
import java.util.StringTokenizer;

import javax.swing.JOptionPane;


public class AIDA_RX_TX_COM_Port implements   SerialPortEventListener   {
	
	
	private static BufferedReader input;
	/** The output stream to the port */
	private static OutputStream output;
	/** Milliseconds to block while waiting for port open */
	private static final int TIME_OUT = 2000;
	/** Default bits per second for COM port. */
	private static final int DATA_RATE = 9600;
	
	
	static FileWriter w;
	static BufferedWriter fw;
	
	//ci metto tutte le porte
    private static  Enumeration ports = null;
    //per mappare le seriali
    private static  HashMap serialPortMap = new HashMap();
    
    //Per mappare le parallele
    private static HashMap parallelPortMap = new HashMap();

    //per contenere la porta aperta
    private static CommPortIdentifier selectedPortIdentifier = null;
    private static SerialPort serialPort = null;
    private ParallelPort parallelPort = null;
    
    static String position="out";  // sed if i am in register, program or name
	static String strLine;	
	static byte recievedByte1,recievedByte2;
	//static String selectAddress="0x6C";
	//static File selectedFile;
	static int Ic1_count=0;
	static String my_port;
    
    //static String pathWrite=null;
    static String fileRead=null;
    public static String ARGS[];
    
    static String logText = "";
    //static BufferedWriter fw, fric;
    
    static byte STX=0x02;
	static byte ETX=0x03;
	static byte NTX=(byte) 0xFF;
	
	static byte [] ACK={STX,ETX};
	static byte [] NACK={STX,NTX};
	
	static byte ack;
	static byte nack;
	
	static int number_of_burst_send=0;  //number of burst send to DSP
	
	
	/*private static int bolt;                // 8-bit buffer of bits to write out
	private static int howManyBit=0;*/
    
	public void loadPorts() {        

        //CommPortIdentifier portId = null;
        ports = CommPortIdentifier.getPortIdentifiers();

        //serial and parallel ports
        while (ports.hasMoreElements()){	    	 
	         CommPortIdentifier curPort = (CommPortIdentifier)ports.nextElement();
	            //get only serial ports
	         if (curPort.getPortType() == CommPortIdentifier.PORT_SERIAL){
	        	 serialPortMap.put(curPort.getName(), curPort);
	         } 
	       //get only parallel ports
	         else if (curPort.getPortType() == CommPortIdentifier.PORT_PARALLEL){
	        	 parallelPortMap.put(curPort.getName(), curPort);
	         }
	     }  
        if (serialPortMap.isEmpty() == true) {
			System.out.println("Could not find SERIAL port.");
			infoBox("Zero serial port present","LoadPort");
			System.exit(0);
			return;
		}
        if (parallelPortMap.isEmpty() == true) {
			System.out.println("Could not find PARALLEL port.");
			return;
		}  
	}
	
	public static void infoBox(String infoMessage, String titleBar)
    {
         
    }	

    
	public static void sendBits(int toSend)
    {
		int bolt=0;                // 8-bit buffer of bits to write out
		int howManyBit=0;
		
		for (int i = 0; i < 8; i++) {
            boolean bit = ((toSend >>> (8 - i - 1)) & 1) == 1;
         // add bit to buffer
            bolt <<= 1;
            if (bit) bolt |= 1;

            // if buffer is full (8 bits), write out as a single byte
            howManyBit++;
            if (howManyBit==8){
            	// write out any remaining bits in buffer to the binary output stream, padding with 0s
            	if (howManyBit == 0) return;
                if (howManyBit > 0) bolt <<= (8 - howManyBit);
                try { 
                	output.write(bolt);
                	output.flush();
                }
                catch (IOException e) { e.printStackTrace(); }
                howManyBit = 0;
                bolt = 0;
            }
        }
    }
	
	//questa funzione legge un byte alla volta
			public static byte ReadData() {
				byte readData = 0;
				//byte[] readBuffer = new byte[20];

		           try 
		            {	        	    
		        	   // int yreadData = (byte)input.read(readBuffer);
		        	    readData = (byte)input.read();  
		        	    //infoBox("I read ","ReadData");
		            }
		            catch (Exception e)
		            {
		                logText = "Failed to read data. (" + e.toString() + ")";
		                infoBox("Zero byte read","ReadData");
		            }
		          
		           return readData;
		    }
			
			public static String clearData(String perl){
	    		 StringTokenizer tok1=new StringTokenizer(perl," ");
	    		 String buf=new String();
	    		 
	    		 while(tok1.hasMoreTokens()){
	    	    		buf=buf+tok1.nextToken().trim(); //elimino gli spazi 0x00,0x00,0x00,
	    	    	//data=data.substring(0, data.length()-2); //elimino anche la virgola finale 	
	    	    }
	    		 return buf;
	    	}
	
	
	public void openPort(String selectedPort){
		try {
			
			selectedPortIdentifier = (CommPortIdentifier)serialPortMap.get(selectedPort);
			
			if(selectedPortIdentifier==null){
				System.out.println("Port " + selectedPort + " Not present");
				infoBox("Port " + selectedPort + " not present","OpenPort");
				System.exit(0);
			}
        	
			// open serial port, and use class name for the appName.
        	
        	serialPort = (SerialPort) selectedPortIdentifier.open(this.getClass().getName(),TIME_OUT);

        	// set port parameters
        	serialPort.setSerialPortParams(DATA_RATE,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);

        	// open the streams
        	input = new BufferedReader(new InputStreamReader(serialPort.getInputStream()));
        	output = serialPort.getOutputStream();
        	
        	serialPort.addEventListener(this);
            serialPort.notifyOnDataAvailable(true);

       	
        } catch (Exception e) {
        	System.err.println(e.toString());
        }
	}
	
	
	public static void writeData(Burst dataTX) throws FileNotFoundException
    {	
        try
        {	            
           /* output.write(STX);
            output.flush();                
            output.write(dataTX.NBYTE);
            output.flush();
            //spedire ADDR_MSB e ADDR_MSB
            output.write(dataTX.ADDR_MSB);
            output.flush();
            output.write(dataTX.ADDR_LSB);	 
            output.flush();*/ 
        	
            sendBits(Integer.parseInt("02",16));
            sendBits(dataTX.NBYTE);
            sendBits(dataTX.ADDR_MSB);
            sendBits(dataTX.ADDR_LSB);
            //spedire ADDR_MSB e ADDR_MSB
           /* output.write(dataTX.ADDR_MSB);
            output.flush();
            output.write(dataTX.ADDR_LSB);	 
            output.flush();*/
            
            
            //now I send the DATA bytes
            //for(int j=0; j<32; j++){
            for(int j=0; j<dataTX.NBYTE; j++){
            	 sendBits(dataTX.intDataToSend[j]);
            }
            
           /* for(int j=0; j<32; j++){
           	 output.write(dataTX.intDataToSend[j]);
           }*/
            	           
            
            /*output.write(ETX);
            output.flush();*/
            sendBits(dataTX.CHECKSUM);
            sendBits(Integer.parseInt("03",16));
          
        }
        catch (Exception e)
        {
            logText = "Failed to write data. (" + e.toString() + ")";
            infoBox("Failed to write data ","WriteData");
        }
    }
	
	/**
	 * This should be called when you stop using the port.
	 * This will prevent port locking on platforms like Linux.
	 */
	public synchronized void close() {
		if (serialPort != null) {
			serialPort.removeEventListener();
			serialPort.close();
		}
	}	
	
	public static String  incrementAddres(String addr,int increment){
		short delta=0x01;
		//ottengo una word dai due byte. Uso short perchè è a 16 bit (una word)
		byte LSB=(byte) (Integer.parseInt(addr) & 0xFF);
		byte MSB=(byte) ((Integer.parseInt(addr)>>8) & 0xFF);				
		short totalAddr=(short) ((MSB<<8)|(LSB&0xFF)); //ora ho indirizzo su 2 byte
		//ora incremento di nByte byte e cioè nByte 0x01
		for(int k=0;k<increment; k++)
			totalAddr=(short) (totalAddr+delta);
		
		return Short.toString(totalAddr);
	}
	
	public static void sendFile() throws IOException, InterruptedException{
		String data = null;
		String name=null;
		String address = null;
		String size = null;			
		//Burst messageToSend = null;
		
	    BufferedReader reader =new BufferedReader(new FileReader(fileRead));
	    
	    while ((strLine = reader.readLine()) != null){
	    	
	    	System.out.println("Riga caricata " + strLine);
	    	strLine=strLine.trim();
	    	WhereIAm(strLine);  //am I in register, program or other?
	        if(position=="Register"){
	        	//System.out.print(strLine);
	        	strLine = reader.readLine().trim(); //now i read <Name>IC 1.CoreRegister</Name>
	            name=strLine.substring(6,strLine.length()-7);
	            //compatto il nome(elimino gli spazi)
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
	            strLine=reader.readLine().trim();   //<Data>0x00, 0x18, </Data>
	            data=strLine.substring(6,strLine.length()-9);  //questi sono i dati
	            InviaBurst(size,address,data);	            
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
	             //unsigned char progrDataData[progDataSize]={
	                	// now I extract data
	            strLine=reader.readLine();   //<Data>0x00, 0x18, </Data>
	            strLine=strLine.trim();
	            data=strLine.substring(6,strLine.length()-9);
	            InviaBurst(size,address,data);
	        }		       
	        if(position=="Cell"){
	        	//System.out.print(strLine);	        	
	            //name=strLine.substring(10,strLine.length()-11);
	            name=CompactName(strLine.substring(10,strLine.length()-11));
	            //if name contains '-' it shall be '_'
	            if(name.contains("-")==true){	            	
	            	name=name.replace('-','_');            	
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
                	// now I extract data
		            strLine=reader.readLine();
		            strLine=reader.readLine().trim();   //<Data>0x00, 0x18, </Data>
		            data=strLine.substring(6,strLine.length()-9);	
		            InviaBurst(size,address,data);
	            }
	            else{
	            	for(int i=0;i<7; i++){
		            	strLine = reader.readLine().trim();
		            }	            
		            address=strLine.substring(9,strLine.length()-10);
		            InviaBurst(size,address,data);
	            }
	        }
	        
	        position="out";        
	 }
	    reader.close();
	}
	
	public static void InviaBurst(String size,String address,String data) throws IOException, InterruptedException{
		Burst messageToSend;
		//ora ho: name
        //address, che è un numero 2076
        //size, che è un numero 2
        //data, che è una stringa enorme del tipo 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,   
        //ma attenzione, sono tutte stringhe	
        //int numByteData=(data.length()+1)/6; //mi dice quanti sono i byte contenuti nella stringa data. Il più 1 tiene conto del fatto che l'ultimo dato non ha la , (0x21,0x24,0x35). Lui quindi è lungo 4 e non 5
        //devo inviare i dati a pacchetti di 32 byte e leggere 4 byte e se ho ricevuto ack proseguo altrimenti ritrasmetto
        //int numeroByteData=data.length();
        //devo sapere quanti burst occorrono per spedire i dati a blocchi di 32 byte
		number_of_burst_send++;
        int nBurst=(int) Math.ceil((double)(Integer.parseInt(size)/32));
        if(nBurst==0)
        	nBurst=1;
        //per inviare tutti i dati ho bisogno di nBurst nBurst 
        //se non sono multipli di 32 allora nBurst-1 conterranno 32 Byte di informazioni e l'ultimo meno di 32
        boolean isMultiple=false;
        if(Integer.parseInt(size)%32==0)
        	isMultiple=true;	//sono multiplo esatto di 32, manderò nBurst pieni
        else{
        	isMultiple=false;	//aggiungo alcuni 0x00 per farlo diventare multiplo di 32, ne aggiungo 32-data.length()%32
        	for(int h=0; h<32-(Integer.parseInt(size)%32); h++)
        		data=data+", 0x00";
        }
   	
    	data=clearData(data);
       
        //ora invio i pacchetti
        for(int t=0; t<nBurst; t++){
        	//creo l'oggetto burst caricandolo con 32 byte alla volta
        	messageToSend=new Burst(size,incrementAddres(address, t),data.substring(t*160, (t+1)*160-1)); //ogni byte occupa 5 caratteri nella mia stringa (0x01,), questo significa che per prenderne 32 di byte devo prendere 160 caratteri
        	//infoBox("Tentativo** ",String.valueOf("2222"));
        	//boolean succes=false;
        	recievedByte1=recievedByte2=0x00;
	        for(int attempt=0;attempt<3;attempt++){        		
	        	
	        	//JOptionPane.showMessageDialog(null,  "Attempt : " + String.valueOf(attempt),"Tentativo", JOptionPane.INFORMATION_MESSAGE);
	        	writeData(messageToSend);
	        	
	        	//Thread.sleep(300); // I stop the proces for 0,3 sec so i can wait mc replay
	        
	        	//leggo 2 byte 
	        	
	        	/*a=ReadData();
	        	b=ReadData();*/
	        	//JOptionPane.showMessageDialog(null, " recievedByte1 " + String.valueOf((Integer.toHexString(recievedByte1))) + "\n recievedByte2 " + String.valueOf((Integer.toHexString(recievedByte2))),"recievedByte", JOptionPane.INFORMATION_MESSAGE);
	        	//SE RICEVO ACK PASSO AL PROSSIMO DATO 
	        	if(recievedByte1==ACK[0] && recievedByte2==ACK[1] ){
		        	//JOptionPane.showMessageDialog(null,  "******* : " ,"recievedByte", JOptionPane.INFORMATION_MESSAGE);
	        		fw.write("Il DSP ha ricevuto il pacchetto numero " + (t+1) + " del burst numero " + number_of_burst_send + " con checksum " + messageToSend.CHECKSUM +  " Tentativo " + (attempt +1) );
	        		fw.newLine();
	        		fw.flush();
	        		break;
	        	}
	        	else if(recievedByte1==NACK[0] && recievedByte2==NACK[1] ){
	        		fw.write("Il DSP ha risp con NACK il pacchetto numero " + (t+1) + " del burst numero " + number_of_burst_send + " con checksum " + messageToSend.CHECKSUM +  " Tentativo " + (attempt +1) );
	        		fw.newLine();
	        		fw.flush();
	        	}
	        	else{
	        		fw.write("Il DSP non ha risposto al pacchetto numero " + (t+1) + " del burst numero " + number_of_burst_send + " con checksum " + messageToSend.CHECKSUM +  " Tentativo " + (attempt +1) );
	        		fw.newLine();
	        		fw.flush();
	        	}
	        	
	        	
	        	//Thread.sleep(300);
	        		
	        }//(faccio 3 tentativi distanziati di 300 msec)
	        
	        
        }
	}
		

	/*@Override
	public synchronized void serialEvent(SerialPortEvent oEvent) {
		if (oEvent.getEventType() == SerialPortEvent.DATA_AVAILABLE) {
			try {
				String inputLine=input.readLine();
				//System.out.println(inputLine);		
				System.out.println("4444444444444444444444444444444444444444444444444444444444");
				ReadData();
			} catch (Exception e) {
				System.err.println(e.toString());
				infoBox("SerialEvent error ","SerialEvent");
			}
		}
		// Ignore all the other eventTypes, but you should consider the other ones.
	}*/
	
	static String WhereIAm(String Element){
			
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
	
	static boolean IsCom(String port){
		boolean isValidPort=false;
		
		for(int i=1; i<=36; i++){
			if(port.compareTo("COM"+ Integer.toString(i))==0){
				isValidPort= true;
				break;
			}
		}	
		return isValidPort;
	}
	
	static void checkPort(String port) throws InterruptedException{
		if(IsCom(port)==false){
			System.out.println("Port " + my_port +" Not correct. Please, insert a correct serial port");
			infoBox("Port " + my_port +" Not correct. Please, insert a correct serial port","IsComPort");
			Thread.sleep(4000);
			System.exit(0);
		}
	}
	
	static void checkFile(String file) throws InterruptedException{
		File f = new File(file);
		if(!f.exists() || f.isDirectory()) {
			System.out.println("Please, insert a correct file name");
			infoBox("File " + file +" does not exist or is a directory","File read");
			Thread.sleep(4000);
			System.exit(0);
		}
	}
	
	public static void main(String[] args) throws Exception {
		 
		w=new FileWriter(System.getProperty("user.dir") + "\\report_pacchetti.txt");		 
		fw=new BufferedWriter(w);
		
		
		ARGS=args;
		//fileRead=ARGS[0];
		/*my_port=ARGS[1];
		my_port=my_port.toUpperCase();*/
		my_port="COM9";
		fileRead="C:\\Users\\Massimo\\Documents\\WorkspaceEclipse\\AidaDSPLoader\\Tutorial_2.xml";		
		
		long start=System.nanoTime();
		
		checkPort(my_port);
		//if fileRead is a directory or does not exist
		checkFile(fileRead);
		
		AIDA_RX_TX_COM_Port main = new AIDA_RX_TX_COM_Port();
		main.loadPorts();
		main.openPort(my_port);//connect port but don't write
		
		sendFile();
		
	    //close();
	    
	    //libero la porta
	    if (serialPort != null) {
			serialPort.removeEventListener();
			serialPort.close();
		}
	    output.close();
	    input.close();
	    
	    long stop=System.nanoTime();
	    System.out.println("\n\n\n\n Fine esecuzione in " + (stop-start)/1000000000 + " secondi.");
	    infoBox("\n\n\n\n Fine esecuzione in " + (stop-start)/1000000000 + " secondi.","Finish");

	}


	@Override
	public synchronized void serialEvent(SerialPortEvent oEvent) {
		if (oEvent.getEventType() == SerialPortEvent.DATA_AVAILABLE) {
			try {
				byte a;				
				recievedByte1=ReadData();
				recievedByte2=ReadData();
				infoBox("\n\n\n\n Serial event " , "Received " + String.valueOf(recievedByte1) + " and " + String.valueOf(recievedByte2));
				
				//Thread.sleep(40000000);
		    }catch (Exception e){
		                logText = "Failed to read data. (" + e.toString() + ")";
		                infoBox("Zero byte read","serialEvent");
		    }
		}
		// Ignore all the other eventTypes, but you should consider the other ones.
	}

}
