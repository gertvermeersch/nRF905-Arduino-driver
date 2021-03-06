#include <NRF905.h>
#include <SPI.h>

/** define a ultity */
//NRF905 nrf905;

char config_info_buf[10]={
        0xCE,                   //CH_NO,433MHZ
        0x0D,                   //output power 10db, resend disable, Current Normal operation
        0x44,                   //4-byte address
        0x20,0x20,              //receive or send data length 32 bytes
        0xCC,0xCC,0xCC,0xCC,    //receiving address
        0x58,                   //CRC enable,8bit CRC,external clock disable,16MHZ Oscillator
};



unsigned int freq_tab[10] = {
    0x13e,
    0x076,
    0x076,
    0x076,
    0x177,
    0x02b,
    0x17f,
    0x17f,
    0x17f,
    0x100,
};

//-------------------initial nRF905---------------------------------------------
NRF905::NRF905(void)
{
    TXEN=5;
    TRX_CE=4;
    PWR=8;
    CSN=10;
    AM=9;
    DR=3;
    CD=7;
}
// NRF905::NRF905(int css)
// {
//     TXEN=5;
//     TRX_CE=4;
//     PWR=3;
//     CSN=css;
//     AM=9;
//     DR=8;
//     CD=7;
// }
NRF905::NRF905(int txen, int trx_ce, int pwr, int csn, int am, int dr, int cd)
{
	TXEN=txen;
	TRX_CE=trx_ce;
	PWR=pwr;
	CSN=csn;
	AM=am;
	DR=dr;
	CD=cd;
}

void NRF905::init(void)
{
    pinMode(CSN, OUTPUT);
	digitalWrite(CSN, HIGH); // Spi disable

	pinMode(DR, INPUT);	// Init DR for input

	pinMode(AM, INPUT);// Init AM for input

	pinMode(CD, INPUT);// Init CD for input

	pinMode(PWR, OUTPUT);
	digitalWrite(PWR, HIGH);// nRF905 power on

	pinMode(TRX_CE, OUTPUT);
	digitalWrite(TRX_CE, LOW);// Set nRF905 in standby mode

	pinMode(TXEN, OUTPUT);
	digitalWrite(TXEN, LOW);// set radio in Rx mode

	SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.begin();

    /** wait for nrf905 STANDBY */
    delay(3);
}

//---------------wite to configuration register-----------------
void NRF905::write_config(char *conf_buf)
{
	digitalWrite(CSN,LOW);						// Spi enable for write a spi command
	/** send write configuration command */
	SPI.transfer(WC);
	for (int i=0;i<10;i++)	// Write configration words
	{
	   SPI.transfer(conf_buf[i]);
	}
	digitalWrite(CSN,HIGH);					// Disable Spi
}

void NRF905::write_config(nrf905_freq_type freq_band)
{
/*  Go back to 434.2 Mhz
    if(freq_band < 10){
        config_info_buf[0] = (unsigned char)freq_tab[freq_band];
        if(freq_tab[freq_band]&0x100){
            config_info_buf[1] |= 0x01;
        }else{
            config_info_buf[1] &= ~0x01;
        }
    }*/

    // Spi enable for write a spi command
	digitalWrite(CSN,LOW);
	/** send write configuration command */
	SPI.transfer(WC);
	for (int i=0;i<10;i++)	// Write configration words
	{
	   SPI.transfer(config_info_buf[i]);
	}
	digitalWrite(CSN,HIGH);					// Disable Spi
}

void NRF905::write_config_address(char *address)
{
	config_info_buf[5] = address[0];
	config_info_buf[6] = address[1];
	config_info_buf[7] = address[2];
	config_info_buf[8] = address[3];
	digitalWrite(CSN,LOW);
	/** send write configuration command */
	SPI.transfer(WC);
	for (int i=0;i<10;i++)	// Write configration words
	{
	   SPI.transfer(config_info_buf[i]);
	}
	digitalWrite(CSN,HIGH);
}

void NRF905::read_config(char *conf_buf)
{
    digitalWrite(CSN,LOW);						// Spi enable for write a spi command

    /** send read configuration command */
    SPI.transfer(RC);
	for (int i=0;i<10;i++)
	{
	   conf_buf[i] = SPI.transfer(0x00);
	}
	digitalWrite(CSN,HIGH);					// Disable Spi
}

void  NRF905::RX(char *TxRxBuffer)
{
    set_rx();			// Set nRF905 in Rx mode
    while (check_ready()==0);
    delay(1);
    RxPacket(TxRxBuffer);
    delay(1);
}

void NRF905::RX(char *TxRxBuf, char *RxAddress) //receive and change own address
{
    if(config_info_buf[5] != RxAddress[0] ||\
       config_info_buf[6] != RxAddress[1] ||\
       config_info_buf[7] != RxAddress[2] ||\
       config_info_buf[8] != RxAddress[3]){

        config_info_buf[5] = RxAddress[0];
        config_info_buf[6] = RxAddress[1];
        config_info_buf[7] = RxAddress[2];
        config_info_buf[8] = RxAddress[3];

        write_config(config_info_buf);
    }

    set_rx();			// Set nRF905 in Rx mode
    while (check_ready()==0);
    delay(1);
    RxPacket(TxRxBuf);
    delay(1);
}

void NRF905::TX(char *TxRxBuf, char *TxAddress)
{

    set_tx();
    delay(1);
    // Send data by nRF905
    TxPacket(TxAddress, TxRxBuf);
	set_rx();

}

void NRF905::TX(char *TxRxBuf)
{

    set_tx();
    delay(1);
    // Send data by nRF905
    TxPacket(config_info_buf+5, TxRxBuf);
	set_rx(); //switch back to receiving mode to set DR low

}

void NRF905::TxPacket(char *TxAddress, char *TxRxBuf) {
	cli(); //timing critical
  //Wait for clear skies
  while(digitalRead(CD));
	int i;
	digitalWrite(CSN,LOW);
	// Write payload command
	SPI.transfer(WTP);
	//Serial.print("deep debug NRF905.cpp: ");
	for (i=0;i<32;i++){
	    // Write 32 bytes Tx data
		SPI.transfer(TxRxBuf[i]);
	//	Serial.print(TxRxBuf[i],DEC);
	}
	//Serial.print("\n");
	digitalWrite(CSN,HIGH);
    // Spi enable for write a spi command
	digitalWrite(CSN,LOW);
	// Write address command
	SPI.transfer(WTA);
	// Write 4 bytes address
	for (i=0;i<4;i++){
		SPI.transfer(TxAddress[i]);
	}
	// Spi disable
	digitalWrite(CSN,HIGH);

	// Set TRX_CE high,start Tx data transmission, CE pulse
	digitalWrite(TRX_CE,HIGH);
	delay(1);
	digitalWrite(TRX_CE,LOW);
	while(digitalRead(DR) == LOW); //wait for the packet to be sent
	sei(); //re-enable the interrupts, DR is LOW so safe to enable again.
}

void NRF905::set_tx(void)
{
	digitalWrite(TRX_CE,LOW);
	digitalWrite(TXEN,HIGH);

	// delay for mode change(>=650us)
	delay(1);
}


void NRF905::set_rx(void)
{
	digitalWrite(TXEN, LOW);
	digitalWrite(TRX_CE, HIGH);

	// delay for mode change(>=650us)
	delayMicroseconds(800);
};

bool NRF905::check_ready(void)
{
    if(digitalRead(DR) == HIGH && digitalRead(AM) == HIGH)	{
		return true;
	}
	else{
		return false;
	}
}

void NRF905::RxPacket(char *TxRxBuffer)
{
	cli(); //timing critical, no interrupts
	int i;
    digitalWrite(TRX_CE,LOW);
	digitalWrite(CSN,LOW);
    delay(1);
	SPI.transfer(RRP);
    delay(1);
	for (i = 0 ;i < 32 ;i++){
		TxRxBuffer[i]=SPI.transfer(NULL);
        delay(1);
	}
	digitalWrite(CSN,HIGH);
    delay(1);
	digitalWrite(TRX_CE,HIGH);
	delay(1);
	sei();
}

void NRF905::powerUp(void) {
  digitalWrite(PWR, HIGH);
  delay(3); //max delay
}

void NRF905::powerDown(void) {
  digitalWrite(PWR, LOW);
}
