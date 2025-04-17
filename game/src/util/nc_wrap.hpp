#pragma once

#include <array>

#include <ncurses.h>


template<NCURSES_COLOR_T GNum = 128, NCURSES_COLOR_T PairOff = 128>
class NCGradient_
{
    static_assert((GNum <= (256 - 8)) && (PairOff >= 8) && (PairOff + GNum <= 256));

public:
    using ColorElemT = NCURSES_COLOR_T;
    using ArrColorT = std::array<ColorElemT, 3>;
    using GradientT = std::array<ArrColorT, GNum>;

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
