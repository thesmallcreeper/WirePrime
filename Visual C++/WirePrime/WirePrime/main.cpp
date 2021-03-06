#include <cstdio>
#include <cstdlib>
#include <ctype.h>

#include <cstring>

#include "SDL.h"

using namespace std;

#define uint8_t unsigned __int8
#define Uint32 unsigned __int32

#define XEXTRA 20
#define YEXTRA 20

#define XOFFDEF 20
#define YOFFDEF 20

void printer(SDL_Surface* screen,int xsize,int ysize, int xoffset,int yoffset, uint8_t* table);
void nextgen(uint8_t* oldtable,uint8_t* newtable,int* todo,int howmany,int xsize,int ysize);
void swap(uint8_t **str1_ptr, uint8_t **str2_ptr);

struct pixelcolor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

void putpixel(SDL_Surface* screen, int x, int y,struct pixelcolor color){
	Uint32* pixels=(Uint32*) screen->pixels;
	Uint32* pixel=pixels+y*screen->pitch/4+x;
	*pixel = SDL_MapRGB(screen->format,color.r,color.g,color.b);
}


SDL_Event event;
SDL_Surface* screen;
SDL_Window* window;

int main(int argc, char *argv[]) {

	FILE *inputfile;
	fopen_s(&inputfile, argv[1], "r");
	
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
	
	for(;;){
		fgets(temp,100,inputfile);
		char* ptrnums;
		if ((temp[0]=='#') && (strncmp(&temp[1],"CXRLE",5)==0)){

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
	
	char* drawinfo= new char [fsize/sizeof(char)];
	memset(drawinfo,0,fsize);
	
	while(fgets(temp,100,inputfile) != NULL){
		strncat(drawinfo,temp,strlen(temp)-1);
	}
	
	fclose(inputfile);

	int size = (xsize + 2)*(ysize + 2);
	uint8_t* table = new uint8_t[size];
	uint8_t* helptable = new uint8_t[size];

	
	memset(table,0,size);
	memset(helptable,0,size);
	
	char* ptr=drawinfo;
	int inarow;
	int value;
	
	int xpos=1;
	int ypos=1;
	
	int howmany=0;
	
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

	free(drawinfo);

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("WirePrime", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, xsize + 2 + xoffset + XEXTRA, ysize + 2 + yoffset + YEXTRA, SDL_SWSURFACE);

	screen = SDL_GetWindowSurface(window);

	int* todo = new int[howmany*2];
	

	int a=0;
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
	
	SDL_Log("Population: %d\n", howmany);

	SDL_Log("Rendering generation: 1\n");

	printer(screen,xsize,ysize,xoffset,yoffset,table);
	
	bool done=false;
	int generation=1;
	
	uint8_t* helpptr;
	uint8_t* ptrtable=table;									
	uint8_t* ptrhelptable=helptable;

	while(!done) {
		while(SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT) {
				done=true;
			}
		}

		int i;
		for(i=0;i<refreshevery;i++){
			nextgen(ptrtable,ptrhelptable,todo,howmany,xsize,ysize);
			swap(&ptrtable,&ptrhelptable);
		}
		generation +=refreshevery;
		SDL_Log("Rendering generation: %d\n",generation);
		printer(screen,xsize,ysize,xoffset,yoffset,ptrtable);
	}
	
	SDL_Quit();
	return 0;
}

void nextgen(uint8_t* oldtable,uint8_t* newtable,int* todo,int howmany,int xsize,int ysize){
	int i;
	int counter;
	int todox;
	int todoy;
	int y;
	for(i=0;i<howmany;i++){
		counter=0;
		todox=*(todo+i*2);
		todoy=*(todo+i*2+1);
		y=todoy*(xsize+2);
		switch (*(oldtable+y+todox)){
			case 1:
				*(newtable+y+todox)=2;
				break;
			case 2:
				*(newtable+y+todox)=3;
				break;
			case 3:
				if (*(oldtable+y+todox-1)==1) counter++;
				if (*(oldtable+y+todox+1)==1) counter++;
				if (*(oldtable+y-xsize+todox-2)==1) counter++;
				if (*(oldtable+y+xsize+todox+2)==1) counter++;
				if (*(oldtable+y-xsize+todox-3)==1) counter++;
				if (*(oldtable+y-xsize+todox-1)==1) counter++;
				if (*(oldtable+y+xsize+todox+1)==1) counter++;
				if (*(oldtable+y+xsize+todox+3)==1) counter++;
				
				if (counter==1 || counter==2) *(newtable+y+todox)=1;
				else *(newtable+y+todox)=3;
				
				break;
		}

	}
}

void swap(uint8_t **str1_ptr, uint8_t **str2_ptr){
  uint8_t *temp = *str1_ptr;
  *str1_ptr = *str2_ptr;
  *str2_ptr = temp;
}  


void printer(SDL_Surface* screen,int xsize,int ysize, int xoffset,int yoffset, uint8_t* table){
	struct pixelcolor orange;
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
	SDL_UpdateWindowSurface(window);
}
