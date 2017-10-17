#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "genetic.h"

// Global variables
int no_pdgames, hardcode;


// returns "time saved" from pd game for Player 1
int pd_payoff(int actions){
	// 1 represents Cooperate, 0 Defect
	// xx is binary representation of (Player 1, Player 2)
	//	eg (Coop, Defect) -> 10 in binary -> 2 in base 10 -> returns 0
	switch(actions){
		case 3:			// (Coop, Coop)
			return 3;
		case 2:			// (Coop, Defect)
			return 0;
		case 1:			// (Defect, Coop)
			return 5;
		case 0:			// (Defect, Defect)
			return 1;

		// default should never be called if coded correctly
		default:		
			printf("*** ERROR 3 ***\nIncorrect argument passed in to pd_payoff\n");
			exit(3);
	}
}

// runs the pd game between two players and adds the resulting payoff to their fitnesses and total_fitness
void pdgame(int i, int j){
	int fitnessi=0;
	int fitnessj=0;
	int pasti=0;
	int pastj=0;

	// Starting strats has either been hardcoded into start of chrmosome or is to be random
	int istrat, jstrat;
	if(hardcode==1){
		istrat = pop[(no30s+1)*i]>>16;
		jstrat= pop[(no30s+1)*i]>>16;
	}
	else{
		istrat=drand48()*4;
		jstrat=drand48()*4;
	}

	// Game 1 - runs the first game based off of starting strats and adds relevant fitnesses
	//		also adds history to the past variables as binary number st:
	//			past i = (P1 last)(P2 last)(P1 2games ago)(P2 2games ago)
	fitnessi+=pd_payoff(((istrat&1)<<1)+(jstrat&1));
	fitnessj+=pd_payoff(((jstrat&1)<<1)+(istrat&1));
	pasti+=((istrat&1)<<1)+(jstrat&1);	// adds game to the 2game ago part
	pastj+=((jstrat&1)<<1)+(istrat&1);

	// Game 2 - similarly for game 2
	fitnessi+=pd_payoff((((istrat&2)<<1)+(jstrat&2))/2);
	fitnessj+=pd_payoff((((jstrat&2)<<1)+(istrat&2))/2);
	pasti+=(((istrat&2)/2)<<3)+(((jstrat&2)/2)<<2);		// adds game to last game part
	pastj+=(((jstrat&2)/2)<<3)+(((istrat&2)/2)<<2);


	int games;
	int index;

	int ioverall=pop[(no30s+1)*i], joverall=pop[(no30s+1)*j];

	// loops through the rest of the games now that a history has been established
	for(games=2;games<no_pdgames;games++){

		index=1<<pasti;			// puts a 1 in the spot we're looking for 
		istrat=ioverall&index;		// compares against ioverall
		if(istrat>0){istrat=1;}		// converts to 1 if it matches
		
		index=1<<pastj;			// similarly for j
		jstrat=joverall&index;
		if(jstrat>0){jstrat=1;}
		
		// adds relevant fitnesses for given strategies
		fitnessi+=pd_payoff((istrat<<1)+jstrat);
		fitnessj+=pd_payoff((jstrat<<1)+istrat);

		// wipes the 2games ago part and shifts over the last game
		pasti=pasti>>2;
		pastj=pastj>>2;

		// adds the latest game in
		pasti+=(istrat<<3)+(jstrat<<2);
		pastj+=(jstrat<<3)+(istrat<<2);
	}
	
	// updates total_fitness and adds i and j's to their current fitness
	total_fitness+=fitnessi+fitnessj;
	pop[(no30s+1)*i+no30s]+=fitnessi;
	pop[(no30s+1)*j+no30s]+=fitnessj;
}

// Loops through the pop and updates fitnesses and finds the total fitness
void calc_total_f(){
	int i,j;

	// clears all fitness values first
	for(i=0;i<popsize;i++){pop[(no30s+1)*i+no30s]=0;}
	total_fitness=0;

	// then loops through a round robin tournament style between all strategies
	for(i=0;i<popsize-1;i++){
		for(j=i+1;j<popsize;j++){
			pdgame(i,j);
		}
	}
}


int main(int argc, char *argv[]){

	// sets default values for program
	no1s=18;
	popsize=100;
	int generations=100;
	int force_print=0;
	double c_rate=0.9;
	double m_rate=0.01;
	char* end;
	int seeded=0;
	hardcode=1;
	no_pdgames=100;

	// parses command line arguments if specified
	int opt;
	while((opt=getopt(argc,argv,"c:m:g:p:n:fsr"))!=-1){
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
			case 'p':
				popsize=atoi(optarg);
				break;
			case 'n':
				no_pdgames=atoi(optarg);
				break;
			case 'f':
				force_print=1;
				break;
			case 's':
				seeded=1;
				break;
			case 'r':
				hardcode=0;
				no1s=16;
				break;
			default:
				fprintf(stderr,"Usage: %s [-c crossover_rate] [-m mutate_rate] [-g num_generations] [-p pop_size] [-n no_pdgames] [-fsr]\n",argv[0]);
				exit(EXIT_FAILURE);
		}
	}
	
	// seeds the program if requested, else randomises
	if(seeded==1){srand48(1);}
	else{srand48(time(NULL));}

	genetic_assign();	// assigns random starting strategies
	calc_total_f();		// calculates initial fitness


	// sets up output file for graphing purposes		
	FILE *result;
	if((result=fopen("result.dat","w"))==NULL){
		printf("*** ERROR 2 ***\nCouldn't open output file\n");
		exit(2);
	}

	fprintf(result,"0 %d\n",total_fitness);

	// prints intial population (if small enough or forced)
	printf("Initial Population\n");
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

	// Prints out expected values
	printf("Expected Optimal Total fitness:\t\t %d\n",(popsize-1)*no_pdgames*3*popsize);


	genetic_free();		// frees up memory used
	return 0;
}
