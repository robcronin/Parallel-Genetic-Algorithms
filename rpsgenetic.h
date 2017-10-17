/*		
 		(MODIFIED FOR ROCK PAPER SCISSORS  - 3 strategies)
		HEADER FILE WITH FUNCTIONS REQUIRED FOR GENETIC ALGORITHM
		To use:
		- Create a "void calc_total_f()" function that assigns fitness values to:
				pop[(no30s+1)*i+no30s]
			and also updates "total_fitness"
		- Set "popsize", "no1s" & "seeded" as required in main
		- Call "genetic_assign()"
		- Call "calc_total_f()"
		- Call "new_generation([c_rate],[m_rate])" to update the population
		- At the end call "genetic_free()"
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

// Global variables
int popsize=100, *pop, *pop_new, total_fitness;

void calc_total_f();


void genetic_assign(){
 	 // since we use two parents to create two offspring we require an even popsize
	if(popsize%2==1){
	popsize++;
                printf("*** WARNING ***\nPopultion needs to be even\nUsing pop_size=%d\n\n\n",popsize);
        }

        // allocates memory for pop and pop_new
        pop=malloc(2*popsize*sizeof(int));
        pop_new=malloc(2*popsize*sizeof(int));

	int i;
        // randomly generates a population
        for(i=0;i<popsize;i++){
		pop[2*i]=drand48()*3;
        }
}

void genetic_free(){
	free(pop);
	free(pop_new);
}


// Print function that prints the population 
void print_pop(int force){

	// only prints by default if popsize is small, but can be overruled by user
	if(popsize<=10 || force==1){
		int i;
		for(i=0;i<popsize;i++){
			printf("%d %d\n",pop[2*i],pop[2*i+1]);
		}
	}
	printf("Total fitness is %d\n\n",total_fitness);
}

// mutates the strategy at a given index at a given point
void mutate(int i){
	int strat;
	do{
		strat=3*drand48();
	}while(strat==pop[2*i]);
	pop[2*i]=strat;
}


// Returns an index of a sample randomly drawn via fitness
int sample(){
	int index=drand48()*total_fitness+1;	// random number between 1 and total_fitness
	int sum=pop[1];				// starts at fitness of first entry
	int ans=0;

	// loops through to find where index slots in to the cumulative fitness
	while(index>sum){
		ans+=2;
		sum+=pop[ans+1];
	}
	return ans;	// returns the randomly drawn index
}

// uses the current generation to populate a new generation for given mutate rates
void new_generation(double m_rate){
	int i;
	int indexa, indexb;
	int counter;

	// loops through the population in twos
	for(i=0;i<popsize;i+=2){
		indexa=sample();	// finds the first sample
		counter=0;

		// finds a second sample (different to the first)
		do{
			indexb=sample();
			counter++;
		}while(indexa==indexb && counter<100);

		// if after 100 attempts it can't find a different index, it checks whether there is more than one suitable index and either exits if so or continues the loop if it is possible to find another index
		if(indexa==indexb){
			counter=0;
			i=0;

			//checks how many entries have >0 fitness (require at least 2)
			while(counter<2 && i<popsize){
				if(pop[2*i+1]>0){counter++;}
				i++;
			}

			// if less than 2 it uses a random 0 fitness index
			if(counter<2){
				do{
					indexb=popsize*drand48();
					indexb*=2;
				}while(indexa==indexb);
			}

			// otherwise keeps searching
			else{
				do{
					indexb=sample();
				}while(indexa==indexb);
			}
		}

		// Now two samples have been chosen to follow into the next gen (no crossover for RPS)
		pop_new[2*i]=pop[indexa];
		pop_new[2*i+2]=pop[indexb];
	}

	// swaps pointers so that pop is updated
	int *t;
	t=pop_new;
	pop_new=pop;
	pop=t;

	// loops through the population and mutates each entry with probability m_rate
	for(i=0;i<popsize;i++){
		if(drand48()<m_rate){
			mutate(i);
		}
	}

	calc_total_f();		// updates fitnesses and total_fitness
}
