//============================================================================
// Name        : gpio.cpp
// Author      : Renan
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================


/*
 * Enter “Test1” for project name
Select “Hello World ANSI C Project” for project type
Select “Cross GCC for toolchains and click “Next”
Enter your name for Author and click “Next”
Be sure to select both “debug” and “release configuration”, or “Select All” and click “Next” to continue
Enter a cross compiler prefix and path to the cross compiler tools
32bit toolchain: “arm-linux-gnueabihf-”
64bit toolchain: “aarch64-linux-gnu-”
Enter “/usr/bin” for the cross compiler path and select “Finish”


 Remote Config:

 http://michaelhleonard.com/cross-compile-for-beaglebone-black/

 */


#include <iostream>

#include "BlackLib/v3_0/BlackGPIO/BlackGPIO.h"
#include <string>
#include <iostream>

#include <unistd.h>


using namespace std;

int main() {

	BlackLib::BlackGPIO   led1(BlackLib::GPIO_66,BlackLib::output, BlackLib::FastMode);   // initialization first output, secure mode

	while (1){
		 led1.setValue(BlackLib::high);

		 usleep(10000);

		 led1.setValue(BlackLib::low);

		 usleep(10000);

	}


	return 0;
}
