// Copyright (c) 2014 Max Planck Society

#include "circular_buffer.h"

CircularDoubleBuffer::CircularDoubleBuffer(int max_size)
: max_size_(max_size),
current_position_(0),
buffer_(max_size),
tmp_(),
max_size_exceeded_(false) {}

double CircularDoubleBuffer::get(int index) {
    return buffer_[index];
}

void CircularDoubleBuffer::append(double data) {
    buffer_[current_position_] = data;
    current_position_ = (current_position_ + 1) % max_size_;
    if (current_position_ == 0) {
      max_size_exceeded_ = true;
    }
}

void CircularDoubleBuffer::clear() {
    current_position_ = 0;
    buffer_.segment(0, max_size_) *= 0.0;
    max_size_exceeded_ = false;
}

int CircularDoubleBuffer::lastElementIndex() {
    return (current_position_ - 1) % max_size_;
}

Eigen::VectorXd* CircularDoubleBuffer::getEigenVector() {
  if (max_size_exceeded_) {
    return &buffer_;
  } else {
    tmp_ = buffer_.head(current_position_);
    return &tmp_;
  }
}
