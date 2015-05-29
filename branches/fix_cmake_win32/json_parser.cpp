/*
 *  json_parser.cpp
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
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
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
 * This file contains a modified version of vjson, which comes with the following
 * license:
 *
 * Copyright (c) 2010, Ivan Vashchaev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include "phd.h"
#include "json_parser.h"

#include <algorithm>
#include <memory.h>

class block_allocator
{
private:
    struct block
    {
        size_t size;
        size_t used;
        block *next;
    };

    block *m_head;
    size_t m_blocksize;

    block_allocator(const block_allocator &);
    block_allocator &operator=(block_allocator &);

    // exchange contents with rhs
    void swap(block_allocator &rhs);

public:
    block_allocator(size_t blocksize);
    ~block_allocator();

    // allocate memory
    void *malloc(size_t size);

    // reset to empty state, keeping one allocated block
    void reset();

    // free all allocated blocks
    void free();
};

block_allocator::block_allocator(size_t blocksize): m_head(0), m_blocksize(blocksize)
{
}

block_allocator::~block_allocator()
{
    while (m_head)
    {
        block *temp = m_head->next;
        ::free(m_head);
        m_head = temp;
    }
}

void block_allocator::reset()
{
    if (m_head)
    {
        block *b = m_head->next;
        m_head->next = 0;
        while (b)
        {
            block *t = b->next;
            ::free(b);
            b = t;
        }
        m_head->used = sizeof(block);
    }
}

void block_allocator::swap(block_allocator& rhs)
{
    std::swap(m_blocksize, rhs.m_blocksize);
    std::swap(m_head, rhs.m_head);
}

void *block_allocator::malloc(size_t size)
{
    if (!m_head || m_head->used + size > m_head->size)
    {
        // calc needed size for allocation
        size_t alloc_size = std::max(sizeof(block) + size, m_blocksize);

        // create new block
        block *b = (block *)::malloc(alloc_size);
        b->size = alloc_size;
        b->used = sizeof(block);
        b->next = m_head;
        m_head = b;
    }

    void *ptr = (char *) m_head + m_head->used;
    m_head->used += size;
    return ptr;
}

void block_allocator::free()
{
    if (m_head)
        block_allocator(0).swap(*this);
}

// true if character represent a digit
#define IS_DIGIT(c) (c >= '0' && c <= '9')

// convert string to integer
static char *atoi(char *first, char *last, int *out)
{
    int sign = 1;
    if (first != last)
    {
        if (*first == '-')
        {
            sign = -1;
            ++first;
        }
        else if (*first == '+')
        {
            ++first;
        }
    }

    int result = 0;
    for (; first != last && IS_DIGIT(*first); ++first)
    {
        result = 10 * result + (*first - '0');
    }
    *out = result * sign;

    return first;
}

// convert hexadecimal string to unsigned integer
static char *hatoui(char *first, char *last, unsigned int *out)
{
    unsigned int result = 0;
    for (; first != last; ++first)
    {
        int digit;
        if (IS_DIGIT(*first))
        {
            digit = *first - '0';
        }
        else if (*first >= 'a' && *first <= 'f')
        {
            digit = *first - 'a' + 10;
        }
        else if (*first >= 'A' && *first <= 'F')
        {
            digit = *first - 'A' + 10;
        }
        else
        {
            break;
        }
        result = 16 * result + digit;
    }
    *out = result;

    return first;
}

// convert string to floating point
static char *atof(char *first, char *last, float *out)
{
    // sign
    float sign = 1;
    if (first != last)
    {
        if (*first == '-')
        {
            sign = -1;
            ++first;
        }
        else if (*first == '+')
        {
            ++first;
        }
    }

    // integer part
    float result = 0;
    for (; first != last && IS_DIGIT(*first); ++first)
    {
        result = 10 * result + (*first - '0');
    }

    // fraction part
    if (first != last && *first == '.')
    {
        ++first;

        float inv_base = 0.1f;
        for (; first != last && IS_DIGIT(*first); ++first)
        {
            result += (*first - '0') * inv_base;
            inv_base *= 0.1f;
        }
    }

    // result w\o exponent
    result *= sign;

    // exponent
    bool exponent_negative = false;
    int exponent = 0;
    if (first != last && (*first == 'e' || *first == 'E'))
    {
        ++first;

        if (*first == '-')
        {
            exponent_negative = true;
            ++first;
        }
        else if (*first == '+')
        {
            ++first;
        }

        for (; first != last && IS_DIGIT(*first); ++first)
        {
            exponent = 10 * exponent + (*first - '0');
        }
    }

    if (exponent)
    {
        float power_of_ten = 10;
        for (; exponent > 1; exponent--)
        {
            power_of_ten *= 10;
        }

        if (exponent_negative)
        {
            result /= power_of_ten;
        }
        else
        {
            result *= power_of_ten;
        }
    }

    *out = result;

    return first;
}

static json_value *json_alloc(block_allocator *allocator)
{
    json_value *value = (json_value *)allocator->malloc(sizeof(json_value));
    memset(value, 0, sizeof(json_value));
    return value;
}

static void json_append(json_value *lhs, json_value *rhs)
{
    rhs->parent = lhs;
    if (lhs->last_child)
    {
        lhs->last_child = lhs->last_child->next_sibling = rhs;
    }
    else
    {
        lhs->first_child = lhs->last_child = rhs;
    }
}

#define JSON_ERROR(it, desc) do { \
    *error_pos = it; \
    *error_desc = desc; \
    *error_line = 1 - escaped_newlines; \
    for (const char *c = it; c != source; --c) \
        if (*c == '\n') ++*error_line; \
    return 0; \
} while (false)

#define CHECK_TOP() if (!top) JSON_ERROR(it, "Unexpected character")

static json_value *json_parse(char *source, const char **error_pos,
                              const char **error_desc, int *error_line,
                              block_allocator *allocator)
{
    json_value *root = 0;
    json_value *top = 0;

    char *name = 0;
    char *it = source;

    int escaped_newlines = 0;

    while (*it)
    {
        switch (*it)
        {
        case '{':
        case '[':
            {
                // create new value
                json_value *object = json_alloc(allocator);

                // name
                object->name = name;
                name = 0;

                // type
                object->type = (*it == '{') ? JSON_OBJECT : JSON_ARRAY;

                // skip open character
                ++it;

                // set top and root
                if (top)
                {
                    json_append(top, object);
                }
                else if (!root)
                {
                    root = object;
                }
                else
                {
                    JSON_ERROR(it, "Second root. Only one root allowed");
                }
                top = object;
            }
            break;

        case '}':
        case ']':
            {
                if (!top || top->type != ((*it == '}') ? JSON_OBJECT : JSON_ARRAY))
                {
                    JSON_ERROR(it, "Mismatch closing brace/bracket");
                }

                // skip close character
                ++it;

                // set top
                top = top->parent;
            }
            break;

        case ':':
            if (!top || top->type != JSON_OBJECT || !name)
            {
                JSON_ERROR(it, "Unexpected character");
            }
            ++it;
            break;

        case ',':
            CHECK_TOP();
            ++it;
            break;

        case '"':
            {
                CHECK_TOP();

                // skip '"' character
                ++it;

                char *first = it;
                char *last = it;
                while (*it)
                {
                    if ((unsigned char)*it < '\x20')
                    {
                        JSON_ERROR(first, "Control characters not allowed in strings");
                    }
                    else if (*it == '\\')
                    {
                        switch (it[1])
                        {
                        case '"':
                            *last = '"';
                            break;
                        case '\\':
                            *last = '\\';
                            break;
                        case '/':
                            *last = '/';
                            break;
                        case 'b':
                            *last = '\b';
                            break;
                        case 'f':
                            *last = '\f';
                            break;
                        case 'n':
                            *last = '\n';
                            ++escaped_newlines;
                            break;
                        case 'r':
                            *last = '\r';
                            break;
                        case 't':
                            *last = '\t';
                            break;
                        case 'u':
                            {
                                unsigned int codepoint;
                                if (hatoui(it + 2, it + 6, &codepoint) != it + 6)
                                {
                                    JSON_ERROR(it, "Bad unicode codepoint");
                                }

                                if (codepoint <= 0x7F)
                                {
                                    *last = (char)codepoint;
                                }
                                else if (codepoint <= 0x7FF)
                                {
                                    *last++ = (char)(0xC0 | (codepoint >> 6));
                                    *last = (char)(0x80 | (codepoint & 0x3F));
                                }
                                else if (codepoint <= 0xFFFF)
                                {
                                    *last++ = (char)(0xE0 | (codepoint >> 12));
                                    *last++ = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                                    *last = (char)(0x80 | (codepoint & 0x3F));
                                }
                            }
                            it += 4;
                            break;
                        default:
                            JSON_ERROR(first, "Unrecognized escape sequence");
                        }

                        ++last;
                        it += 2;
                    }
                    else if (*it == '"')
                    {
                        *last = 0;
                        ++it;
                        break;
                    }
                    else
                    {
                        *last++ = *it++;
                    }
                }

                if (!name && top->type == JSON_OBJECT)
                {
                    // field name in object
                    name = first;
                }
                else
                {
                    // new string value
                    json_value *object = json_alloc(allocator);

                    object->name = name;
                    name = 0;

                    object->type = JSON_STRING;
                    object->string_value = first;

                    json_append(top, object);
                }
            }
            break;

        case 'n':
        case 't':
        case 'f':
            {
                CHECK_TOP();

                // new null/bool value
                json_value *object = json_alloc(allocator);

                if (top->type == JSON_OBJECT && !name)
                {
                    JSON_ERROR(it, "Missing name");
                }

                object->name = name;
                name = 0;

                // null
                if (it[0] == 'n' && it[1] == 'u' && it[2] == 'l' && it[3] == 'l')
                {
                    object->type = JSON_NULL;
                    it += 4;
                }
                // true
                else if (it[0] == 't' && it[1] == 'r' && it[2] == 'u' && it[3] == 'e')
                {
                    object->type = JSON_BOOL;
                    object->int_value = 1;
                    it += 4;
                }
                // false
                else if (it[0] == 'f' && it[1] == 'a' && it[2] == 'l' && it[3] == 's' && it[4] == 'e')
                {
                    object->type = JSON_BOOL;
                    object->int_value = 0;
                    it += 5;
                }
                else
                {
                    JSON_ERROR(it, "Unknown identifier");
                }

                json_append(top, object);
            }
            break;

        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            {
                CHECK_TOP();

                // new number value
                json_value *object = json_alloc(allocator);

                if (top->type == JSON_OBJECT && !name)
                {
                    JSON_ERROR(it, "Missing name");
                }

                object->name = name;
                name = 0;

                object->type = JSON_INT;

                char *first = it;
                while (*it != '\x20' && *it != '\x9' && *it != '\xD' && *it != '\xA' && *it != ',' && *it != ']' && *it != '}')
                {
                    if (*it == '.' || *it == 'e' || *it == 'E')
                    {
                        object->type = JSON_FLOAT;
                    }
                    ++it;
                }

                if (object->type == JSON_INT && atoi(first, it, &object->int_value) != it)
                {
                    JSON_ERROR(first, "Bad integer number");
                }

                if (object->type == JSON_FLOAT && atof(first, it, &object->float_value) != it)
                {
                    JSON_ERROR(first, "Bad float number");
                }

                json_append(top, object);
            }
            break;

        default:
            JSON_ERROR(it, "Unexpected character");
        }

        // skip white space
        while (*it == '\x20' || *it == '\x9' || *it == '\xD' || *it == '\xA')
        {
            ++it;
        }
    }

    if (top)
    {
        JSON_ERROR(it, "Not all objects/arrays have been properly closed");
    }

    if (!root)
    {
        JSON_ERROR(it, "empty string");
    }

    return root;
}

// ===== public interface ======

struct JsonParserImpl
{
    block_allocator alloc;

    json_value *root;

    const char *error_pos;
    const char *error_desc;
    int error_line;

    JsonParserImpl() : alloc(4096) { }
};

JsonParser::JsonParser()
    : m_impl(new JsonParserImpl())
{
}

JsonParser::~JsonParser()
{
    delete m_impl;
}

bool JsonParser::Parse(char *str)
{
    m_impl->alloc.reset();
    m_impl->root = json_parse(str, &m_impl->error_pos, &m_impl->error_desc, &m_impl->error_line, &m_impl->alloc);
    return m_impl->root != 0;
}

const char *JsonParser::ErrorPos() const
{
    return m_impl->error_pos;
}

const char *JsonParser::ErrorDesc() const
{
    return m_impl->error_desc;
}

int JsonParser::ErrorLine() const
{
    return m_impl->error_line;
}

json_value *JsonParser::Root()
{
    return m_impl->root;
}
