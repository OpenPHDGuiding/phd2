/*
 *  circbuf.h
 *  PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2013 Andy Galasso
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs,
 *     Bret McKee, Dad Dog Development, Ltd, nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef CIRCBUF_INCLUDED
#define CIRCBUF_INCLUDED

template<typename T>
class circular_buffer
{
    T *m_ary;
    unsigned int m_head;
    unsigned int m_tail;
    unsigned int m_size;
    unsigned int m_capacity;
public:
    class iterator
    {
        friend class circular_buffer<T>;
        circular_buffer<T>& m_cb;
        unsigned int m_pos;
        iterator(circular_buffer<T>& cb, unsigned int pos) : m_cb(cb), m_pos(pos) { }
    public:
        iterator& operator++() { ++m_pos; return *this; }
        iterator operator++(int) { iterator it(*this); m_pos++; return it; }
        bool operator==(const iterator& rhs) const { assert(&m_cb == &rhs.m_cb); return m_pos == rhs.m_pos; }
        bool operator!=(const iterator& rhs) const { assert(&m_cb == &rhs.m_cb); return m_pos != rhs.m_pos; }
        T& operator*() const { return m_cb.m_ary[m_pos % m_cb.m_capacity]; }
        T* operator->() const { return &m_cb.m_ary[m_pos % m_cb.m_capacity]; }
    };
    friend class circular_buffer<T>::iterator;
    circular_buffer();
    circular_buffer(unsigned int capacity);
    ~circular_buffer();
    void resize(unsigned int capacity);
    void push_front(const T& t);
    void pop_back(unsigned int n = 1);
    void clear();
    T& operator[](unsigned int n) const;
    unsigned int size() const { return m_size; }
    unsigned int capacity() const { return m_capacity; }
    iterator begin() { return iterator(*this, m_tail); }
    iterator end() { return iterator(*this, m_tail + m_size); }
};

template<typename T>
circular_buffer<T>::circular_buffer()
    : m_ary(0),
    m_head(0),
    m_tail(0),
    m_size(0),
    m_capacity(0)
{
}

template<typename T>
circular_buffer<T>::circular_buffer(unsigned int capacity)
    : m_ary(new T[capacity]),
    m_head(0),
    m_tail(0),
    m_size(0),
    m_capacity(capacity)
{
    assert(capacity > 0);
}

template<typename T>
circular_buffer<T>::~circular_buffer()
{
    delete [] m_ary;
}

template<typename T>
void circular_buffer<T>::resize(unsigned int capacity)
{
    assert(capacity > 0);
    assert(m_ary == 0);
    m_ary = new T[capacity];
    m_capacity = capacity;
}

template<typename T>
void circular_buffer<T>::clear()
{
    m_head = m_tail = m_size = 0;
}

template<typename T>
void circular_buffer<T>::push_front(const T& t)
{
    m_ary[m_head] = t;
    m_head = (m_head + 1) % m_capacity;
    if (m_size == m_capacity)
    {
        m_tail = (m_tail + 1) % m_capacity;
    }
    else
    {
        ++m_size;
    }
}

template<typename T>
void circular_buffer<T>::pop_back(unsigned int n)
{
    assert(m_size >= n);
    m_tail = (m_tail + n) % m_capacity;
    m_size -= n;
}

template<typename T>
T& circular_buffer<T>::operator[](unsigned int n) const
{
    assert(n < m_size);
    return m_ary[(m_tail + n) % m_capacity];
}

#endif
