#ifndef IO_H
#define IO_H

#include "board.h"
#include "move.h"
#include "movelist.h"
#include "searchinfo.h"
#include <string>

namespace IO
{
	const std::string PceChar = ".PNBRQKpnbrqk";
	const std::string SideChar = "wb-";
	const std::string RankChar = "12345678";
	const std::string FileChar = "abcdefgh";
	const std::unordered_map<uint32_t, std::string> epstr = 
	{{71,"a6"}, {72,"b6"}, {73,"c6"}, {74,"d6"}, {75,"e6"}, {76,"f6"}, {77,"g6"}, {78,"h6"},
	 {41,"a3"}, {42,"b3"}, {43,"c3"}, {44,"d3"}, {45,"e3"}, {46,"f3"}, {47,"g3"}, {48,"h3"}, {99, "None"}};

	void printBoard(const Board& pos) noexcept;
	void printBitBoard(const uint64_t) noexcept;
	void printMoveList(const MoveList& list) noexcept;
	void printSearchDetails(Board& pos, const SearchInfo& info, int32_t curDepth, int32_t bestScore) noexcept;
	void printBestMove(Board& pos, const SearchInfo& info, const Move& bestMove) noexcept;
	Move parseMove(std::string input, Board& pos) noexcept;
}

#endif
