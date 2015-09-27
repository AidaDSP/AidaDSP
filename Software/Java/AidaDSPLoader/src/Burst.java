package AIDA;

import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.util.StringTokenizer;

import javax.swing.JOptionPane;


public class Burst{
	int NBYTE;
	byte ADDR_MSB;
	byte ADDR_LSB;
	//List <String> intDataToSend=new  ArrayList<String>() ;
	int [] intDataToSend ={ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							0x00,0x00,0x00,0x00,0x00};  //32byte
	byte CHECKSUM;
	
	String [] stringData ={ "0x00","0x00","0x00","0x00","0x00","0x00",
							"0x00","0x00","0x00","0x00","0x00","0x00",
							"0x00",	"0x00","0x00","0x00","0x00","0x00",
							"0x00","0x00","0x00","0x00","0x00","0x00",
							"0x00","0x00","0x00","0x00","0x00","0x00",
							"0x00","0x00"};
	
	public Burst(String nbytes, String addr, String loadData){   		
		
		NBYTE=Integer.parseInt(nbytes);
		ADDR_LSB=(byte) (Integer.parseInt(addr) & 0xFF);
		ADDR_MSB=(byte) ((Integer.parseInt(addr)>>8) & 0xFF);		
    	//CHECKSUM=Byte.parseByte(check);
		int sum_for_check=0;
		
		sum_for_check=sum_for_check+ Integer.decode(nbytes)+(int) ADDR_LSB & 0xFF + (int) ADDR_MSB & 0xFF;  //(int) ADDR_LSB & 0xFF; in order to obtain integer from byte
    	
		//JOptionPane.showMessageDialog(null, String.valueOf(sum_for_check), "InfoBox: " + "chech", JOptionPane.INFORMATION_MESSAGE);
		
		//la variabile data è del tipo 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
        //ora spacchetto i singoli byte
        StringTokenizer dataString=new StringTokenizer(loadData,",");
        //ora ho uno stringtokenizer: 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
        int j=0;
        while(dataString.hasMoreTokens()) { 
        	String singleByte=null;
        	singleByte=dataString.nextToken();	//0x00	
        	stringData[j]=singleByte;
        	singleByte=singleByte.trim();  //elimino eventuali spazi e ottengo 0x01
        	singleByte=singleByte.substring(2,singleByte.length()); // elimino 0x        	       	
        	//intDataToSend[j]=FromHexToByte(singleByte);
        	intDataToSend[j]=Integer.parseInt(singleByte,16);
        	sum_for_check=sum_for_check+intDataToSend[j];
        	//System.out.println(intDataToSend[j]);
        	j++;
        }
        //now i get only first 8 bit of CHECKSUM
        CHECKSUM=(byte) (sum_for_check  & 0xFF);
	}
	
	public static byte FromHexToByte(String hexNumber){
		 
	     int stringToInt=Integer.parseInt(hexNumber,16);
	     int buf=(byte) ((byte) stringToInt &0xFF);
	     byte convertito=(byte) buf;
	     
	     return convertito;
	 }
	
}