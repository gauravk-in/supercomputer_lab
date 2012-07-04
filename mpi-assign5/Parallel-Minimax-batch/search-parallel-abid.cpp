/**
 * A real world, sequential strategy:
 * Alpha/Beta with Iterative Deepening (ABID)
 *
 * (c) 2005, Josef Weidendorfer
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "search.h"
#include "board.h"

#define pretty_print(name, val) printf("%s = %d: %s: %s: %d\n", name, val,  __FILE__,__FUNCTION__,__LINE__);

extern int thread_rank;
extern int num_threads;



class ABIDStrategy: public SearchStrategy
{
	public:
		ABIDStrategy(): SearchStrategy("PARALLEL-ABID", 2) {}
		SearchStrategy* clone() { return new ABIDStrategy(); }

		Move& nextMove() { return _pv[1]; }

	private:
		void searchBestMove();
		/* recursive alpha/beta search */
		int alphabeta(int depth, int alpha, int beta);

		/* prinicipal variation found in last search */
		Variation _pv;
		Move _currentBestMove;
		bool _inPV;
		int _currentMaxDepth;
};


/**
 * Entry point for search
 *
 * Does iterative deepening and alpha/beta width handling, and
 * calls alpha/beta search
 */
void ABIDStrategy::searchBestMove()
{    
	int alpha = -15000, beta = 15000;
	int nalpha, nbeta, currentValue = 0;

	_pv.clear(_maxDepth);
	_currentBestMove.type = Move::none;
	_currentMaxDepth=1;

	/* iterative deepening loop */
	do {

		/* searches on same level with different alpha/beta windows */
		while(1) {

			nalpha = alpha, nbeta = beta;
			_inPV = (_pv[0].type != Move::none);

			if (_sc && _sc->verbose()) {
				char tmp[100];
				sprintf(tmp, "Alpha/Beta [%d;%d] with max depth %d", alpha, beta, _currentMaxDepth);
				_sc->substart(tmp);
			}

			currentValue = alphabeta(0, alpha, beta);

			/* stop searching if a win position is found */
			if (currentValue > 14900 || currentValue < -14900)
				_stopSearch = true;

			/* Don't break out if we haven't found a move */
			if (_currentBestMove.type == Move::none)
				_stopSearch = false;

			if (_stopSearch) break;

			/* if result is outside of current alpha/beta window,
			 * the search has to be rerun with widened alpha/beta
			 */
			if (currentValue <= nalpha) {
				alpha = -15000;
				if (beta<15000) beta = currentValue+1;
				continue;
			}
			if (currentValue >= nbeta) {
				if (alpha > -15000) alpha = currentValue-1;
				beta=15000;
				continue;
			}
			break;
		}

		/* Window in both directions cause of deepening */
		alpha = currentValue - 200, beta = currentValue + 200;

		if (_stopSearch) break;

		_currentMaxDepth++;
	}
	while(_currentMaxDepth <= _maxDepth);

	_bestMove = _currentBestMove;
}


/*
 * Alpha/Beta search
 *
 * - first, start with principal variation
 * - depending on depth, we only do depth search for some move types
 */
int ABIDStrategy::alphabeta(int depth, int alpha, int beta)
{
	int currentValue = -14999+depth, value;
	Move m;
	Move bestMove;
	MoveList list;
	bool depthPhase, doDepthSearch;
	int i=0;
	int movecounter =-1;
	int flag=0;

	/* We make a depth search for the following move types... */
	int maxType = (depth < _currentMaxDepth-1)  ? Move::maxMoveType :
		(depth < _currentMaxDepth)    ? Move::maxPushType :
		Move::maxOutType;

	_board->generateMoves(list);

	if (_sc && _sc->verbose()) {
		char tmp[100];
		sprintf(tmp, "Alpha/Beta [%d;%d], %d moves (%d depth)", alpha, beta,
				list.count(Move::none), list.count(maxType));
		_sc->startedNode(depth, tmp);
	}

	/* check for an old best move in principal variation */
	if (_inPV) {
		m = _pv[depth];

		if ((m.type != Move::none) &&
				(!list.isElement(m, 0, true)))
			m.type = Move::none;

		if (m.type == Move::none) _inPV = false;
	}

	// first, play all moves with depth search
	depthPhase = true;

	while (1) {

		movecounter++;
		// get next move
		if (m.type == Move::none) {
			if (depthPhase)
				depthPhase = list.getNext(m, maxType);
			if (!depthPhase)
				if (!list.getNext(m, Move::none)) break;
		}
		if((thread_rank == movecounter% num_threads) || (depth>0))// we could start with a non-depth move from principal variation
		{
			doDepthSearch = depthPhase && (m.type <= maxType);

			_board->playMove(m);

			/* check for a win position first */
			if (!_board->isValid()) {

				/* Shorter path to win position is better */
				value = 14999-depth;
			}
			else {

				if (doDepthSearch) {
					/* opponent searches for its maximum; but we want the
					 * minimum: so change sign (for alpha/beta window too!)
					 */
					value = -alphabeta(depth+1, -beta, -alpha);
				}
				else {
					value = evaluate();
				}
			}

			_board->takeBack();

			/* best move so far? */
			if (value > currentValue) {
				currentValue = value;
				_pv.update(depth, m);

				if (_sc) _sc->foundBestMove(depth, m, currentValue);
				if (depth == 0)
					_currentBestMove = m;

				/* alpha/beta cut off or win position ... */
				if (currentValue>14900 || currentValue >= beta) {
					if (_sc) _sc->finishedNode(depth, _pv.chain(depth));
					flag = 1;
					break;
					//return currentValue;
				}

				/* maximize alpha */
				if (currentValue > alpha) alpha = currentValue;
			}
		}
		if (_stopSearch) break; // depthPhase=false;
		m.type = Move::none;
	}



	if(depth == 0){
		//all threads send value to thread 0
		if(thread_rank == 0)
		{
			Move *moves=NULL;
			Variation *PVs=NULL;
			int *eval_results;
			PVs=(Variation*)malloc((num_threads -1)*sizeof(Variation));
			moves=(Move*)malloc((num_threads -1)*sizeof(Move));
			eval_results=(int*)malloc((num_threads - 1)*sizeof(int));




			for(i=0;i<num_threads-1;i++)
			{		
				MPI_Status status;	
				MPI_Recv (&moves[i], sizeof(Move), MPI_BYTE, i+1 ,10, MPI_COMM_WORLD, &status);
				MPI_Recv (&PVs[i], sizeof(Variation), MPI_BYTE, i+1 ,10, MPI_COMM_WORLD, &status);
				MPI_Recv (&eval_results[i], 1, MPI_INT, i+1 ,10, MPI_COMM_WORLD, &status);
			}

			for(i=0;i<num_threads-1;i++)
			{
				if(eval_results[i] > currentValue)
				{
					currentValue = eval_results[i];
					_pv=PVs[i];
					_currentBestMove = moves [i];
				}

			}
			for(i=0;i<num_threads-1;i++)
			{
				MPI_Send(&_currentBestMove, sizeof(Move), MPI_BYTE, i+1 ,10, MPI_COMM_WORLD);
				MPI_Send(&_pv, sizeof(Variation), MPI_BYTE, i+1 ,10, MPI_COMM_WORLD);
				MPI_Send(&currentValue, 1, MPI_INT, i+1 ,10, MPI_COMM_WORLD);
			}
		}
		else
		{
			MPI_Status status;
			MPI_Send(&_currentBestMove, sizeof(Move), MPI_BYTE, 0 ,10, MPI_COMM_WORLD);
			MPI_Send(&_pv, sizeof(Variation), MPI_BYTE, 0 ,10, MPI_COMM_WORLD);
			MPI_Send(&currentValue, 1, MPI_INT, 0 ,10, MPI_COMM_WORLD);
			MPI_Recv(&_currentBestMove, sizeof(Move), MPI_BYTE, 0 ,10, MPI_COMM_WORLD, &status);
			pretty_print("thread_rank", thread_rank);
			printf("currentBestMove = %s\n",_currentBestMove.name());
			MPI_Recv(&_pv, sizeof(Variation), MPI_BYTE, 0 ,10, MPI_COMM_WORLD, &status);
			MPI_Recv(&currentValue, 1, MPI_INT, 0 ,10, MPI_COMM_WORLD, &status);

		}	
	
		foundBestMove(0,_currentBestMove,currentValue);
	}

	if(!flag)
	if (_sc) _sc->finishedNode(depth, _pv.chain(depth));

	return currentValue;
}

// register ourselve
ABIDStrategy abidStrategy;
