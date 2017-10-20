#include <cstdio>
#include <cstdlib>
#include <ctype.h>

#include <cstring>

#include "SDL.h"
#include "CL/cl.h"

using namespace std;

//I use it in order to have compability with GCC version
#define uint8_t unsigned __int8																	
#define Uint32 unsigned __int32

//Used in moving the graph 20x20 pixel down-right
#define XEXTRA 20																				
#define YEXTRA 20
#define XOFFDEF 20
#define YOFFDEF 20

void printer(SDL_Surface* screen,int xsize,int ysize, int xoffset,int yoffset, uint8_t* table);

const char *source =																			//The OpenCL code, if you can move it to a file, bariemai!
"__kernel void nextgen1(__global uchar *oldtable, __global uchar *newtable,	    \n"				//This code find the next generation off ONE living cell
"						__global short2	  *todo,								\n"
"						int	xsize, int ysize)									\n"
"{																				\n"
"	short counter = 0;															\n"
"	short todox = todo[get_global_id(0)].x;										\n"				//get_global_id(0) gives which (1st, 2nd... 10000th) living cell we calculate
"	short todoy = todo[get_global_id(0)].y;										\n"
"	int y = todoy*(xsize+2);													\n"
"	switch (*(oldtable + y + todox)) {											\n"				//Those branches maybe hurt performance
"		case 1:																	\n"				//Check .pdf in order to understand what each number mean
"			*(newtable + y + todox) = 2;										\n"
"			break;																\n"
"		case 2:																	\n"
"			*(newtable + y + todox) = 3;										\n"
"			break;																\n"
"		case 3:																	\n"
"			counter += (*(oldtable + y + todox - 1) == 1);						\n"
"			counter += (*(oldtable + y + todox + 1) == 1);						\n"
"			counter += (*(oldtable + y - xsize + todox - 2) == 1);				\n"
"			counter += (*(oldtable + y + xsize + todox + 2) == 1);				\n"
"			counter += (*(oldtable + y - xsize + todox - 3) == 1);				\n"
"			counter += (*(oldtable + y - xsize + todox - 1) == 1);				\n"
"			counter += (*(oldtable + y + xsize + todox + 1) == 1);				\n"
"			counter += (*(oldtable + y + xsize + todox + 3) == 1);				\n"
"																				\n"
"			if (counter == 1 || counter == 2) *(newtable + y + todox) = 1;		\n"
"			else *(newtable + y + todox) = 3;									\n"
"																				\n"
"			break;																\n"
"		}																		\n"
"}																				\n"
"																				\n"
"__kernel void nextgen2(__global uchar *oldtable, __global uchar *newtable,	    \n"				//This code find the next generation off ONE living cell
"						__global short2	  *todo,								\n"
"						int	xsize, int ysize)									\n"
"{																				\n"
"	short counter = 0;															\n"
"	short todox = todo[get_global_id(0)].x;										\n"				//get_global_id(0) gives which (1st, 2nd... 10000th) living cell we calculate
"	short todoy = todo[get_global_id(0)].y;										\n"
"	int y = todoy*(xsize+2);													\n"
"	switch (*(oldtable + y + todox)) {											\n"				//Those branches maybe hurt performance
"		case 1:																	\n"				//Check .pdf in order to understand what each number mean
"			*(newtable + y + todox) = 2;										\n"
"			break;																\n"
"		case 2:																	\n"
"			*(newtable + y + todox) = 3;										\n"
"			break;																\n"
"		case 3:																	\n"
"			counter += (*(oldtable + y + todox - 1) == 1);						\n"
"			counter += (*(oldtable + y + todox + 1) == 1);						\n"
"			counter += (*(oldtable + y - xsize + todox - 2) == 1);				\n"
"			counter += (*(oldtable + y + xsize + todox + 2) == 1);				\n"
"			counter += (*(oldtable + y - xsize + todox - 3) == 1);				\n"
"			counter += (*(oldtable + y - xsize + todox - 1) == 1);				\n"
"			counter += (*(oldtable + y + xsize + todox + 1) == 1);				\n"
"			counter += (*(oldtable + y + xsize + todox + 3) == 1);				\n"
"																				\n"
"			if (counter == 1 || counter == 2) *(newtable + y + todox) = 1;		\n"
"			else *(newtable + y + todox) = 3;									\n"
"																				\n"
"			break;																\n"
"		}																		\n"
"}																				\n"
"																				\n";


struct pixelcolor																				//RGB struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

void putpixel(SDL_Surface* screen, int x, int y,struct pixelcolor color){						//Fills one pixel of the window, dont try understand it
	Uint32* pixels=(Uint32*) screen->pixels;
	Uint32* pixel=pixels+y*screen->pitch/4+x;
	*pixel = SDL_MapRGB(screen->format,color.r,color.g,color.b);
}


SDL_Event event;																				//All SDL_shit is about the window
SDL_Surface* screen;
SDL_Window* window;

int main(int argc, char *argv[]) {

	FILE *inputfile;
	fopen_s(&inputfile, argv[1], "r");															//1st argument is the .rle file
	
	fseek(inputfile, 0, SEEK_END);
	long fsize = ftell(inputfile);
	fseek(inputfile, 0, SEEK_SET);
	
	int refreshevery;
	if(argc==3) refreshevery=strtol(argv[2],NULL,10);
	else refreshevery=1000;

	char temp[100];
	int xoffset=0;
	int yoffset=0;
	
	int xsize=0;
	int ysize=0;
	
	for(;;){																					//Start reading the file
		fgets(temp,100,inputfile);
		char* ptrnums;
		if ((temp[0]=='#') && (strncmp(&temp[1],"CXRLE",5)==0)){								//Finds where the living cells start and how big is MOSTLY pattern

			ptrnums = strchr(temp,'=');
			
			xoffset=strtol(++ptrnums,&ptrnums,10) - 1;
			if (xoffset<0) xoffset=XOFFDEF;
			yoffset=strtol(++ptrnums,&ptrnums,10) - 1;
			if (yoffset<0) yoffset=YOFFDEF;
			
		}
		else if(temp[0]!='#'){
			ptrnums = strchr(temp,'=');
			
			xsize= strtol(++ptrnums,&ptrnums,10);
			
			ptrnums = strchr(ptrnums,'=');
			
			ysize= strtol(++ptrnums,&ptrnums,10);
					
			break;
		}
	}
	
	char* drawinfo= new char [fsize/sizeof(char)];												//Preparing memory anus
	memset(drawinfo,0,fsize);
	
	while(fgets(temp,100,inputfile) != NULL){
		strncat(drawinfo,temp,strlen(temp)-1);
	}
	
	fclose(inputfile);

	int size = (xsize + 2)*(ysize + 2);
	uint8_t* table = new uint8_t[size];
	uint8_t* helptable = new uint8_t[size];

	
	memset(table,0,size);																		//Into the main part of the program, at least at CPU verisons, "table" would hold the old generation and the "helptable" the new one-upcoming
	memset(helptable,0,size);
	
	char* ptr=drawinfo;
	int inarow;
	int value;
	
	int xpos=1;
	int ypos=1;
	
	int howmany=0;																				//At the following WHILE we will find how many cells are the the living ones (aka "not black")

	SDL_Log("Starting decoding .rle file\n");													//Starts decoding the rest of the file into "table". At .pdf there is a link about how to read .rle files
																								//btw due to shity SDL in order to printout you will need to use SDL_LOG. Syntax like C printf
	while(*ptr!='!'){
		inarow=1;
		if(isdigit(*ptr)){
			inarow=strtol(ptr,&ptr,10);
		}
		if(*ptr!='$'){
			switch (*ptr){
				case '.':
					value=0;
					break;
				case 'A':
					value=1;
					howmany+=inarow;
					break;
				case 'B':
					value=2;
					howmany+=inarow;
					break;
				case 'C':
					value=3;
					howmany+=inarow;
					break;
			}
			int i;
			for(i=0;i<inarow;i++){
				*(table+ypos*(xsize+2)+xpos++)=value;
				if(xpos>xsize+1){
					ypos++;
					xpos=1;
				} 
			}
		}
		else{
			xpos=1;
			ypos++;
		}
		ptr++;
	}
	
	xpos=1;
	ypos=1;

	free(drawinfo);																				//Finally we have all the pattern as a matrix in our memory

	SDL_Log("Population: %d\n", howmany);														

	SDL_Log("Creating window\n");																//Start creating window (cpt. Obvious)

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("WirePrime", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, xsize + 2 + xoffset + XEXTRA, ysize + 2 + yoffset + YEXTRA, SDL_SWSURFACE);

	screen = SDL_GetWindowSurface(window);

	SDL_Log("Initializing OpenCL device");

	cl_int ret;																					//Usefull to debug. Check https://streamhpc.com/blog/2013-04-28/opencl-error-codes/ for each code

	cl_platform_id platform;																	// 1. Get a platform.
	clGetPlatformIDs(1, &platform, NULL);

	cl_device_id device;																		// 2. Find a gpu device.
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
	cl_uint compute_units;
	clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &compute_units, NULL);//Great way to know how many compute units your GPU have.

	cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);					// 3. Create a context and command queue on that device.
	cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);

	cl_program program = clCreateProgramWithSource(context, 1, &source, NULL, NULL);			// 4. Perform runtime source compilation, and obtain kernel entry point.
	ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

	if (ret != CL_SUCCESS)																		//Copypasta this shit in order to get debug infos. This one give us the build infos/debug errors
	{
		printf("clBuildProgram failed: %d\n", ret);
		char buf[0x10000];
		clGetProgramBuildInfo(program,
			device,
			CL_PROGRAM_BUILD_LOG,
			0x10000,
			buf,
			NULL);
		SDL_Log("\n%s\n", buf);
		return(-1);
	}

	cl_kernel nextgen1 = clCreateKernel(program, "nextgen1", NULL);
	cl_kernel nextgen2 = clCreateKernel(program, "nextgen2", NULL);

	SDL_Log("Compute units: %d\n", compute_units);												//5. Calculating optimal global work size and faking cells in order to make livingcell *time* global work size

	size_t ws = 64;																				// Every work group (according to AMD) is better to have 64,128,192 or 256 items
	size_t min_global_work_size = compute_units * 7 * ws;										// 7 is just a magic number. It is how many wavefronts a SIMD unit should calculate in parallel. At nvidia manuals this number may vary
	size_t global_work_size = min_global_work_size;												//Ok now this shit is complicated. Living cells should be N times global_work_size in order all the compute units work in parallel

	size_t local_work_size = ws;

	SDL_Log("Default global work size: %d\n", min_global_work_size);

	if(howmany%global_work_size)																//So.. this trick will help as find a number x, so (A+x)=n*B , n E N. A = howmany(living cells), B = min_global_work_size and x = the fake cells (can be BLACK or living-NOT BLACK)
		global_work_size = howmany + (global_work_size - howmany%global_work_size);				

	short* todo = new short[global_work_size *2];													//Creating a array of the positions at the pattern (NOT ADDRESS!) of all living cells
	
	SDL_Log("Optimized fake population and final global work size: %d\n", global_work_size);
	SDL_Log("Starting populating list of all living cells (and the fake ones)\n");

	int a=0;																					//Filling that array
	int b=0;
	int found=0;
	for(a=0;a<ysize+2;a++){
		for(b=0;b<xsize+2;b++){
			if(*(table + a*(xsize + 2) + b) !=0){
				*(todo+found*2) = b;
				*(todo+found*2+1) = a;
				found++;
			}
		}
	}
	for (int i = howmany; i < global_work_size; i++) {											//Filling the added the fake cells with the the position off the cell down-right
		*(todo + i*2) = xsize + 1;																//we could take any cell (living or not) but I chose the last one in order to minimize memory latency since the last calculated living cell will be almost next to it. Ofcourse we can optimize this
		*(todo + i*2 + 1) = ysize + 1;
	}

	SDL_Log("Rendering generation: 1\n");
	printer(screen,xsize,ysize,xoffset,yoffset,table);											//Yeahh.. this shit render a generation on the window.

	SDL_Log("Creating GPU's memory buffers");													//cpt. obvious

	// 5. Create a data buffer.

	cl_mem table_buf		= clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR , size * sizeof(uint8_t)    , table    , NULL);					
	cl_mem helptable_buf	= clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR , size * sizeof(uint8_t)    , helptable, NULL);
	cl_mem todo_buf			= clCreateBuffer(context, CL_MEM_READ_ONLY	| CL_MEM_COPY_HOST_PTR , global_work_size * sizeof(short) * 2 , todo     , NULL);			//We gota check if todo list can be kinda local so access to it will be faster. Even make it SHORT type instead of INT (we can for primes.rle) will help GPU to cache it better (maybe)

	// 6. Set default kernel arguments.

	clSetKernelArg(nextgen1, 0, sizeof(void *), (void*)&table_buf);								//Passing arguments to kernel
	clSetKernelArg(nextgen1, 1, sizeof(void *), (void*)&helptable_buf);
	clSetKernelArg(nextgen1, 2, sizeof(void *), (void*)&todo_buf);
	clSetKernelArg(nextgen1, 3, sizeof(xsize), (void*)&xsize);
	clSetKernelArg(nextgen1, 4, sizeof(ysize), (void*)&ysize);


	clSetKernelArg(nextgen2, 0, sizeof(void *), (void*)&helptable_buf);								
	clSetKernelArg(nextgen2, 1, sizeof(void *), (void*)&table_buf);
	clSetKernelArg(nextgen2, 2, sizeof(void *), (void*)&todo_buf);
	clSetKernelArg(nextgen2, 3, sizeof(xsize), (void*)&xsize);
	clSetKernelArg(nextgen2, 4, sizeof(ysize), (void*)&ysize);

	cl_event ev;
	
	bool done=false;
	int generation=1;

	while(!done) {																				//HERE WE ARE BOYZ! THE HOTTEST LOOP. Every off <<--- this WHILE happen every "refreshevery"(<inited at the start of main) generations.
		while(SDL_PollEvent(&event) != 0) {														//Check if the user *clicked* to close the program (yeahh thats why the program at big "refreshevery" is laggy. Optimale window should be a another process
			if (event.type == SDL_QUIT) {
				done=true;
			}
		}

		int i;
		for(i=0;i<refreshevery;i+=2){															//Dont kill me. This shit has BIG overhead. i+=2 means that we calculated 2 generation in the following loop

			clEnqueueNDRangeKernel(queue,														//Calculate next generation.
				nextgen1,																		//Google this shit. For me it was copy pasta from a example
				1,
				NULL,
				&global_work_size,
				&local_work_size,
				0, NULL, &ev);																	//One of this settings it is vital in order finish this kernel call BEFORE calling the next one

			clEnqueueNDRangeKernel(queue,														//Calculate one more generation so the again the table_buf will have the latest.
				nextgen2,
				1,
				NULL,
				&global_work_size,
				&local_work_size,
				0, NULL, &ev);
		}

		SDL_Log("Reading buffer\n");															//After "refreshevery" we render the latest generation
		// 8. Look at the results via synchronous buffer map.

		uint8_t* ptr_table = (uint8_t*) clEnqueueMapBuffer(queue,
			table_buf,
			CL_TRUE,
			CL_MAP_READ,
			0,
			size * sizeof(uint8_t),
			0, NULL, NULL, NULL);

		generation +=refreshevery;																//Tell at the command line which generation we will calculate
		SDL_Log("Rendering generation: %d\n",generation);
		printer(screen,xsize,ysize,xoffset,yoffset,ptr_table);									//Print itt!
	}
	
	SDL_Quit();
	return 0;
}

void printer(SDL_Surface* screen,int xsize,int ysize, int xoffset,int yoffset, uint8_t* table){	//This is the function that renders. I dont give a shit about optimizing it.
	struct pixelcolor orange;																	//You can change colors with these structs
	orange.r=255;
	orange.g=127;
	orange.b=80;
	
	struct pixelcolor blue;
	blue.r=0;
	blue.g=0;
	blue.b=255;
	
	struct pixelcolor white;
	white.r=255;
	white.g=255;
	white.b=255;	
	
	int i;
	int size=(xsize+2)*(ysize+2);
	for(i=1;i<=size;i++){
		switch (*(table+i)){
			case 1:
				putpixel(screen,i%(xsize+2)+xoffset,i/(xsize+2)+yoffset,blue);
				break;
			case 2:
				putpixel(screen,i%(xsize+2)+xoffset,i/(xsize+2)+yoffset,white);
				break;
			case 3:
				putpixel(screen,i%(xsize+2)+xoffset,i/(xsize+2)+yoffset,orange);
				break;
		}
	}
	SDL_UpdateWindowSurface(window);														//Updates the window.
}

//Thats all Lami!!! :D <3