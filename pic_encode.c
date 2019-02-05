/*
 * Enhance definition of edges
 * General note:  Error checking is kind of a circus in this file
 */

#include<CL/cl.h>
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<SOIL/SOIL.h>
#include<sys/time.h>
#include "scolor.h"
#include<string.h>

#define EXTRABUF 200000

typedef struct timeval timeval;

cl_device_info query_items[] = {CL_DEVICE_TYPE, CL_DEVICE_VENDOR_ID, CL_DEVICE_MAX_COMPUTE_UNITS, CL_DEVICE_GLOBAL_MEM_SIZE, CL_DEVICE_LOCAL_MEM_SIZE, CL_DEVICE_NAME, CL_DRIVER_VERSION, CL_DEVICE_VENDOR, CL_DEVICE_EXTENSIONS};

char* query_names[] = {"CL_DEVICE_TYPE", "CL_DEVICE_VENDOR_ID", "CL_DEVICE_MAX_COMPUTE_UNITS", "CL_DEVICE_GLOBAL_MEM_SIZE", "CL_DEVICE_LOCAL_MEM_SIZE", "CL_DEVICE_NAME", "CL_DRIVER_VERSION", "CL_DEVICE_VENDOR", "CL_DEVICE_EXTENSIONS"};

#define check(m) if(errcode != CL_SUCCESS){ puts(m); goto err; }

double timediff(timeval* start, timeval* end)
{
	double s_diff = end->tv_sec - start->tv_sec;
	double us_diff = end->tv_usec - start->tv_usec;
	s_diff += us_diff / 1000000;
	return s_diff;
}

int main(int argc, char ** argv){
	char outfilename[1024];
	if(argc < 2){
		printf("Usage: %s [infile] [encode text]\n", argv[0]);
		return 1;
	}
	char raw = 0;
	int width, height;
	unsigned char* image = SOIL_load_image(argv[1], &width, &height, 0, SOIL_LOAD_RGB);
	size_t imgsize = width*height*3;
	const size_t aspace = ((imgsize)/8)-4;
	size_t secretlen = 0;
	char encoding = 0;
	char secret[aspace+EXTRABUF];
	timeval start, end;
	cl_platform_id plt_ids[10];
	cl_uint plt_count;
	cl_device_id dev_ids[10];
	cl_uint dev_count;
	char pname[1024];

	if(argc > 2 && !strcmp(argv[2],"-p")){
		ssize_t readsize;
		do{
			readsize = read(0, secret+secretlen, aspace-1);
			secretlen += readsize;
			if(secretlen > aspace){
				printf("Not enough space!\nSpace available: %ld\n", aspace);
				
				return 1;
			}
		}while(readsize > 0);
		if(secretlen < 1){
			printf("Nothing to encode\n");
			return 1;
		}
		secret[secretlen] = 0;
		printf("Read %ld bytes\n", secretlen);
		printf("%ld bytes available\n", aspace);
		printf("Space used: %3.2f%%\n", ((float)secretlen/(float)aspace)*100.0);
		encoding = 1;

	}else if(argc > 2 && !strcmp(argv[2],"-raw")){
		raw = 1;
	}else{
		printf("[E]ncode or [D]ecode? ");
		char process;
		scanf("%[^\n]%*c",&process);
		if(process == 'e' || process == 'E'){
			encoding = 1;
			printf("%ld Characters Available\n", aspace);
			printf("Enter String: ");
		
			scanf ("%[^\n]%*c", secret);
			secretlen = strlen(secret);
			printf("Length of encode: %ld\n", secretlen);
			printf("Space used: %3.2f\n", ((float)secretlen/(float)aspace)*100.0);
			
		}else{
			printf("Decoding image...\n");
		}
	}

	cl_int retval = clGetPlatformIDs(10, plt_ids, &plt_count);
	if(retval != CL_SUCCESS){
		printf(RED("clGetPlatformIDs failed with error %d\n"), retval);
		goto err;
	}
	
	for(int i = 0; i < plt_count; i++){
		//printf(PURPLE("Platform %d (ID %lu):\n"), i, (size_t)plt_ids[i]);
		cl_int retval = clGetDeviceIDs(plt_ids[i], CL_DEVICE_TYPE_ALL, 10, dev_ids, &dev_count);
		if(retval != CL_SUCCESS){
			printf(RED("clGetPlatformIDs failed with error %d\n"), retval);
			goto err;
		}/*
		for(int j = 0; j < dev_count; j++){
			printf(GREEN("Device %d (ID %lu):\n"), j, (size_t)dev_ids[j]);	
			for(int q = 0; q < sizeof(query_items)/sizeof(cl_device_info); q++){
				size_t psize;
				retval = clGetDeviceInfo(dev_ids[j], query_items[q], 1024, pname, &psize);
				if(retval != CL_SUCCESS){
					printf(RED("Query Failed:  %s\n"), query_names[q]);
					continue;
				}
				if(isalnum(pname[0]) || (psize != 4 && psize != 8)){ // Not bulletproof
					pname[psize] = 0;
					printf("%s : "BBLUE("%s")" (%ld)\n", query_names[q], pname, psize);
				} else if(psize == 4) {
					printf("%s : "BBLUE("%u")" (%ld)\n", query_names[q], *((cl_uint*)pname), psize);
				} else {
					printf("%s : "BBLUE("%lu")" (%ld)\n", query_names[q], *((size_t*)pname), psize);
				}
			}
		}*/
	}
	/* We'll assume platform 0, device 0 from this point on */
	cl_int errcode;
	cl_context context = clCreateContext(0, 1, dev_ids, 0, 0, &errcode);
	if(errcode != CL_SUCCESS) goto err;

	/* Note here:  You won't find a manpage for clCreateCommandQueueWithProperties on Debian Linux, and probably others
	 * The deprecated version is just called clCreateCommandQueue, and they take the same parameters.
	 * On Debian at least, the compiler will warn that clCreateCommandQueue is deprecated, and accept this version
	 * nVidia has been slow with OpenCL support and clCreateCommandQueueWithProperties is OpenCL 2.0+, but it works
	 */
	cl_command_queue queue = clCreateCommandQueueWithProperties(context, dev_ids[0], 0, &errcode);
	check("clCreateCommandQueue");

	// Get the picture into memory somewhere
	cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imgsize, image, &errcode);
	if(errcode != CL_SUCCESS) goto err;
	
	cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, imgsize, 0, &errcode);
	if(errcode != CL_SUCCESS) goto err;

	cl_mem secret_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, aspace, secret, &errcode);
	if(errcode != CL_SUCCESS) goto err;

	cl_mem size_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 8, (char*)&secretlen, &errcode);
	if(errcode != CL_SUCCESS) goto err;

	const char* decode = "\
		__kernel void squares(__global unsigned char *in, __global unsigned char *out, __global unsigned char* secret, __global char* len){\
			const int i = get_global_id(0);\
			if(i/8 < 4){\
				len[i/8 + i%8] = len[i/8] | ((in[i/8 + i%8] & 1) << i%8);\
			}else{\
				secret[(i/8 + i%8)-4] = 0;\
				secret[(i/8 + i%8)-4] = secret[(i/8)-4] | ((in[i/8 + i%8] & 1) << i%8);\
			}\
		}";
	const char* encode = "\
		__kernel void squares(__global unsigned char *in, __global unsigned char *out, __global unsigned char* secret, __global char *len){\
			const int i = get_global_id(0);\
			if(i>((size_t)len)+4){\
				out[i] = in[i];\
				return;\
			}\
			out[i/8 + i%8] = (in[i/8 + i%8] & 254);\
			if(i<4){\
				out[i/8 + i%8] = out[i/8 + i%8] | ((len[i/8 + i%8]>>i%8) & 1);\
			}else{\
				out[i/8 + i%8] = out[i/8 + i%8] | ((secret[(i/8 + i%8)-4]>>i%8) & 1);\
			}\
		}";
	
	cl_program program;
	if(encoding){
		program = clCreateProgramWithSource(context, 1, &encode, 0, &errcode);
		check("clCreateProgramWithSource");
	}else{
		program = clCreateProgramWithSource(context, 1, &decode, 0, &errcode);
		check("clCreateProgramWithSource");
	}

	if(clBuildProgram(program, 1, dev_ids, 0, 0, 0) != CL_SUCCESS){
		puts("clBuildProgram");
		goto err;
	}

	cl_kernel kernel = clCreateKernel(program, "squares", &errcode);
	check("clCreateKernel");

	errcode = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
	check("clSetKernelArg");

	if(CL_SUCCESS != (errcode = clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer))){
		printf("clSetKernelArg #2\n");
		goto err;
	}

	errcode = clSetKernelArg(kernel, 2, sizeof(cl_mem), &secret_buffer);
	check("clSetKernelArg");
	
	errcode = clSetKernelArg(kernel, 3, sizeof(cl_mem), &size_buffer);
	check("clSetKernelArg");

	const size_t worklen = imgsize;
	gettimeofday(&start, NULL);
	if(CL_SUCCESS != (errcode = clEnqueueNDRangeKernel(queue, kernel, 1, 0, &worklen, 0, 0, 0, 0))){
		perror("clEnqueueNDRangeKernel");
		goto err;
	}
	gettimeofday(&end, NULL);
	if(!raw)printf("Time: %lf\n",  timediff(&start, &end));
	if(encoding){
		errcode = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, imgsize, image, 0, 0, 0);
		check("clEnqueueReadBuffer");
		SOIL_save_image("done.bmp", SOIL_SAVE_TYPE_BMP, width, height, 3, image);
	}else{
		errcode = clEnqueueReadBuffer(queue, size_buffer, CL_TRUE, 0, sizeof(size_t), &secretlen, 0, 0, 0);
		check("clEnqueueReadBuffer");

		errcode = clEnqueueReadBuffer(queue, secret_buffer, CL_TRUE, 0, aspace, secret, 0, 0, 0);
		check("clEnqueueReadBuffer");
		if(raw){
			for(size_t i=0; i < secretlen; i++){
				printf("%c",secret[i]);
			}
		}else printf("Finished string: %s\nFinished stringlen: %ld\n", secret, secretlen);
	}

	return 0;
err:
	printf(RED("We have failed to discover the wonders of OpenCL, errcode = %d\n"), errcode);
	return 1;
}
