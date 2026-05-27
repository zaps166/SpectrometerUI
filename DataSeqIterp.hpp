// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "Headers.hpp"

template<ranges::input_range R>
class DataSeqIterp
{
public:
    DataSeqIterp(R &range);

    double get(double wl);

    void reset();

private:
#if 0 // Unavailable in libc++
    const ranges::const_iterator_t<R> m_begin;
    const ranges::const_sentinel_t<R> m_end;
    ranges::const_iterator_t<R> m_it;
#else
    const decltype(ranges::cbegin(declval<R &>())) m_begin;
    const decltype(ranges::cend(declval<R &>())) m_end;
    decltype(ranges::cbegin(declval<R &>())) m_it;
#endif
};

/* Implementation */

template<ranges::input_range R>
DataSeqIterp<R>::DataSeqIterp(R &range)
    : m_begin(ranges::begin(range))
    , m_end(ranges::end(range))
{
    reset();
}

template<ranges::input_range R>
double DataSeqIterp<R>::get(double wl)
{
    double ret = 0.0;
    const auto it = ranges::find_if(m_it, m_end, [wl](auto &&entry) {
        return std::get<0>(entry) >= wl;
    });
    if (it != m_end)
    {
        auto &&[wlIt, valIt] = *it;
        if (it != m_begin)
        {
            auto &&[wlPrevIt, valPrevIt] = *ranges::prev(it);
            const double t = (wl - wlIt) / (wlPrevIt - wlIt);
            ret = valIt + t * (valPrevIt - valIt);
            m_it = it;
        }
        else if (wlIt == wl)
        {
            ret = valIt;
            m_it = it;
        }
    }
    return ret;
}

template<ranges::input_range R>
void DataSeqIterp<R>::reset()
{
    m_it = m_begin;
}
