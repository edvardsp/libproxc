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

#include <proxc/config.hpp>

#include <proxc/process.hpp>
#include <proxc/runtime/scheduler.hpp>

PROXC_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
// Process implementation
////////////////////////////////////////////////////////////////////////////////

Process::~Process() noexcept
{
    ctx_.reset();
}

Process::Process( Process && other ) noexcept
    : ctx_{}
{
    ctx_.swap( other.ctx_ );
}

Process & Process::operator = ( Process && other ) noexcept
{
    if ( this != & other ) {
        ctx_.swap( other.ctx_ );
    }
    return *this;
}

void Process::swap( Process & other ) noexcept
{
    ctx_.swap( other.ctx_ );
}

auto Process::get_id() const noexcept
    -> Id
{
    return ctx_->get_id();
}

void Process::launch() noexcept
{
    runtime::Scheduler::self()->commit( ctx_.get() );
}

void Process::join()
{
    runtime::Scheduler::self()->join( ctx_.get() );
    ctx_.reset();
}

void Process::detach()
{
    ctx_.reset();
}

PROXC_NAMESPACE_END

