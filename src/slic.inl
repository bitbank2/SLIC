//
// SLIC - Simple Lossless Image Code
//
// Lossless compression starting point based on QOI by Dominic Szablewski - https://phoboslab.org
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

#ifdef ARDUINO
#include <Arduino.h>
#endif

// Simple callback example for Harvard architecture FLASH memory access
int slic_flash_read(SLICFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
int iBytesRead = 0;
#ifdef PROGMEM
    if (iLen + pFile->iPos > pFile->iSize)
        iLen = pFile->iSize - pFile->iPos;
    if (iLen < 0)
        return 0;
    memcpy_P(pBuf, &pFile->pData[pFile->iPos], iLen);
    pFile->iPos += iLen;
    iBytesRead = iLen;
#endif // PROGMEM
    return iBytesRead;
} /* slic_flash_read() */

int slic_init_encode(const char *filename, SLICSTATE *pState, uint16_t iWidth, uint16_t iHeight, int iBpp, uint8_t *pPalette, SLIC_OPEN_CALLBACK *pfnOpen, SLIC_WRITE_CALLBACK *pfnWrite, uint8_t *pOut, int iOutSize) {
    slic_header hdr;
    int rc;
    
    if (pState == NULL || iWidth < 1 || iHeight < 1 || (iBpp != 8 && iBpp != 16 && iBpp != 24 && iBpp != 32)) {
        return SLIC_INVALID_PARAM;
    }
    if (pfnOpen || pfnWrite || filename) {
        if (!(pfnOpen && filename)) {
            return SLIC_INVALID_PARAM; // if one is defined, so must the other
        }
        rc = (*pfnOpen)(filename, &pState->file);
        if (rc != SLIC_SUCCESS)
            return rc;
    }
    pState->run = 0;
    pState->bad_run = 0;
    pState->extra_pixel = 0;
    pState->width = iWidth;
    pState->height = iHeight;
    pState->bpp = iBpp;
    pState->pfnWrite = pfnWrite;
    if (pfnWrite) {
        pState->pOutPtr = pState->ucFileBuf;
        pState->pOutBuffer = pState->ucFileBuf;
        pState->iOutSize = FILE_BUF_SIZE;
    } else {
        pState->pOutPtr = pOut;
        pState->pOutBuffer = pOut;
        pState->iOutSize = iOutSize;
    }
    pState->iOffset = 0;
    pState->prev_op = -1;
    pState->curr_pixel = pState->prev_pixel = 0xff000000;
    pState->iPixelCount = pState->width * pState->height;
    // encode the header in the output to start
    hdr.width = iWidth;
    hdr.height = iHeight;
    hdr.bpp = iBpp;
    hdr.magic = SLIC_MAGIC;
    if (iBpp >= 24)
        hdr.colorspace = SLIC_SRGB;
    else if (iBpp == 16)
        hdr.colorspace = SLIC_RGB565;
    else if (iBpp == 8 && pPalette == NULL)
        hdr.colorspace = SLIC_GRAYSCALE;
    else
        hdr.colorspace = SLIC_PALETTE;
    memcpy(pState->pOutPtr, &hdr, SLIC_HEADER_SIZE);
    pState->pOutPtr += SLIC_HEADER_SIZE;
    if (pPalette && iBpp == 8) {
        memcpy(pState->pOutPtr, pPalette, 768);
        pState->pOutPtr += 768;
    }
    // for file writing, init output data size var
    pState->iOffset = (int)(pState->pOutPtr - pState->pOutBuffer);
    return SLIC_SUCCESS;
} /* slic_init_encode() */
//
// Output buffer is full and we need to write it
//
static uint8_t * dump_encoded_data(SLICSTATE *pState, uint8_t *pOut)
{
int iLen;
    
    iLen = (int)(pOut - pState->pOutBuffer); // length of data to write
    (*pState->pfnWrite)(&pState->file, pState->pOutBuffer, iLen);
    pState->iOffset += iLen;
    return pState->ucFileBuf;
} /* dump_encoded_data() */
//
// Encode 1 or more pixels into the output stream
//
int slic_encode(SLICSTATE *pState, uint8_t *s, int iPixelCount) {
	int iBpp, run, bad_run, prev_op;
    uint8_t *d;
    const uint8_t *pEnd, *pDstEnd;
    uint32_t *index;
    uint32_t px, px_prev;
    uint16_t px16, px16_prev, px16_next;
    uint16_t *index16, *s16, *pEnd16;

	if (pState == NULL || s == NULL || iPixelCount < 1)
        return SLIC_INVALID_PARAM;
    run = pState->run;
    d = pState->pOutPtr;
    bad_run = pState->bad_run;
    px_prev = pState->prev_pixel;
    prev_op = pState->prev_op;
    px = pState->curr_pixel;
    index = pState->index;
    iBpp = pState->bpp >> 3;
    if (iPixelCount > pState->iPixelCount)
        iPixelCount = pState->iPixelCount;
    pState->iPixelCount -= iPixelCount;
    pEnd = &s[iPixelCount * iBpp];
    pDstEnd = &pState->pOutBuffer[pState->iOutSize-5]; // leave extra bytes for multi-byte ops
    
    if (iBpp == 1) { // grayscale or 8-bit palette image
        uint8_t px8, px8_prev, px8_next;
        uint8_t *index8 = (uint8_t *)pState->index;
        px8 = (uint8_t)pState->prev_pixel;
        px8_prev = (uint8_t)pState->curr_pixel;
        if (pState->extra_pixel) {
            pState->extra_pixel = 0;
            px8_next = s[0];
            goto restart_8bit; // try again
        }
        while (s < pEnd) {
            if (d >= pDstEnd) {
                if (pState->pfnWrite) {
                    d = dump_encoded_data(pState, d);
                    bad_run = 0; // can't update bad_run count once written
                } else {
                    return SLIC_ENCODE_OVERFLOW;
                }
            }
            px8 = *s++;
            px8_next = s[0];
            if (px8 == px8_prev) {
                run++;
                prev_op = SLIC_OP_RUN8;
            }
            else {
                int index_pos, index_next;
                while (run >= 1024) {
                    *d++ = SLIC_OP_RUN8_1024;
                    run -= 1024;
                }
                while (run >= 256) {
                    *d++ = SLIC_OP_RUN8_256;
                    run -= 256;
                }
                while (run >= 62) {
                    *d++ = SLIC_OP_RUN8 | 61;
                    run -= 62;
                }
                if (run > 0) {
                    *d++ = SLIC_OP_RUN8 | (run - 1);
                    run = 0;
                }
                if (s == pEnd && pState->iPixelCount != 0) {
                    // We're out of input on this run, but still have more pixels before the image is finished. Stop here and let it test this pair of pixels on the next call
                    pState->extra_pixel = 1;
                    goto exit_8bit; // save state and leave
                }
// Entry point to retry compressing the last pixel as a pair with the current
restart_8bit:
                index_pos = SLIC_GRAY_HASH(px8);
                index_next = SLIC_GRAY_HASH(px8_next);
                if (index8[index_pos] == px8 && index8[index_next] == px8_next && s < pEnd) {
                    // store the pair as indices
                    *d++ = SLIC_OP_INDEX8 | (index_pos | (index_next <<3));
                    s++; // count the next pixel too
                    px8 = px8_next; // skipped ahead 1 pixel
                    prev_op = SLIC_OP_INDEX8;
                } else { // try to do a pair of differences
                    int d0, d1;
                    
                    index8[index_pos] = px8;
                    d0 = px8 - px8_prev;
                    d1 = px8_next - px8;
                    if (d0 > -5 && d0 < 4 && d1 > -5 && d1 < 4) {
                        d0 += 4; d1 += 4;
                        *d++ = SLIC_OP_DIFF8 | (d0 | (d1 << 3));
                        index8[index_next] = px8_next; // we worked on a pair of pixels
                        s++; // count the next pixel too
                        px8 = px8_next; // skipped ahead 1 pixel
                        prev_op = SLIC_OP_DIFF8;
                    } else { // last resort - 'bad' pixels
                        if (prev_op == SLIC_OP_BADRUN8 && bad_run < 64) {
                            bad_run++; // add this bad pixel to an existing run
                            *d++ = px8;
                            d[-bad_run -1]++;
                        } else { // start a new run of bad pixels
                            *d++ = SLIC_OP_BADRUN8 | 0;
                            *d++ = px8;
                            bad_run = 1;
                            prev_op = SLIC_OP_BADRUN8;
                        }
                    }
                }
            }
            px8_prev = px8;
        } // for each pixel
        if (pState->iPixelCount == 0) { // wrap up last repeats
            while (run >= 1024) {
                *d++ = SLIC_OP_RUN8_1024;
                run -= 1024;
            }
            while (run >= 256) {
                *d++ = SLIC_OP_RUN8_256;
                run -= 256;
            }
            while (run >= 62) {
                *d++ = SLIC_OP_RUN8 | 61;
                run -= 61;
            }
            if (run > 0) {
                *d++ = SLIC_OP_RUN8 | (run - 1);
                run = 0;
            }
            // If using a write callback, flush the last of the data
            if (pState->pfnWrite) {
                (*pState->pfnWrite)(&pState->file, pState->pOutBuffer, (int)(d - pState->pOutBuffer));
            } else  {
                pState->iOffset = (int)(d - pState->pOutBuffer);
            }
        }
        // save state
exit_8bit:
        pState->curr_pixel = px8;
        pState->prev_pixel = px8_prev;
        pState->pOutPtr = d;
        pState->run = run;
        pState->bad_run = bad_run;
        pState->prev_op = prev_op;
        return (pState->iPixelCount == 0) ? SLIC_DONE : SLIC_SUCCESS;
    } // grayscale (channels == 1)
    
    else if (iBpp == 2) { // RGB565
        index16 = (uint16_t *)pState->index;
        s16 = (uint16_t *)s;
        pEnd16 = (uint16_t *)pEnd;
        px16 = (uint16_t)pState->curr_pixel;
        px16_prev = (uint16_t)pState->prev_pixel;
        if (pState->extra_pixel) {
            pState->extra_pixel = 0;
            px16_next = s16[0];
            goto restart_rgb565; // try again
        }
        while (s16 < pEnd16) {
            if (d >= pDstEnd) {
                if (pState->pfnWrite) {
                    d = dump_encoded_data(pState, d);
                    bad_run = 0; // can't update bad_run count once written
                } else {
                    return SLIC_ENCODE_OVERFLOW;
                }
            }
            px16 = *s16++;
            px16_next = s16[0];
            if (px16 == px16_prev) {
                run++;
                prev_op = SLIC_OP_RUN16;
            }
            else {
                int index_pos, index_next;
                while (run >= 1024) {
                    *d++ = SLIC_OP_RUN16_1024;
                    run -= 1024;
                }
                while (run >= 256) {
                    *d++ = SLIC_OP_RUN16_256;
                    run -= 256;
                }
                while (run >= 62) {
                    *d++ = SLIC_OP_RUN16 | 61;
                    run -= 62;
                }
                if (run > 0) {
                    *d++ = SLIC_OP_RUN16 | (run - 1);
                    run = 0;
                }
                if (s16 == pEnd16 && pState->iPixelCount != 0) {
                    // We're out of input on this run, but still have more pixels before the image is finished. Stop here and let it test this pair of pixels on the next call
                    pState->extra_pixel = 1;
                    goto exit_rgb565; // save state and leave
                }
// Entry point to retry compressing the last pixel as a pair with the current
restart_rgb565:
                index_pos = SLIC_RGB565_HASH(px16);
                index_next = SLIC_RGB565_HASH(px16_next);
                if (index16[index_pos] == px16 && index16[index_next] == px16_next && s16 < pEnd16) {
                    // store the pair as indices (if we can access the second pixel)
                    *d++ = SLIC_OP_INDEX16 | (index_pos | (index_next <<3));
                    s16++; // count the next pixel too
                    px16 = px16_next; // skipped ahead 1 pixel
                    prev_op = SLIC_OP_INDEX16;
                } else { // try to do a difference from prev pixel
                    int dr, dg, db;
                    
                    index16[index_pos] = px16;
                    dr = (px16 >> 11) - (px16_prev >> 11);
                    dg = ((px16 >> 5) & 0x3f) - ((px16_prev >> 5) & 0x3f);
                    db = (px16 & 0x1f) - (px16_prev & 0x1f);
                    if (dr > -3 && dr < 2 && dg > -3 && dg < 2 && db > -3 && db < 2) {
                        *d++ = SLIC_OP_DIFF16 | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2);
                        prev_op = SLIC_OP_DIFF16;
                    } else { // last resort - 'bad' pixels
                        if (prev_op == SLIC_OP_BADRUN16 && bad_run < 64) {
                            bad_run++; // add this bad pixel to an existing run
                            *d++ = (uint8_t)px16;
                            *d++ = (uint8_t)(px16 >> 8);
                            d[-1-(bad_run*2)]++;
                        } else { // start a new run of bad pixels
                            *d++ = SLIC_OP_BADRUN16 | 0;
                            *d++ = (uint8_t)px16;
                            *d++ = (uint8_t)(px16 >> 8);
                            bad_run = 1;
                            prev_op = SLIC_OP_BADRUN16;
                        }
                    }
                }
            }
            px16_prev = px16;
        } // for each pixel
        if (pState->iPixelCount == 0) { // wrap up any remaining repeats
            while (run >= 1024) {
                *d++ = SLIC_OP_RUN16_1024;
                run -= 1024;
            }
            while (run >= 256) {
                *d++ = SLIC_OP_RUN16_256;
                run -= 256;
            }
            while (run >= 62) {
                *d++ = SLIC_OP_RUN16 | 61;
                run -= 62;
            }
            if (run > 0) {
                *d++ = SLIC_OP_RUN16 | (run - 1);
                run = 0;
            }
            // If using a write callback, flush the last of the data
            if (pState->pfnWrite) {
                (*pState->pfnWrite)(&pState->file, pState->pOutBuffer, (int)(d - pState->pOutBuffer));
            } else  {
                pState->iOffset = (int)(d - pState->pOutBuffer);
            }
        }
        // save state
exit_rgb565:
        pState->curr_pixel = px16;
        pState->prev_pixel = px16_prev;
        pState->pOutPtr = d;
        pState->run = run;
        pState->bad_run = bad_run;
        pState->prev_op = prev_op;
        return (pState->iPixelCount == 0) ? SLIC_DONE : SLIC_SUCCESS;
    } // RGB565 (channels == 2)

    else {
        for (; s < pEnd; s += iBpp) { // RGB & RGBA
            if (d >= pDstEnd) {
                if (pState->pfnWrite) {
                    d = dump_encoded_data(pState, d);
                    bad_run = 0; // can't update bad_run count once written
                } else {
                    return SLIC_ENCODE_OVERFLOW;
                }
            }
#ifdef UNALIGNED_ALLOWED
            px = *(uint32_t *)s;
#else
            px = (uint32_t)s[0] | ((uint32_t)s[1] << 8) | ((uint32_t)s[2] << 16) | ((uint32_t)s[3] << 24);
#endif
            if (iBpp == 3) px |= 0xff000000;
            if (px == px_prev) {
                run++;
            }
            else {
                int index_pos;
                while (run >= 1024) {
                    *d++ = SLIC_OP_RUN1024;
                    run -= 1024;
                }
                while (run >= 256) {
                    *d++ = SLIC_OP_RUN256;
                    run -= 256;
                }
                while (run >= 60) {
                    *d++ = SLIC_OP_RUN | 59;
                    run -= 60;
                }
                if (run > 0) {
                    *d++ = SLIC_OP_RUN | (run - 1);
                    run = 0;
                }
                index_pos = px * 3;
                index_pos += ((px >> 8) * 5);
                index_pos += ((px >> 16) * 7);
                index_pos += ((px >> 24) * 11);
                index_pos &= 63;

                if (index[index_pos] == px) {
                    *d++ = SLIC_OP_INDEX | index_pos;
                }
                else {
                    index[index_pos] = px;
                    if ((px & 0xff000000) == (px_prev & 0xff000000)) {
                        signed char vr = (uint8_t)px - (uint8_t)px_prev; //px.rgba.r - px_prev.rgba.r;
                        signed char vg = (uint8_t)(px >> 8) - (uint8_t)(px_prev >> 8); //px.rgba.g - px_prev.rgba.g;
                        signed char vb = (uint8_t)(px >> 16) - (uint8_t)(px_prev >> 16); //px.rgba.b - px_prev.rgba.b;

                        signed char vg_r = vr - vg;
                        signed char vg_b = vb - vg;

                        if (
                            vr > -3 && vr < 2 &&
                            vg > -3 && vg < 2 &&
                            vb > -3 && vb < 2
                        ) {
                            *d++ = SLIC_OP_DIFF | (vr + 2) << 4 | (vg + 2) << 2 | (vb + 2);
                        }
                        else if (
                            vg_r >  -9 && vg_r <  8 &&
                            vg   > -33 && vg   < 32 &&
                            vg_b >  -9 && vg_b <  8
                        ) {
                            *d++ = SLIC_OP_LUMA     | (vg   + 32);
                            *d++ = (vg_r + 8) << 4 | (vg_b +  8);
                        }
                        else {
                            *d++ = SLIC_OP_RGB;
#ifdef UNALIGNED_ALLOWED
                            *(uint32_t *)d = px;
                            d += 3;
#else
                            *d++ = (uint8_t)px;
                            *d++ = (uint8_t)(px >> 8);
                            *d++ = (uint8_t)(px >> 16);
#endif
                        }
                    }
                    else {
                        *d++ = SLIC_OP_RGBA;
#ifdef UNALIGNED_ALLOWED
                        *(uint32_t *)d = px;
                        d += 4;
#else
                        *d++ = (uint8_t)px;
                        *d++ = (uint8_t)(px >> 8);
                        *d++ = (uint8_t)(px >> 16);
                        *d++ = (uint8_t)(px >> 24);
#endif
                    }
                }
            }
            px_prev = px;
        }
        if (pState->iPixelCount == 0) { // clean up any remaining repeats
            while (run >= 62) { // store pending run
                *d++ = SLIC_OP_RUN | 61;
                run -= 62;
            }
            if (run > 0) { // save pending run
                *d++ = SLIC_OP_RUN | (run - 1);
            }
            // If using a write callback, flush the last of the data
            if (pState->pfnWrite) {
                (*pState->pfnWrite)(&pState->file, pState->pOutBuffer, (int)(d - pState->pOutBuffer));
            } else  {
                pState->iOffset = (int)(d - pState->pOutBuffer);
            }
        }
    } // RGB/RGBA
    // save state
    pState->curr_pixel = px;
    pState->prev_pixel = px_prev;
    pState->pOutPtr = d;
    pState->run = run;
    return (pState->iPixelCount == 0) ? SLIC_DONE : SLIC_SUCCESS;
} /* slic_encode() */

int slic_init_decode(const char *filename, SLICSTATE *pState, uint8_t *pData, int iDataSize, uint8_t *pPalette, SLIC_OPEN_CALLBACK *pfnOpen, SLIC_READ_CALLBACK *pfnRead) {
    slic_header hdr;
    int rc, i;
    uint8_t *s;

    if (pState == NULL) {
        return SLIC_INVALID_PARAM;
    }
    memset(pState, 0, sizeof(SLICSTATE));
    if (pfnOpen) {
        rc = (*pfnOpen)(filename, &pState->file);
        if (rc != SLIC_SUCCESS)
            return rc;
    }
    // Set up the SLICFILE structure in case data will come from a file or FLASH
    pState->pfnRead = pfnRead;
    pState->file.pData = pData;
    pState->file.iPos = 0;
    pState->file.iSize = iDataSize;

    if (pfnRead) {
        i = (*pfnRead)(&pState->file, pState->ucFileBuf, FILE_BUF_SIZE);
        memcpy(&hdr, pState->ucFileBuf, SLIC_HEADER_SIZE);
        pState->pInPtr = &pState->ucFileBuf[SLIC_HEADER_SIZE];
        pState->pInEnd = &pState->ucFileBuf[i];
    } else { // memory to memory
        memcpy(&hdr, pData, SLIC_HEADER_SIZE);
        pState->pInPtr = pData + SLIC_HEADER_SIZE;
        pState->pInEnd = &pData[iDataSize];
    }
    if (hdr.magic == SLIC_MAGIC) {
        pState->width = hdr.width;
        pState->height = hdr.height;
        pState->curr_pixel = pState->prev_pixel = 0xff000000;
        pState->bpp = hdr.bpp;
        pState->colorspace = hdr.colorspace;
        if (pState->bpp != 8 && pState->bpp != 16 && pState->bpp != 24 && pState->bpp != 32)
            return SLIC_BAD_FILE; // invalid bits per pixel
        if (pState->colorspace >= SLIC_COLORSPACE_COUNT)
            return SLIC_BAD_FILE;
        if (pState->colorspace == SLIC_PALETTE) {
            // DEBUG - fix for file based
            if (pPalette) { // copy the palette if the user wants it
                memcpy(pPalette, pState->pInPtr, 768);
            }
            pState->pInPtr += 768; // fixed size palette
        }
        pState->iPixelCount = (uint32_t)pState->width * (uint32_t)pState->height;
    } else {
        return SLIC_BAD_FILE;
    }
    return SLIC_SUCCESS;
} /* slic_init_decode() */

//
// Read more data from the data source
// if none exists --> error
// returns 0 for success, 1 for error
//
static int get_more_data(SLICSTATE *pState)
{
int i;
    if (pState->pfnRead) { // read more data
        i = (*pState->pfnRead)(&pState->file, pState->ucFileBuf, FILE_BUF_SIZE);
        pState->pInEnd = &pState->ucFileBuf[i];
        return 0;
    }
    return 1;
} /* get_more_data() */

//
// Decode N pixels into the user-supplied output buffer
//
int slic_decode(SLICSTATE *pState, uint8_t *pOut, int iOutSize) {
	uint8_t op, px8, *s, *d;
    const uint8_t *pEnd, *pSrcEnd;
    int32_t iBpp;
    uint32_t px, *index;
	int32_t run, bad_run;
    uint16_t *d16, *pEnd16, px16, *index16;

    if (pState == NULL || pOut == NULL) {
        return SLIC_INVALID_PARAM;
	}
    iBpp = pState->bpp >> 3;
    index = pState->index;
    d = pOut;
    run = pState->run;
    bad_run = pState->bad_run;
    if (iOutSize > pState->iPixelCount)
        iOutSize = pState->iPixelCount; // don't decode too much
    pState->iPixelCount -= iOutSize;
    pEnd = &d[iOutSize * (pState->bpp >> 3)];
    s = pState->pInPtr;
    pSrcEnd = pState->pInEnd;
    if (s >= pSrcEnd) {
        // Either we're at the end of the file or we need to read more data
        if (get_more_data(pState) && run == 0)
            return SLIC_DECODE_ERROR; // we're trying to go past the end, error
        s = pState->ucFileBuf;
        pSrcEnd = pState->pInEnd;
    }

    if (iBpp == 1) { // 8-bit grayscale/palette
        uint8_t *index8 = (uint8_t *)pState->index;
        px8 = (uint8_t)pState->curr_pixel;
        if (pState->extra_pixel) {
            pState->extra_pixel = 0;
            *d++ = px8;
        }
        while (d < pEnd) {
            if (run) {
                *d++ = px8; // need to deal with pixels 1 at a time
                run--;
                continue;
            }
            if (s >= pSrcEnd) {
                // Either we're at the end of the file or we need to read more data
                if (get_more_data(pState))
                    return SLIC_DECODE_ERROR; // we're trying to go past the end, error
                s = pState->ucFileBuf;
                pSrcEnd = pState->pInEnd;
            }
            if (bad_run) {
                px8 = *s++;
                *d++ = px8;
                index8[SLIC_GRAY_HASH(px8)] = px8;
                bad_run--;
                continue;
            }
            op = *s++; // get next compression op
            if ((op & SLIC_OP_MASK) == SLIC_OP_RUN8) {
                if (op == SLIC_OP_RUN8_1024) {
                    run = 1024;
                } else if (op == SLIC_OP_RUN8_256) {
                    run = 256;
                } else {
                    run = op + 1;
                }
                continue;
            } else if ((op & SLIC_OP_MASK) == SLIC_OP_BADRUN8) {
                bad_run = (op & 0x3f) + 1;
                continue;
            } else if ((op & SLIC_OP_MASK) == SLIC_OP_INDEX8) {
                *d++ = index8[op & 7];
                px8 = index8[(op >> 3) & 7];
                if (d < pEnd) { // fits in the requested output size?
                    *d++ = px8;
                } else {
                    pState->extra_pixel = 1; // get it next time through
                }
            } else { // must be DIFF8
                px8 += (op & 7)-4;
                index8[SLIC_GRAY_HASH(px8)] = px8;
                *d++ = px8;
                px8 += ((op >> 3) & 7)-4;
                index8[SLIC_GRAY_HASH(px8)] = px8;
                if (d < pEnd) {
                    *d++ = px8;
                } else {
                    pState->extra_pixel = 1; // get it next time through
                }
            }
        }
        pState->run = run;
        pState->bad_run = bad_run;
        pState->curr_pixel = px8;
        pState->pInPtr = s;
        return (pState->iPixelCount == 0) ? SLIC_DONE : SLIC_SUCCESS;
    } // 8-bit grayscale/palette decode

    if (iBpp == 2) { // RGB565
        d16 = (uint16_t *)d;
        index16 = (uint16_t *)pState->index;
        pEnd16 = (uint16_t *)pEnd;
        px16 = (uint16_t)pState->curr_pixel;
        if (pState->extra_pixel) {
            pState->extra_pixel = 0;
            *d16++ = px16;
        }
        while (d16 < pEnd16) {
            if (run) {
                *d16++ = px16;
                run--;
                continue;
            }
            if (s >= pSrcEnd) {
                // Either we're at the end of the file or we need to read more data
                if (get_more_data(pState))
                    return SLIC_DECODE_ERROR; // we're trying to go past the end, error
                s = pState->ucFileBuf;
                pSrcEnd = pState->pInEnd;
            }
            if (bad_run) {
                px16 = *s++;
                if (s >= pSrcEnd) {
                    // Either we're at the end of the file or we need to read more data
                    if (get_more_data(pState))
                        return SLIC_DECODE_ERROR; // we're trying to go past the end, error
                    s = pState->ucFileBuf;
                    pSrcEnd = pState->pInEnd;
                }
                px16 |= (*s++ << 8);
                *d16++ = px16;
                index16[SLIC_RGB565_HASH(px16)] = px16;
                bad_run--;
                continue;
            }
            op = *s++;
            if ((op & SLIC_OP_MASK) == SLIC_OP_RUN16) {
                if (op == SLIC_OP_RUN16_1024) {
                    run = 1024;
                    continue;
                } else if (op == SLIC_OP_RUN16_256) {
                    run = 256;
                    continue;
                } else {
                    run = op + 1;
                    continue;
                }
            } else if ((op & SLIC_OP_MASK) == SLIC_OP_BADRUN16) {
                bad_run = (op & 0x3f) + 1;
                continue;
            } else if ((op & SLIC_OP_MASK) == SLIC_OP_INDEX16) {
                *d16++ = index16[op & 7];
                px16 = index16[(op >> 3) & 7];
                if (d16 < pEnd16) { // fits in the requested output size?
                    *d16++ = px16;
                } else {
                    pState->extra_pixel = 1; // get it next time through
                }
            } else { // must be DIFF16
                uint8_t r, g, b;
                r = (uint8_t)(px16 >> 11);
                g = (uint8_t)((px16 >> 5) & 0x3f);
                b = (uint8_t)(px16 & 0x1f);
                r += ((op >> 4) & 3) - 2;
                g += ((op >> 2) & 3) - 2;
                b += ((op >> 0) & 3) - 2;
                px16 = (r << 11) | ((g & 0x3f) << 5) | (b & 0x1f);
                index16[SLIC_RGB565_HASH(px16)] = px16;
                *d16++ = px16;
            }
        } // for each output pixel
        pState->run = run;
        pState->bad_run = bad_run;
        pState->curr_pixel = px16;
        pState->pInPtr = s;
        return (pState->iPixelCount == 0) ? SLIC_DONE : SLIC_SUCCESS;
    } // RGB565 decode

    px = pState->curr_pixel;
    while (d < pEnd) {
        int iHash;
        if (run) {
#ifdef UNALIGNED_ALLOWED
            *(uint32_t *)d = px;
#else
            d[0] = (uint8_t)px;
            d[1] = (uint8_t)(px >> 8);
            d[2] = (uint8_t)(px >> 16);
            d[3] = (uint8_t)(px >> 24);
#endif
            run--;
            d += iBpp;
            continue;
        }
        if (s >= pSrcEnd) {
            // Either we're at the end of the file or we need to read more data
            if (get_more_data(pState))
                return SLIC_DECODE_ERROR; // we're trying to go past the end, error
            s = pState->ucFileBuf;
            pSrcEnd = pState->pInEnd;
        }
			op = *s++;
            if (op >= SLIC_OP_RUN && op < SLIC_OP_RGB) {
                if (op == SLIC_OP_RUN1024) {
                    run = 1024;
                    continue;
                } else if (op == SLIC_OP_RUN256) {
                    run = 256;
                    continue;
                } else {
                    run = (op & 0x3f) + 1;
                    continue;
                }
            }
            else if ((op & SLIC_OP_MASK) == SLIC_OP_INDEX) {
                px = index[op];
            }
			else if (op == SLIC_OP_RGB) {
                px &= 0xff000000;
#ifdef UNALIGNED_ALLOWED
                px |= (*(uint32_t *)s) & 0xffffff;
#else
                px |= s[0];
                px |= ((uint32_t)s[1] << 8);
                px |= ((uint32_t)s[2] << 16);
#endif
                s += 3;
			}
			else if (op == SLIC_OP_RGBA) {
#ifdef UNALIGNED_ALLOWED
                px = *(uint32_t *)s;
                s += 4;
#else
                px = *s++;
                px |= ((uint32_t)*s++ << 8);
                px |= ((uint32_t)*s++ << 16);
                px |= ((uint32_t)*s++ << 24);
#endif
			}
			else if ((op & SLIC_OP_MASK) == SLIC_OP_DIFF) {
                uint8_t r, g, b;
                r = px;
                g = (px >> 8);
                b = (px >> 16);
                r += ((op >> 4) & 0x03) - 2;
                g += ((op >> 2) & 0x03) - 2;
                b += ( op       & 0x03) - 2;
                px &= 0xff000000;
                px |= (r & 0xff);
                px |= ((uint32_t)(g & 0xff) << 8);
                px |= ((uint32_t)(b & 0xff) << 16);
			}
			else if ((op & SLIC_OP_MASK) == SLIC_OP_LUMA) {
				int b2 = *s++;
				int vg = (op & 0x3f) - 32;
                uint8_t r, g, b;
                r = px;
                g = (px >> 8);
                b = (px >> 16);
                r += vg - 8 + ((b2 >> 4) & 0x0f);
                g += vg;
                b += vg - 8 +  (b2       & 0x0f);
                px &= 0xff000000;
                px |= (r & 0xff);
                px |= ((uint32_t)(g & 0xff) << 8);
                px |= ((uint32_t)(b & 0xff) << 16);
			}
        iHash = px * 3;
        iHash += ((px >> 8) * 5);
        iHash += ((px >> 16) * 7);
        iHash += ((px >> 24) * 11);
        index[iHash & 63] = px;

#ifdef UNALIGNED_ALLOWED
        *(uint32_t *)d = px;
#else
        d[0] = (uint8_t)px;
        d[1] = (uint8_t)(px >> 8);
        d[2] = (uint8_t)(px >> 16);
        d[3] = (uint8_t)(px >> 24);
#endif
        d += iBpp;
	} // while decoding each pixel (3/4 bpp)

    pState->run = run;
    pState->bad_run = bad_run;
    pState->curr_pixel = px;
    pState->pInPtr = s;
    return (pState->iPixelCount == 0) ? SLIC_DONE : SLIC_SUCCESS;
} /* slic_decode() */
