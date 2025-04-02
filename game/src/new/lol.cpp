#include <type_traits>

#include <ncurses.h>

#include "../dungeon_config.h"


static inline int nc_init()
{
    initscr();
    raw();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();
    set_escdelay(0);
}


class NCWindowBase
{
protected:
    inline static size_t nwin{ 0 };

};

template<class Derived_T>
class NCWindow : public NCWindowBase
{
    static_assert(std::is_base_of<NCWindow<Derived_T>, Derived_T>::value);

public:
    inline NCWindow(
        int szy,
        int szx,
        int y,
        int x )
    {
        if(!NCWindowBase::nwin)
        {
            ic_init();
        }
        NCWindowBase::nwin++;

        this->win = newwin(szy, szx, y, x);
    }
    virtual inline ~NCWindow()
    {
        delwin(this->win);

        NCWindowBase::nwin--;
        if(!NCWindowBase::nwin)
        {
            endwin();
        }
    }

public:
    WINDOW* win;

};

class MapWindow : public NCWindow<MapWindow>
{
public:
    enum
    {
        MAP_FOG = 0,
        MAP_DUNGEON,
        MAP_HARDNESS,
        MAP_FWEIGHT,
        MAP_TWEIGHT
    };

public:
    // inline MapWindow() : 
    // ~MapWindow() = default;

protected:
    

};
