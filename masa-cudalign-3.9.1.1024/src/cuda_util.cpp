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

#include <cuda.h>

#define DEBUG (1)

#include "cuda_util.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "config.h"

static cudaDeviceProp getDeviceProperties(int dev=-1);
static int cutGetMaxGflopsDeviceId();

/**
 * Allocates and copies sequence data in GPU.
 *
 * @param data the sequence data string.
 * @param len the length of the sequence.
 * @param padding_len extra padding length.
 * @param padding_char character to be used as padding.
 *
 * @return the pointer to the GPU memory allocated for the sequence.
 */
unsigned char* allocCudaSeq(const char* data, const int len, const int padding_len, const char padding_char) {
	unsigned char* out = (unsigned char*)allocCuda0(len+padding_len);
	if (DEBUG) printf("allocCudaSeq(%p, %d, %d, %d): %p\n", data, len, padding_len, padding_char, out);
	cutilSafeCall( cudaMemcpy(out, data, len, cudaMemcpyHostToDevice));
	cutilSafeCall( cudaMemset(out+len, padding_char, padding_len) );
    return out;
}

/**
 * Allocates a vector in GPU and initializes it to zero.
 *
 * @param size length of the vector to be allocated.
 * @return the pointer to the GPU allocated memory.
 */
void* allocCuda0(int size) {
    void* out;
    cutilSafeCall( cudaMalloc((void**) &out, size));
    cutilSafeCall( cudaMemset(out, 0, size));
	if (DEBUG) printf("allocCuda(%d): %p\n", size, out);
    return out;
}

/**
 * Returns the number of multiprocessors in the selected GPU.
 * @return the number of multiprocessors in the selected GPU.
 */
int getGPUMultiprocessors() {
	cudaDeviceProp prop = getDeviceProperties();
	return prop.multiProcessorCount;
}

/**
 * Prints the device properties.
 * @param file the file to print out the properties.
 */
void printDevProp(FILE* file) {
    cudaDeviceProp devProp = getDeviceProperties();

	fprintf(file, "Major revision number:         %d\n",  devProp.major);
	fprintf(file, "Minor revision number:         %d\n",  devProp.minor);
	fprintf(file, "Name:                          %s\n",  devProp.name);
	fprintf(file, "Total global memory:           %lu\n", (unsigned long) devProp.totalGlobalMem);
	fprintf(file, "Total shared memory per block: %lu\n", (unsigned long) devProp.sharedMemPerBlock);
	fprintf(file, "Total registers per block:     %d\n",  devProp.regsPerBlock);
	fprintf(file, "Warp size:                     %d\n",  devProp.warpSize);
	fprintf(file, "Maximum memory pitch:          %lu\n", (unsigned long) devProp.memPitch);
	fprintf(file, "Maximum threads per block:     %d\n",  devProp.maxThreadsPerBlock);
	for (int i = 0; i < 3; ++i)
		fprintf(file, "Maximum dimension %d of block:  %d\n", i, devProp.maxThreadsDim[i]);
	for (int i = 0; i < 3; ++i)
		fprintf(file, "Maximum dimension %d of grid:   %d\n", i, devProp.maxGridSize[i]);
	fprintf(file, "Clock rate:                    %d\n",  devProp.clockRate);
	fprintf(file, "Total constant memory:         %lu\n", (unsigned long) devProp.totalConstMem);
	fprintf(file, "Texture alignment:             %lu\n", (unsigned long) devProp.textureAlignment);
	fprintf(file, "Concurrent copy and execution: %s\n",  (devProp.deviceOverlap ? "Yes" : "No"));
	fprintf(file, "Number of multiprocessors:     %d\n",  devProp.multiProcessorCount);
	fprintf(file, "Kernel execution timeout:      %s\n",  (devProp.kernelExecTimeoutEnabled ? "Yes" : "No"));
	fflush(file);
	return;
}

/**
 * Returns the compute capability of the selected GPU.
 * @return the compute capability in the integer format (210 means
 * version 2.1)
 */
int getDevCapability() {
    cudaDeviceProp devProp;
    int dev;
    cudaGetDevice(&dev);
    cutilSafeCall(cudaGetDeviceProperties(&devProp, dev));
    return devProp.major*100+devProp.minor*10;
}

/**
 * Returns the compiled compute capability.
 * @return the compute capability in the integer format (210 means
 * version 2.1)
 */
int getCompiledCapability() {
	int major;
	int minor;
	char* str = COMPILED_CUDA_ARCH;
	if (strstr(str, "sm_") == str && strlen(str) == 5) {
		major = str[3]-'0';
		minor = str[4]-'0';
	} else {
		major = 0;
		minor = 0;
	}
	return major*100+minor*10;
}

/**
 * Prints all available GPUs.
 * @param file the file to print out the properties.
 */
void printGPUDevices(FILE* file) {
    int count;
    cudaError_t err = cudaGetDeviceCount(&count);
    if (err == cudaErrorInsufficientDriver) {
        fprintf(file, "NVIDIA CUDA driver is older than the CUDA runtime library\n");
    } else if (err == cudaErrorNoDevice) {
		fprintf(file, "Available GPUs: %d\n", 0);
    	fprintf(file, "no CUDA-capable devices were detected.\n");
    } else {
		cudaDeviceProp devProp;
		fprintf(file, "Detected GPUs: %d\n", count);
		fprintf(file, "ID: NAME (RAM)\n");
		fprintf(file, "---------------------------\n");
		int id=0;
		for (int deviceId=0; deviceId<count; deviceId++) {
			cudaGetDeviceProperties(&devProp, deviceId);
			bool compatible = (getCompiledCapability() <= (devProp.major*100+devProp.minor*10));
			bool timeout = devProp.kernelExecTimeoutEnabled;
			if (compatible) {
				fprintf(file, "%2d: ", id++);
			} else {
				fprintf(file, "    ");
			}
			fprintf(file, "%s (%4d MB)", devProp.name,
					(int)ceil(devProp.totalGlobalMem/1024.0/1024.0));
			if (!compatible) {
				fprintf(file, " (Unsupported capability %d.%d)", devProp.major, devProp.minor);
			}
			if (timeout) {
				fprintf(file, " (Timeout Enabled)");
			}
			fprintf(file, "\n");
		}
		fprintf(file, "\n");
    }
}

/**
 * Returns the GPU weights for all available GPUs.
 *
 * @param proportion vector that will receive the weights.
 * @param n maximum number of weights to be stored in the vector.
 * @return the number of weights stored in the vector.
 */
int getGPUWeights(int* proportion, int n) {
	/*
	 * When any CUDA runtime function is called, the CUDA context is initialized.
	 * If we call a fork after this initialization, the same context is shared
	 * among the processes, what causes initialization errors and abnormal
	 * execution. The getGPUWeights function are called before the fork procedure.
	 * in the libmasa_entry_point, so, we must obtain all the GPU weights/proportion
	 * using another process. This method fork a process only to obtain these
	 * CUDA dependent values and the child process dies with its own CUDA context.
	 * This context is not shared with the parent process, so we can continue
	 * the Aligner execution without any problem.
	 */

	/* Communication using PIPE */
	int pipe_fd[2];
	if (pipe(pipe_fd)) {
		fprintf(stderr, "ERROR: GPU weights could not be obtained (1).\n");
		exit(-1);
	}

	int pid = fork();
	if (pid == 0) {
		/* Child */
		close(pipe_fd[0]);
		int count;
		cudaGetDeviceCount(&count);

		cudaDeviceProp devProp;
		for (int deviceId=0; deviceId<count; deviceId++) {
			cutilSafeCall(cudaGetDeviceProperties(&devProp, deviceId));
	        int cores;
	        if (devProp.major <= 1) {
	        	cores = 8;
	        } else if (devProp.major == 2 && devProp.minor == 0) {
	        	cores = 32;
	        } else if (devProp.major == 2 && devProp.minor == 1) {
	        	cores = 48;
	        } else {
	        	cores = 192;
	        }
	        if (getCompiledCapability() <= (devProp.major*100+devProp.minor*10)) {
	        	int speed = devProp.clockRate*devProp.multiProcessorCount*cores/1000;
	        	if (write(pipe_fd[1], &speed, sizeof(speed)) == -1) {
	    			fprintf(stderr, "ERROR: GPU weights could not be obtained (2).\n");
	    			exit(1);
	        	}
	        }
		}
		cudaDeviceReset();
		close(pipe_fd[1]);
		exit(7);
	} else {
		/* Parent */
		close(pipe_fd[1]);
		int val;
		int count = 0;
		while (read(pipe_fd[0], &val, sizeof(val)) > 0) {
			if (count < n) {
				proportion[count++] = val;
			}
		}
		close(pipe_fd[0]);
		waitpid(pid, NULL, 0); // Join processes
		return count;
	}

}

/**
 * Returns a vector containing the IDs of the supported GPUs.
 *
 * @param ids vector that will receive the IDs.
 * @param n maximum number of IDs to be stored in the vector.
 * @return the number of IDs stored in the vector.
 */
int getAvailableGPU(int* ids, int n) {
	int count = 0;
	cudaGetDeviceCount(&count);

	cudaDeviceProp devProp;
	int i=0;
	for (int deviceId=0; deviceId<count; deviceId++) {
		cutilSafeCall(cudaGetDeviceProperties(&devProp, deviceId));
        if (getCompiledCapability() <= (devProp.major*100+devProp.minor*10)) {
        	if (count < n) {
        		ids[i++] = deviceId;
        	}
        }
	}
	return i;
}

/**
 * Selects a GPU for the CUDA execution.
 * @param id id of the selected GPU.
 */
void selectGPU(int id) {
    int deviceId;
    if (id == -1) {
        deviceId = cutGetMaxGflopsDeviceId();
    } else {
        deviceId = id;
    }

    cutilSafeCall(cudaSetDevice( deviceId ));
    cutilCheckMsg("cudaSetDevice failed");
}

/**
 * Returns the memory usage statistics.
 *
 * @param usedMem receives the current used memory.
 * @param totalMem receives the total memory of the device.
 */
void getMemoryUsage(size_t* usedMem, size_t* totalMem) {
	size_t myFreeMem;
	size_t myTotalMem;
	cudaFree(0); // Ensures the CUDA context creation. Otherwise cuMemGetInfo (driver API) will return zero.
	cuMemGetInfo(&myFreeMem, &myTotalMem);
	if (usedMem != NULL) {
		*usedMem = myTotalMem-myFreeMem;
	}
	if (totalMem != NULL) {
		*totalMem = myTotalMem;
	}
}

/**
 * Returns the cudaDeviceProp structure with the properties of a given GPU.
 *
 * @param dev id of the GPU to be queried.
 * @return the cudaDeviceProp structure containing the properties of the GPU.
 */
cudaDeviceProp getDeviceProperties(int dev) {
    cudaDeviceProp devProp;
    if (dev == -1) {
    	cudaGetDevice(&dev);
    }
    cutilSafeCall(cudaGetDeviceProperties(&devProp, dev));
    return devProp;
}

/**
 *  This function returns the best GPU (with maximum GFLOPS).
 *  @remarks copied from cutil_inline_runtime.h - Copyright 1993-2010 NVIDIA Corporation.
 */
int cutGetMaxGflopsDeviceId() {
	int current_device   = 0, sm_per_multiproc = 0;
	int max_compute_perf = 0, max_perf_device  = 0;
	int device_count     = 0, best_SM_arch     = 0;
    int arch_cores_sm[3] = { 1, 8, 32 };
	cudaDeviceProp deviceProp;

	cudaGetDeviceCount( &device_count );
	// Find the best major SM Architecture GPU device
	while ( current_device < device_count ) {
		cudaGetDeviceProperties( &deviceProp, current_device );
		if (deviceProp.major > 0 && deviceProp.major < 9999) {
			if (best_SM_arch < deviceProp.major) {
				best_SM_arch = deviceProp.major;
			}
		}
		current_device++;
	}

    // Find the best CUDA capable GPU device
	current_device = 0;
	while( current_device < device_count ) {
		cudaGetDeviceProperties( &deviceProp, current_device );
		if (deviceProp.major == 9999 && deviceProp.minor == 9999) {
		    sm_per_multiproc = 1;
		} else if (deviceProp.major <= 2) {
			sm_per_multiproc = arch_cores_sm[deviceProp.major];
		} else {
			sm_per_multiproc = arch_cores_sm[2];
		}

		int compute_perf  = deviceProp.multiProcessorCount * sm_per_multiproc * deviceProp.clockRate;
		if( compute_perf  > max_compute_perf ) {
            // If we find GPU with SM major > 2, search only these
			if ( best_SM_arch > 2 ) {
				// If our device==dest_SM_arch, choose this, or else pass
				if (deviceProp.major == best_SM_arch) {	
					max_compute_perf  = compute_perf;
					max_perf_device   = current_device;
				}
			} else {
				max_compute_perf  = compute_perf;
				max_perf_device   = current_device;
			}
		}
		++current_device;
	}
	return max_perf_device;
}

