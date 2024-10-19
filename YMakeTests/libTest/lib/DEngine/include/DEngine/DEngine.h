#ifndef DENGINE_H
#define DENGINE_H

namespace DEngine {

class CoreEngine
{
    private:
    int windowHandle;

    public:
    CoreEngine() = default;

    void Run();

    int GetWindowHandle() { return windowHandle; }
    void SetWindowHandle(int handle) { windowHandle = handle; }

    ~CoreEngine() = default;
};

} // namespace DEngine

#endif // DENGINE_H