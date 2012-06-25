/*
 * Very simple example strategy:
 * Search all possible positions reachable via one move,
 * and return the move leading to best position
 *
 * (c) 2006, Josef Weidendorfer
 */
#include <stdio.h>
#include "search.h"
#include "board.h"
#include "eval.h"

#define pretty_print(name, val) printf("%s = %d: %s: %s: %d\n", name, val,  __FILE__,__FUNCTION__,__LINE__);

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

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
	MoveList list;
//	int maxEval, minEval;
	int bestEval;
	int eval;	
	int sign;

	if(current_depth == MAX_DEPTH)
		return evaluate();

	bestEval = -17000;
//	maxEval = -17000;
//	minEval = 17000;

	generateMoves(list);
	
	if(evaluate() == 16000)
	{
		if(current_depth == 0)
			finishedNode(0,0);
		pretty_print("current_depth", current_depth);
		return ((current_depth % 2) ==0) ? -16000 : 16000;
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

#if 0
		if((MAX_DEPTH - current_depth +1) % 2 == 0)
		{
			if(eval > maxEval)
			{
				maxEval=eval;
				if(current_depth == 0) {
					pretty_print("Eval", eval);
					foundBestMove(0, m, eval);
				}
			}
		}
		else
		{
			if(eval < minEval)
			{
				minEval=eval;                   
				if(current_depth == 0) {
					pretty_print("Eval2", eval);
					foundBestMove(0, m, eval);
				}

			}
		}
#endif
	}
	bestEval = sign*bestEval;

	if(current_depth == 0)
		finishedNode(0,0);
#if 0
	if((MAX_DEPTH - current_depth +1) % 2 == 0)
		return maxEval;
	else
                return minEval;
#endif

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
