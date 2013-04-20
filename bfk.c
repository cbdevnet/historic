/*
Brainfuck Interpreter
[CodeBlue]

07.11.2009

USAGE:
bfk <codefile> [v]
*/

#include <stdbool.h>
#include <stdio.h>

#define STREAMLEN 1024
#define MAXLOOPS 100

FILE *fp;
int bf_stream[STREAMLEN];
bool debug=false;
int location=0;
int recursion=0;
fpos_t loop_begin[MAXLOOPS];

int help(){
	printf("[CodeBlue] Brainfuck Interpreter 0.2\n");
	printf("Usage:\n");
	printf("\tbfk <inputfile> [v]");
	return 1;
}

int initStream(){
	for(int i=0;i<STREAMLEN;i++){
		bf_stream[i]=0;
	}
	if(debug){
		printf("[OK]\tStream Init\n");
	}
}

int bfParse(int command){
	switch(command){
		case '>':
			location++;
			break;
		case '<':
			location--;
			break;
		case '+':
			bf_stream[location]++;
			break;
		case '-':
			bf_stream[location]--;
			break;
		case '.':
			putchar(bf_stream[location]);
			break;
		case ',':
			bf_stream[location]=getchar();
			break;
		case '[':
			int cur_rec=recursion;
			recursion++;
			fgetpos(fp,&loop_begin[recursion]);
			if(bf_stream[location]==0){
				//find end of loop
				if(debug){printf("[Msg]\tEmpty Loop\n");}
				while(!feof(fp)&&recursion>cur_rec){
					int a=fgetc(fp);
					if(a=='['){recursion++;}
					if(a==']'){recursion--;}
				}
			}
			else{
				if(debug){printf("[Msg]\tOpening Loop (Depth: %d)\n",recursion);}
				while(!feof(fp)&&recursion>cur_rec){
					int c=fgetc(fp);
					bfParse(c);
				}
			}
			break;
		case ']':
			if(bf_stream[location]==0){
				if(debug){printf("[Msg]\tClosing Loop (Depth: %d)\n",recursion);}
				recursion--;
			}
			else{
				fsetpos(fp,&loop_begin[recursion]);
			}
			break;
	}
}

int main(int argc, char *argv[]){
	
	if(argc==1){
		exit(help());
	}
	
	fp=fopen(argv[1], "r");
	if(fp==NULL){
		exit(help());
	}
	
	for(int i=2;i<argc;i++){
		if(!strcmp(argv[i],"v")){debug=true;}
	}
	
	initStream();
	
	if(debug){
		printf("[OK]\tInput File\n--------------------\n");
	}

	while(!feof(fp)){
		int current=fgetc(fp);
		bfParse(current);
	}
}
