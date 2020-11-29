#pragma once

namespace Graphics
{
    int setup (int const width, int const height, int const tex_width, int const tex_height);
    void draw (float const* frame);

    int get_width ();
    int get_height ();
};
