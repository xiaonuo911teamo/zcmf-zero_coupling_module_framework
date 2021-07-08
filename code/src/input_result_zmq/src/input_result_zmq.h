
#include <pipe/timer_element.hpp>
#include <zmq/zmq.hpp>
#include <core/double_buffer_data.hpp>
#include <data/road_level_result.pb.h>

class InputResultZmqElement : public basic::PipeElement
{
public:
    InputResultZmqElement();
    virtual ~InputResultZmqElement();
    
private:
    ZmqSubscriber _zmq_subscriber;
    std::string _zmq_buffer;
    DoubleBufferData<message::RoadLevelResult> _buffered_road_result;

private:    
    virtual void thread_func() override;

    // PipeElement interface
private:
    virtual void thread_closing() override;
};
