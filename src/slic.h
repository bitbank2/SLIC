//
// SLIC - Simple Lossless Image Code
//
// Lossless compression starting point based on QOI by Dominic Szablewski - https://phoboslab.org
//
// written by Larry Bank
// bitbank@pobox.com
// Project started 2/11/2022
//
// Copyright 2022 BitBank Software, Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===========================================================================

#ifndef __SLIC__
#define __SLIC__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__x86__) || defined(__x86_64__) || defined(__aarch64__)
#define UNALIGNED_ALLOWED
#endif

/* A pointer to a slic_header struct has to be supplied to all of qoi's functions.
It describes either the input format (for slic_write and slic_encode), or is
filled with the description read from the file header (for slic_read and
slic_decode).

The colorspace in this slic_header is an enum where
	0 = sRGB, i.e. gamma scaled RGB channels and a linear alpha channel
	1 = all channels are linear
You may use the constants SLIC_SRGB or SLIC_LINEAR. The colorspace is purely
informative. It will be saved to the file header, but does not affect
how chunks are en-/decoded. */

enum {
 SLIC_SRGB = 0,
 SLIC_LINEAR,
// additional colorspace types for images with 1 channel
 SLIC_GRAYSCALE,
 SLIC_PALETTE,
 SLIC_RGB565,
 SLIC_COLORSPACE_COUNT
};

typedef struct pal_tag {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} slic_palette_entry;

typedef struct header_tag {
    uint32_t magic;
	uint16_t width;
	uint16_t height;
	uint8_t bpp;
	uint8_t colorspace;
} slic_header;

#define SLIC_HEADER_SIZE 10

typedef struct slic_file_tag
{
  int32_t iPos; // current file position
  int32_t iSize; // file size
  uint8_t *pData; // memory file pointer
  void * fHandle; // class pointer to File/SdFat or whatever you want
} SLICFILE;
//
// Define a small buffer to cache incoming and outgoing data
// on AVR make it tiny since there's not much RAM to work with
//
#ifdef __AVR__
#define FILE_BUF_SIZE 128
#else
#define FILE_BUF_SIZE 1024
#endif

typedef int (SLIC_READ_CALLBACK)(SLICFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int (SLIC_WRITE_CALLBACK)(SLICFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int (SLIC_OPEN_CALLBACK)(const char *filename, SLICFILE *pFile);

typedef struct state_tag {
    int32_t run; // number of consecutive identical pixels
    int32_t bad_run; // number of consecutive uncompressible pixels
    uint16_t width, height;
    int32_t iOffset; // input or output data offset
    uint8_t bpp, colorspace, extra_pixel, prev_op;
    uint8_t *pOutBuffer; // start of output buffer
    uint8_t *pOutPtr; // current output pointer
    uint8_t *pInPtr; // current input pointer
    uint8_t *pInEnd;
    uint32_t curr_pixel, prev_pixel;
    int32_t iPixelCount;
    int32_t iOutSize; // output buffer size
    SLIC_READ_CALLBACK *pfnRead;
    SLIC_WRITE_CALLBACK *pfnWrite;
    uint32_t index[64];
    SLICFILE file;
    uint8_t ucFileBuf[FILE_BUF_SIZE];
} SLICSTATE;

int slic_init_encode(const char *filename, SLICSTATE *pState, uint16_t iWidth, uint16_t iHeight, int iBpp, uint8_t *pPalette, SLIC_OPEN_CALLBACK *pfnOpen, SLIC_WRITE_CALLBACK *pfnWrite, uint8_t *pOut, int iOutSize);
int slic_encode(SLICSTATE *pState, uint8_t *pPixels, int iPixelCount);

int slic_init_decode(const char *filename, SLICSTATE *pState, uint8_t *pData, int iDataSize, uint8_t *pPalette, SLIC_OPEN_CALLBACK *pfnOpen, SLIC_READ_CALLBACK *pfnRead);
int slic_decode(SLICSTATE *pState, uint8_t *pOut, int iOutSize);

#ifdef __cplusplus
}
#endif

#define SLIC_OP_INDEX   0x00 /* 00xxxxxx */
#define SLIC_OP_DIFF    0x40 /* 01xxxxxx */
#define SLIC_OP_LUMA    0x80 /* 10xxxxxx */
#define SLIC_OP_RUN     0xc0 /* 11xxxxxx */
#define SLIC_OP_RUN256  0xfc /* 11111100 */
#define SLIC_OP_RUN1024 0xfd /* 11111101 */
#define SLIC_OP_RGB     0xfe /* 11111110 */
#define SLIC_OP_RGBA    0xff /* 11111111 */

// gray / palette ops
#define SLIC_OP_RUN8      0x00
#define SLIC_OP_RUN8_256  0x3F
#define SLIC_OP_RUN8_1024 0x3E
#define SLIC_OP_BADRUN8   0x40
#define SLIC_OP_DIFF8     0x80
#define SLIC_OP_INDEX8    0xc0

#define SLIC_GRAY_HASH(C) (((C * 1) + ((C >> 4) * 6)) & 0x7)

// RGB565 ops
#define SLIC_OP_RUN16      0x00
#define SLIC_OP_RUN16_256  0x3F
#define SLIC_OP_RUN16_1024 0x3E
#define SLIC_OP_BADRUN16   0x40
#define SLIC_OP_DIFF16     0x80
#define SLIC_OP_INDEX16    0xc0

#define SLIC_RGB565_HASH(C) ((((C & 0x1f) * 1) + (((C >> 5) & 0x3f) * 6) + ((C >> 11) * 12)) & 0x7)

#define SLIC_OP_MASK    0xc0 /* 11000000 */

// SLIC_MAGIC = "SLIC"
#define SLIC_MAGIC 0x43494C53

enum {
    SLIC_SUCCESS = 0,
    SLIC_DONE,
    SLIC_INVALID_PARAM,
    SLIC_BAD_FILE,
    SLIC_DECODE_ERROR,
    SLIC_IO_ERROR,
    SLIC_ENCODE_OVERFLOW
};

#ifdef __cplusplus
//
// The SLIC class wraps portable C code which does the actual work
//
class SLIC
{
  public:
    int init_encode_ram(uint16_t iWidth, uint16_t iHeight, int iBpp, uint8_t *pPalette, uint8_t *pOut, int iOutSize);
    int init_encode(const char *filename, uint16_t iWidth, uint16_t iHeight, int iBpp, uint8_t *pPalette, SLIC_OPEN_CALLBACK *pfnOpen, SLIC_WRITE_CALLBACK *pfnWrite);
    int encode(uint8_t *pPixels, int iPixelCount);

    int init_decode_ram(uint8_t *pData, int iDataSize, uint8_t *pPalette);
    int init_decode_flash(uint8_t *pData, int iDataSize, uint8_t *pPalette);
    int init_decode(const char *filename, uint8_t *pPalette, SLIC_OPEN_CALLBACK *pfnOpen, SLIC_READ_CALLBACK *pfnRead);
    int decode(uint8_t *pOut, int iOutSize);
    int get_width();
    int get_height();
    int get_bpp();
    int get_colorspace();
    int get_output_size();
private:
  SLICSTATE _slic;
};
#endif // c++

#endif // __SLIC__
