#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <chrono>

const int width  = 80;
const int height = 22;

const float cubeSize   = 20.0f;
const float distance   = 3.0f;
const float angleSpeed = 0.05f;

// clang-format off
struct Vertex {
    float x, y, z;
};

std::vector<Vertex> vertices = {
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
    {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
};

std::vector<std::pair<int, int>> edges = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

// clang-format on

void rotate(Vertex &v, float angleX, float angleY, float angleZ)
{
    float cosX = cos(angleX), sinX = sin(angleX);
    float cosY = cos(angleY), sinY = sin(angleY);
    float cosZ = cos(angleZ), sinZ = sin(angleZ);

    float y = v.y * cosX - v.z * sinX;
    float z = v.y * sinX + v.z * cosX;
    v.y     = y;
    v.z     = z;

    float x = v.x * cosY + v.z * sinY;
    z       = -v.x * sinY + v.z * cosY;
    v.x     = x;
    v.z     = z;

    x   = v.x * cosZ - v.y * sinZ;
    y   = v.x * sinZ + v.y * cosZ;
    v.x = x;
    v.y = y;
}

void project(const Vertex &v, int &x, int &y)
{
    float factor = distance / (distance + v.z);

    x = static_cast<int>(v.x * factor * cubeSize + width / 2);
    y = static_cast<int>(-v.y * factor * cubeSize + height / 2);
}

void drawCube(float angleX, float angleY, float angleZ)
{
    std::vector<Vertex> transformedVertices = vertices;
    for(auto &v : transformedVertices)
    {
        rotate(v, angleX, angleY, angleZ);
    }

    std::vector<std::string> screen(height, std::string(width, ' '));
    for(const auto &edge : edges)
    {
        int x1, y1, x2, y2;
        project(transformedVertices[edge.first], x1, y1);
        project(transformedVertices[edge.second], x2, y2);

        int dx = abs(x2 - x1), dy = abs(y2 - y1);
        int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
        int err = dx - dy;

        while(true)
        {
            if(x1 >= 0 && x1 < width && y1 >= 0 && y1 < height)
            {
                screen[y1][x1] = '#';
            }
            if(x1 == x2 && y1 == y2)
                break;
            int e2 = err * 2;
            if(e2 > -dy)
            {
                err -= dy;
                x1 += sx;
            }
            if(e2 < dx)
            {
                err += dx;
                y1 += sy;
            }
        }
    }

    for(const auto &line : screen)
    {
        std::cout << line << '\n';
    }
}

int main()
{
    float angleX = 0, angleY = 0, angleZ = 0;
    while(true)
    {
        std::cout << "\033[2J\033[H"; // Clear the screen
        drawCube(angleX, angleY, angleZ);
        angleX += angleSpeed;
        angleY += angleSpeed;
        angleZ += angleSpeed;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}