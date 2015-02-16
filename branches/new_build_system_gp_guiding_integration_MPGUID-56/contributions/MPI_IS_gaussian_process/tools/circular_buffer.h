/*
 * Copyright 2014-2015, Max Planck Society.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
/*!@file
 * @author Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de>
 * @brief The file holds a CircularBuffer class.
 * @deprecated Raffi: should be replaced by the circular buffer class of the open-phd-guiding project
 */


#ifndef PHD_CIRCULAR_BUFFER
#define PHD_CIRCULAR_BUFFER

#include <Eigen/Dense>

/*!
 * The CircularDoubleBuffer class provides a double vector of limited size.
 * If the vector is full, new data will be appended to the front of the vector, 
 * overriding any previous data.
 *
 * Usage:
 * @code
 *  int max_size = 10;
 *  CircularDoubleBuffer buffer(max_size);
 *  for (int i = 0; i < 20; ++i {
 *    buffer.append(i);
 *  }
 * @endcode
 *
 * When passing the Vector to functions that expect an
 * @code Eigen::VectorXd& @endcode use:
 * @code buffer.getEigenVector() @endcode
 *
 * @note Using @code buffer.get(int) @endcode expects that append has been 
 * called often enough!
 *
 */
class CircularDoubleBuffer {
 private:
  int max_size_;
  int current_position_;
  Eigen::VectorXd buffer_;
  Eigen::VectorXd trimmed_buffer_;
  bool max_size_exceeded_;

 public:
  /*!
   * Constructor of the CircularBuffer class.
   *
   * @param max_size The maximum size of the Buffer.
   */
  explicit CircularDoubleBuffer(int max_size);

  ~CircularDoubleBuffer() {}

  /*!
   * Returns the element at given index.
   *
   * @param index The index of the element in the vector
   * @return The element at the given index
   * @pre The index must fulfill: 0 <= index < max_size_
   */
  double get(int index);

  /**
   * Returns element that was most recently appended.
   * @note Assumes that append has been called at least 1 time
   * @return The last appended element.
   */
  double getLastElement();

  /**
   * Returns the second last appended element.
   * @note Assumes that append has been called at least 2 times.
   * @return The second last appended element.
   */
  double getSecondLastElement();

  /*!
   * Appends a datapoint to the buffer. Potentially overriding previous data,
   * if @code append(data) @endcode was called more than 
   * @code max_size_ @endcode times.
   * 
   * @param data The datapoint that will be appended
   */
  void append(double data);

  /*!
   * Clears the buffer, by setting all values to 0.0 and resetting the current
   * position to 0
   */
  void clear();

  /*!
   * Returns (a pointer to) the underlying @code Eigen::VectorXd @endcode 
   * object. If max_size_ has not been exceeded so far, the vector is trimmed to
   * represent only the vector of already appended data.
   * 
   * We need this function in order to pass the CircularBuffer object to a
   * function that expects a @code Eigen::VectorXd @endcode
   */
  Eigen::VectorXd* getEigenVector();
};


#endif  // PHD_CIRCULAR_BUFFER
