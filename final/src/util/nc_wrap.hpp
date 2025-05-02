#pragma once

#include <array>

#include <cstdarg>

#include <ncurses.h>

#include "math.hpp"


template<NCURSES_COLOR_T GNum = 128, NCURSES_COLOR_T PairOff = 128>
class NCGradient_
{
    static_assert((GNum <= (256 - 8)) && (PairOff >= 8) && (PairOff + GNum <= 256));

public:
    using ColorElemT = NCURSES_COLOR_T;
    using ArrColorT = std::array<ColorElemT, 3>;
    using GradientT = std::array<ArrColorT, GNum>;

    enum
    {
        COLORMAP_VIRIDIS
    };

public:
    static void generate(
        GradientT& grad,
        const ArrColorT& a,
        const ArrColorT& b )
    {
        const ColorElemT
            dr = b[0] - a[0],
            dg = b[1] - a[1],
            db = b[2] - a[2];

        for(ColorElemT i = 0; i < static_cast<ColorElemT>(GNum); i++)
        {
            grad[i][0] = a[0] + ((dr * i) / static_cast<ColorElemT>(GNum - 1));
            grad[i][1] = a[1] + ((dg * i) / static_cast<ColorElemT>(GNum - 1));
            grad[i][2] = a[2] + ((db * i) / static_cast<ColorElemT>(GNum - 1));
        }
    }

    static void generate_viridis(GradientT& grad)
    {
        constexpr NCGradient_<128>::GradientT
            VIRIDIS_RGB{ {
                {267,  4, 329}, {269, 14, 341}, {272, 25, 353}, {274, 37, 364},
                {277, 50, 375}, {279, 63, 386}, {281, 75, 397}, {283, 88, 408},
                {285,101,418}, {286,113,428}, {287,125,437}, {288,137,446},
                {289,149,455}, {289,160,463}, {289,172,471}, {289,183,478},
                {289,194,485}, {289,204,491}, {288,215,496}, {287,225,501},
                {286,235,506}, {284,245,510}, {282,254,513}, {280,263,516},
                {278,272,518}, {275,281,519}, {272,289,520}, {269,297,521},
                {266,305,521}, {262,313,520}, {258,320,519}, {254,328,517},
                {250,335,515}, {245,341,513}, {241,348,510}, {236,354,507},
                {231,360,503}, {226,366,498}, {221,372,493}, {215,377,488},
                {210,382,482}, {204,387,476}, {198,392,470}, {193,396,463},
                {187,401,456}, {181,405,449}, {175,409,441}, {169,412,434},
                {163,416,426}, {157,419,418}, {151,422,410}, {145,425,402},
                {139,428,393}, {133,430,385}, {127,433,376}, {121,435,368},
                {115,437,359}, {110,439,350}, {104,440,341}, { 98,442,332},
                { 93,443,323}, { 87,444,314}, { 82,445,305}, { 76,446,296},
                { 71,446,287}, { 66,447,278}, { 61,447,269}, { 56,447,260},
                { 51,448,251}, { 46,448,242}, { 42,447,234}, { 37,447,225},
                { 33,447,216}, { 29,446,208}, { 25,445,199}, { 21,444,191},
                { 18,443,183}, { 14,442,175}, { 11,440,167}, {  8,439,159},
                {  5,437,151}, {  3,435,144}, {  1,433,137}, {  0,431,130},
                {  0,428,123}, {  0,426,116}, {  1,423,109}, {  1,420,102},
                {  2,417, 96}, {  3,414, 89}, {  4,410, 83}, {  5,407, 77},
                { 6,403, 71}, { 7,399, 65}, { 8,395, 60}, {10,391, 54},
                {11,386, 49}, {12,382, 43}, {13,377, 38}, {14,372, 33},
                {15,367, 28}, {16,362, 24}, {17,357, 19}, {18,352, 15},
                {18,346, 10}, {19,340,  6}, {20,334,  2}, {20,328,  0},
                {21,322,  0}, {21,315,  0}, {22,309,  0}, {22,302,  0},
                {23,296,  0}, {23,289,  0}, {23,282,  0}, {24,275,  0},
                {24,268,  0}, {24,261,  0}, {24,253,  0}, {24,246,  0},
                {25,238,  0}, {25,231,  0}, {25,223,  0}, {25,215,  0},
                {25,207,  0}, {25,199,  0}, {25,191,  0}, {25,183,  0}
            } };

        if constexpr(GNum == 128)
        {
            grad = VIRIDIS_RGB;
        }
        else
        {
            for(ColorElemT i = 0; i < GNum; i++)
            {
                grad[i] = VIRIDIS_RGB[i * 128 / (GNum - 1)];
            }
        }
    }

    inline static NCURSES_COLOR_T floatToIdx(float f)
    {
        return static_cast<NCURSES_COLOR_T>(MAX(MIN(f, 1.), 0.) * (GNum - 1));
    }

public:
    inline NCGradient_(
        ColorElemT ar,
        ColorElemT ag,
        ColorElemT ab,
        ColorElemT br,
        ColorElemT bg,
        ColorElemT bb ) : NCGradient_( {ar, ag, ab}, {br, bg, bb} )
    {}
    inline NCGradient_(const ArrColorT& a, const ArrColorT& b)
    {
        generate(this->grad, a, b);
    }
    inline NCGradient_(int colormap)
    {
        switch(colormap)
        {
            default:
            case COLORMAP_VIRIDIS:
            {
                generate_viridis(this->grad);
                break;
            }
        }
    }
    ~NCGradient_() = default;

public:
    void initialize()
    {
        for(ColorElemT i = 0; i < GNum; i++)
        {
            init_color(PairOff + i, this->grad[i][0], this->grad[i][1], this->grad[i][2]);
        }
    }
    void applyForeground(ColorElemT bg)
    {
        for(ColorElemT i = 0; i < GNum; i++)
        {
            const ColorElemT idx = (PairOff + i);

            init_color(idx, this->grad[i][0], this->grad[i][1], this->grad[i][2]);
            init_pair(idx, idx, bg);
        }
    }
    void applyBackground()
    {
        for(ColorElemT i = 0; i < GNum; i++)
        {
            const ColorElemT idx = (PairOff + i);

            init_color(idx, grad[i][0], grad[i][1], grad[i][2]);
            init_pair(idx, (grad[i][0] + grad[i][1] + grad[i][2] > 1500 ? COLOR_BLACK : COLOR_WHITE), idx);
        }
    }

    void printChar(WINDOW* w, int y, int x, NCURSES_PAIRS_T idx, chtype c)
    {
        wattron(w, COLOR_PAIR(PairOff + idx));
        mvwaddch(w, y, x, c);
        wattroff(w, COLOR_PAIR(PairOff + idx));
    }
    template<typename... ArgT>
    void printf(WINDOW* w, int y, int x, NCURSES_PAIRS_T idx, const char* fmt, ArgT ...args)
    {
        wattron(w, COLOR_PAIR(PairOff + idx));
        mvwprintw(w, y, x, fmt, args...);
        wattroff(w, COLOR_PAIR(PairOff + idx));
    }

protected:
    GradientT grad;

};

using NCGradient = NCGradient_<>;

template<NCURSES_COLOR_T PairOff>
using NCGradient8 = NCGradient_<8, PairOff>;
template<NCURSES_COLOR_T PairOff>
using NCGradient16 = NCGradient_<16, PairOff>;
template<NCURSES_COLOR_T PairOff>
using NCGradient32 = NCGradient_<32, PairOff>;
template<NCURSES_COLOR_T PairOff>
using NCGradient64 = NCGradient_<64, PairOff>;



class NCInitializer
{
public:
    static inline void use()
    {
        if(!nwin)
        {
            initscr();

            raw();
            noecho();
            curs_set(0);
            keypad(stdscr, TRUE);
            start_color();
            set_escdelay(0);

            for(NCURSES_COLOR_T i = 0; i < 8; i++) init_pair(i, i, COLOR_BLACK);
        }
        nwin++;
    }
    static inline void unuse()
    {
        if(nwin)
        {
            nwin--;
            if(!nwin)
            {
                endwin();
            }
        }
    }
    static inline void shutdown()
    {
        if(nwin)
        {
            nwin = 0;
            endwin();
        }
    }

protected:
    inline NCInitializer()
    {
        NCInitializer::use();
    }
    inline virtual ~NCInitializer()
    {
        NCInitializer::unuse();
    }

protected:
    inline static size_t nwin{ 0 };

};


class NCWindow : public NCInitializer
{
public:
    inline NCWindow( int szy, int szx, int y, int x )
        : NCInitializer(), win{ newwin(szy, szx, y, x) }
    {}
    inline virtual ~NCWindow()
    {
        delwin(this->win);
    }

public:
    inline void setScrollable(bool s)
    {
        scrollok(this->win, s);
        idlok(this->win, s);
    }
    inline void printBox()
    {
        box(this->win, 0, 0);
    }

    // touches and refreshes
    inline void overwrite()
    {
        touchwin(this->win);
        wrefresh(this->win);
    }
    inline void refresh()
    {
        wrefresh(this->win);
    }

public:
    WINDOW* const win;

};
