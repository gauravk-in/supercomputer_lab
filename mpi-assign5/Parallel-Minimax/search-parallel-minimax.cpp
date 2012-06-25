/*
 * Very simple example strategy:
 * Search all possible positions reachable via one move,
 * and return the move leading to best position
 *
 * (c) 2006, Josef Weidendorfer
 */
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "search.h"
#include "board.h"
#include "eval.h"

#define pretty_print(name, val) printf("%s = %d: %s: %s: %d\n", name, val,  __FILE__,__FUNCTION__,__LINE__);

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

extern int thread_rank;
extern int num_threads;

/**
 * To create your own search strategy:
 * - copy this file into another one,
 * - change the class name one the name given in constructor,
 * - adjust clone() to return an instance of your class
 * - adjust last line of this file to create a global instance
 *   of your class
 * - adjust the Makefile to include your class in SEARCH_OBJS
 * - implement searchBestMove()
 *
 * Advises for implementation of searchBestMove():
 * - call foundBestMove() when finding a best move since search start
 * - call finishedNode() when finishing evaluation of a tree node
 * - Use _maxDepth for strength level (maximal level searched in tree)
 */
class MinimaxStrategy: public SearchStrategy
{
	public:
		// Defines the name of the strategy
		MinimaxStrategy(): SearchStrategy("MINIMAX") {}

		// Factory method: just return a new instance of this class
		SearchStrategy* clone() { return new MinimaxStrategy(); }

	private:

		/**
		 * Implementation of the strategy.
		 */
		void searchBestMove();
		int minimax();
};

int current_depth=0;
#define MAX_DEPTH 3

int MinimaxStrategy::minimax()
{
	Move m;
	Move bestestMove; //;-p This is the cumulative best move among all threads.
	MoveList list;
	int bestEval;
	int eval;	
	int sign;
	int move_counter=0;
	int i=0;

	if(current_depth == MAX_DEPTH)
		return evaluate();

	bestEval = -17000;

	generateMoves(list);

	if(evaluate() == 16000)
	{
		if(current_depth == 0)
			finishedNode(0,0);
		pretty_print("current_depth", current_depth);
		return ((current_depth % 2) ==0) ? -16000 : 16000;
	}

	if(current_depth == 0)
	{
		for(i=0;i<thread_rank;i++)
			list.getNext(m);
	}

	if((MAX_DEPTH-current_depth)%2 == 1)
		sign = 1;
	else
		sign = -1;

	while(list.getNext(m))	
	{
		if(current_depth < MAX_DEPTH)
		{
			current_depth++;
			playMove(m);
			eval=minimax();
			takeBack();
			current_depth--;
		}

		if(sign*eval > bestEval)
		{
			bestEval = sign*eval;
			if(unlikely(current_depth == 0)) {
				pretty_print("Eval", bestEval);
				foundBestMove(0, m, eval);
			}
		}

		if(current_depth == 0)
		{	
			for(i=0;i<num_threads;i++)
				list.getNext(m);
		}
	}
	bestEval = sign*bestEval;

//	if(thread_rank==0)
//	{
		Move *moves=NULL;
		int *eval_results;
		moves=(Move*)malloc((num_threads -1)*sizeof(Move));
		eval_results=(int*)malloc((num_threads - 1)*sizeof(int));
//	}

	//all threads send value to thread 0
	MPI_Gather (&_bestMove, sizeof(Move), MPI_BYTE, 
			moves, sizeof(Move), MPI_BYTE, 0, MPI_COMM_WORLD);
	MPI_Gather (&bestEval, 1, MPI_INT, 
			eval_results, 1, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	if(thread_rank == 0)
	{
		for(i=0;i<num_threads-1;i++)
		{
			if(sign*eval_results[i] > sign*bestEval)
			{
				bestEval = eval_results[i];
				bestestMove=moves[i];
			}
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Bcast (&bestestMove, sizeof(Move), MPI_BYTE, 0,
			MPI_COMM_WORLD);
        MPI_Bcast (&bestEval, 1, MPI_INT, 0,
                        MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	foundBestMove(0, bestestMove, bestEval);

	if(current_depth == 0)
		finishedNode(0,0);

	return bestEval;
}

void MinimaxStrategy::searchBestMove()
{

// KUKU : Here we have to implement the minimax strategy
// Minimax strategy tries to minimize the maximum possible outcome of opponent.
// At each turn, we check for each move the max positive outcome for opponent.
// We choose the move for which the max is least.
// To check this, we look at more than one levels in the Game Tree.
	// we try to maximize bestEvaluation
/*	int bestEval = minEvaluation();
	int eval;

	Move m;
	MoveList list;

	// generate list of allowed moves, put them into <list>
	generateMoves(list);

	// loop over all moves
	while(list.getNext(m)) {

		// draw move, evalute, and restore position
		playMove(m);
		eval = evaluate();
		takeBack();

		if (eval > bestEval) {
			bestEval = eval;
			foundBestMove(0, m, eval);
		}
	}

	finishedNode(0,0);
*/
	minimax();
}

// register ourselve as a search strategy
MinimaxStrategy minimaxStrategy;
