export module input;

import <GLFW/glfw3.h>;
import <fstream>;
import <nlohmann/json.hpp>;
import <set>;
using json = nlohmann::json;

export class Input
{

    friend class App;

private:
    GLFWwindow *window;
    std::map<int, std::string> mappings;
    std::set<std::string> pressedActions;
    std::set<std::string> downThisFrameActions;

protected:
    void EndFrame(){
        downThisFrameActions.clear();
    }

    void OnKey(int key, int actionType)
    {
        if (mappings.find(key) == mappings.end())
            return;

        if (actionType == GLFW_PRESS)
        {
            pressedActions.insert({mappings[key]});
            downThisFrameActions.insert({mappings[key]});
        }
        else if (actionType == GLFW_RELEASE)
        {
            pressedActions.erase(mappings[key]);
        }
    }

    void OnMouseMove(float, float){

    }

public:
    Input() {}

    Input(GLFWwindow *inWindow)
    {

        window = inWindow;

        std::ifstream f("resources/config/input.json");
        auto data = json::parse(f);
        for (auto &[key, value] : data.items())
        {
            mappings.insert({value.get<int>(), key});
        }
    }

    bool IsDown(std::string action){
        return downThisFrameActions.find(action) != downThisFrameActions.end();
    }

    bool IsPressed(std::string action){
        return pressedActions.find(action) != pressedActions.end();
    }
};