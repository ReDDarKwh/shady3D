
#include "./app.hpp"

int main()
{

	App app;

	if (!app.Initialize())
	{
		return 1;
	}

	app.Start();

	return 0;
}
