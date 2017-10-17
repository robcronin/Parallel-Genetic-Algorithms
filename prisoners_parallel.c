#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "genetic.h"

//Global variables
int hardcode, seeded, no_pdgames;
int island, mig_size, mig_inter, i_global_fitness;
int rank, size;
MPI_Status stat;

// returns "time saved" from pd game for Player 1
int pd_payoff(int actions){
	// 1 represents Cooperate, 0 Defect
	// xx is binary representation of (Player 1, Player 2)
	//	eg (Coop, Defect) -> 10 in binary -> 2 -> returns 0
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
			printf("*** WARNING ***\nIncorrect argument passed in to pd_payoff\n");
			return 0;
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


void calc_total_f(){

	int i,j;

	// Reset current values
	for(i=0;i<popsize;i++){pop[(no30s+1)*i+no30s]=0;}
	total_fitness=0;

	// ISLAND USES SERIAL VERSION
	if(island==1){
		for(i=0;i<popsize-1;i++){
			for(j=i+1;j<popsize;j++){
				pdgame(i,j);
			}
		}
	}

	// MASTER SLAVE USES PARALLEL VERSION
	else{
		int temp, recvs;
	
	
		// Start new calculation
		for(i=0;i<(size-1);i++){
			// Send "still needed signal"
			MPI_Send(&i,1,MPI_INT,i+1,1,MPI_COMM_WORLD);
			// Send pop
			MPI_Send(pop,(no30s+1)*popsize,MPI_INT,i+1,1,MPI_COMM_WORLD);
		}
	
		// Send new jobs
		for(i=size-1;i<popsize-1;i++){
			// get finished signal
			MPI_Recv(&temp,1,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,&stat);
			recvs=stat.MPI_SOURCE;
	
			//send next job
			MPI_Send(&i,1,MPI_INT,recvs,1,MPI_COMM_WORLD);
		}
	
		//Send kill signal
		for(i=1;i<size;i++){
			// get last finished signal
			MPI_Recv(&temp,1,MPI_INT,i,1,MPI_COMM_WORLD,&stat);
	
			// send complete signal (tag 2)
			MPI_Send(&i,1,MPI_INT,i,2,MPI_COMM_WORLD);
			
			// Receive updated fitnesses (use pop_new as receiver)
			MPI_Recv(pop_new,popsize,MPI_INT,i,1,MPI_COMM_WORLD,&stat);

			// Update current values
			for(j=0;j<popsize;j++){
				pop[(no30s+1)*j+no30s]+=pop_new[j];
				total_fitness+=pop_new[j];
			}
		}
	}
}

// ISLAND: gathers each island's fitness and adds them to i_global_fitness on rank 0
void i_calculate_global_f(){
	int i;
	int temp;

	// non zero ranks send their fitness to 0
	if(rank!=0){
		MPI_Send(&total_fitness,1,MPI_INT,0,1,MPI_COMM_WORLD);
	}
	else{
		// 0 receives them all and adds to i_global_fitness
		i_global_fitness=total_fitness;
		for(i=1;i<size;i++){
			MPI_Recv(&temp,1,MPI_INT,i,1,MPI_COMM_WORLD,&stat);
			i_global_fitness+=temp;
		}
	}
}	

// ISLAND: migrates mig_size of each island to the next island via checkerboard sending
void i_migrate(){
	int i;

	// even ranks send forward (excluding last one if odd number ranks)
	if(rank%2==0 && rank!=size-1){
		MPI_Send(pop,(no30s+1)*mig_size,MPI_INT,rank+1,1,MPI_COMM_WORLD);
	}

	// odd ranks receive into pop_new and send on their values (rank + 1 modded for looping around)
	else if(rank%2==1){
		MPI_Recv(pop_new,(no30s+1)*mig_size,MPI_INT,rank-1,1,MPI_COMM_WORLD,&stat);
		MPI_Send(pop,(no30s+1)*mig_size,MPI_INT,(rank+1)%size,1,MPI_COMM_WORLD);
	}

	// if even number of ranks
	if(size%2==0){
		// the evens recieve from one before (modded again)
		if(rank%2==0){
			MPI_Recv(pop_new,(no30s+1)*mig_size,MPI_INT,(rank-1+size)%size,1,MPI_COMM_WORLD,&stat);
		}
	}
	// if odd number of ranks
	else{
		// even ones apart from 0 receives
		if(rank%2==0 && rank!=0){
			MPI_Recv(pop_new,(no30s+1)*mig_size,MPI_INT,rank-1,1,MPI_COMM_WORLD,&stat);
		}

		// then the final send between the last and first one takes place
		if(rank==size-1){
			MPI_Send(pop,(no30s+1)*mig_size,MPI_INT,0,1,MPI_COMM_WORLD);
		}
		if(rank==0){
			MPI_Recv(pop_new,(no30s+1)*mig_size,MPI_INT,size-1,1,MPI_COMM_WORLD,&stat);
		}
	}

	// each island updates their population
	for(i=0;i<(no30s+1)*mig_size;i++){
		pop[i]=pop_new[i];
	}
	
}



int main(int argc, char *argv[]){

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int *cline=malloc(10*sizeof(int));
	double *clined=malloc(2*sizeof(double));

	// sets default values for program	
	no1s=18;
	popsize=120;
	int generations=100;	
	int force_print=0;
	double c_rate=0.9;
	double m_rate=0.01;
	seeded=0;
	hardcode=1;
	no_pdgames=100;
	island =0;
	mig_size=20;
	mig_inter=10;

	if(rank==0){
		char* end;

		// parses command line arguments if specified
		int opt;
		while((opt=getopt(argc,argv,"c:m:g:p:n:fsrit:a:"))!=-1){
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
				case 'i':
					island=1;
					break;
				case 'a':
					mig_size=atoi(optarg);
					break;
				case 't':
					mig_inter=atoi(optarg);
					break;
				default:
					fprintf(stderr,"Usage: %s [-c crossover_rate] [-m mutate_rate] [-g num_generations] [-p pop_size] [-n no_pdgames] [-a mig_size] [-t mig_inter][-fsri]\n",argv[0]);
					exit(EXIT_FAILURE);
			}
		}

		// stores the variables
		cline[0]=no1s;cline[1]=popsize;cline[2]=generations;cline[3]=force_print;
		cline[4]=seeded;cline[5]=hardcode;cline[6]=no_pdgames;cline[7]=island;
		cline[8]=mig_size;cline[9]=mig_inter;
		clined[0]=c_rate;clined[1]=m_rate;
	}

	// Broadcasts updated values to everyone else
	MPI_Bcast(cline, 10, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(clined, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	// Everyone updates their values
	no1s=cline[0];popsize=cline[1];generations=cline[2];force_print=cline[3];
	seeded=cline[4];hardcode=cline[5];no_pdgames=cline[6];island=cline[7];
	mig_size=cline[8];mig_inter=cline[9];
	c_rate=clined[0];m_rate=clined[1];
		
	free(cline);free(clined);

	// seeds if requested or else randomises
	if(seeded==0){srand48(time(NULL)+rank*rank-1082*rank);}
	else{srand48(2938*rank*rank-8493*rank+9351);}



	int i,j;

	// sets up output file for graphing purposes
	FILE *result;
	if(rank==0){
		if((result=fopen("result.dat","w"))==NULL){
			printf("*** ERROR 2 ***\nCouldn't open output file\n");
			exit(2);
		}
	}
	/*
	   *****************************************************************************
	   *****************************************************************************
 	   ************************ MASTER SLAVE VERSION *******************************
	   *****************************************************************************
	   *****************************************************************************
	*/
	if(island==0){

		// assigns population variables to everyone
		genetic_assign();

		// *********** MASTER ****************
		if(rank==0){

			// calculates initial fitness and prints
			calc_total_f();
			printf("Initial Population\n");
			print_pop(force_print);
			fprintf(result,"0 %d\n",total_fitness);
	
			// loops through the generations, updates and prints
			for(i=0;i<generations;i++){
				new_generation(c_rate,m_rate);
	
				fprintf(result,"%d %d\n",i+1,total_fitness);
			}
			fclose(result);

			// Prints out final population
			printf("Final Population\n");
			print_pop(force_print);
	
			// Prints out expected values
			printf("Expected Optimal Total Fitness:\t\t %d\n",(popsize-1)*no_pdgames*3*popsize);
	
	
	
			// Send KILL Signal(tag 2) to all SLAVES to finish working
			for(i=1;i<size;i++){
				MPI_Send(&i,1,MPI_INT,i,2,MPI_COMM_WORLD);
			}
		}

		// *********** SLAVE ***************
		else{
			while(1){
				// Check if still needed
				MPI_Recv(&i,1,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&stat);
				// if KILL Signal(tag 2) received then finish up
				if(stat.MPI_TAG==2){
					break;
				}
	
				// Otherwise receive population
				MPI_Recv(pop,(no30s+1)*popsize,MPI_INT,0,1,MPI_COMM_WORLD,&stat);
	
				// do first bit of work (i based off of rank)
				i=rank-1;
				for(j=i+1;j<popsize;j++){
					pdgame(i,j);
				}
	
				// Send finish signal
				MPI_Send(&i,1,MPI_INT,0,1,MPI_COMM_WORLD);
	
				// Check for new jobs
				while(1){
					// receive new i value
					MPI_Recv(&i,1,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&stat);
	
					// If job found(tag 1)
					if(stat.MPI_TAG==1){
						// do work
						for(j=i+1;j<popsize;j++){
							pdgame(i,j);
						}
	
						// Send finish signal
						MPI_Send(&i,1,MPI_INT,0,1,MPI_COMM_WORLD);
					}
	
					//else send results (use pop_new as carrier for fitnesses)
					else{
						for(i=0;i<popsize;i++){
							pop_new[i]=pop[(no30s+1)*i+no30s];
						}

						// Sends just the start of pop_new (with fitness values)
						MPI_Send(pop_new,popsize,MPI_INT,0,1,MPI_COMM_WORLD);
						break;
					}
				}

			}
		}	
				
		genetic_free();
	}

	
	/*
	   *****************************************************************************
	   *****************************************************************************
 					ISLAND MODEL
	   *****************************************************************************
	   *****************************************************************************
	*/
	else{
	
		// Does some error checking for values
		// In order for each island to have an equal even population we need popsize to be divisible by twice the number of islands
		if(popsize%(2*size)!=0){
			if(rank==0){
				printf("*** WARNING ***\nRequire popsize to be divisible by twice the number of processes\nUsing popsize of %d\n\n",popsize+2*size-popsize%(2*size));
			}
			popsize+=2*size-popsize%(2*size);
		}
		popsize/=size;	// update the local popsizes

		// makes sure we aren't trying to send more than island population
		if(mig_size>popsize){
			if(rank==0){
				printf("*** WARNING ***\nMigration size is bigger than island size\nUsing migration size of %d\n\n",popsize/2);
			}
			mig_size=popsize/2;
		}

		// assigns initial populations, updates fitnesses and prints out intial global fitness
		genetic_assign();
		calc_total_f();
		i_calculate_global_f();
		if(rank==0){printf("Initial Population\nGlobal fitness is %d\n\n",i_global_fitness);}

		
		int loops=generations/mig_inter;	// number of migration loops
		for(i=0;i<loops;i++){

			// updates mig_inter generations
			for(j=0;j<mig_inter;j++){
				new_generation(c_rate,m_rate);
				i_calculate_global_f();// updates global fitness

				// saves result to file
				if(rank==0){
					fprintf(result,"%d %d\n",(i)*mig_inter+j+1,i_global_fitness);
				}
			}
			i_migrate();		// migrates
		}

		// checks if there is any left over generations to run
		loops=generations%mig_inter;
		for(i=0;i<loops;i++){
			new_generation(c_rate,m_rate);
			i_calculate_global_f();// updates global fitness

			// saves result to file
			if(rank==0){
				fprintf(result,"%d %d\n",generations-loops+i+1,i_global_fitness);
			}
		}

		// printd out final global fitness and expected value	
		if(rank==0){
			printf("Final Population\nGlobal fitness is %d\n\n",i_global_fitness);
			printf("Expected Optimal Total Fitness:\t\t %d\n",size*(popsize-1)*no_pdgames*3*popsize);
		}

		genetic_free();
	}
	MPI_Finalize();
	return 0;
}
