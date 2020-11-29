#include "fluid.hpp"
#include "graphics.h"
#include <fstream>
#include <random>
#include <algorithm>

int main () 
{
    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<> dis(0.001, 0.005);

    int w{100}, h{100};
    Fluid fluid(w, h, 1, 1, 1);
    auto& prob = fluid.GetNeighborhoods();
    for(int y = 0; y < h; ++y)
        for(int x = 0; x < w; ++x)
            for(int i = 0; i < 9; ++i)
            {
                if(x > 50 && x < 75 && y > 50 && y < 75 && i == 1)
                    prob[y][x][i] = 2./9;
                else 
                    prob[y][x][i] = 1./9;
            }

//    for(int y = h/5; y < 2*h/5; ++y)
//        for(int x = 0; x < 20; ++x)
//        {
//            prob[y][x][0] = 2. / 4;
//            prob[y][x][1] = 2. / 4;
//        }

//    for(int y = 0; y < 20; ++y)
//    {
//        for(int x = 0; x < 3; ++x)
//        {
//            prob[y + h/2 + 10][x + w/2][1] += 1./10;
//            prob[h/2 - y - 10][x + w/2][3] += 1./10;
//        }
//    }
//
//    for(int y = 0; y < 3; ++y)
//    {
//        for(int x = 0; x < 20; ++x)
//        {
//            prob[h/2 + y][x + w/2 + 10][2] += 1./10;
//            prob[h/2 + y][w/2 - x - 10][4] += 1./10;
//        }
//    }

    Graphics::setup(1920, 1080, w, h);
    std::vector<float> color(w*h);
    for(int t = 0;; ++t) 
    {
        auto const& rho{fluid.GetDensity()};
        float mx = -1, mn = std::numeric_limits<int>::max();
        for(int y = 0; y < h; ++y)
            #pragma omp parallel for
            for(int x = 0; x < w; ++x)
            {
                if(rho[y][x] > mx)
                    mx = rho[y][x];
                if(rho[y][x] < mn)
                    mn = rho[y][x];
            }

        for(int y = 0; y < h; ++y)
            #pragma omp parallel for
            for(int x = 0; x < w; ++x)
                color[(h - 1 - y) * h + x] = (rho[y][x] - mn) / (mx - mn);

        Graphics::draw(color.data());
        fluid.Sim();
    }
}
