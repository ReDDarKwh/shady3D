#pragma once

#include "engine/app.hpp"

class Game : public App
{

protected:
    virtual void Tick()
    {
        if (input.IsDown("forward"))
        {
            std::cout << "forward" << std::endl;
        }
    }
};