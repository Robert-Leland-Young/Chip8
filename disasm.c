#include "stdio.h"
#include "time.h"

int main(int argc, char **argv)
{
int i;                      /* Loop index */
unsigned int addr;          /* Disassembly address & Input File offset */
unsigned char msb,lsb;      /* Instruction High & Low order bytes */
unsigned char msbh,msbl;    /* Instruction MSB High & Low order Nibbles */
unsigned char lsbh,lsbl;    /* Instruction LSB High & Low order Nibbles */
FILE *fin;                  /* Input Chip8 Binary File */
FILE *fout;                 /* Output ASCII Disassembly Listing File */
int line;                   /* Listing line counter */    
int pagesize=30;            /* listing page size */
time_t now;                 /* Current Time structure */
    
    
if (argc<2) { /* Check Command Line Parameters */

    printf("\r\nDisassembler for Chip-8 & Chip-48 Binary Files\n\n");
    printf("Usage:\n");
    printf("%s input_file output_file begin_addr\n\n",argv[0]);
    
    printf("input_file  - Chip-8/48 Binary executable (required)\n");
    printf("output_file - Disassembly output ASCII filename (optional)\n");
    printf("begin_addr  - Hexadecimal offset at which to begin Disassembly.\n");
    printf("              i.e. The Load address. Optional, default is 0x200.\n"); 

    return(argc);  /* Leave */
    }

/* Set beginning disassembly Address */
    if (argc<4) { /* Default 0x200 */
        addr=0x200;
    }
    else {
       sscanf(argv[3],"%x",&addr); 
    }

/* Display Command Line Arguments */
    
    printf("%s Executing:\n",argv[0]);
    printf("Input Binary: %s \n",argv[1]);
    if (argc<3) { /* no output file */
      printf("Output: Screen\n");
        } /* end, no output file */
    else {
        printf("Output File: %s \n",argv[2]);
        }
    printf("Begin Offset: %0X\n",addr);
    
    
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

    line=1;                     /* initialize listing line counter */
    fprintf(fout,"\r\n                Chip-8 & Chip-48 Disassembler            \r\n\r\n"); /* Column header */
    /* Read Chip 8 Binary File */
        fprintf(fout,"Addr  Data     Mnemonic             Comment\r\n\r\n"); /* Column header */
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
            
            fprintf(fout,"%04X  %02X %02X    ",addr,msb,lsb);

            switch (msbh) {     /* Decode the Instruction Switch */
 
              case 0x0:  /* 0 - OpCode */
                  /* Handle SuperChip or Chip-48 Additions */
                  if (((lsbh == 0xC) || (lsb >= 0xFB)) && (msbl==0)) {
                    /* it's a Chip-48 Opcode */
                     if (lsbh==0xC) { /* 00CN Scroll Down */
                        fprintf(fout,"SCD   #%1X             ' Scroll Down #%1X Lines",lsbl,lsbl); 
                      } /* end, 00CN Scroll Down */
                     if (lsbh==0xF) { /* handle screen & EXIT Opcodes */
                        if (lsbl==0xB) fprintf(fout,"SCR                  ' Scroll 4 Pixels Right");
                        if (lsbl==0xC) fprintf(fout,"SCL                  ' Scroll 4 Pixels Left");
                        if (lsbl==0xD) fprintf(fout,"EXIT                 ' Exit Interpreter");
                        if (lsbl==0xE) fprintf(fout,"LOW                  ' Disable Extended Graphics");
                        if (lsbl==0xF) fprintf(fout,"HIGH                 ' Enable Extended Graphics");
                        
                      } /* end, handle screen & EXIT Opcodes */
                      break; /* leave 0 - Opcode for Chip-48 Opcodes */
                    } /* end, it's a Chip-48 Opcode */
                  if (lsb==0xE0) { /* if lsb=0xE0 */
                      fprintf(fout,"CLS                  ' Clear Screen"); /* Clear Screen */
                    } /* end if, lsb=0xE0 */
                    else { /* else if, lsb=0xEE */
                      if (lsb==0xEE) { 
                        fprintf(fout,"RET                  ' Return"); /* Return from Subroutine */
                      }
                      else { /* else if, 0!=(msb+lsb) */
                        if (0 != (msbl+lsb)) { /* JMP non-Zero */
                            fprintf(fout,"JMP   #%1X%02X           ' JMP to Address or Data",msbl,lsb); /* Jump to ADDRESS */
                            } /* end, JMP non-Zero */
                            else { /* else if JMP 0 or Data */
                                fprintf(fout,"JMP                  ' JMP 0000 or Data"); /* No Operation */                   
                                } /* end, else if JMP 0 or Data */
                            } /* end, else if, 0!=(msb+lsb) */
                    } /* end, else if, lsb=0xEE */
                      
              break; /* end, 0 - OpCode */
              
              case 0x1:  /* 1 - OpCode */
               fprintf(fout,"JMP   #%1X%02X           ' JMP to Address",msbl,lsb); /* Jump to ADDRESS */
                 
              break;  /* end, 1 - OpCode */
              
              case 0x2:  /* 2 - OpCode */
               fprintf(fout,"CALL  #%1X%02X           ' Call Subroutine",msbl,lsb); /* Call Subroutine */
                 
              break;  /* end, 2 - OpCode */

              case 0x3:  /* 3 - OpCode */
               fprintf(fout,"SE    V%1X,#%02X         ' Skip Next OP if V%1X=#%02X",msbl,lsb,msbl,lsb);   
              break;  /* end, 3 - OpCode */
              
              case 0x4:  /* 4 - OpCode */
                fprintf(fout,"SNE   V%1X,#%02X         ' Skip Next OP if V%1X!=#%02X",msbl,lsb,msbl,lsb);   
                 
              break;  /* end, 4 - OpCode */
         
              case 0x5:  /* 5 - OpCode */
                fprintf(fout,"SE    V%1X,V%1X          ' Skip Next OP if V%1X=V%1X",msbl,lsbh,msbl,lsbh);   
                  
              break;  /* end, 5 - OpCode */

              case 0x6:  /* 6 - OpCode */
                fprintf(fout,"LD    V%1X,#%02X         ' Load V%1X with #%1X",msbl,lsb,msbl,lsb);   
                  
              break;  /* end, 6 - OpCode */
         
              case 0x7:  /* 7 - OpCode */
                 fprintf(fout,"ADD   V%1X,#%02X         ' Set V%1X = V%1X + #%02X",msbl,lsb,msbl,msbl,lsb);   
                 
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
                 
                 fprintf(fout,"LD    I,#%03X         ' Load I with #%3X",lsb+(msbl << 8),lsb+(msbl << 8));   
                  
              break;  /* end, A - OpCode */
         
              case 0xB:  /* B - OpCode */
                 fprintf(fout,"JP    V0,#%03X        ' Jump to Address (V0)+#%03X",lsb+(msbl << 8),lsb+(msbl << 8)); /* Jump to ADDRESS */
                  
              break;  /* end, B - OpCode */
         
              case 0xC:  /* C - OpCode */
                 fprintf(fout,"RND   V%1X,#%02X         ' Set V%1X = (RND) AND #%02X",msbl,lsb,msbl,lsb);   
                  
              break;  /* end, C - OpCode */
         
              case 0xD:  /* D - OpCode */
                fprintf(fout,"DRW   V%1X,V%1X,#%1x       ' Display #%1X Sprite(s) from (I) at V%1X,V%1X",msbl,lsbh,lsbl,lsbl,msbl,lsbh);   
                  
              break;  /* end, D - OpCode */
         
              case 0xE:  /* E - OpCode */
               if (lsb == 0x9E) fprintf(fout,"SKP   V%1X             ' Skip Next OP if (V%1X) = (KEY) Down",msbl,msbl);   
               if (lsb == 0xA1) fprintf(fout,"SKNP  V%1X             ' Skip Next OP if (V%1X) = (KEY) UP",msbl,msbl);   
                  
              break;  /* end, E - OpCode */
         
              case 0xF:  /* F - OpCode */
               if (lsb == 0x7) fprintf(fout,"LD    V%1X,DT          ' V%1X = (DT) Get Delay Timer",msbl,msbl);   
               if (lsb == 0x0A) fprintf(fout,"LD    V%1X,K           ' V%1X = (KEY) Get Key Input",msbl,msbl);   
               if (lsb == 0x15) fprintf(fout,"LD    DT,V%1X          ' (DT) = V%1X  Set Delay Timer",msbl,msbl);   
               if (lsb == 0x18) fprintf(fout,"LD    ST,V%1X          ' (ST) = V%1X  Set Sound Timer",msbl,msbl);   
               if (lsb == 0x1E) fprintf(fout,"ADD   I,V%1X           ' Set I = I + V%1X",msbl,msbl);   
               if (lsb == 0x29) fprintf(fout,"LD    F,V%1X           ' Set I = Address of Sprite in V%1X",msbl,msbl);   
               if (lsb == 0x30) fprintf(fout,"LD    HF,V%1X          ' Set I = Address of 10 byte Sprite Digit in V%1X",msbl,msbl);   
               if (lsb == 0x33) fprintf(fout,"LD    B,V%1X           ' Store BCD of V%1X at (I) to (I+2)",msbl,msbl);   
               if (lsb == 0x55) fprintf(fout,"LD    [I],V%1X         ' Store V0 thru V%1X at (I)",msbl,msbl);   
               if (lsb == 0x65) fprintf(fout,"LD    V%1X,[I]         ' Read V0 thru V%1X From (I)",msbl,msbl);   
               if (msbl <=7 && lsb == 0x75) fprintf(fout,"LD    R,V%1X           ' Store V0-V%1X in RPL User Flags",msbl,msbl);   
               if (msbl <=7 && lsb == 0x85) fprintf(fout,"LD    V%1X,R           ' Read V0-V%1X from RPL User Flags",msbl,msbl);   

              break;  /* end, F - OpCode */
         

              default: 
                fprintf(fout,"                       ' Data ");   
                
            } /* end, Decode the Instruction Switch */
            fprintf(fout,"\r\n");  /* Terminate output line */
            addr+=2;  /* Increment Offset Index */    
            if (line++ >= pagesize && !feof(fin)) { /* display Page column header */
                line=1;
                fprintf(fout,"\r\nAddr  Data     Mnemonic             Comment\r\n\r\n"); /* Column header */
                }  /* end, display Page column header */
            } /* end, if not EOF get LSB of instruction */  
        } /* end, Process input Binary */

fprintf(fout,"\r\n\r\nEnd of File, Processing Complete.\r\n");
fprintf(fout,"\r\nInput Binary File: %s\r\n",argv[1]);
time(&now);  /* Get current time */
fprintf(fout,"\r\nTime: %24.24s\r\n",ctime(&now));

if (fout!=stdout) printf("End of File, Processing Complete.\r\n");
          
          
fclose(fin);
fclose(fout);
return(argc); /* Leave */
    
}
