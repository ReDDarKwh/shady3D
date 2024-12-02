export module mygame;

import app;
import <iostream>;

export class Game : public App
{

protected:
    virtual void Tick()
    {
        if (input.IsDown("forward"))
        {
            std::cout << "forwardd" << std::endl;
        }
    }
};