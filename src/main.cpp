
import mygame;

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
