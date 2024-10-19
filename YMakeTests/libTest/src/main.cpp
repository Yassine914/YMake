#include <iostream>

#include <engine/engine.h>

#include <DEngine/DEngine.h>

int main()
{
    std::cout << "Hello, World!" << std::endl;
    std::cout << "SEE: " << __LINE__ << std::endl << std::endl;

    Engine engine;
    engine.run();

    DEngine::CoreEngine coreEngine;
    coreEngine.Run();
}