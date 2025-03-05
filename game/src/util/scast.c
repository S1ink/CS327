#include <math.h>

typedef unsigned int uint;

static int multipliers[4][8] =
{
    {1, 0, 0, -1, -1, 0, 0, 1},
    {0, 1, -1, 0, 0, -1, 1, 0},
    {0, 1, 1, 0, 0, -1, -1, 0},
    {1, 0, 0, 1, -1, 0, 0, -1}
};

typedef struct
{
    
}
Map;


void cast_light(Map* map, uint x, uint y, uint radius, uint row,
        float start_slope, float end_slope, uint xx, uint xy, uint yx,
        uint yy)
{
    if(start_slope < end_slope)
    {
        return;
    }
    float next_start_slope = start_slope;
    for(uint i = row; i <= radius; i++)
    {
        int blocked = 0;
        for(int dx = -i, dy = -i; dx <= 0; dx++)
        {
            float l_slope = (dx - 0.5) / (dy + 0.5);
            float r_slope = (dx + 0.5) / (dy - 0.5);
            if(start_slope < r_slope)
            {
                continue;
            }
            else if(end_slope > l_slope)
            {
                break;
            }

            int sax = dx * xx + dy * xy;
            int say = dx * yx + dy * yy;
            if( (sax < 0 && (uint)abs(sax) > x) || (say < 0 && (uint)abs(say) > y) )
            {
                continue;
            }
            uint ax = x + sax;
            uint ay = y + say;
            if(ax >= map.get_width() || ay >= map.get_height())
            {
                continue;
            }

            uint radius2 = radius * radius;
            if((uint)(dx * dx + dy * dy) < radius2)
            {
                map.set_visible(ax, ay, 1);
            }

            if(blocked)
            {
                if (map.is_opaque(ax, ay))
                {
                    next_start_slope = r_slope;
                    continue;
                }
                else
                {
                    blocked = 0;
                    start_slope = next_start_slope;
                }
            }
            else if(map.is_opaque(ax, ay))
            {
                blocked = 1;
                next_start_slope = r_slope;
                cast_light(map, x, y, radius, i + 1, start_slope, l_slope, xx,
                        xy, yx, yy);
            }
        }
        if(blocked)
        {
            break;
        }
    }
}

void do_fov(Map* map, uint x, uint y, uint radius)
{
    for (uint i = 0; i < 8; i++)
    {
        cast_light(map, x, y, radius, 1, 1.0, 0.0, multipliers[0][i],
                multipliers[1][i], multipliers[2][i], multipliers[3][i]);
    }
}