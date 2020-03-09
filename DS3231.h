#define BUFFER_SIZE 19      //0x00 to 0x12

class DS3231 {
private:
int file;
char b;
char buf[BUFFER_SIZE];
unsigned char value;
unsigned int registerAddress;
bool PM, twelve;
int RS;
int bit;
public:
DS3231(bool twelve, bool PM, int RS);
virtual int Fileopen(int file);
virtual int resetAdd(int file);
virtual int readReg(int file);
virtual int writeReg(unsigned int registerAddress, unsigned char value);

void writeHours(unsigned int registerAddress, char value);

int bcdToDec(char b);
int decToBcd(int d);

void readTime();
void readTwelve();
void readTwentyfour();
void readTemp();
void readDate();

void writeTime();
void writeDate();

void writeAlarm(unsigned int registerAddress, char value);
void writeAlarmHours(unsigned int registerAddress, char value);

void writeAlarmDate(unsigned int registerAddress, char value);

void setAlarmIE(unsigned int registerAddress, int bit);

void setAlarm1();
void setAlarm2();
void readAlarm2();
void readAlarm1();

virtual unsigned char* readRegisters(unsigned int number, unsigned int fromAddress=0);
virtual void debugDumpRegisters(unsigned int number = 0xff);

void rateSelect(unsigned int registerAddress);
void readRS();

void fileWrite();

virtual void close();
virtual ~DS3231();
};