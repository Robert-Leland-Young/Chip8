#include "stdio.h"

int main(int argc, char **argv)
{
int i;                  /* Loop index */
unsigned int addr;      /* Disassembly address & Input File Fffset */

if (argc<3) { /* Check Command Line Parameters */

    printf("Disassembler for Chip-8 Binary Files\n\n");
    printf("Usage:\n");
    printf("%s input_file output_file begin_hex_addr\n\n",argv[0]);
    
    printf("input_file  - Chip-8 Binary executable (required)\n");
    printf("output_file - Disassembly output ASCII filename (required)\n");
    printf("begin_addr  - Hexadecimal offset at which to begin Disassembly\n");
    printf("              Optional parameter. Default is 0x200.\n"); 

    return(argc);  /* Leave */
    }

/* Display Command Line Arguments */
    
    printf("argc=%u\n",argc);

    for (i=0;i<argc;i++) {
        printf("argv[%u]=%s\n",i,argv[i]);
    }

/* Set beginning disassembly Address */
    if (argc<4) { /* Default 0x200 */
        addr=0x200;
    }
    else {
       sscanf(argv[3],"%x",&addr); 
    }


return(argc); /* Leave */
    
}
