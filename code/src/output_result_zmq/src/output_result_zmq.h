
#include <pipe/pipe_element.hpp>
#include <zmq/zmq.hpp>
#include <data/data_info.pb.h>
#include <core/double_buffered_vector.hpp>

class OutputResultZmqElement : public basic::PipeElement
{
public:
    OutputResultZmqElement();
    virtual ~OutputResultZmqElement();
private:
    ZmqPublisher _zmq_publisher;

    DoubleBufferedVector<std::shared_ptr<message::BytesItem>> _buffered_data;
private:
    virtual void thread_func() override;

    // PipeElement interface
private:
    virtual void thread_closing() override;
};

