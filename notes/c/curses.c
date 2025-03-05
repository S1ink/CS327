#include <ncurses.h>
#include <unistd.h>


int main(int argc, char** argv)
{
    int i;
    char a[4] = "\\|/-";

    initscr();
    // raw();       --> necessary for keystrokes that are more than a single character, ex. Ctrl+X
    // noecho();    --> "when I type a key, don't type the key"
    // curs_set();  --> turn off display of the cursor
    // keypad(stdscr, TRUE);    --> turn on the keypad
    // start_color();
    // init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK) --> defeine a color scheme (name, forground, background)

    // getch() --> get a composed key (int32_t)
    // mvprintw(y, x, fmt, ...)  --> move and print to a window --> printf but at a specific location in the window
    // attron(...) --> turn on an attrubute on (bold, underline, etc)
    // attroff(...) --> turn off an attribute

    for(i = 0; i < 320; i++)
    {
        usleep(25000);
        mvaddch(23, i / 4, a[i % 4]);
        refresh();
    }

    endwin();

    return 0;
}
