#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "genetic.h"

// counts the number of 1s in the binary representation of a number
int fitness(int a){
	int ans=0;

	// checks the last digit in the number and then shifts it down one place
	while(a>0){
		ans+=a&1;
		a=a>>1;
	}
	return ans;
}

// Loops through the pop and updates fitnesses and finds the total fitness
void calc_total_f(){
	total_fitness=0;		// resets total fitness
	int i,j;
	int fitness_counter;

	for(i=0;i<popsize;i++){
		fitness_counter=0;

		// loops through each chunk and calculates the fitness at that point
		for(j=0;j<no30s;j++){
			fitness_counter+=fitness(pop[(no30s+1)*i+j]);
		}
		pop[(no30s+1)*i+no30s]=fitness_counter;	// stores individual fitness
		total_fitness+=fitness_counter;		// adds to total fitness
	}
}

int main(int argc, char *argv[]){

	// sets default values for program
	no1s=10;
	popsize=100;
	int generations=100;
	int force_print=0;
	double c_rate=0.9;
	double m_rate=0.01;
	char* end;
	int seeded=0;

	// parses command line arguments if specified
	int opt;
	while((opt=getopt(argc,argv,"c:m:g:n:p:fs"))!=-1){
		switch(opt){
			case 'c':
				c_rate=	strtod(optarg,&end);
				break;
			case 'm':
				m_rate=	strtod(optarg,&end);
				break;
			case 'g':
				generations=atoi(optarg);
				break;
			case 'n':
				no1s=atoi(optarg);
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
				fprintf(stderr,"Usage: %s [-c crossover_rate] [-m mutate_rate] [-g num_generations] [-n string_length] [-p pop_size] [-fs]\n",argv[0]);
				exit(EXIT_FAILURE);
		}
	}
	
	// seeds the program at 1 if requested, else randomises
	if(seeded==1){srand48(1);}
	else{srand48(time(NULL));}

	genetic_assign();	// creates an initial population
	calc_total_f();		// calculates initial fitness

	// sets up output file for graphing purposes		
	FILE *result;
	if((result=fopen("result.dat","w"))==NULL){
		printf("*** ERROR 2 ***\nCouldn't open output file\n");
		exit(2);
	}

	// prints intial population (if small enough or forced)
	printf("Intial population\n");
	print_pop(force_print);

	int i;
	// loops for a specified number of generations
	for(i=0;i<generations;i++){
		new_generation(c_rate,m_rate);
		
		// prints gen and total_fitness to output file
		fprintf(result,"%d %d\n",i+1,total_fitness);
	}
	fclose(result);

	// prints final population if <10 or forces
	printf("Final Population\n");
	print_pop(force_print);

	printf("Expected Optimal Total Fitness: %d\n",popsize*no1s);
	
	genetic_free();		// frees up memory used
	return 0;
}
