/*
 * Enhance definition of edges
 * General note:  Error checking is kind of a circus in this file
 */

#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<SOIL/SOIL.h>
#include<sys/time.h>
#include<string.h>

#define EXTRABUF 200000

typedef struct timeval timeval;

void encode(char* in,char* out, char* secret, char* msglen, size_t worklen){
	for(size_t i = 0; i < worklen; i++){
		if(i>*((size_t*)msglen)+4){
			out[i*8+0] = in[i*8+0];
			out[i*8+1] = in[i*8+1];
			out[i*8+2] = in[i*8+2];
			out[i*8+3] = in[i*8+3];
			out[i*8+4] = in[i*8+4];
			out[i*8+5] = in[i*8+5];
			out[i*8+6] = in[i*8+6];
			out[i*8+7] = in[i*8+7];
		}else{
			out[i*8+0] = (in[i*8+0] & 254);
			out[i*8+1] = (in[i*8+1] & 254);
			out[i*8+2] = (in[i*8+2] & 254);
			out[i*8+3] = (in[i*8+3] & 254);
			out[i*8+4] = (in[i*8+4] & 254);
			out[i*8+5] = (in[i*8+5] & 254);
			out[i*8+6] = (in[i*8+6] & 254);
			out[i*8+7] = (in[i*8+7] & 254);
			if(i<4){
				out[i*8+0] = out[i*8+0] | ((msglen[i]>>7) & 1);
				out[i*8+1] = out[i*8+1] | ((msglen[i]>>6) & 1);
				out[i*8+2] = out[i*8+2] | ((msglen[i]>>5) & 1);
				out[i*8+3] = out[i*8+3] | ((msglen[i]>>4) & 1);
				out[i*8+4] = out[i*8+4] | ((msglen[i]>>3) & 1);
				out[i*8+5] = out[i*8+5] | ((msglen[i]>>2) & 1);
				out[i*8+6] = out[i*8+6] | ((msglen[i]>>1) & 1);
				out[i*8+7] = out[i*8+7] | ( msglen[i] & 1);
			}else{
				out[i*8+0] = out[i*8+0] | ((secret[i-4]>>7) & 1);
				out[i*8+1] = out[i*8+1] | ((secret[i-4]>>6) & 1);
				out[i*8+2] = out[i*8+2] | ((secret[i-4]>>5) & 1);
				out[i*8+3] = out[i*8+3] | ((secret[i-4]>>4) & 1);
				out[i*8+4] = out[i*8+4] | ((secret[i-4]>>3) & 1);
				out[i*8+5] = out[i*8+5] | ((secret[i-4]>>2) & 1);
				out[i*8+6] = out[i*8+6] | ((secret[i-4]>>1) & 1);
				out[i*8+7] = out[i*8+7] | ((secret[i-4]) & 1);
			}
		}
	}
}

void decode(char* in, char* secret, char* msglen, size_t worklen){
	for(size_t i = 0; i < worklen; i++){
		if(i < 4){
			msglen[i] = msglen[i] | ((in[i*8+0] & 1) << 7);
			msglen[i] = msglen[i] | ((in[i*8+1] & 1) << 6);
			msglen[i] = msglen[i] | ((in[i*8+2] & 1) << 5);
			msglen[i] = msglen[i] | ((in[i*8+3] & 1) << 4);
			msglen[i] = msglen[i] | ((in[i*8+4] & 1) << 3);
			msglen[i] = msglen[i] | ((in[i*8+5] & 1) << 2);
			msglen[i] = msglen[i] | ((in[i*8+6] & 1) << 1);
			msglen[i] = msglen[i] | ( in[i*8+7] & 1);
		}else{
			secret[i-4] = 0;
			secret[i-4] = secret[i-4] | ((in[i*8+0] & 1) << 7);
			secret[i-4] = secret[i-4] | ((in[i*8+1] & 1) << 6);
			secret[i-4] = secret[i-4] | ((in[i*8+2] & 1) << 5);
			secret[i-4] = secret[i-4] | ((in[i*8+3] & 1) << 4);
			secret[i-4] = secret[i-4] | ((in[i*8+4] & 1) << 3);
			secret[i-4] = secret[i-4] | ((in[i*8+5] & 1) << 2);
			secret[i-4] = secret[i-4] | ((in[i*8+6] & 1) << 1);
			secret[i-4] = secret[i-4] | ( in[i*8+7] & 1);
		}
	}
}

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
	char* outimage = (char*)malloc(imgsize);
	const size_t aspace = ((imgsize)/8)-4;
	size_t secretlen = 0;
	char encoding = 0;
	char secret[aspace+EXTRABUF];
	timeval start, end;
	
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


	size_t worklen = aspace+4;
	printf("Worklen: %lu\n", worklen);
	
	gettimeofday(&start, NULL);
	if(encoding)encode(image, outimage, secret, (char*)&secretlen, worklen);
	else decode(image, secret, (char*)&secretlen, worklen);
	gettimeofday(&end, NULL);
	if(!raw)printf("Time: %lf\n",  timediff(&start, &end));
	if(encoding){
		SOIL_save_image("done.bmp", SOIL_SAVE_TYPE_BMP, width, height, 3, outimage);
		free(outimage);
	}else{
		if(raw){
			for(size_t i=0; i < secretlen; i++){
				printf("%c",secret[i]);
			}
		}else printf("Finished string: %s\nFinished stringlen: %ld\n", secret, secretlen);
	}

	return 0;
}
