#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>

class Engine
{
    public:
    Engine()  = default;
    ~Engine() = default;

    void run() { std::cout << "Hello From Engine!" << std::endl; }
};

#endif // ENGINE_H