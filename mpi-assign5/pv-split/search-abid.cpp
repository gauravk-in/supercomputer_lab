/**
 * A real world, sequential strategy:
 * Alpha/Beta with Iterative Deepening (ABID)
 *
 * (c) 2005, Josef Weidendorfer
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#include "search.h"
#include "board.h"

extern int thread_rank;
extern int num_threads;
extern int *slaveleaves;
extern int *slavenodes;

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

	    currentValue = pv_split(alpha, beta);
	    printf("Subsearch finished with currentValue = %d\n", currentValue);

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

int ABIDStrategy::pv_split(int alpha0, int beta0)
{   
	bool cutoff;
	int depth;
	int value;
    	int currentValue = -15000;
	int slave_id;
	int num_slaves;
	int pending_jobs;
	Slave_Input slave_input;
	Slave_Output *slave_output;
	MPI_Request *rcv_rq;
	MoveList list;
	Move m, *movechain;
	int *alpha, *beta;

	rcv_rq = (MPI_Request *) malloc (num_threads * sizeof(MPI_Request));
	slave_output = (Slave_Output *) malloc (num_threads * sizeof(Slave_Output));
	movechain = (Move*) malloc ((_currentMaxDepth+1)* sizeof (Move));
	alpha = (int*) malloc((_currentMaxDepth+2)*sizeof(int));
	beta = (int*) malloc((_currentMaxDepth+2)*sizeof(int));


	alpha[0] = alpha0;
	beta[0] = beta0;
    	_currentBestMove.type = Move::none;

	// Play moves from the pv until you reach the lowest level
	// If pv-moves are not possible use random moves
	// Store the sequence of moves in movechain
	depth = 0;
	while (depth < (_currentMaxDepth-1)) 
	{
		list.clear();
	    	_board->generateMoves(list);
		m = _pv[depth];

		// check if pv-move is possible
		if ((m.type != Move::none) && (!list.isElement(m, 0, false)))
			m.type = Move::none;
		
		// get random move if pv-move is not possible
		if (m.type == Move::none)
			list.getNext(m, Move::none);

		_board->playMove(m);
		movechain[depth] = m;

		alpha[depth+1] = -beta[depth];
		beta[depth+1] = -alpha[depth];

		depth++;
	}

	// Start at the second lowest level and move back up the gametree
	// Devide the work at each level
	depth = _currentMaxDepth-1;
	while (depth >= 0)
    	{	
		slave_id = 0;
		list.clear();
		_board->generateMoves(list);
		// delete the move we already checked from the list
		if (depth < _currentMaxDepth-1)
			list.isElement(movechain[depth], 0, true);

		strcpy(slave_input.boardstate,_board->getState());
		slave_input.alpha = -beta[depth];
		slave_input.beta = -alpha[depth];
		slave_input.depth = depth+1;
		slave_input.currentMaxDepth = _currentMaxDepth;

		printf("Thread 0 testing %d moves at depth = %d\n",list.getLength(), depth);
		while ( list.getNext(m, Move::none) ) 
		{
			slave_input.move = m;	
			MPI_Send(&slave_input, sizeof(Slave_Input), MPI_BYTE, slave_id+1, 10, MPI_COMM_WORLD);
			MPI_Irecv(&slave_output[slave_id], sizeof(Slave_Output), MPI_BYTE, slave_id+1,
				10, MPI_COMM_WORLD, &rcv_rq[slave_id]);

			slave_id++;

			if (slave_id >= (num_threads-1))
				break;
		}

		num_slaves = slave_id;
		pending_jobs = num_slaves;
		cutoff = false;

		while (pending_jobs > 0)
		{
				MPI_Waitany(num_slaves, rcv_rq, &slave_id, MPI_STATUS_IGNORE);
				_sc->_leavesVisited += slave_output[slave_id].num_leaves;
				_sc->_nodesVisited += slave_output[slave_id].num_nodes;

				slaveleaves[slave_id] += slave_output[slave_id].num_leaves;
				slavenodes[slave_id] += slave_output[slave_id].num_nodes;

				value = slave_output[slave_id].eval;
				if (value > currentValue)
				{
					currentValue = value;
					_pv = slave_output[slave_id].pv;

					if (_sc) 
						_sc->foundBestMove(depth, _pv[depth], currentValue);
					if (currentValue > alpha[depth]) 
					{
						alpha[depth] = currentValue;	
						slave_input.beta = -alpha[depth];
					}
					 /* alpha/beta cut off or win position ... */
	    				if (currentValue>14900 || currentValue >= beta[depth])
						cutoff = true;
				}

				if ((list.getNext(m, Move::none)) && (cutoff == false))
				{
					slave_input.move = m;	

					MPI_Send(&slave_input, sizeof(Slave_Input), MPI_BYTE, slave_id+1, 10, MPI_COMM_WORLD);
					MPI_Irecv(&slave_output[slave_id], sizeof(Slave_Output), MPI_BYTE, slave_id+1,
						10, MPI_COMM_WORLD, &rcv_rq[slave_id]);
				}
				else
					pending_jobs--;
		}

		if (depth > 0)
		{
			_board->takeBack();
			_pv.update(depth-1, movechain[depth-1]);
			currentValue = -currentValue;
			if (currentValue > alpha[depth -1])
				alpha[depth-1] = currentValue;
		}
		if (depth == 0)
		    	_currentBestMove = _pv[0];
		if (_sc) 
			_sc->finishedNode(depth, _pv.chain(depth));
		depth--;
    	}

	free(slave_output);
	free(rcv_rq);
	free(movechain);
	free(alpha);
	free(beta);

	return currentValue;
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
    MoveList list;
    bool depthPhase, doDepthSearch;

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

    // don't use moves from the principal variation
    m.type = Move::none;

    // first, play all moves with depth search
    depthPhase = true;

    while (1) {

	// get next move
	if (m.type == Move::none) {
            if (depthPhase)
		depthPhase = list.getNext(m, maxType);
            if (!depthPhase)
		if (!list.getNext(m, Move::none)) break;
	}
	// we could start with a non-depth move from principal variation
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

	    /* alpha/beta cut off or win position ... */
	    if (currentValue>14900 || currentValue >= beta) {
		if (_sc) _sc->finishedNode(depth, _pv.chain(depth));
		return currentValue;
	    }

	    /* maximize alpha */
	    if (currentValue > alpha) alpha = currentValue;
	}

	if (_stopSearch) break; // depthPhase=false;
	m.type = Move::none;
    }
    
    if (_sc) _sc->finishedNode(depth, _pv.chain(depth));

    return currentValue;
}

// register ourselve
ABIDStrategy abidStrategy;
