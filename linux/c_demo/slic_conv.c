#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../../src/slic.h"
#include "../../src/slic.inl"

/* Windows BMP header for RGB565 images */
uint8_t winbmphdr_rgb565[138] =
        {0x42,0x4d,0,0,0,0,0,0,0,0,0x8a,0,0,0,0x7c,0,
         0,0,0,0,0,0,0,0,0,0,1,0,8,0,3,0,
         0,0,0,0,0,0,0x13,0x0b,0,0,0x13,0x0b,0,0,0,0,
         0,0,0,0,0,0,0,0xf8,0,0,0xe0,0x07,0,0,0x1f,0,
         0,0,0,0,0,0,0x42,0x47,0x52,0x73,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0};

/* Windows BMP header for 8/24/32-bit images (54 bytes) */
uint8_t winbmphdr[54] =
        {0x42,0x4d,
         0,0,0,0,         /* File size */
         0,0,0,0,0x36,4,0,0,0x28,0,0,0,
         0,0,0,0, /* Xsize */
         0,0,0,0, /* Ysize */
         1,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       /* number of planes, bits per pel */
         0,0,0,0};

//
// RGB565 image conversion test program
//
//
// Minimal code to save frames as Windows BMP files
//
void WriteBMP(char *fname, uint8_t *pBitmap, uint8_t *pPalette, int cx, int cy, int bpp)
{
FILE * oHandle;
int i, bsize, lsize;
uint32_t *l;
uint8_t *s;
uint8_t ucTemp[1024];
uint8_t *pHdr;
int iHeaderSize;

    if (bpp == 16) {
        pHdr = winbmphdr_rgb565;
        iHeaderSize = sizeof(winbmphdr_rgb565);
    } else {
        pHdr = winbmphdr;
        iHeaderSize = sizeof(winbmphdr);
    }
    
    oHandle = fopen(fname, "w+b");
    bsize = (cx * bpp) >> 3;
    lsize = (bsize + 3) & 0xfffc; /* Width of each line */
    pHdr[26] = 1; // number of planes
    pHdr[28] = (uint8_t)bpp;

   /* Write the BMP header */
   l = (uint32_t *)&pHdr[2];
    i =(cy * lsize) + iHeaderSize;
    if (bpp <= 8)
        i += 1024;
   *l = (uint32_t)i; /* Store the file size */
   l = (uint32_t *)&pHdr[34]; // data size
   i = (cy * lsize);
   *l = (uint32_t)i; // store data size
   l = (uint32_t *)&pHdr[18];
   *l = (uint32_t)cx;      /* width */
   *(l+1) = (uint32_t)cy;  /* height */
    if (bpp >= 24) { // non-palette write
        l = (uint32_t *)&pHdr[10]; // offset to image bits is less (no palette)
        *l = iHeaderSize;
    }
   fwrite(pHdr, 1, iHeaderSize, oHandle);
    if (bpp <= 8) {
        if (pPalette == NULL) {// create a grayscale palette
            int iDelta, iCount = 1<<bpp;
            int iGray = 0;
            iDelta = 255/(iCount-1);
            for (i=0; i<iCount; i++) {
                ucTemp[i*4+0] = (uint8_t)iGray;
                ucTemp[i*4+1] = (uint8_t)iGray;
                ucTemp[i*4+2] = (uint8_t)iGray;
                ucTemp[i*4+3] = 0;
                iGray += iDelta;
            }
        } else {
            for (i=0; i<256; i++) // change palette to WinBMP format
            {
                ucTemp[i*4 + 0] = pPalette[(i*3)+2];
                ucTemp[i*4 + 1] = pPalette[(i*3)+1];
                ucTemp[i*4 + 2] = pPalette[(i*3)+0];
                ucTemp[i*4 + 3] = 0;
            }
        }
        fwrite(ucTemp, 1, 1024, oHandle);
    }
   /* Write the image data */
   for (i=cy-1; i>=0; i--)
    {
        s = &pBitmap[i*bsize];
        if (bpp >= 24) { // swap R/B for Windows BMP byte order
            uint8_t ucTemp[2048];
            int j, iBpp = bpp/8;
            uint8_t *d = ucTemp;
            for (j=0; j<cx; j++) {
                d[0] = s[2]; d[1] = s[1]; d[2] = s[0];
                d += iBpp; s += iBpp;
            }
            fwrite(ucTemp, 1, (size_t)lsize, oHandle);
        } else {
            fwrite(s, 1, (size_t)lsize, oHandle);
        }
    }
   fclose(oHandle);
} /* WriteBMP() */

//
// Read a Windows BMP file into memory
// For this demo, the only supported files are 24 or 32-bits per pixel
//
uint8_t * ReadBMP(const char *fname, int *width, int *height, int *bpp, unsigned char *pPal)
{
    int y, w, h, bits, offset;
    uint8_t *s, *d, *pTemp, *pBitmap;
    int pitch, bytewidth;
    int iSize, iDelta, iColorsUsed;
    FILE *infile;
   
    infile = fopen(fname, "r+b");
    if (infile == NULL) {
        printf("Error opening input file %s\n", fname);
        return NULL;
    }
    // Read the bitmap into RAM
    fseek(infile, 0, SEEK_END);
    iSize = (int)ftell(infile);
    fseek(infile, 0, SEEK_SET);
    pBitmap = (uint8_t *)malloc(iSize);
    pTemp = (uint8_t *)malloc(iSize);
    fread(pTemp, 1, iSize, infile);
    fclose(infile);
   
    if (pTemp[0] != 'B' || pTemp[1] != 'M' || pTemp[14] < 0x28) {
        free(pBitmap);
        free(pTemp);
        printf("Not a Windows BMP file!\n");
        return NULL;
    }
    iColorsUsed = pTemp[46];
    w = *(int32_t *)&pTemp[18];
    h = *(int32_t *)&pTemp[22];
    bits = *(int16_t *)&pTemp[26] * *(int16_t *)&pTemp[28];
    if (iColorsUsed == 0 || iColorsUsed > (1<<bits))
        iColorsUsed = (1 << bits); // full palette

    if (bits <= 8) { // it has a palette, copy it
        uint8_t *p = pPal;
        int iOff = pTemp[10] + (pTemp[11] << 8);
        iOff -= (iColorsUsed*4);
        for (int i=0; i<iColorsUsed; i++)
        {
           *p++ = pTemp[iOff+2+i*4];
           *p++ = pTemp[iOff+1+i*4];
           *p++ = pTemp[iOff+i*4];
        }
    }
    offset = *(int32_t *)&pTemp[10]; // offset to bits
    bytewidth = (w * bits) >> 3;
    pitch = (bytewidth + 3) & 0xfffc; // DWORD aligned
// move up the pixels
    d = pBitmap;
    s = &pTemp[offset];
    iDelta = pitch;
    if (h > 0) {
        iDelta = -pitch;
        s = &pTemp[offset + (h-1) * pitch];
    } else {
        h = -h;
    }
    for (y=0; y<h; y++) {
        if (bits == 32) {// need to swap red and blue
            for (int i=0; i<bytewidth; i+=4) {
                d[i] = s[i+2];
                d[i+1] = s[i+1];
                d[i+2] = s[i];
                d[i+3] = s[i+3];
            }
        } else {
            memcpy(d, s, bytewidth);
        }
        d += bytewidth;
        s += iDelta;
    }
    *width = w;
    *height = h;
    *bpp = bits;
    free(pTemp);
    return pBitmap;

} /* ReadBMP() */

//
// ReadFile
//
uint8_t * ReadFile(char *fname, int *pFileSize)
{
FILE *infile;
unsigned long ulSize;
uint8_t *pData;
   
    infile = fopen(fname, "rb");
    if (infile != NULL) {
      // get the file size
        fseek(infile, 0L, SEEK_END);
        ulSize = ftell(infile);
        fseek(infile, 0L, SEEK_SET);
        pData = (uint8_t *)malloc(ulSize);
        if (pData) {
            *pFileSize = (int)ulSize;
            fread(pData, 1, ulSize, infile);
        }
        fclose(infile);
        return pData;
    }
    return NULL; // failure to open
} /* ReadFile() */

int slic_read_fake(SLICFILE *pFile, uint8_t *pBuf, int iLen)
{
    if (pFile->iPos + iLen > pFile->iSize) {
        iLen = pFile->iSize - pFile->iPos;
        if (iLen <= 0)
            return 0; // past the end of file
    }
    memcpy(pBuf, &pFile->pData[pFile->iPos], iLen);
    pFile->iPos += iLen;
    return iLen;
} /* slic_read_fake() */

int main(int argc, const char * argv[]) {
    int i, rc, iOutIndex;
    int iDataSize;
    FILE *ohandle;
    uint8_t *pOutput;
    int iWidth, iHeight, iBpp, iPitch;
    uint8_t ucPalette[1024];
    uint8_t *pData, *pBitmap;
    SLICSTATE state;
   
    if (argc != 3 && argc != 2) {
       printf("SLIC codec library demo program\n");
       printf("Usage: slic_conv <infile> <outfile>\n");
       printf("\nor (to generate an image dynamically)\n       slic_conv <outfile.slc>\n");
        printf("The input and output files can be either WinBMP (*.bmp) or SLIC (*.slc)\n");
       return 0;
    }

    if (argc == 3) {
       iOutIndex = 2; // argv index of output filename
       i = (int)strlen(argv[1]);
       if (memcmp(&argv[1][i-4], ".slc", 4) == 0)  { // input is SLIC file
           pData = ReadFile((char *)argv[1], &iDataSize);
           if (pData != NULL) {
               rc = slic_init_decode(NULL, &state, pData, iDataSize, ucPalette, NULL, slic_read_fake);
               printf("decompressing a slic %d x %d x %d-bpp file\n", state.width, state.height, state.bpp);
               pBitmap = (uint8_t *)malloc(state.width * state.height * (state.bpp >> 3) + 8);
               // do it in small runs for testing Arduino code
               for (int y=0; y<state.height; y++) {
                   rc = slic_decode(&state, &pBitmap[y * state.width * (state.bpp >> 3)], state.width);
               } // for y
                if (rc == SLIC_SUCCESS || rc == SLIC_DONE) {
                    printf("success!\n");
                    WriteBMP((char *)argv[2], pBitmap, ucPalette, state.width, state.height, state.bpp);
                    free(pBitmap);
                    free(pData);
            } else {
                printf("slic_decode() returned %d\n", rc);
            }
          return 0;
           } else {
               printf("Error opening file %s\n", argv[1]);
               return -1;
           }
       }
       pBitmap = ReadBMP(argv[1], &iWidth, &iHeight, &iBpp, ucPalette);
       if (pBitmap == NULL)
       {
           fprintf(stderr, "Unable to open file: %s\n", argv[1]);
           return -1; // bad filename passed?
       }
        if (iBpp == 4) {
            // convert to 8-bpp to support this bitmap
            uint8_t uc, *s, *d, *pTemp = (uint8_t *)malloc(iWidth*iHeight);
            s = pBitmap; d = pTemp;
            for (int i=0; i<iWidth*iHeight; i+=2) {
                uc = *s++;
                *d++ = (uc >> 4);
                *d++ = uc & 0xf;
            }
            free(pBitmap);
            pBitmap = pTemp;
            iBpp = 8;
        }
        iPitch = (iWidth * iBpp) >> 3;
    } else { // create the bitmap in code
       iOutIndex = 1;  // argv index of output filename
       iWidth = iHeight = 128;
        iBpp = 16;
       iPitch = iWidth*sizeof(uint16_t); // create a RGB565 image
       pBitmap = (uint8_t *)malloc(128*128*sizeof(uint16_t));
       for (int y=0; y<iHeight; y++) {
           uint16_t *pLine = (uint16_t *)&pBitmap[iPitch*y];
           if (y==0 || y == iHeight-1) {
               for (int x=0; x<iWidth; x++) { // top+bottom red lines
                   pLine[x] = 0xffff; // pure white
               } // for x
           } else {
               for (int x=1; x<127; x++) pLine[x] = 0x1f; // blue background
               pLine[0] = pLine[iWidth-1] = 0xffff; // left/right border = white
               pLine[y] = pLine[iWidth-1-y] = 0x7e0; // green X in the middle
           }
       } // for y
    }
    pOutput = malloc(iWidth * iHeight); // output buffer
    iDataSize = iWidth * iHeight;
    rc = slic_init_encode(NULL, &state, iWidth, iHeight, iBpp, ucPalette, NULL, NULL, pOutput, iDataSize);
    printf("Compressing a %d x %d x %d bitmap as SLIC data\n", iWidth, iHeight, iBpp);
    // Encode one line at a time
    for (int y=0; y<iHeight && rc == SLIC_SUCCESS; y++) {
        rc = slic_encode(&state, &pBitmap[iPitch * y], iWidth);
    } // for y
    if (rc == SLIC_DONE) {
        iDataSize = state.iOffset;
        printf("SLIC image successfully created. %d bytes = %d:1 compression\n", iDataSize, ((iWidth*iHeight*iBpp)>>3) / iDataSize);
        ohandle = fopen(argv[iOutIndex], "w+b");
        if (ohandle != NULL) {
            fwrite(pOutput, 1, iDataSize, ohandle);
            fclose(ohandle);
        }
        free(pOutput);
    } else {
        printf("Something went wrong...slic_encode returned NULL\n");
    }
    free(pBitmap);
    return 0;
} /* main() */

