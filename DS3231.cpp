#include<stdio.h>
#include"DS3231.h"
#include<fcntl.h>
#include<sys/ioctl.h>
#include<linux/i2c.h>
#include<linux/i2c-dev.h>
#include<iomanip>
#include<iostream>
#include<unistd.h> //needed for read and write in C++
#include <fstream> //needed for novel functionality

using namespace std;

#define HEX(x) setw(2) << setfill('0') << hex << (int)(x)

// the time is in the registers in encoded decimal form

DS3231::DS3231(bool twelvex, bool PMx, int RSx)
{ 
   twelve=twelvex;
   PM=PMx;
   RS=RSx;
	this->Fileopen(file);
}

int DS3231::Fileopen(int file)
{
   if((this->file=::open("/dev/i2c-1", O_RDWR)) < 0){
      perror("failed to open the bus\n");
      return 1;
   }
   if(ioctl(this->file, I2C_SLAVE, 0x68) < 0){
      perror("Failed to connect to the sensor\n");
      return 1;
   }
   return 0;
}

unsigned char* DS3231::readRegisters(unsigned int number, unsigned int fromAddress){
	this->resetAdd(fromAddress);
	unsigned char* data = new unsigned char[number];
    if(::read(this->file, data, number)!=(int)number){
       perror("IC2: Failed to read in the full buffer.\n");
	   return NULL;
    }
	return data;
}

void DS3231::debugDumpRegisters(unsigned int number){
	cout << "Dumping Registers for Debug Purposes:" << endl;
	unsigned char *registers = this->readRegisters(number);
	for(int i=0; i<(int)number; i++){
		cout << HEX(*(registers+i)) << " ";
		if (i%16==15) cout << endl;
	}
	cout << dec;
}

int DS3231::bcdToDec(char b)
{ 
   return (b/16)*10 + (b%16); 
}

int DS3231::decToBcd(int d)
{ 
   return (d/10)*16 + (d%10); 
}

int DS3231::resetAdd(int file)
{
   char writeBuffer[1] = {0x00};
   if(::write(this->file, writeBuffer, 1)!=1){
      perror("Failed to reset the read address\n");
      return 1;
   }
   return 0;
}

int DS3231::writeReg(unsigned int registerAddress, unsigned char value){
   unsigned char buffer[2];
   buffer[0]=registerAddress;
   buffer[1]=value;
   if (::write(this->file, buffer, 2)!=2){
      perror("I2C: Failed to write to the device\n");
      return 1;
   }
   return 0;
}

int DS3231::readReg(int file)
{
   if(::read(this->file, this->buf, BUFFER_SIZE)!=BUFFER_SIZE){
      perror("Failed to read in the buffer\n");
      return 1;
   }
   return 0;
}




void DS3231::readTime(){
   if(twelve==1)
      readTwelve();
   else{
      readTwentyfour();
   }
}

void DS3231::readTwelve(){
   resetAdd(this->file);
   readReg(this->file);
   int hours, mask, y;
   hours = bcdToDec(buf[2]);
   mask = (1 << 4) - 1;
   y = hours & mask;
   printf("The RTC time is %02d:%02d:%02d\n", y, bcdToDec(buf[1]), bcdToDec(buf[0]));
}

void DS3231::readTwentyfour(){
   resetAdd(this->file);
   readReg(this->file);
   int hours, mask, y;
   hours = bcdToDec(buf[2]);
   mask = (1 << 5) - 1;
   y = hours & mask;
   printf("The RTC time is %02d:%02d:%02d\n", y, bcdToDec(buf[1]), bcdToDec(buf[0]));
}

void DS3231::readTemp(){
   resetAdd(this->file);
   readReg(this->file);
   printf("The temperature is %02d.%02d\n", buf[17], (buf[18]>>6)*25); //right bit shift LSB by 
}                                                                      //6 bits to print relevant bits

void DS3231::readDate(){
   resetAdd(this->file);
   readReg(this->file);
   printf("The RTC date is %02d/%02d/%02d\n", bcdToDec(buf[4]), bcdToDec(buf[5]), bcdToDec(buf[6]));
}

void DS3231::writeHours(unsigned int registerAddress, char value){
      if(twelve == 1) //if object created is setting time in 12 hours
      {
         value |= (1 << 6);
         if(PM == 1) //if time to be set is pm
            value |= (1 << 5);         
      }
   writeReg(registerAddress, decToBcd(value)); //write hours
}

void DS3231::writeTime(){
   writeReg(0x00, decToBcd(0)); //write seconds
   writeReg(0x01, decToBcd(0)); //write minutes
   writeHours(0x02, 12); //write hours
}

void DS3231::writeAlarm(unsigned int registerAddress, char value){
   value &= ~(1 << 7); //clear bit
   writeReg(registerAddress, decToBcd(value));
}

void DS3231::writeAlarmHours(unsigned int registerAddress, char value){
     if(twelve == 1) //if object created is setting time in 12 hours
      {
         value |= (1 << 6);
         if(PM == 1) //if time to be set is pm
            value |= (1 << 5);         
      }
   writeReg(registerAddress, decToBcd(value)); //write hours
}

void DS3231::writeAlarmDate(unsigned int registerAddress, char value){
   value &= ~(1 << 7); //clear bit 7
   value &= ~(1 << 6); //clear bit 6
   writeReg(registerAddress, decToBcd(value)); //write hours
}

void DS3231::setAlarmIE(unsigned int registerAddress, int bit){
   buf[14] |= (1 << bit); //set bit
   writeReg(registerAddress, decToBcd(buf[14]));
}

void DS3231::setAlarm1(){
   writeAlarm(0x07, 1); //write seconds
   writeAlarm(0x08, 1); //write minutes
   writeAlarmHours(0x09, 12); //write hours
   writeAlarmDate(0x0a, 06); //set alarm date

   setAlarmIE(0x0e, 2);//set interrupt condition
   setAlarmIE(0x0e, 0);//set alarm1 interrupt enable
}

void DS3231::readAlarm1(){
   resetAdd(this->file);
   readReg(this->file);
   printf("The RTC alarm time is %02d:%02d:%02d on date: %02d\n", bcdToDec(buf[9]), bcdToDec(buf[8]), 
                                                                  bcdToDec(buf[7]), bcdToDec(buf[10]));
}

void DS3231::setAlarm2(){
   writeAlarm(0x0b, 3); //write minutes
   writeAlarmHours(0x0c, 12); //write hours
   writeAlarmDate(0x0d, 6); //set alarm date

   setAlarmIE(0x0e, 2);//set interrupt condition
   setAlarmIE(0x0e, 1);//set alarm1 interrupt enable
}

void DS3231::readAlarm2(){
   resetAdd(this->file);
   readReg(this->file);
   printf("The RTC alarm time is %02d:%02d on date: %02d\n", bcdToDec(buf[12]), bcdToDec(buf[11]), 
                                                                                bcdToDec(buf[13]));
}

void DS3231::writeDate(){
   writeReg(0x04, decToBcd(6)); //date
   writeReg(0x05, decToBcd(03)); //month
   writeReg(0x06, decToBcd(20)); //year
}

void DS3231::rateSelect(unsigned int registerAddress){
   if(RS==1){
      buf[14] &= ~(1 << 3); //clear bit 3
      buf[14] &= ~(1 << 4); //clear bit 4
      printf("RS1\n");
   }
   if(RS==2){
      buf[14] |= (1 << 3); //set bit 3
      buf[14] &= ~(1 << 4); //clear bit 4
      printf("RS2\n");
   }
   if(RS==3){
      buf[14] |= (1 << 4); //set bit 4
      buf[14] &= ~(1 << 3); //clear bit 3
      printf("RS3\n");
   }
   if(RS==4){
      buf[14] |= (1 << 3); //set bit 3
      buf[14] |= (1 << 4); //set bit 4
      printf("RS4\n");
   }
   writeReg(registerAddress, decToBcd(buf[14]));
}

void DS3231::fileWrite(){
   ofstream myfile;
   myfile.open ("DS3231_Output.txt");

   myfile << "--------------------------------------------------\n";
   myfile << "------Reading the date, time and temperature------\n";
   myfile << "--------------------------------------------------\n";
   int hours, mask, y;
   hours = bcdToDec(buf[2]);
   mask = (1 << 4) - 1;
   y = hours & mask;
   myfile << "The RTC time is " << setfill('0') << setw(2) << y << ":" << setfill('0') << setw(2) << bcdToDec(buf[1]) 
          << ":" << setfill('0') << setw(2) << bcdToDec(buf[0]) << "\n";

   myfile << "The RTC date is " << setfill('0') << setw(2) << bcdToDec(buf[4]) << "/" << setfill('0') << setw(2) 
          << bcdToDec(buf[5]) << "/" << bcdToDec(buf[6]) << "\n" ;

   myfile << "The temperature is " << (int)buf[17] << "." << setfill('0') << setw(2) 
          << (buf[18]>>6)*25 << "\n"; //cast to int as buf is a char, bit shift seems to convert to int

   myfile.close();
}



void DS3231::close()
{
   ::close(this->file);
	this->file = -1;
}

DS3231::~DS3231() {
	if(file!=-1) this->close();
}

 int main(){
   printf("Starting the DS3231 application\n");
   DS3231 i2c(1, 1, 1); //set first parameter to 1 if you want 12 hour clock, otherwise 0 for it to be 24 hours
                        //set second parameter to 0 for AM and 1 for PM
                        //Third parameter can be set to 1 = 1kHz, 2 = 1.024kHz, 
                        //3 = 4.096kHz or 4 = 8.192kHz, for different frequency rates

   printf("--------------------------------------------------\n");
   printf("------------Reading the date and time-------------\n");
   printf("--------------------------------------------------\n");
   i2c.readTime();
   i2c.readDate();

   printf("--------------------------------------------------\n");
   printf("-------------Reading the temperature--------------\n");
   printf("--------------------------------------------------\n");
   i2c.readTemp();


   printf("--------------------------------------------------\n");
   printf("----Setting and then reading the date and time----\n");
   printf("--------------------------------------------------\n");
   i2c.writeTime();
   i2c.writeDate();

   DS3231 read(0, 0, 1);

   i2c.readTime();
   read.readDate();

   printf("--------------------------------------------------\n");
   printf("-----Setting and then reading the alarm1 time-----\n");
   printf("--------------------------------------------------\n");
   read.setAlarm1();
   read.readAlarm1();

   printf("--------------------------------------------------\n");
   printf("-----Setting and then reading the alarm2 date-----\n");
   printf("--------------------------------------------------\n");
   read.setAlarm2();
   read.readAlarm2();

   printf("--------------------------------------------------\n");
   printf("------------Setting the frequency rate------------\n");
   printf("--------------------------------------------------\n");
   read.rateSelect(0x0e);

   printf("--------------------------------------------------\n");
   printf("---------------Novel functionality----------------\n");
   printf("--------------------------------------------------\n"); 
   printf("Output date, time and temperature to DS3231_Output.txt\n");     
   read.fileWrite();

   printf("--------------------------------------------------\n");
   printf("-----------Register dumping for debug-------------\n");
   printf("--------------------------------------------------\n");
   read.debugDumpRegisters(18);

   i2c.close();
   read.close();

   return 0;
 }