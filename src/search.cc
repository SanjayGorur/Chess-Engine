/**
*	@file search.cc
*	@brief Contains definitions of functions declared in search.h.
*	@author Michael Lee
*	@date 1/9/2019
*/
#include "search.h"
#include "stopwatch.h"
#include "movelist.h"
#include "utils.h"
#include "searchinfo.h"
#include "engine.h"
#include "movemaker.h"
#include "io.h"
#include <sstream>
#include <iostream>
#include <cstdio>

SearchAgent::SearchAgent() noexcept : eval(), pv() {}

bool SearchAgent::threeFoldRepetition(const Board& pos) noexcept
{
	int32_t numRep = 0;
	for(int32_t i = 0; i < pos.hist_ply; ++i)
	{
		if(pos.history[i].pos_key == pos.pos_key)
		{
			++numRep;
		}
	}
	return numRep > 2;
}

bool SearchAgent::drawnMaterial(const Board& pos) noexcept
{
	auto pieceList = pos.piece_list;
	if(!pieceList[wP].empty() || !pieceList[bP].empty()) return false;
	if(!pieceList[wQ].empty() || !pieceList[bQ].empty()) return false;
	if(!pieceList[wR].empty() || !pieceList[bR].empty()) return false;
	if(pieceList[wB].size() > 1 || pieceList[bB].size() > 1) return false;
	if(pieceList[wN].size() > 1 || pieceList[bN].size() > 1) return false;
	if(!pieceList[wN].empty() && !pieceList[wB].empty()) return false;
	if(!pieceList[bN].empty() && !pieceList[bB].empty()) return false;
	return true;
}

int32_t SearchAgent::isRepetition(const Board& pos) noexcept
{
	for(int32_t idx = pos.hist_ply - pos.fifty_move; idx < pos.hist_ply - 1; ++idx)
	{
		if(pos.pos_key == pos.history[idx].pos_key) 
		{
			return true;
		}
	}
	return false;
}

bool SearchAgent::isGameOver(Board& pos) noexcept
{
	if(pos.fifty_move > 100)
	{
		std::cout << "1/2-1/2 {fifty move rule (claimed by " << kAppName << ")}" << std::endl;
		return true;
	}
	if(threeFoldRepetition(pos))
	{
		std::cout << "1/2-1/2 {3-fold repetition (claimed by " << kAppName << ")}" << std::endl;
		return true;
	}
	if(drawnMaterial(pos))
	{
		std::cout << "1/2-1/2 {insufficient material (claimed by " << kAppName << ")}" << std::endl;
		return true;
	}

	MoveList m = pos.getAllMoves();
	bool legalMoveFound = false;
	for(uint32_t i = 0; i < m.size(); ++i)
	{
		Move curMove = m[i];
		if(!MM::makeMove(pos, curMove))
		{
			continue;
		}
		legalMoveFound = true;
		MM::takeMove(pos);
		break;
	}

	if(legalMoveFound) return false;

	if(pos.inCheck())
	{
		if(pos.side_to_move == WHITE)
		{
			std::cout << "0-1 {black mates (claimed by "<< kAppName<< ")}" << std::endl;
			return true;			
		}
		else
		{
			std::cout << "0-1 {white mates (claimed by "<< kAppName<< ")}" << std::endl;
			return true;
		}
	}
	else
	{
			std::cout << "\n1/2-1/2 {stalemate (claimed by "<< kAppName<< ")}" << std::endl;
			return true;
	}
	return false;
}

void SearchAgent::checkStop(SearchInfo& info)
{
	if((info.nodes & kInterval ) == 0)
	{
		if(info.timeLimit && Stopwatch::getTimeInMilli() > info.stopTime)
		{
			info.stopped = true;
		}
		ReadInput(info);
	}
}

void SearchAgent::clearForSearch(Board& pos, SearchInfo& info) noexcept
{
	for(uint32_t i = 0; i < kNumPceTypes; ++i)
	{
		for(uint32_t j = 0; j < kBoardArraySize; ++j)
		{
			pos.search_hist[i][j] = 0;
		}
	}

	for(uint32_t i = 0; i < kNumPlayers; ++i)
	{
		for(uint32_t j = 0; j < kMaxSearchDepth; ++j)
		{
			pos.search_killers[i][j] = NOMOVE;
		}
	}

	pos.ply = 0;
	info.startTime = Stopwatch::getTimeInMilli();
	info.stopped = false;
	info.nodes = 0;
}

int32_t SearchAgent::alphaBeta(int32_t alpha, int32_t beta, uint32_t depth, Board& pos, SearchInfo& info, bool doNull) noexcept
{
	if(depth == 0) 
	{
		return this->quiescenceSearch(alpha, beta, pos, info);
    }

	this->checkStop(info);

    ++info.nodes;

    if((isRepetition(pos) || pos.fifty_move >= 100) && pos.ply)
    {
    	return 0;
    }

    if(static_cast<unsigned>(pos.ply) >= kMaxSearchDepth)
    {
    	return this->eval.evaluatePosition(pos);
    }

    //extend depth if in check since check move is usually trivial
    if(pos.inCheck())
    {
    	depth ++;
    }

    // Check if we have a valid entry in our transposition table
    int32_t score = -Value::kInfinity;
    Move pvMove = NOMOVE;
    if(this->pv.getHashEntry(pos, pvMove, score, alpha, beta, depth))
    {
    	return score;
    }

    // search null move
    if(doNull && !pos.inCheck() && pos.ply && (pos.big_pce[pos.side_to_move] > 1) && depth >= R)
    {
    	MM::makeNullMove(pos);
    	score = -1 * this->alphaBeta(-beta, -beta + 1, depth - R, pos, info, false);
    	MM::takeNullMove(pos);
    	if(info.stopped)
    	{
    		return 0;
    	}
    	if(score >= beta)
    	{
    		return beta;
    	}
    }

    MoveList m = pos.getAllMoves();

	//assign pv bonus
    if(!pvMove.isNull())
    {
		for(uint32_t moveNum = 0; moveNum < m.size(); ++moveNum) 
		{
			if(m[moveNum] == pvMove)
			{
				m[moveNum].score = m.kPvMoveBonus;
				break;
			}
		}
    }

    int32_t legalMoves = 0;
    int32_t prevAlpha = alpha;
    Move bestMove = NOMOVE;
	score = -Value::kInfinity;
	int32_t bestScore = -Value::kInfinity;

    bool foundPv = false;
	for(uint32_t moveNum = 0; moveNum < m.size(); ++moveNum) 
	{
		m.reorderList(moveNum);

        Move curMove = m[moveNum];
        if ( !MM::makeMove(pos,curMove))  
        {
            continue;
        }

        ++legalMoves;

        if(foundPv)
        {
        	score = -1 * this->alphaBeta(-alpha - 1, -alpha, depth - 1, pos, info, true);
        	if(score > alpha && score < beta)
        	{
        		score = -1 * this->alphaBeta(-beta, -alpha, depth - 1, pos, info, true);
        	}
        }
        else
        {
        	score = -1 * this->alphaBeta(-beta, -alpha, depth - 1, pos, info, true);
        }

        MM::takeMove(pos);

        if(info.stopped)
        {
        	return 0;
        }

        // we found a new best move, make sure it's within bounds
        if(score > bestScore)
        {
        	bestScore = score;
        	bestMove = curMove;
	        if(score > alpha)
	        {
	        	if(score >= beta)
	        	{
	        		if(!curMove.wasCapture())
	        		{
	        			pos.search_killers[1][pos.ply] = pos.search_killers[0][pos.ply];
	        			pos.search_killers[0][pos.ply] = curMove;
	        		}
	        		this->pv.insert(pos, bestMove, beta, depth, HFBETA);
	        		return beta;
	        	}
	        	foundPv = true;
	        	alpha = score;
	        	if(!curMove.wasCapture())
	        	{
	        		pos.search_hist[pos.pieces[bestMove.from()]][bestMove.to()] += depth;
	        	}
	        }
        }
    }

    // Check for mate
    if(legalMoves == 0)
    {
    	if(pos.sqAttacked(pos.king_sq[pos.side_to_move], !pos.side_to_move))
    	{
    		return -Value::kInfinity + pos.ply;
    	}
    	else return 0;
    }
    // Found new, better move so let's stash it
    if(alpha != prevAlpha)
    {
    	this->pv.insert(pos, bestMove, bestScore, depth, HFEXACT);
    }
    else
    {
    	this->pv.insert(pos, bestMove, alpha, depth, HFALPHA);
    }
	return alpha;
}

int32_t SearchAgent::quiescenceSearch(int32_t alpha, int32_t beta, Board& pos, SearchInfo& info) noexcept
{

	this->checkStop(info);

	++info.nodes;
	if(isRepetition(pos) || pos.fifty_move >= 100)
	{
		return 0;
	}

	if(static_cast<unsigned>(pos.ply) > kMaxSearchDepth - 1)
	{
		return eval.evaluatePosition(pos);
	}

	int32_t initialScore = eval.evaluatePosition(pos);

	if(initialScore >= beta)
	{
		return beta;
	}

	else if(initialScore > alpha)
	{
		alpha = initialScore;
	}

	MoveList m = pos.getAllCaptureMoves();
    int32_t legalMoves = 0;
    int score = -Value::kInfinity;

    for(uint32_t moveNum = 0; moveNum < m.size(); ++moveNum) 
	{
		m.reorderList(moveNum);
        Move curMove = m[moveNum];
        if ( !MM::makeMove(pos,curMove))  {
            continue;
        }
        ++legalMoves;
        score = -1 * this->quiescenceSearch(-beta, -alpha, pos, info);
        MM::takeMove(pos);
        if(info.stopped)
        {
        	return 0;
        }
        if(score > alpha)
        {
        	if(score >= beta)
        	{
        		return beta;
        	}
        	alpha = score;
        }
    }
	return alpha;
}

//uses iterative deepening
void SearchAgent::searchPosition(Board& pos, SearchInfo& info) noexcept
{
	//use a string stream to build up gui string
	Move bestMove = NOMOVE;
	int32_t bestScore = -Value::kInfinity;
	this->clearForSearch(pos, info);
	if(Engine::getConfig().useBook)
	{
		bestMove = Engine::getBook().getBookMove(pos);
	}
	if(bestMove.isNull())
	{
		// only iterative deepening search if no book move was found
		for(uint32_t curDepth = 1 ; curDepth <= info.depth; ++curDepth)
		{
			std::stringstream guiStr;
			bestScore = this->alphaBeta(-Value::kInfinity, Value::kInfinity, curDepth, pos, info, true);
			if(info.stopped)
			{
				break;
			}
			int32_t pvMoves = pv.getPvLine(pos, curDepth);
			bestMove = this->pv.pv_arr[0];
			IO::printSearchDetails(info, curDepth, bestScore, pv, pvMoves);
		}
	}
	IO::printBestMove(pos, info, bestMove);
}
