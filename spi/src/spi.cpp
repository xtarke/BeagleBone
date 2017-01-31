//============================================================================
// Name        : spi.cpp
// Author      : Renan
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>

#include "BlackLib/v3_0/BlackSPI/BlackSPI.h"
#include "BlackLib/v3_0/BlackGPIO/BlackGPIO.h"

using namespace std;

#define IRQ_PIN 60

#define cpl(y)   (y^=(1))

int get_lead(int fd);

int main() {
	cout << "SPI:" << endl;

    BlackLib::BlackSPI spi(BlackLib::SPI0_0, 8, BlackLib::SpiDefault , 4000000);
    BlackLib::BlackGPIO  sync_pin(BlackLib::GPIO_30,BlackLib::output, BlackLib::FastMode);

    FILE *io,*iodir, *edge;
    int ioval;

    /* Configuration through devide tree:
     *
     * echo 70 > /sys/class/gpio/export
     * echo 71 > /sys/class/gpio/export

     * echo in > /sys/class/gpio/gpio70/direction
     * echo in > /sys/class/gpio/gpio71/direction

     * echo both > /sys/class/gpio/gpio70/edge
     * echo both > /sys/class/gpio/gpio71/edge */


	io = fopen("/sys/class/gpio/export", "w");
	fseek(io,0,SEEK_SET);
	fprintf(io,"%d",IRQ_PIN);
	fflush(io);

	iodir = fopen("/sys/class/gpio/gpio60/direction", "w");
	fseek(iodir,0,SEEK_SET);
	fprintf(iodir,"in");
	fflush(iodir);

	edge = fopen("/sys/class/gpio/gpio60/edge", "w");
	fseek(edge,0,SEEK_SET);
	fprintf(edge,"rising");
	//fprintf(edge,"both");
	fflush(edge);


	// to reset ADC, we need SCLK HIGH for min of 4 CONVCYCLES
	// so here, hold SCLK HIGH for 5 CONVCYCLEs = 1440 usec
	usleep(1440);

	//readByte = spiDemo.transfer(0,0);
	// release ADC from reset; now we're at a known point
	// in the timing diagram, and just have to wait for
	// the beginning of a conversion cycle

	ioval = open("/sys/class/gpio/gpio60/value", O_RDONLY);
	struct pollfd pfd;

	pfd.fd = ioval;
	pfd.events = POLLPRI;
	pfd.revents = 0;

	volatile uint8_t byte = 0;

	while (1){
		int lead = 0;


//		int ready = poll(&pfd, 2, -1);
//
//		if (pfd.revents != 0) {
//			//printf("\t Lead A\n");
//			lead = get_lead(ioval);
//
//			if (lead)
//				sync_pin.setValue(BlackLib::high);
//			else
//				sync_pin.setValue(BlackLib::low);
//		}

		cpl(byte);

		if (byte)
			sync_pin.setValue(BlackLib::high);
		else
			sync_pin.setValue(BlackLib::low);


	    //printf("\t\t A: %d \n", byte);

	}


	fclose(io);
	fclose(iodir);
	fclose(edge);
	close(ioval);

	exit(0);


	bool isOpened = spi.open(BlackLib::ReadWrite|BlackLib::NonBlock);

	if( !isOpened )
	{
		std::cerr << "SPI DEVICE CAN\'T OPEN.;" << std::endl;
		exit(1);
	}

	uint8_t writeArr[3] = { 0x00, 0x00, 0x00 } ;
    uint8_t readArr[3];

	while (1){

		//if  (adc_rdy.getNumericValue() == 1){

			// to be safe, 30 usec, instead of 27 usec, which is
			// the expected period of DRDY phase
			//usleep(30);

		 memset(readArr,0,sizeof(readArr));

		 spi.transfer(writeArr, readArr, sizeof readArr);


		cout << (int)readArr[0] << endl;
		////}

		//usleep(10000);
	}


	fclose(io);
	fclose(iodir);
	fclose(edge);
	close(ioval);

	return 0;
}

int get_lead(int fd) {
        int value;
        lseek(fd, 0, 0);

        char buffer[1024];
        int size = read(fd, buffer, sizeof(buffer));

        if (size != -1) {
			buffer[size] = NULL;
			value = atoi(buffer);
        }
        else {
                value = -1;
        }

        return value;
}
