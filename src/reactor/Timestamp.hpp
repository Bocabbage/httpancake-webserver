#pragma once

#include <string>
#include <stdint.h>

class Timestamp
{
public:
    Timestamp();
    Timestamp(const Timestamp&) = default;

    explicit Timestamp(int64_t us_since_epoch);
    void swap(Timestamp &t)
    { std::swap(us_since_epoch_, t.us_since_epoch_); }

    std::string to_string() const;
    std::string to_formatted_str() const;

    static Timestamp now();
    static Timestamp invalid();

    bool valid() const { return us_since_epoch_ > 0; }

    int64_t us_since_epoch() { return us_since_epoch_; }

    static const int us_per_s = 1000 * 1000; // 

private:
    int64_t us_since_epoch_;
    // epoch means 'Unix Epoch':
    // The accumulate time from UTC1970.1.1:00:00:00
};


/* Compare Operator */
inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.us_since_epoch() < rhs.us_since_epoch();
}

inline bool operator>(Timestamp lhs, Timestamp rhs)
{
    return lhs.us_since_epoch() > rhs.us_since_epoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.us_since_epoch() == rhs.us_since_epoch();
}

inline Timestamp add_time(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::us_per_s);
    return Timestamp(timestamp.us_since_epoch() + delta);
}

inline double time_diff(Timestamp high, Timestamp low)
{
    int64_t diff = high.us_since_epoch() - low.us_since_epoch();
    return static_cast<double>(diff) / Timestamp::us_per_s;
}

