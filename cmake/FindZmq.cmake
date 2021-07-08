set(ZMQ_FOUND TRUE)
message(zmq found: ${THIRD_PARTY_DIR}/zeromq)
set(ZMQ_INCLUDE_DIR ${THIRD_PARTY_DIR}/zeromq/include)
file(GLOB_RECURSE ZMQ_LIBRARIES ${THIRD_PARTY_DIR}/zeromq/lib/lib*.so)
