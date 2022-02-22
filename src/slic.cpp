//
// SLIC - C++ wrapper class
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

#include <stdint.h>
#include <string.h>

#include "slic.h"
// include the C code which does the actual work
#include "slic.inl"

int SLIC::init_encode_ram(uint16_t iWidth, uint16_t iHeight, int iBpp, uint8_t *pPalette, uint8_t *pOut, int iOutSize)
{
    return slic_init_encode(NULL, &_slic, iWidth, iHeight, iBpp, pPalette, NULL, NULL, pOut, iOutSize);
} /* init_encode() */

int SLIC::init_encode(const char *filename, uint16_t iWidth, uint16_t iHeight, int iBpp, uint8_t *pPalette, SLIC_OPEN_CALLBACK *pfnOpen, SLIC_WRITE_CALLBACK *pfnWrite)
{
    return slic_init_encode(filename, &_slic, iWidth, iHeight, iBpp, pPalette, pfnOpen, pfnWrite, NULL, 0);
} /* init_encode() */

int SLIC::encode(uint8_t *pPixels, int iPixelCount)
{
    return slic_encode(&_slic, pPixels, iPixelCount);
} /* encode() */

int SLIC::init_decode_ram(uint8_t *pData, int iDataSize, uint8_t *pPalette)
{
    return slic_init_decode(NULL, &_slic, pData, iDataSize, pPalette, NULL, NULL);
} /* init_decode_ram() */

int SLIC::init_decode_flash(uint8_t *pData, int iDataSize, uint8_t *pPalette)
{
    return slic_init_decode(NULL, &_slic, pData, iDataSize, pPalette, NULL, slic_flash_read);
} /* init_decode_flash() */

int SLIC::init_decode(const char *filename, uint8_t *pPalette, SLIC_OPEN_CALLBACK *pfnOpen, SLIC_READ_CALLBACK *pfnRead)
{
    return slic_init_decode(filename, &_slic, NULL, 0, pPalette, pfnOpen, pfnRead);
} /* init_decode() */

int SLIC::decode(uint8_t *pOut, int iOutSize)
{
    return slic_decode(&_slic, pOut, iOutSize);
} /* decode() */

int SLIC::get_output_size()
{
    return _slic.iOffset;
} /* get_output_size() */

int SLIC::get_width()
{
    return _slic.width;
} /* get_width() */

int SLIC::get_height()
{
    return _slic.height;
} /* get_height() */

int SLIC::get_bpp()
{
    return _slic.bpp;
} /* get_bpp() */

int SLIC::get_colorspace()
{
    return _slic.colorspace;
} /* get_colorspace() */

