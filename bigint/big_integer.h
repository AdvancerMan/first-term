#pragma once

#include <cstddef>
#include <iosfwd>
#include <stdint.h>
#include <utility>
#include <vector>

struct big_integer {
    using int_t = uint32_t;
    using double_int_t = uint64_t;
    using quad_int_t = __uint128_t;
    static const int INT_T_BITS = 32;
    static const int_t INT_T_MAX = static_cast<int_t>(-1);
    using storage_t = std::vector<int_t>;

    big_integer();
    big_integer(big_integer const& other);
    big_integer(int a);
    explicit big_integer(std::string const& str);
    ~big_integer() = default;

    big_integer& operator=(big_integer const& other);

    big_integer& operator+=(big_integer const& rhs);
    big_integer& operator-=(big_integer const& rhs);
    big_integer& operator*=(big_integer const& rhs);
    big_integer& operator/=(big_integer const& rhs);
    big_integer& operator%=(big_integer const& rhs);

    big_integer& operator&=(big_integer const& rhs);
    big_integer& operator|=(big_integer const& rhs);
    big_integer& operator^=(big_integer const& rhs);

    big_integer& operator<<=(int rhs);
    big_integer& operator>>=(int rhs);

    big_integer operator+() const;
    big_integer operator-() const;
    big_integer operator~() const;

    big_integer& operator++();
    big_integer operator++(int);

    big_integer& operator--();
    big_integer operator--(int);

    friend std::ostream& operator<<(std::ostream& s, big_integer a);

    int compare_to(big_integer const&) const;
    big_integer& negate();
    void swap(big_integer&);
    big_integer& negate_bits();
    bool is_negative() const;
    std::tuple<big_integer, big_integer> divide(big_integer);

private:
    big_integer(int_t);
    int_t get(size_t) const;
    int_t get_rest() const;
    size_t size() const;
    big_integer& push_zero();
    big_integer& bit_operation(int_t (int_t, int_t), big_integer const&);
    void shrink_to_fit();
    std::tuple<big_integer, int_t> divide(int_t rhs);
    std::tuple<big_integer, big_integer> long_divide(big_integer const& rhs);
    std::tuple<big_integer, big_integer> divide_positive(big_integer const&);
    friend int_t trial(big_integer const&, big_integer const&, size_t);

    storage_t values;
};

bool operator==(big_integer const& a, big_integer const& b);
bool operator!=(big_integer const& a, big_integer const& b);
bool operator<(big_integer const& a, big_integer const& b);
bool operator>(big_integer const& a, big_integer const& b);
bool operator<=(big_integer const& a, big_integer const& b);
bool operator>=(big_integer const& a, big_integer const& b);

std::string to_string(big_integer const& a);

big_integer operator+(big_integer a, big_integer const& b);
big_integer operator-(big_integer a, big_integer const& b);
big_integer operator*(big_integer a, big_integer const& b);
big_integer operator/(big_integer a, big_integer const& b);
big_integer operator%(big_integer a, big_integer const& b);

big_integer operator&(big_integer a, big_integer const& b);
big_integer operator|(big_integer a, big_integer const& b);
big_integer operator^(big_integer a, big_integer const& b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);
