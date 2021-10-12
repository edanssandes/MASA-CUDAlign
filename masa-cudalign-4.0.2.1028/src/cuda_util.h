/*******************************************************************************
 *
 * Copyright (c) 2010-2015   Edans Sandes
 *
 * This file is part of MASA-CUDAlign.
 * 
 * MASA-CUDAlign is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * MASA-CUDAlign is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MASA-CUDAlign.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

/**
 * @file cuda_util.h
 * @brief File with basic functions and macros for CUDA calls.
 */

#include <stdio.h>
#include <stdlib.h>

#include <cuda_runtime.h>

#define cutilSafeCall(err)           __cudaSafeCall      (err, __FILE__, __LINE__)
inline void __cudaSafeCall( cudaError err, const char *file, const int line )
{
    if( cudaSuccess != err) {
		fprintf(stderr, "%s(%i) : cudaSafeCall() Runtime API error : %s.\n",
                file, line, cudaGetErrorString( err) );
        exit(-1);
    }
}

#define cutilCheckMsg(msg)           __cutilCheckMsg     (msg, __FILE__, __LINE__)
inline void __cutilCheckMsg( const char *errorMessage, const char *file, const int line )
{
    cudaError_t err = cudaGetLastError();
    if( cudaSuccess != err) {
        fprintf(stderr, "%s(%i) : cutilCheckMsg() CUTIL CUDA error : %s : %s.\n",
                file, line, errorMessage, cudaGetErrorString( err) );
        exit(-1);
    }
#ifdef _DEBUG
    err = cudaThreadSynchronize();
    if( cudaSuccess != err) {
		fprintf(stderr, "%s(%i) : cutilCheckMsg cudaThreadSynchronize error: %s : %s.\n",
                file, line, errorMessage, cudaGetErrorString( err) );
        exit(-1);
    }
#endif
}

void* allocCuda0(int size);
unsigned char* allocCudaSeq(const char* data, const int len, const int padding_len=0, const char padding_char='\0');
void printDevProp(FILE* file=stdout);
void getMemoryUsage(size_t* freeMem, size_t* totalMem=NULL);
void printGPUDevices(FILE* file=stdout);
int getGPUMultiprocessors();
void selectGPU(int id);
int getGPUWeights(int* proportion, int n);
int getAvailableGPU(int* ids, int n);
int getDevCapability();
int getCompiledCapability();
