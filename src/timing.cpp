#include "../include/timing.hpp"

void timing::start(){
    this->_start = std::chrono::system_clock::now();
}
void timing::end(){
    this->_end = std::chrono::system_clock::now();
    this->_timeElapse = (_end - _start);
}

double timing::get_elapse(){
    return this->_timeElapse.count();
}