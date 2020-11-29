// Implements https://cims.nyu.edu/~billbao/report930.pdf.
#pragma once
#include <vector>
#include <cstdint>
#include <numeric>
#include <iostream>
#include <omp.h>

struct I2 
{
    int x,y;
};
struct P2 
{
    float x,y;
};

struct Nbhd 
{
    float vals[9];
    float& operator[](std::size_t idx) { return vals[idx]; }
    const float& operator[](std::size_t idx) const { return vals[idx]; }
};
typedef std::vector<std::vector<Nbhd>> NbhdGrid;

class Fluid
{
private:
    int const m_w, m_h;
    std::vector<std::vector<float>> m_density;
    std::vector<std::vector<P2>> m_vel;
    NbhdGrid m_prob_bufs[2];
    uint8_t m_write_buf; // Buffer index to write to.

    // Lookup table for stream offsets
    // Order: 
    // |-----|
    // |6|2|5|
    // |-----|
    // |3|0|1|
    // |-----|
    // |7|4|8|
    // |-----|
    static const I2 ms_e[9];
    static const float ms_w[9];

    void __streaming (NbhdGrid& prob_write, NbhdGrid const& prob_read)
    {
        // Streaming step for interior. 
        for(int y = 0; y < m_h; ++y)
            #pragma omp parallel for
            for(int x = 0; x < m_w; ++x)
                for(int move_idx = 0; move_idx < 9; ++move_idx)
                {
                    I2 const& off = ms_e[move_idx];
                    
                    int y_plus = (m_w + (y+off.y) % m_w) % m_w;
                    int x_plus = (m_h + (x+off.x) % m_h) % m_h;
                    prob_write[y_plus][x_plus][move_idx] = prob_read[y][x][move_idx];
                } 

//        // Streaming on edges excluding vertices.
//        for(int x = 1; x < m_w - 1; ++x)
//        {
//            // Bottom edge.
//            prob_write[m_h-1][x+1][1] = prob_read[m_h-1][x][1];
//            prob_write[m_h-1][x-1][3] = prob_read[m_h-1][x][3];
//
//            // Top edge.
//            prob_write[0][x+1][1] = prob_read[m_h-1][x][1];
//            prob_write[0][x-1][3] = prob_read[m_h-1][x][3];
//        }
//
//        for(int y = 1; y < m_h - 1; ++x)
//        {
//            // Bottom edge.
//            prob_write[m_h-1][x+1][1] = prob_read[m_h-1][x][1];
//            prob_write[m_h-1][x-1][3] = prob_read[m_h-1][x][3];
//
//            // Top edge.
//            prob_write[0][x+1][1] = prob_read[m_h-1][x][1];
//            prob_write[0][x-1][3] = prob_read[m_h-1][x][3];
//        }
    }

    void __streaming_boundary (NbhdGrid& prob_write)
    {
        // Streaming step for boundary. 

        // First do top and bottom boundaries from left to right.
        for(int x = 1; x < m_w - 1; ++x)
        {
            // Bottom
            prob_write[m_h-1][x][5] = prob_write[m_h-2][x+1][7];
            prob_write[m_h-1][x][2] = prob_write[m_h-2][x][4];
            prob_write[m_h-1][x][6] = prob_write[m_h-2][x-1][8];

            // Top
            prob_write[0][x][7] = prob_write[1][x-1][5];
            prob_write[0][x][4] = prob_write[1][x][2];
            prob_write[0][x][8] = prob_write[1][x+1][6];
        }

        // Next do left and right boundaries from bottom to top.
        for(int y = 1; y < m_h - 1; ++y)
        {
            // Left 
            prob_write[y][0][5] = prob_write[y+1][1][7];
            prob_write[y][0][1] = prob_write[y][1][3];
            prob_write[y][0][8] = prob_write[y-1][1][6];

            // Top
            prob_write[y][m_w-1][7] = prob_write[y-1][m_w-2][5];
            prob_write[y][m_w-1][3] = prob_write[y][m_w-2][1];
            prob_write[y][m_w-1][6] = prob_write[y+1][m_w-2][8];
        }
    }

    void __compute_macroscopic (NbhdGrid const& prob_write)
    {
        for(int y = 0; y < m_h; ++y)
            #pragma omp parallel for
            for(int x = 0; x < m_w; ++x)
            {
                auto const& f = prob_write[y][x].vals;

                // Compute density as sum of probabilities.
                // Formula (3).
                float& rho = m_density[y][x];
                rho = 0;
                for(int i = 0; i < 9; ++i)
                    rho += f[i];

                // Formula (4).
                // Compute velocity as weighted sum.
                m_vel[y][x] = {0,0};
                for(int i = 0; i < 9; ++i)
                {
                    m_vel[y][x].x += f[i] * ms_e[i].x;
                    m_vel[y][x].y += f[i] * ms_e[i].y;
                }
                m_vel[y][x].x *= c / rho;
                m_vel[y][x].y *= c / rho;
            }
    }

    // Inplace collision step that also calculates f_eq intermediately. 
    void __collision_step (NbhdGrid& prob_write)
    {
        for(int y = 0; y < m_h; ++y)
            #pragma omp parallel for
            for(int x = 0; x < m_w; ++x)
                for(int move_idx = 0; move_idx < 9; ++move_idx)
                {
                    P2 const& u = m_vel[y][x];
                    I2 const& e = ms_e[move_idx];
                    float const eu = u.x * e.x + u.y * e.y;
                    float const uu = u.x * u.x + u.y * u.y;

                    // Formula (8).
                    float const w = ms_w[move_idx];

                    // Formula (7).
                    float const s = w * (3*eu/c + 9./2*eu*eu/(c*c) - 3./2*uu/(c*c));

                    // Formula (6).
                    float const f_eq = m_density[y][x] * (w + s);

                    // Formula (5). Update f. 
                    float& f = prob_write[y][x][move_idx];
                    f -= 1/tau * (f - f_eq);
                }
    }

public:
    // Constants. Dx, dt, relaxation time, viscosity, lattice speed, respectively.
    float const dx, dt, tau, v, c;

    Fluid (int const w, int const h, float const dx_, float const dt_, float const tau_)
        : m_w{w}, m_h{h}, 
          m_density(h, std::vector<float>(w, 0)),
          m_vel(h, std::vector<P2>(w, {0,0})),
          m_prob_bufs{
              std::vector<std::vector<Nbhd>>(h, std::vector<Nbhd>(w, {0,0,0,0,0,0,0,0,0})),
              std::vector<std::vector<Nbhd>>(h, std::vector<Nbhd>(w, {0,0,0,0,0,0,0,0,0}))},
          m_write_buf{0},

          dx{dx_}, dt{dt_}, tau{tau_},
          v{(2 * tau - 1)/6 * dx*dx / dt},
          c{dx / dt}
    {
    }

    void Sim ()
    {
        // Get read/write buffers.
        uint8_t const read_buf = (m_write_buf + 1) % 2;
        auto& prob_read{m_prob_bufs[read_buf]}; 
        auto& prob_write{m_prob_bufs[m_write_buf]};

        __streaming(prob_write, prob_read);
//        __streaming_boundary(prob_write);
        __compute_macroscopic(prob_write);
        __collision_step(prob_write);

        // Swap buffers.
        m_write_buf = read_buf;
    }
    float GetViscosity () const {return v;}
    std::vector<std::vector<float>> const& GetDensity () const {return m_density;}
    NbhdGrid& GetNeighborhoods () {return m_prob_bufs[(m_write_buf + 1) % 2];}
};

const I2 Fluid::ms_e[9]
{
    {0,0}, {1,0}, {0,-1}, {-1,0}, {0,1}, {1,-1}, {-1,-1}, {-1,1}, {1,1}
};
const float Fluid::ms_w[9]
{
    4./9, 1./9, 1./9, 1./9, 1./9, 1./36, 1./36, 1./36, 1./36
};
