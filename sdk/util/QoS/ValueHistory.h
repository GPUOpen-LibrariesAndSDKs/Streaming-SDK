#pragma once

#include <list>

namespace ssdk::util
{
    template <typename T, size_t Size>
    class ValueHistory
    {
    public:
        ValueHistory() : m_HistorySize(Size) {}

        void AddValue(T val, amf_pts time)
        {
            if (time > m_LastUpdateTime)
            {
                T accum = 0;
                m_History.push_back(val);

                size_t len = m_History.size();
                for (T cur : m_History)
                {
                    accum += cur;
                }
                T newFloatingAvg = accum / len;
                m_Trend = newFloatingAvg - m_FloatingAvg;
                m_FloatingAvg = newFloatingAvg;
                if (len > m_HistorySize && len > 1)
                {
                    m_History.pop_front();
                }

                m_LastUpdateTime = time;
            }
        }

        inline T GetAverage() const noexcept { return m_FloatingAvg; }

        inline T GetTrend() const noexcept { return m_Trend; }

        inline amf_pts GetLastUpdateTime() const noexcept { return m_LastUpdateTime; }

        void Clear() noexcept { m_History.clear(); }

        bool IsHistoryFull() const noexcept { return m_History.size() == m_HistorySize; }

        bool IsEmpty() const noexcept{ return m_History.size() == 0; }

    private:
        typedef std::list<T>    History;
        History     m_History;
        size_t      m_HistorySize;
        T           m_FloatingAvg = 0;
        T           m_Trend = 0;
        amf_pts     m_LastUpdateTime = 0;
    };

    template <typename T>
    class ValueAverage {
        T m_RunningSum = T{};
        size_t m_Count = 0;

    public:
        void Add(T value)
        {
            m_RunningSum += value;
            m_Count++;
        }

        void Clear()
        {
            m_RunningSum = T{};
            m_Count = 0;
        }

        float GetAverageAndClear()
        {
            if (m_Count == 0)
            {
                return 0.0;
            }

            float avg = static_cast<float>(m_RunningSum) / m_Count;
            Clear();
            return avg;
        }

        float GetCurrentAverage() const
        {
            return m_Count > 0 ? static_cast<float>(m_RunningSum) / m_Count : 0.0;
        }
    };
}
