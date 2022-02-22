//
//  main.cpp
//  SLIC test
//
//  Created by Larry Bank on 2/14/22.
//  Demonstrates the SLIC library
//  by generating a compressed image dynamically
//
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/slic.h"
SLIC slic; // static instance of class

int main(int argc, const char * argv[]) {
    int rc;
    int iOutSize, iBufferSize;
    FILE *ohandle;
    uint8_t *pOutput;
    uint16_t usLine[128]; // temp line
    int iWidth, iHeight, iBpp, iPitch;
    
    if (argc != 2) {
       printf("SLIC demo program\n");
       printf("Usage: slic_demo <outfile.slc>\n");
       return 0;
    }
    iWidth = iHeight = 128;
    iPitch = iWidth*2;
    iBpp = 16;
    iBufferSize = iWidth*iHeight*2;
    pOutput = (uint8_t *)malloc(iBufferSize);
    // Initialize the encoder
    rc = slic.init_encode_ram(iWidth, iHeight, iBpp, NULL, pOutput, iBufferSize);
    if (rc == SLIC_SUCCESS) {
        for (int y=0; y<iHeight && rc == SLIC_SUCCESS; y++) {
            if (y==0 || y == iHeight-1) {
                for (int x=0; x<iWidth; x++) { // top+bottom red lines
                    usLine[x] = 0xf800; // pure red
                } // for x
            } else {
                memset(usLine, 0, iWidth*sizeof(uint16_t)); // black background
                usLine[0] = usLine[iWidth-1] = 0xf800; // left/right border = red
                usLine[y] = usLine[iWidth-1-y] = 0x1f; // blue X in the middle
            }
            rc = slic.encode((uint8_t *)usLine, iWidth); // add each line 1 at a time
        } // for y
        iOutSize = slic.get_output_size();
        printf("32768 bytes of image compressed to %d bytes of slic output\n", iOutSize);
        ohandle = fopen(argv[1], "w+b");
        if (ohandle != NULL) {
            fwrite(pOutput, 1, iOutSize, ohandle);
            fclose(ohandle);
        }
    } else {
        printf("init_encode() returned error %d\n", rc);
    }
    free(pOutput);
    return 0;
} /* main() */
