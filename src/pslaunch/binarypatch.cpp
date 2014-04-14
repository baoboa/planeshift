/*
 * binarypatch.cpp - Author: Mike Gist
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * Thanks to Josh MacDonald and Ralf Junker.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <sys/stat.h>

#include "binarypatch.h"

#ifdef WIN32
#pragma warning( disable : 4996 )
#endif

extern "C"
{
#include "../tools/xdelta3/xdelta3.h"
}

/* Helper function for conversion. Not in class to avoid basic type redefinition. */
int PatchFileHelper(FILE* srcFile, FILE* patchFile, FILE* outFile, unsigned int bufferSize)
{
    int r, res;
    xd3_stream stream;
    xd3_config config;
    xd3_source source;
    void* input_Buffer;
    unsigned int input_Buffer_Read;

    if (bufferSize < XD3_ALLOCSIZE)
    {
        bufferSize = XD3_ALLOCSIZE;
    }

    memset (&stream, 0, sizeof (stream));
    memset (&source, 0, sizeof (source));

    xd3_init_config(&config, XD3_ADLER32);
    config.winsize = bufferSize;
    xd3_config_stream(&stream, &config);

    if (srcFile)
    {
        source.blksize = bufferSize;
        source.curblk = (const uint8_t*)malloc(source.blksize);

        /* Load the first block of the stream. */
        r = fseek(srcFile, 0, SEEK_SET);
        if (r)
            return r;
        source.onblk = (usize_t)fread((void*)source.curblk, 1, source.blksize, srcFile);
        source.curblkno = 0;
        /* Set the stream. */
        xd3_set_source(&stream, &source);
    }

    input_Buffer = malloc(bufferSize);

    fseek(patchFile, 0, SEEK_SET);
    do
    {
        input_Buffer_Read = (unsigned int)fread(input_Buffer, 1, bufferSize, patchFile);
        if (input_Buffer_Read < bufferSize)
        {
            xd3_set_flags(&stream, XD3_FLUSH);
        }
        xd3_avail_input(&stream, (const uint8_t*)input_Buffer, input_Buffer_Read);

process:
        res = xd3_decode_input(&stream);

        switch (res)
        {
        case XD3_INPUT:
            {
                continue;
            }
        case XD3_OUTPUT:
            {
                r = (int)fwrite(stream.next_out, 1, stream.avail_out, outFile);
                if (r != (int)stream.avail_out)
                    return r;
                xd3_consume_output(&stream);
                goto process;
            }
        case XD3_GETSRCBLK:
            {
                if (srcFile)
                {
                    r = fseek(srcFile, source.blksize * (long)source.getblkno, SEEK_SET);
                    if (r)
                        return r;
                    source.onblk = (usize_t)fread((void*)source.curblk, 1,
                        source.blksize, srcFile);
                    source.curblkno = source.getblkno;
                }
                goto process;
            }
        case XD3_GOTHEADER:
        case XD3_WINSTART:
        case XD3_WINFINISH:
            {
                goto process;
            }
        default:
            {
                return res;
            }
        }
    }
    while (input_Buffer_Read == bufferSize);

    free(input_Buffer);

    free((void*)source.curblk);
    xd3_close_stream(&stream);
    xd3_free_stream(&stream);

    return 0;
}

bool PatchFile(const char *oldFilePath, const char *vcdiff, const char *newFilePath)
{
    FILE* srcFile = fopen(oldFilePath, "rb");

    if(!srcFile)
    {
        return false;
    }

    FILE* patchFile = fopen(vcdiff, "rb");

    if(!patchFile)
    {
        fclose(srcFile);
        return false;
    }

    FILE* outFile = fopen(newFilePath, "wb");

    if(!outFile)
    {
        fclose(patchFile);
        fclose(srcFile);
        return false;
    }

    int res = PatchFileHelper(srcFile, patchFile, outFile, 0x1000);

    fclose(outFile);
    fclose(patchFile);
    fclose(srcFile);

    return (!res);
}
