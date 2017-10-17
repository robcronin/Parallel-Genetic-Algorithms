/*		
		HEADER FILE WITH FUNCTIONS REQUIRED FOR GENETIC ALGORITHM
		To use:
		- Create a "void calc_total_f()" function that assigns fitness values to:
				pop[(no30s+1)*i+no30s]
			and also updates "total_fitness"
		- Set "popsize" & "no1s" as required in main
		- Call "genetic_assign()"
		- Call "calc_total_f()"
		- Call "new_generation([c_rate],[m_rate])" to update the population
		- At the end call "genetic_free()"
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

// Global variables
int no1s=10, no30s, popsize=100, *pop, *pop_new, total_fitness, leftover;
int no=30;	// size of chunks - don't set above 30
void calc_total_f();


void genetic_assign(){
 	 // since we use two parents to create two offspring we require an even popsize
	if(popsize%2==1){
	popsize++;
                printf("*** WARNING ***\nPopultion needs to be even\nUsing pop_size=%d\n\n\n",popsize);
        }

        // calculates how many chunks are needed and the max int size for each type of chunk
        no30s=no1s/no+1;
        leftover=no1s%no;
        if(leftover==0){no30s--;leftover=no;}
        int maxint30 = pow(2,no);
        int maxint = pow(2,leftover);

        // allocates memory for pop and pop_new
        pop=malloc(2*(no30s+1)*popsize*sizeof(int));
        pop_new=malloc(2*(no30s+1)*popsize*sizeof(int));

	int i,j;
        // randomly generates a population based on maxint sizes
        for(i=0;i<popsize;i++){
                for(j=0;j<(no30s-1);j++){
                        pop[(no30s+1)*i+j+1]=drand48()*maxint30;
                }
                pop[(no30s+1)*i]=drand48()*maxint;
        }
}

// function to free up memory
void genetic_free(){
	free(pop);
	free(pop_new);
}


// function to print a number as binary
void print_bin(int a, int places){
	int comp=1<<(places-1);		// creates a "binary" number with one 1 at the highest place
	
	// loops that 1 down through the places and checks if there is a 1 in the number to be printed
	while(comp>0){
		if((comp&a)>0){printf("1");}
		else{printf("0");}
		comp=comp>>1;
	}
	printf("\t");
}

// Print function that prints the population 
void print_pop(int force){

	// only prints by default if popsize is small, but can be overruled by command line arg [-f]
	if(popsize<=10 || force==1){
		int i,j;
		for(i=0;i<popsize;i++){
			print_bin(pop[(no30s+1)*i],leftover);
			for(j=1;j<no30s;j++){
				print_bin(pop[(no30s+1)*i+j],no);
			}
			printf("%u\n",pop[(no30s+1)*i+no30s]);
		}
	}
	printf("Total fitness is %d\n\n",total_fitness);
}

// Does a crossover at two given indexes, at a given crossover point and puts the result in pop_new at another given index (i)
void crossover(int a, int b, int i, int crosspoint){
	int as, ae, bs, be;
	int index=no30s-1;
	
	// goes chunk by chunk (via decreasing "index")

	// swaps over the chunks after the crossover point
	while(crosspoint>=no){
		pop_new[(no30s+1)*i+index]=pop[b+index];
		pop_new[(no30s+1)*(i+1)+index]=pop[a+index];
		crosspoint-=no;
		index--;
	}

	// does the crossover in the correct chunk
	int across=pop[a+index], bcross=pop[b+index];
	as=across>>crosspoint;	
	as=as<<crosspoint;	// finds the start of a (before crossover)
	ae=across-as;		// and the end
	bs=bcross>>crosspoint;
	bs=bs<<crosspoint;
	be=bcross-bs;		// likewise for b

	// swaps over the end bits
	across=as+be;
	bcross=bs+ae;
	
	// stores the new numbers
	pop_new[(no30s+1)*i+index]=across;
	pop_new[(no30s+1)*(i+1)+index]=bcross;

	index--;

	// copies the chunks before the crossover directly
	while(index>=0){
		pop_new[(no30s+1)*i+index]=pop[a+index];
		pop_new[(no30s+1)*(i+1)+index]=pop[b+index];
		index--;
	}
}


// mutates the chromosome at a given index at a given point (point indexed from 1 to no1s)
void mutate(int i, int mpoint){
	int index=no30s-1;

	// finds the chunk where the mutation occurs
	while(mpoint>30){
		mpoint-=30;
		index--;
	}
	int m=1;
	m=m << (mpoint-1);	// creates number with 1 at mutate point, 0s elsewhere
	pop[(no30s+1)*i+index]=pop[(no30s+1)*i+index]^m;	// use exclusive OR to mutate
}


// Returns an index of a sample randomly drawn via fitness
int sample(){
	int index=drand48()*total_fitness+1;	// random number between 1 and total_fitness
	int sum=pop[no30s];			// starts at fitness of first entry
	int ans=0;

	// loops through to find where index slots in to the cumulative fitness
	while(index>sum){
		ans+=no30s+1;
		sum+=pop[ans+no30s];
	}
	return ans;	// returns the randomly drawn index
}

// uses the current generation to populate a new generation for given crossover and mutate rates
void new_generation(double c_rate, double m_rate){
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

		// if after 100 attempts it can't find a different index, it checks whether there is more than one suitable index and either continues the loop if so or chooses a random index with fitness 0
		if(indexa==indexb){
			counter=0;
			i=0;

			//checks how many entries have >0 fitness (require at least 2)
			while(counter<2 && i<popsize){
				if(pop[(no30s+1)*i+no30s]>0){counter++;}
				i++;
			}

			// if less than 2 it uses a random 0 fitness index (i.e. any index other than indexa)
			if(counter<2){
				do{
					indexb=popsize*drand48();
					indexb*=(no30s+1);
				}while(indexa==indexb);
			}

			// otherwise keeps searching
			else{
				do{
					indexb=sample();
				}while(indexa==indexb);
			}
		}

		// Now two samples have been chosen to be the two parents
		// Checks if there is a crossover, otherwise copies the two parents
		if(drand48()<c_rate){
			// Note the crossover point is between [1,no1s-1] as at 0 or no1s there is effectively no crossover
			crossover(indexa,indexb,i,drand48()*(no1s-1)+1);
		}
		else{
			crossover(indexa,indexb,i,0);	// i.e. no crossover (but stores the values)
		}
	}

	// swaps pointers so that pop is updated
	int *t;
	t=pop_new;
	pop_new=pop;
	pop=t;

	// loops through the population and mutates each entry with probability m_rate
	for(i=0;i<popsize;i++){
		if(drand48()<m_rate){
			// random point from [1,no1s] due to indexing of mutate function
			mutate(i,no1s*drand48()+1);
		}
	}

	calc_total_f();		// updates fitnesses and total_fitness
}
