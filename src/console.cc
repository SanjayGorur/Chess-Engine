#include "console.h"
#include "stopwatch.h"
#include "defs.h"
#include "movemaker.h"
#include "io.h"
#include <iostream>
#include <sstream>
ConsoleManager::ConsoleManager() noexcept : ProtocolManager() {}
ConsoleManager::~ConsoleManager() noexcept {}

void ConsoleManager::loop()
{
	std::cout << "\nConsole Mode! \n";
	std::cout << "Type help for commands \n";

	info.GAME_MODE = CONSOLE_MODE;
	info.POST_THINKING = true;

	std::string buf;
	std::string cmd;
	int32_t depth = 5;
	std::vector<int32_t> movesToGo { 30, 30 };
	int32_t moveTime = 10 * Stopwatch::kMilliPerSecond;
	uint32_t engineSide = BLACK;

	while(true)
	{
		std::cout<<std::flush;
		if(this->pos.side_to_move == engineSide && !sa.isGameOver(this->pos))
		{
			this->info.startTime = Stopwatch::getTimeInMilli();
			this->info.depth = depth;
			if(moveTime != 0)
			{
				this->info.timeLimit = true;
				this->info.stopTime = info.startTime + moveTime;
			}
			sa.searchPosition(pos, this->info);
		}
		std::cout << '\n'<<kAppName<<" > " << std::flush;
		if(!(getline (std::cin, buf))) { continue; }
		if(buf == "\n") { continue; }
		std::stringstream ss(buf);
		ss >> buf;
		if(buf == "help")
		{
			std::cout << ("Commands:\n");
			std::cout << ("quit - quit game\n");
			std::cout << ("force - computer will not think\n");
			std::cout << ("print - show board\n");
			std::cout << ("post - show thinking\n");
			std::cout << ("nopost - do not show thinking\n");
			std::cout << ("new - start new game\n");
			std::cout << ("go - set computer thinking\n");
			std::cout << ("depth x - set depth to x\n");
			std::cout << ("time x - set thinking time to x seconds (depth still applies if set)\n");
			std::cout << ("view - show current depth and movetime settings\n");
			std::cout << ("** note ** - to reset time and depth, set to 0\n");
			std::cout << ("enter moves using b7b8q notation\n\n\n");
			std::cout << std::flush;
		}
		else if (buf == "quit" || buf[0] == 'q')
		{
			this->info.quit = true;
			break;
		}
		else if(buf == "post")
		{
			this->info.POST_THINKING = true;
		}
		else if(buf == "print")
		{
			IO::printBoard(pos);
		}
		else if(buf == "nopost")
		{
			this->info.POST_THINKING = false;
		}
		else if(buf == "force")
		{
			engineSide = BOTH;
		}
		else if(buf == "view")
		{
			std::stringstream viewSS;
			if(depth == kMaxDepth)
			{
				viewSS << "depth not set";
			}
			else
			{
				viewSS << "depth " << depth;
			}
			if(moveTime != 0)
			{
				viewSS << " movetime "<< (moveTime/1000)<<"s\n";
			}
			else 
			{
				viewSS << " movetime not set\n";
			}
			std::cout << viewSS.str();
		}
		else if (buf == "depth")
		{
			ss >> depth;
			if(depth == 0)
			{
				depth = kMaxDepth;
			}
		}
		else if (buf == "time")
		{
			ss >> moveTime;
			moveTime *= Stopwatch::kMilliPerSecond;
		}
		else if (buf == "new")
		{
			engineSide = BLACK;
			this->pos.parseFEN(STARTFEN);
		}
		else if (buf == "setboard")
		{
			engineSide = BOTH;
			std::stringstream fen;
			ss >> buf;
			while(ss.good() && buf != "moves")
			{
				fen << buf << ' ';
				ss >> buf;
			}
			fen << buf;
			this->pos.parseFEN(fen.str());
		}
		else if(buf == "go")
		{
			engineSide = pos.side_to_move;
		}
		else if(buf == "undo")
		{
			// Go to previous user turn
			MM::takeMove(this->pos);
			MM::takeMove(this->pos);
			IO::printBoard(pos);
		}
		else
		{
			Move move = IO::parseMove(buf, this->pos);
			if(move == NOMOVE)
			{
				std::cout << "Unrecognized Command: "<<buf<<'\n';
			}
			else 
			{
				MM::makeMove(this->pos, move);
				this->pos.ply = 0;
			}
		}
	}
}