#pragma once 
namespace math{
template<typename T,
    template <typename, typename = std::allocator<T> > class Container>
double average(const Container<T>& data)
{
    int len  = data.size();
    double sum = 0.0;
    for (auto x : data)
        sum += x;
    return sum / len;
}

template<typename T,
    template <typename, typename = std::allocator<T> > class Container>
double variance(const Container<T>& data)
{
    int len  = data.size();
    double sum = 0.0;
    double avg = average(data);
    for (auto x : data)
        sum += pow(x - avg, 2);
    return sum / len;
}

template<typename T,
    template <typename, typename = std::allocator<T> > class Container>
double standar_deviation(const Container<T>& data)
{
    double sd = variance(data);
    return sqrt(sd);
}
}