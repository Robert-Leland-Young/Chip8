#include "stdio.h"

int main(int argc, char **argv)
{
int i;                      /* Loop index */
unsigned int addr;          /* Disassembly address & Input File offset */
unsigned char msb,lsb;      /* Instruction High & Low order bytes */
unsigned char msbh,msbl;    /* Instruction MSB High & Low order  */
unsigned char lsbh,lsbl;    /* Instruction LSB High & Low order  */
FILE *fin;                  /* Input Chip8 Binary File */
FILE *fout;                 /* Output ASCII Disassembly Listing File */
fpos_t fp;                  /* Input file offset */
    
    
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
        fprintf(fout,"Addr  Data     Mnemonic             Comment\r\n"); /* Column header */
        while (!feof(fin)) { /* Process input Binary */
            msb=fgetc(fin);     /* high order byte of instruction */
            if (!feof(fin)) {   /* if not EOF get LSB of instruction */  
            lsb=fgetc(fin);     /* low order byte of instruction */
            /* Get Instruction Arguments */
            /* Get MSB Nibbles */
            msbh=msb >> 4;      /* Shift out lower Nibble of MSB */
            msbl=0x0F & msb;    /* Mask out upper Nibble of MSB */
            /*  Get LSB Nibbles */
            lsbh=lsb >> 4;      /* Shift out lower Nibble of LSB */
            lsbl=0x0F & lsb;    /* Mask out upper Nibble of LSB */
            
            fprintf(stdout,"%04X  %02X %02X  %1X  %1X  %1X  %1X\r\n",fp,msb,lsb,msbh,msbl,lsbh,lsbl);
                
            fprintf(fout,"%04X  %02X %02X    ",fp,msb,lsb);

            switch (msbh) {     /* Decode the Instruction Switch */
 
              case 0x0:  /* 0 - OpCode */
                  if (lsb==0xE0) { 
                      fprintf(fout,"CLS                  ' Clear Screen"); /* Clear Screen */
                    }
                    else {
                      if (lsb==0xEE) { 
                        fprintf(fout,"RET                  ' Return"); /* Return from Subroutine */
                      }
                      else { 
                        if (0 != (msbl+lsb)) {
                            fprintf(fout,"JMP   0x%1X%02X          ' JMP to Address",msbl,lsb); /* Jump to ADDRESS */
                            }   
                            else { 
                                fprintf(fout,"NOP                  ' No Operation"); /* No Operation */                   
                                }
                            }
                    }
                      
              break; /* end, 0 - OpCode */
              
              case 0x1:  /* 1 - OpCode */
               fprintf(fout,"JMP   0x%1X%02X          ' JMP to Address",msbl,lsb); /* Jump to ADDRESS */
                 
              break;  /* end, 1 - OpCode */
              
              case 0x2:  /* 2 - OpCode */
               fprintf(fout,"CALL  0x%1X%02X          ' Call Subroutine",msbl,lsb); /* Call Subroutine */
                 
              break;  /* end, 2 - OpCode */

              case 0x3:  /* 3 - OpCode */
               fprintf(fout,"SE    V%1X,0x%02X        ' Skip Next OP if V%1X=0x%02X",msbl,lsb,msbl,lsb);   
              break;  /* end, 3 - OpCode */
              
              case 0x4:  /* 4 - OpCode */
                fprintf(fout,"SNE   V%1X,0x%02X        ' Skip Next OP if V%1X!=0x%02X",msbl,lsb,msbl,lsb);   
                 
              break;  /* end, 4 - OpCode */
         
              case 0x5:  /* 5 - OpCode */
                fprintf(fout,"SE    V%1X,V%1X          ' Skip Next OP if V%1X=V%1X",msbl,lsbh,msbl,lsbh);   
                  
              break;  /* end, 5 - OpCode */

              case 0x6:  /* 6 - OpCode */
                fprintf(fout,"LD    V%1X,0x%02X        ' Load V%1X with 0x%1X",msbl,lsb,msbl,lsb);   
                  
              break;  /* end, 6 - OpCode */
         
              case 0x7:  /* 7 - OpCode */
                 fprintf(fout,"ADD   V%1X,0x%02X        ' Set V%1X = V%1X + 0x%02X",msbl,lsb,msbl,msbl,lsb);   
                 
              break;  /* end, 7 - OpCode */
         
              case 0x8:  /* 8 - OpCode */
                if (lsbl==0) fprintf(fout,"LD    V%1X,V%1X          ' Load V%1X with V%1X",msbl,lsbh,msbl,lsbh);   
                if (lsbl==1) fprintf(fout,"OR    V%1X,V%1X          ' V%1X = V%1X OR V%1X",msbl,lsbh,msbl,msbl,lsbh);   
                if (lsbl==2) fprintf(fout,"AND   V%1X,V%1X          ' V%1X = V%1X AND V%1X",msbl,lsbh,msbl,msbl,lsbh);   
                if (lsbl==3) fprintf(fout,"XOR   V%1X,V%1X          ' V%1X = V%1X XOR V%1X",msbl,lsbh,msbl,msbl,lsbh);   
                if (lsbl==4) fprintf(fout,"ADD   V%1X,V%1X          ' V%1X = V%1X + V%1X, VF=1 for Carry",msbl,lsbh,msbl,msbl,lsbh);   
                if (lsbl==5) fprintf(fout,"SUB   V%1X,V%1X          ' V%1X = V%1X - V%1X, VF=1 for V%1X>V%1X",msbl,lsbh,msbl,msbl,lsbh,msbl,lsbh);   
                if (lsbl==6) fprintf(fout,"SHR   V%1X,V%1X          ' V%1X = V%1X/2, VF=1 if V%1X bit0=1",msbl,lsbh,msbl,msbl,msbl);   
                if (lsbl==7) fprintf(fout,"SUBN  V%1X,V%1X          ' V%1X = V%1X - V%1X, VF=1 for V%1X>V%1X",msbl,lsbh,msbl,lsbh,msbl,lsbh,msbl);   
                if (lsbl==0xE) fprintf(fout,"SHL   V%1X,V%1X          ' V%1X = V%1X*2, VF=1 if V%1X bit7=1",msbl,lsbh,msbl,msbl,msbl);   
                                         
              break;  /* end, 8 - OpCode */
         
              case 0x9:  /* 9 - OpCode */
                fprintf(fout,"SNE   V%1X,V%1X          ' Skip Next OP if V%1X!=V%1X",msbl,lsbh,msbl,lsbh);   
                  
              break;  /* end, 9 - OpCode */
         
              case 0xA:  /* A - OpCode */
                 
                 fprintf(fout,"LD    I,0x%03X        ' Load I with 0x%3X",lsb+(msbl << 8),lsb+(msbl << 8));   
                  
              break;  /* end, A - OpCode */
         
              case 0xB:  /* B - OpCode */
                 fprintf(fout,"JP    V0,0x%03X       ' Jump to Address (V0)+0x%03X",lsb+(msbl << 8),lsb+(msbl << 8)); /* Jump to ADDRESS */
                  
              break;  /* end, B - OpCode */
         
              case 0xC:  /* C - OpCode */
                 fprintf(fout,"RND   V%1X,0x%02X        ' Set V%1X = (RND) AND 0x%02X",msbl,lsb,msbl,lsb);   
                  
              break;  /* end, C - OpCode */
         
              case 0xD:  /* D - OpCode */
                fprintf(fout,"DRW   V%1X,V%1X,0x%1x      ' Display 0x%1X Sprite(s) from (I) at V%1X,V%1X",msbl,lsbh,lsbl,lsbl,msbl,lsbh);   
                  
              break;  /* end, D - OpCode */
         
              case 0xE:  /* E - OpCode */
               if (lsb == 0x9E) fprintf(fout,"SKP   V%1X             ' Skip Next OP if (V%1X) = (KEY) Down",msbl,msbl);   
               if (lsb == 0xA1) fprintf(fout,"SKNP  V%1X             ' Skip Next OP if (V%1X) = (KEY) UP",msbl,msbl);   
                  
              break;  /* end, E - OpCode */
         
              case 0xF:  /* F - OpCode */
                  
              break;  /* end, F - OpCode */
         

              default: 
                
            } /* end, Decode the Instruction Switch */
            fprintf(fout,"\r\n");  /* Terminate output line */
            fp+=2;  /* Increment Offset Index */    
            } /* end, if not EOF get LSB of instruction */  
        } /* end, Process input Binary */
    } /* end, go to start of Disassembly */
     else { /* Couldn't Seek to Offset */
          printf("File Offset I/O error: %s\n",argv[1]);
          } /* end, Couldn't Seek to Offset */

fprintf(fout,"\r\nProcessing Complete.\r\n");
    
fclose(fin);
fclose(fout);
return(argc); /* Leave */
    
}
