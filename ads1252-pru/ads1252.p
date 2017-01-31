// PRUSS program to read ads1252 (24 bit, 40k samples @ 16MHz)
//
// "P9.27" (pru0_pru_r30_5)     -> clock
// "P9.28" (pru0_pru_r31_3)     -> Data in pin 
// "P9.29" (pr1_pru0_pru_r31_1) -> Abort pin
//
//
// -----------------------------------------------------------------------------------
// This program is based on the book "Exploring BeagleBone: Tools and 
// Techniques for Building with Embedded Linux" by John Wiley & Sons, 2014

.origin 0                        // start of program in PRU memory
.entrypoint START                // program entry point (for a debugger)


#define CLOCK_DELAY 15            // set up a 3 MHZ clock signal
#define DRDY_DELAY 1000           // set up a 10us delay  

#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back

#define CLOCK_PIN r30.t5
#define ABORT_PIN r31.t3
#define DATAIN_PIN r31.t1

#define BIT_2_TRANSFER 25

INIT: 
    // Enable the OCP master port -- allows transfer of data to Linux userspace
    LBCO    r0, C4, 4, 4     // load SYSCFG reg into r0 (use c4 const addr)
    CLR     r0, r0, 4        // clear bit 4 (STANDBY_INIT)
    SBCO    r0, C4, 4, 4     // store the modified r0 back at the load addr

    MOV   r3, 0x0   
    mov   r5, 0x0           // load SRAM init address to r5

    LBBO    r8, r5, 0, 4     // load to r8 the Linux address that is passed throgh PRU SRAM
    LBBO	r9, r5, 4, 4	 // load to r9 the DDR size allocated

    mov r6, r9

START:

    MOV r3, 0x0
    QBBS END, ABORT_PIN              // halt if P9_28 is set
    QBBC	START, DATAIN_PIN        // wait for AD to clear
    
    MOV r0, DRDY_DELAY
DRDY_WAIT:    
    SUB	r0, r0, 1 
    QBNE	DRDY_WAIT, r0, 0

    MOV r1, BIT_2_TRANSFER

START_CLOCK:
    SET	CLOCK_PIN               // turn on the clock pin 
    MOV	r0, CLOCK_DELAY        // store the length of the delay in REG0

CLK_ONE:
    SUB	r0, r0, 1                // Decrement REG0 by 1
    QBNE	CLK_ONE, r0, 0      // Loop to CLK_ONE, unless REG0=0

    clr r3.t0
    QBBC	DATAINLOW, DATAIN_PIN   // check if the bit that is read in is low? jump
    set r3.t0

DATAINLOW:
   
	CLR	CLOCK_PIN               // clear the output bin (LED off)
	MOV	r0, CLOCK_DELAY        // Reset REG0 to the length of the delay
CLK_OFF:
	SUB	r0, r0, 1              // decrement REG0 by 1
	QBNE	CLK_OFF, r0, 0     // Loop to CLK_OFF, unless REG0=0

    LSL	r3, r3, 1               // shift the captured data left by one position 
   
    SUB r1, r1, 1               // Another bit captured
    QBNE START_CLOCK, r1, 0     // restart while r1 > 0 (24 bits to capture)

    LSR r3,r3, 2                // CLOCK shifts left too many times left, shift right twice
    SBBO  r3, r8, 0, 4          // store the new data
    ADD r8, r8, 4               // increment pointer
    

    sub r6, r6, 1    
    QBNE START, r6, 0         // stop if captrured all the samples


END:                             // notify the calling app that finished
	MOV	R31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	HALT                     // halt the pru program
