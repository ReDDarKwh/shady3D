
#include "game.hpp"

int main()
{
	Game game;

	if (!game.Initialize())
	{
		return 1;
	}

	game.Start();

	return 0;
}
