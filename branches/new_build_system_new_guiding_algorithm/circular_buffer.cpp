//
//  circular_buffer.cpp
//  PhD2
//
//  Created by Stephan Wenninger on 30/10/14.
//
//

#include "circular_buffer.h"

CircularDoubleBuffer::CircularDoubleBuffer(int max_size)
: max_size_(max_size),
current_position_(0),
buffer(max_size) {}

double CircularDoubleBuffer::get(int index) {
    return buffer[index];
}

void CircularDoubleBuffer::append(double data) {
    buffer[current_position_] = data;
    current_position_ = (current_position_ + 1) % max_size_;
}

void CircularDoubleBuffer::clear() {
    current_position_ = 0;
    buffer.segment(0, max_size_) *= 0.0;
}

int CircularDoubleBuffer::lastElementIndex() {
    return (current_position_ - 1) % max_size_;
}
