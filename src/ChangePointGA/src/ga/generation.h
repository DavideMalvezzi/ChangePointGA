/*
 ============================================================================
 Name        : generation.h
 Author      : Davide Malvezzi
 Version     : 1.0
 Copyright   :
 Description : GA generation
 ============================================================================
 */

#ifndef POPULATION_H
#define POPULATON_H

#include <math.h>
#include <string.h>
#include <omp.h>

#include "strategy.h"


typedef struct Statistics{
	float fitnessMax;
	float fitnessMin;
	float fitnessMedian;
	float fitnessAvg;

	float lengthAvg;

	int invalidCount;
}Statistics;

typedef struct Generation{
	int count;							//Current num of individuals
	int size;							//Max num of individuals
	Strategy_ptr individuals;			//Individuals array
	Statistics statistics;				//Population statistics
}Generation;

typedef Generation* Generation_ptr;

typedef int (*SelectionFunction)(Generation_ptr);
typedef void (*CrossoverFunction)(Strategy_ptr parent1, Strategy_ptr parent2, Strategy_ptr child1, Strategy_ptr child2, int cut);
typedef void (*MutationFunction)(Strategy_ptr strategy);

/*****Init functions*****/
//Initialize a empty population
Generation_ptr initEmptyGeneration(int size);

//Initialize a random population
Generation_ptr initRandomGeneration(int size);

//Dispose generation
void disposeGeneration(Generation_ptr generation);

//Print population individuals
void printGeneration(Generation_ptr population);



/*****Fitness eval functions*****/
//Eval fitness for all individuals
void evalGenerationFitness(Generation_ptr population, float startVelocity, int startMap);

//Sort individual by fitness value
void sortGenerationByFitness(Generation_ptr population);

//Calculate population statistics
void updateGenerationStatistics(Generation_ptr population);



/*****Selection eval functions*****/
//Copy individual from current generation to the next generation
//The individual must be sorted by fitness asc and statistics update
void elitism(Generation_ptr currentGeneration, Generation_ptr nextGeneration, float elitismPercentage);

//Select an individuals with fitness proportional method
//The statistics must be updated
int fitnessProportionalSelection(Generation_ptr population);

//Select an individuals with tournament selection
int tournamentSelection(Generation_ptr population);



/*****Crossover functions*****/
//Apply single point crossover and create 2 children
void singlePointCrossover(Strategy_ptr parent1, Strategy_ptr parent2, Strategy_ptr child1, Strategy_ptr child2, int cut);

//Apply crossover to current generation and save it in next generation
void crossOver(Generation_ptr currentGeneration, Generation_ptr nextGeneration, SelectionFunction selection, CrossoverFunction crossover);



/*****Mutation function*****/
//Add new random change point
void addRandomChangePoint(Strategy_ptr strategy);

//Remove a random change point
void removeRandomChangePoint(Strategy_ptr strategy);

//Move random change point position
void moveRandomChangePoint(Strategy_ptr strategy);

//Change random change point action
void changeRandomChangePointAction(Strategy_ptr strategy);

//Apply mutation to the next generation
void mutation(Generation_ptr nextGeneration, MutationFunction mutationFunction, float mutationRate);


#endif
