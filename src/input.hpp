#pragma once

#include <GLFW/glfw3.h>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// ...

struct ActionMapping
{
    std::string name;
    int key;
};

void from_json(const nlohmann::json &j, ActionMapping &mapping)
{
    j.at("name").get_to(mapping.name);
    j.at("key").get_to(mapping.key);
}

class Input
{

private:
    GLFWwindow *window;

public:
    Input() {}

    Input(GLFWwindow *inWindow)
    {

        window = inWindow;

        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, [](GLFWwindow *, int key, int, int, int)
                           { std::cout << key << std::endl; });

        std::ifstream f("resources/config/input.json");
        auto data = json::parse(f);

        auto mappings = data.get<std::vector<ActionMapping>>();

        std::cout << data << std::endl;
    }

    // void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
    // {
    //     std::cout << key << std::endl;
    // }
};