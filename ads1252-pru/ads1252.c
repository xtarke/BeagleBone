/*
 ============================================================================
 Name        : ads1252.c
 Author      : Renan Augusto Starke
 Version     :
 Copyright   : Your copyright notice
 Description : User space code for loading pru code for ads1252 interface
               must be linked with -lpthread -lprussdrv
               This program is based on, he book "Exploring BeagleBone: Tools and 
               Techniques for Building with Embedded Linux" by John Wiley & Sons, 2014
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#define PRU_NUM	0   // using PRU0 

#define MAP_SIZE 0x0FFFFFFF
#define MAP_MASK (MAP_SIZE - 1)
#define MMAP0_LOC   "/sys/class/uio/uio0/maps/map0/"
#define MMAP1_LOC   "/sys/class/uio/uio0/maps/map1/"

static void *pru0DataMemory;
static unsigned int *pru0DataMemory_int;

int dump_samples();


// Short function to load a single unsigned int from a sysfs entry
unsigned int readFileValue(char filename[]){
   FILE* fp;
   unsigned int value = 0;
   fp = fopen(filename, "rt");
   fscanf(fp, "%x", &value);
   fclose(fp);
   return value;
}

int main (void)
{
    int i;
    int ret = 0;
   
    /* Mapped DDR memory: 
     * modprobe args:   options uio_pruss extram_pool_sz=0x927C00 */
    unsigned int PRU_DDR_data_addr;
    unsigned int PRU_DDR_data_size;
    unsigned int DDR_data[2];
       
 
    if(getuid()!=0){
        printf("You must run this program as root. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    // Initialize structure used by prussdrv_pruintc_intc
    // PRUSS_INTC_INITDATA is found in pruss_intc_mapping.h
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    
    
    /* MMAP1_LOC is DDR address space mapped to pru */
    PRU_DDR_data_addr = readFileValue(MMAP1_LOC "addr");
    PRU_DDR_data_size = readFileValue(MMAP1_LOC "size");
    printf("The DDR External Memory pool has location: 0x%x and size: 0x%x bytes\n", PRU_DDR_data_addr, PRU_DDR_data_size);
    
    DDR_data[0] = PRU_DDR_data_addr;
    /* Size in words or number of samples */
    DDR_data[1] =  PRU_DDR_data_size >> 2;   
    
    prussdrv_init ();

    // Allocate and initialize memory
    ret = prussdrv_open (PRU_EVTOUT_0);
    
    if (ret != 0) {
        perror("init");
        exit(-1);
    }
    // MAP PRU local sram data
    prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, &pru0DataMemory);
    pru0DataMemory_int = (unsigned int *) pru0DataMemory;
  
    /* Pass DDR address and size to PRU */
    prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, DDR_data, sizeof(DDR_data));
      
    unsigned int bytes_available = prussdrv_extmem_size();     
    printf("Reading %d bytes of data\n", bytes_available);
    printf("Number of samples: %d   (32-bit per sample)\n", bytes_available >> 2);
    printf("Reading...\n");
    
    // Map PRU's interrupts
    prussdrv_pruintc_init(&pruss_intc_initdata);
    // Load and execute the PRU program on the PRU
    prussdrv_exec_program (PRU_NUM, "./ads1252.bin");
    
     // Wait for event completion from PRU, returns the PRU_EVTOUT_0 number
    int n = prussdrv_pru_wait_event (PRU_EVTOUT_0);
    printf("EBB PRU program completed, event number %d.\n", n);
       
    // Disable PRU and close memory mappings
    prussdrv_pru_disable(PRU_NUM);
    prussdrv_exit ();
    
    /* Read all samples stored in DDR */
    dump_samples();
    
    
    return EXIT_SUCCESS;
}


int dump_samples (){

    int fd;
    int i;
    FILE* fp;
    void *map_base, *virt_addr;
    unsigned long read_result, writeval;
    unsigned int addr = readFileValue(MMAP1_LOC "addr");
    /* Datasize in bytes */
    unsigned int dataSize = readFileValue(MMAP1_LOC "size");
    /* Each sample is word (32-bit) aligned */
   
    unsigned int numberOutputSamples = dataSize >> 2;
    off_t target = addr;

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){
        printf("Failed to open memory!");
        return -1;
    }
    fflush(stdout);

    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) {
       printf("Failed to map base address");
       return -1;
    }
    fflush(stdout);

    
    fp = fopen("data.csv", "w");
    
    if (fp == NULL) {
        perror("dump_samples");
        return -1;
    }    
    

    printf("Writing %d samples.\n", numberOutputSamples);
    
    for(i=0; i < numberOutputSamples; i++){
        virt_addr = map_base + (target & MAP_MASK);
        read_result = *((uint32_t *) virt_addr);
        //printf("Value at address 0x%X (%p): 0x%X\n", target, virt_addr, read_result);
        
        fprintf(fp, "%d;0x%x\n",i, read_result);
        
        target+=4;                   // 2 bytes per sample
    }
    
    
    fflush(stdout);

    if(munmap(map_base, MAP_SIZE) == -1) {
       printf("Failed to unmap memory");
       return -1;
    }

    fclose(fp);
    close(fd);
    return 0;
    
    
}
