/*
 * MIT License
 *
 * Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <exception>
#include <mutex>
#include <string>
#include <sstream>
#include <iostream>

// Thread-safe cout
struct SafeCout
{
    template<typename... Args>
    static void print(Args... args)
    {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        print_aux(args...);
        std::cout << std::endl;
        std::cout.flush();
    }

private:
    template<typename Arg>
    static void print_aux(Arg arg)
    {
        std::cout << arg;
    }

    template<typename Arg, typename... Args>
    static void print_aux(Arg arg, Args... args)
    {
        std::cout << arg;
        print_aux(args...);
    }
};



// Exception type for assertion failures
class AssertionFailureException : public std::exception
{
private:
    const char* m_expression;
    const char* m_file;
    int m_line;
    std::string m_message;
    std::string m_report;

public:
    // Helper class for formatting assertion message
    class StreamFormatter
    {
    private:
        std::ostringstream stream;

    public:
        operator std::string() const
        {
            return stream.str();
        }

        template<typename T>
        StreamFormatter& operator << (const T& value)
        {
            stream << value;
            return *this;
        }
    };

    // Log error before throwing
    void LogError()
    {
#ifdef THROWASSERT_LOGGER
        THROWASSERT_LOGGER(m_report);
#else
        std::cerr << m_report << std::endl;
#endif
    }

    // Construct an assertion failure exception
    AssertionFailureException(const char* expression, const char* file, int line, const std::string& message)
        : m_expression(expression)
        , m_file(file)
        , m_line(line)
        , m_message(message)
    {
        std::ostringstream outputStream;

        if (!m_message.empty())
        {
            outputStream << m_message << ": ";
        }

        std::string expressionString = m_expression;

        if (expressionString == "false" || expressionString == "0" || expressionString == "FALSE")
        {
            outputStream << "Unreachable code assertion";
        }
        else
        {
            outputStream << "Assertion '" << m_expression << "'";
        }

        outputStream << " failed in file '" << m_file << "' line " << m_line;
        m_report = outputStream.str();

        LogError();
    }

    // The assertion message
    virtual const char* what() const noexcept
    {
        return m_report.c_str();
    }

    // The expression which was asserted to be true
    const char* Expression() const noexcept
    {
        return m_expression;
    }

    // Source file
    const char* File() const noexcept
    {
        return m_file;
    }

    // Source line
    int Line() const noexcept
    {
        return m_line;
    }

    // Description of failure
    const char* Message() const noexcept
    {
        return m_message.c_str();
    }
};

#define _stringify(expr) #expr
// Assert that EXPRESSION evaluates to true, otherwise raise AssertionFailureException with associated MESSAGE (which may use C++ stream-style message formatting)
#define throw_assert(EXPRESSION, MESSAGE) do { \
    if ( ! (EXPRESSION) ) { \
        throw AssertionFailureException( \
            #EXPRESSION, __FILE__, __LINE__, \
            ( AssertionFailureException::StreamFormatter() << MESSAGE ) ); \
    } \
} while (false)

#define _throw_assert_logic(LEFT, RIGHT, LEFT_EXPR, RIGHT_EXPR, OP, MESSAGE) do { \
    auto left = (LEFT); auto right = (RIGHT); \
    if ( ! (left OP right) ) { \
        throw AssertionFailureException( \
            LEFT_EXPR " " #OP " " RIGHT_EXPR, __FILE__, __LINE__, \
            ( AssertionFailureException::StreamFormatter() << MESSAGE \
                << " | Left: " << left << ", Right: " << right << " | " ) ); \
    } \
} while (false)

#define throw_assert_equ(LEFT, RIGHT, MESSAGE) _throw_assert_logic(LEFT, RIGHT, _stringify(LEFT), _stringify(RIGHT), ==, MESSAGE)
#define throw_assert_neq(LEFT, RIGHT, MESSAGE) _throw_assert_logic(LEFT, RIGHT, _stringify(LEFT), _stringify(RIGHT), !=, MESSAGE)
#define throw_assert_lss(LEFT, RIGHT, MESSAGE) _throw_assert_logic(LEFT, RIGHT, _stringify(LEFT), _stringify(RIGHT),  <, MESSAGE)
#define throw_assert_leq(LEFT, RIGHT, MESSAGE) _throw_assert_logic(LEFT, RIGHT, _stringify(LEFT), _stringify(RIGHT), <=, MESSAGE)
#define throw_assert_gtr(LEFT, RIGHT, MESSAGE) _throw_assert_logic(LEFT, RIGHT, _stringify(LEFT), _stringify(RIGHT),  >, MESSAGE)
#define throw_assert_geq(LEFT, RIGHT, MESSAGE) _throw_assert_logic(LEFT, RIGHT, _stringify(LEFT), _stringify(RIGHT), >=, MESSAGE)

