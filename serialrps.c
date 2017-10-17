#include <stdio.h>
#include <stdlib.h>
#include "rpsgenetic.h"

//Global variables
int seeded;
int rf, sf, pf;
int r, s, p;

// Rock=2, Scissors=1, Paper=0
void calc_total_f(){

	int i;
	r=0,s=0, p=0;
	for(i=0;i<popsize;i++){
		if(pop[2*i]==0){p++;}
		else if(pop[2*i]==1){s++;}
		else if(pop[2*i]==2){r++;}
	}
	rf=(r-1)*2+10*s;
	sf=(s-1)*2+10*p;
	pf=(p-1)*2+10*r;
	total_fitness=r*rf+s*sf+p*pf;
	for(i=0;i<popsize;i++){
		if(pop[2*i]==0){pop[2*i+1]=pf;}
		else if(pop[2*i]==1){pop[2*i+1]=sf;}
		else if(pop[2*i]==2){pop[2*i+1]=rf;}
	}

}


int main(int argc, char *argv[]){
	// sets default values for program	
	popsize=100;
	int generations=100;	
	int force_print=0;
	double m_rate=0.01;
	seeded=0;
	char* end;
	// parses command line arguments if specified
	int opt;
	while((opt=getopt(argc,argv,"m:g:p:fsit:a:"))!=-1){
		switch(opt){
			case 'm':
				m_rate=	strtod(optarg,&end);
				break;
			case 'g':
				generations=atoi(optarg);
				break;
			case 'p':
				popsize=atoi(optarg);
				break;
			case 'f':
				force_print=1;
				break;
			case 's':
				seeded=1;
				break;
			default:
					fprintf(stderr,"Usage: %s [-m mutate_rate] [-g num_generations] [-p pop_size] [-fs]\n",argv[0]);
					exit(EXIT_FAILURE);
		}
	}
	if(seeded==0){srand48(time(NULL));}
	else{srand48(1);}

	int i;

	// assign population variables to everyone
	genetic_assign();

	
	// sets up 3 output files for graphing purposes
	FILE *rock, *scissor, *paper;
	if((rock=fopen("rock.dat","w"))==NULL){
		printf("*** ERROR 2 ***\nCouldn't open output file\n");
		exit(2);
	}
	if((scissor=fopen("scissor.dat","w"))==NULL){
		printf("*** ERROR 2 ***\nCouldn't open output file\n");
		exit(2);
	}
	if((paper=fopen("paper.dat","w"))==NULL){
		printf("*** ERROR 2 ***\nCouldn't open output file\n");
		exit(2);
	}

	// Calculates initial fitness and prints to screen
	calc_total_f();
	printf("Intial Population\n");
	print_pop(force_print);
	
	// loops through geneerations
	for(i=0;i<generations;i++){
		new_generation(m_rate);
		fprintf(rock,"%d %d\n",i+1,rf);
		fprintf(scissor,"%d %d\n",i+1,sf);
		fprintf(paper,"%d %d\n",i+1,pf);
	}

	// prints out final population
	printf("Final Population\n");
	print_pop(force_print);

	fclose(rock);fclose(scissor);fclose(paper);
	
	// Expected total fitness is between the truly random case where the expected value is 4 and the case where on strategy dominates for a generation dragging the expected value towards 2 (a draw)
	printf("Expcted Total Fitness between  %d and %d\n",2*(popsize-1)*popsize,4*(popsize-1)*popsize);
	genetic_free();
	return 0;
}
