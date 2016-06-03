#include "stdio.h"

int main(int argc, char **argv)
{
int i;                  /* Loop index */
unsigned int addr;      /* Disassembly address & Input File offset */
unsigned char msb,lsb;   /* Instruction High & Low order bytes */
FILE *fin;              /* Input Chip8 Binary File */
FILE *fout;             /* Output ASCII Disassembly Listing File */
fpos_t fp;              /* Input file offset */
    
    
if (argc<2) { /* Check Command Line Parameters */

    printf("Disassembler for Chip-8 Binary Files\n\n");
    printf("Usage:\n");
    printf("%s input_file output_file begin_hex_addr\n\n",argv[0]);
    
    printf("input_file  - Chip-8 Binary executable (required)\n");
    printf("output_file - Disassembly output ASCII filename (optional)\n");
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

    
/* Try to open the input file */
    fin = fopen(argv[1],"rb");
    if (NULL==fin) { /* failed to open Input file */
    printf("Failed to open Input: %s\n",argv[1]);    
    return(0);
    }
    
/* Set output file pointer if no output file is specified */
    if (argc<3) { /* Default stdout */
        fout=stdout;
    }
    else {

    /* Try to open the output file */
      fout = fopen(argv[2],"wb");
      if (NULL==fout) { /* failed to open Output file */
        printf("Failed to open Output: %s\n",argv[2]);    
        return(0);
        }
    }
    /* Read Chip 8 Binary File */
    fp=addr; /* Initial File offset */
    if (0==fseek(fin,fp,SEEK_SET)) {  /* go to start of Disassembly */
        fprintf(fout,"Addr  Data     Mnemonic\r\n"); /* Column header */
        while (!feof(fin)) { /* Process input Binary */
            msb=fgetc(fin);  /* high order byte of instruction */
            if (!feof(fin)) { /* if not EOF get LSB of instruction */  
            lsb=fgetc(fin);  /* low order byte of instruction */
            fprintf(fout,"%04X  %02X %02X    \r\n",fp,msb,lsb);
            fp+=2;  /* Increment Offset Index */    
            }
        }
    }
     else {
          printf("File Offset I/O error: %s\n",argv[1]);
          }

fprintf(fout,"\r\nProcessing Complete.\r\n");
    
fclose(fin);
fclose(fout);
return(argc); /* Leave */
    
}
