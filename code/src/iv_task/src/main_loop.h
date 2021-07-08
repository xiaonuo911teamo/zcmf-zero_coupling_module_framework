#ifndef MAINLOOP_H
#define MAINLOOP_H
#include <pipe/timer_element.hpp>

class MainLoop : public TimerElement
{
public:
    MainLoop();

    // TimerElement interface
    bool is_restart() const;

private:
    virtual void timer_func() override;
    bool _is_restart = false;
};

#endif // MAINLOOP_H
