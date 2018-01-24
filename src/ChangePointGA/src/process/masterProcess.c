/*
 * masterProcess.c
 *
 *  Created on: Jan 20, 2018
 *      Author: davide
 */

#include "masterProcess.h"

static void genetic(MasterProcess* mProcess);

void initMaster(MasterProcess* mProcess, int worldId, int color){
	//Init process
	mProcess->worldId = worldId;
	MPI_Comm_split(MPI_COMM_WORLD, color, worldId, &mProcess->comm);

	//Loop flag
	mProcess->loop = 1;

	#ifdef SAVE_STATISTICS
		//Create statistics file
		mProcess->statisticsFile = fopen(STATISTICS_FILE, "at");
		fprintf(mProcess->statisticsFile, "generation, energyBest, fitnessBest, fitnessMin, fitnessMax, fitnessMedian, fitnessAvg, lenghtAvg, similarityAvg, %%invalid, lastChange\n");
	#endif

	#ifdef KEEP_BEST
		//Create best file
		mProcess->bestFile = fopen("bestSim.csv", "at");
		mProcess->currBest = INFINITY;
	#endif

	//Init console
	initConsole();

	//Init ga
	initGA(&mProcess->ga, linearRankWithPressureSelection, singlePointCrossover, energyFitness);
	addMutation(&mProcess->ga, addRandomChangePoint, 			ADD_POINT_MUTATION_RATE);
	addMutation(&mProcess->ga, removeRandomChangePoint, 		REMOVE_POINT_MUTATION_RATE);
	addMutation(&mProcess->ga, moveRandomChangePoint, 			CHANGE_POS_MUTATION_RATE);
	addMutation(&mProcess->ga, changeRandomChangePointAction, 	CHANGE_ACT_MUTATION_RATE);
	addMutation(&mProcess->ga, filterStrategy, 					FILTER_MUTATION_RATE);
}

void execMaster(MasterProcess* mProcess){
	//Refresh console
	printGAParams(&mProcess->ga);
	printSimulationParams();

	//Loop
	while(mProcess->loop){
		//Apply genetics
		genetic(mProcess);

		#ifdef SAVE_STATISTICS
			//Save statistic
			statisticsToFile(mProcess->ga.currentGeneration, mProcess->ga.generationCount, mProcess->statisticsFile);
		#endif

		#ifdef KEEP_BEST
			//Save the new best
			if(mProcess->currBest > mProcess->ga.currentGeneration->statistics.best.fitness){
				mProcess->currBest = mProcess->ga.currentGeneration->statistics.best.fitness;
				for(int i = 0; i < SIM_STEP_COUNT; i++){
					fprintf(mProcess->bestFile, "%d,", mProcess->ga.currentGeneration->statistics.best.simulation.steps[i].map);
				}
				fprintf(mProcess->bestFile, "\n");
			}
		#endif

		//Update console
		updateConsole(&mProcess->ga, &mProcess->loop);
	}

	//Save strategy and generation
	saveSimulationParams(SIMULATION_FILE);
	saveGAParams(&mProcess->ga, GA_FILE);
	strategyToFile(&mProcess->ga.currentGeneration->statistics.best, BEST_FILE);
	generationToFile(mProcess->ga.currentGeneration, GENERATION_FILE);

}

void disposeMaster(MasterProcess* mProcess){
	//Close all slave process
	char cmd = SLAVE_QUIT_CMD;
	MPI_Bcast(&cmd, 1, MPI_CHAR, 0, mProcess->comm);

	//Dispose GA
	disposeGA(&mProcess->ga);

	#ifdef SAVE_STATISTICS
		//Close statistics file
		fclose(mProcess->statisticsFile);
	#endif

	#ifdef KEEP_BEST
		//Close best file
		fclose(mProcess->bestFile);
	#endif

	//Dispose windows
	disposeConsole();
}


void genetic(MasterProcess* mProcess){
	GA* ga = &mProcess->ga;

	//Counters
	int childCount = 0, mutationCount = 0;

	//Timers
	unsigned long long generationTimer, timer;
	double generationTime, fitnessTime, sortTime, statTime, elitismTime, crossoverTime, mutationTime;

	//Start timer
	generationTimer = getTime();

	//Eval fitness
	timer = getTime();
	evalGenerationFitness(ga->currentGeneration, ga->fitnessFunction, &mProcess->comm);
	fitnessTime = getTimeElapsed(timer);

	//Sort individual by fitness
	timer = getTime();
	sortGenerationByFitness(ga->currentGeneration);
	sortTime = getTimeElapsed(timer);

	//Calculate the statistics
	timer = getTime();
	updateGenerationStatistics(ga->currentGeneration);
	statTime = getTimeElapsed(timer);

	if (ga->currentGeneration->statistics.invalidCount / ga->currentGeneration->size > INVALID_THRESHOLD
			|| ga->currentGeneration->statistics.lastChange > MAX_LAST_CHANGE) {

		//Add new valid random strategy

	}

	//Perform crossover
	timer = getTime();
	childCount = crossOver(ga->currentGeneration, ga->nextGeneration, ga->selectionFunction, ga->crossoverFunction);
	crossoverTime = getTimeElapsed(timer);

	//Perform mutations
	timer = getTime();
	for(int i = 0; i < ga->mutationCount; i++){
		mutationCount += mutation(ga->nextGeneration, ga->mutationsFunction[i], ga->mutationRates[i]);
	}
	mutationTime = getTimeElapsed(timer);

	//Perform elitism selection
	timer = getTime();
	elitism(ga->currentGeneration, ga->nextGeneration, ELITISM_PERCENTAGE);
	FUSS(ga->currentGeneration, ga->nextGeneration);
	elitismTime = getTimeElapsed(timer);

	//Prints
	if(ga->generationCount % 10 == 0){
		//Get time
		generationTime = getTimeElapsed(generationTimer);

		int row = 0;
		wclear(gaOutputWindow);
		box(gaOutputWindow,0,0);
		//Print generation info
		mvwprintw(gaOutputWindow, row++, 1, "GA output");
		mvwprintw(gaOutputWindow, row++, 1, "Gen %lu   individuals %d/%d   child %d   mutations %d",
				ga->generationCount,
				ga->currentGeneration->count,
				ga->currentGeneration->size,
				childCount,
				mutationCount
			);

		//Print best
		mvwprintw(gaOutputWindow, row++, 1, "Best fitness %.2f   energy %.2f   time %.2f/%.2f",
				ga->currentGeneration->statistics.best.fitness,
				ga->currentGeneration->statistics.best.simulation.energy,
				ga->currentGeneration->statistics.best.simulation.time,
				MAX_TIME
			);

		//Print stats

		mvwprintw(gaOutputWindow, row++, 1, "Stat fmin %.2f   fmed %.2f   lavg %d   inv %.2f   sim %.2f   lch %lu",
				ga->currentGeneration->statistics.fitnessStat.min,
				ga->currentGeneration->statistics.fitnessStat.median,
				(int)ga->currentGeneration->statistics.lengthStat.avg,
				(float)ga->currentGeneration->statistics.invalidCount / ga->currentGeneration->count,
				ga->currentGeneration->statistics.fenotypeSimilarityStat.avg,
				ga->currentGeneration->statistics.lastChange
			);

		//Print time
		mvwprintw(gaOutputWindow, row++, 1, "Time ft %0.3lf   st %0.3lf   ut %0.3lf   et %0.3lf   ct %0.3lf   mt %0.3lf",
				fitnessTime,
				sortTime,
				statTime,
				elitismTime,
				crossoverTime,
				mutationTime
			);
		mvwprintw(gaOutputWindow, row++, 1, "Total time %lf", generationTime);
		wrefresh(gaOutputWindow);
	}

	//Next generation
	swap(ga->currentGeneration->individuals, ga->nextGeneration->individuals);
	ga->nextGeneration->count = 0;
	ga->generationCount++;
}
