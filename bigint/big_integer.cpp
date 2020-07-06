#include "big_integer.h"

#include <string>
#include <stdexcept>
#include <iostream>
#include <cstddef>
#include <iosfwd>
#include <sstream>
#include <algorithm>
#include <tuple>
#include <vector>
#include <limits>
#include <utility>

big_integer::big_integer()
    : values(1, 0) {}

big_integer::big_integer(big_integer const& other)
    : values(other.values) {}

big_integer::big_integer(int a)
    : values((std::numeric_limits<int>::digits + INT_T_BITS) / INT_T_BITS, 0) {

    for (size_t i = 0; i < size(); i++) {
        values[i] = static_cast<int_t>(a);

        if (std::numeric_limits<int>::digits + 1 > INT_T_BITS) {
            a >>= INT_T_BITS;
        } else {
            a = 0;
        }
    }

    if ((std::numeric_limits<int>::digits + 1) % INT_T_BITS != 0) {
        values.back() |= INT_T_MAX << ((std::numeric_limits<int>::digits + 1) % INT_T_BITS);
    }
}

big_integer::big_integer(int_t a)
    : values(1, static_cast<int_t>(a)) {
    push_zero();
}

big_integer::big_integer(std::string const& str)
    : big_integer() {
    if (str.empty()) {
        throw std::runtime_error("Empty string argument for big_integer(string)");
    }

    for (size_t i = str[0] == '-' ? 1 : 0; i < str.size(); i++) {
        if (str[i] < '0' || '9' < str[i]) {
            std::string msg = "Invalid character for string integer: ";
            msg.push_back(str[i]);
            throw std::runtime_error(msg);
        }
        *this *= big_integer(int_t(10));
        *this += str[i] - '0';
    }

    if (str.data()[0] == '-') {
        negate();
    }
}

big_integer& big_integer::operator=(big_integer const& other)  {
    values = other.values;
    return *this;
}

big_integer& big_integer::sum_with(big_integer const& rhs, size_t my_offset, int_t carry) {
    int_t rest = get_rest();
    values.reserve(rhs.size() + my_offset + 1);
    while (size() < rhs.size() + my_offset) {
        values.push_back(rest);
    }

    for (size_t i = my_offset; i < size(); i++) {
        values[i] += carry;
        carry = values[i] < carry;

        int_t value = rhs.get(i - my_offset);
        values[i] += value;
        carry = carry || values[i] < value;
    }

    values.push_back(carry + rest + rhs.get_rest());
    shrink_to_fit();
    return *this;
}

big_integer& big_integer::sum_with(big_integer const& rhs, int_t carry) {
    return sum_with(rhs, 0, carry);
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    return sum_with(rhs, 0);
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    return sum_with(~rhs, 1);
}

big_integer& big_integer::diff_with(big_integer const& rhs, size_t my_offset) {
    return sum_with(~rhs, my_offset, 1);
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    const big_integer rhs_copy = rhs.is_negative() ? -rhs : rhs;
    const big_integer copy = is_negative() ? -*this : *this;
    bool was_neg = (is_negative() + rhs.is_negative()) == 1;

    values.assign(size() + rhs.size() + 1, 0);

    for (size_t i = 0; i < copy.size(); i++) {
        int_t carry = 0;
        for (size_t j = i; j - i < rhs_copy.size() || (carry && j < size()); j++) {
            double_int_t res = static_cast<double_int_t>(carry)
                             + static_cast<double_int_t>(values[j]);
            if (j - i < rhs_copy.size()) {
                res += static_cast<double_int_t>(copy.values[i])
                     * static_cast<double_int_t>(rhs_copy.get(j - i));
            }
            values[j] = static_cast<int_t>(res);
            carry = static_cast<int_t>(res >> INT_T_BITS);
        }
    }

    shrink_to_fit();
    return was_neg ? this->negate() : *this;
}

// *this >= 0, rhs >= 0
std::tuple<big_integer, big_integer::int_t> big_integer::divide(int_t rhs) {
    if (rhs == 0) {
        throw std::runtime_error("Division by zero");
    }

    double_int_t carry = 0;
    big_integer res = 0;
    res.values.assign(size(), 0);

    for (size_t i = size(); i > 0; i--) {
        carry = (carry << INT_T_BITS) + values[i - 1];
        res.values[i - 1] = static_cast<int_t>(carry / rhs);
        carry = carry % rhs;
    }
    res.shrink_to_fit();
    return {res, carry};
}

// r and d have 0 on back() ==> size >= 2, d > 0
big_integer::int_t trial(big_integer const& r, big_integer const& d, size_t k) {
    using qi = big_integer::quad_int_t;
    int BITS = big_integer::INT_T_BITS;

    qi r3 = (static_cast<qi>(r.get(k    )) << (BITS << 1))
          | (static_cast<qi>(r.get(k - 1)) <<  BITS      )
          |  static_cast<qi>(r.get(k - 2));

    qi d2 = (static_cast<qi>(d.values[d.size() - 2]) << BITS)
           | static_cast<qi>(d.values[d.size() - 3]);
    return static_cast<big_integer::int_t>(std::min(r3 / d2, static_cast<qi>(big_integer::INT_T_MAX)));
}

// this >= rhs > 0, this and rhs have 0 on back() ==> size >= 2
std::tuple<big_integer, big_integer> big_integer::long_divide(big_integer const& rhs) {
    int_t f = static_cast<int_t>(
                    (static_cast<double_int_t>(1) << INT_T_BITS)
                  / (static_cast<double_int_t>(rhs.values[rhs.size() - 2]) + 1)
              );
    big_integer r = (*this * f).push_zero();
    big_integer d = (  rhs * f).push_zero();
    big_integer q = 0;
    q.values.assign(size() - rhs.size() + 1, 0);

    for (size_t k = size() - rhs.size() + 1; k > 0; k--) {
        int_t qt = trial(r, d, k + rhs.size() - 2);
        big_integer dq = d * qt;

        if (dq.compare_to(r, k - 1) == 1) {
            dq = d * --qt;
        }
        q.values[k - 1] = qt;

        r.diff_with(dq, k - 1);
        r.push_zero();
    }

    std::tie(r, std::ignore) = r.divide(f);
    return {q, r};
}

// this and rhs have 0 on back()
std::tuple<big_integer, big_integer> big_integer::divide_positive(big_integer const& rhs) {
    if (rhs.size() <= 2) {
        big_integer q;
        int_t r;
        std::tie(q, r) = divide(rhs.values[0]);
        return {q, r};
    }

    if (*this < rhs) {
        return {0, *this};
    }
    return long_divide(rhs);
}

// if unsigned bigint has bit 1 on last position ==> push 0 to make signed bigint == usigned
big_integer& big_integer::push_zero() {
    shrink_to_fit();
    if (values.back() != 0) {
        values.push_back(0);
    }
    return *this;
}

std::tuple<big_integer, big_integer> big_integer::divide(big_integer rhs)  {
    // Division by zero is checked in divide(int_t)
    bool neg = is_negative();
    if (neg) {
        negate();
    }
    push_zero();

    rhs.shrink_to_fit();
    big_integer q, r;
    std::tie(q, r) = divide_positive(rhs.is_negative() ? (-rhs).push_zero() : rhs.push_zero());

    // from unsigned big_int to signed
    q.push_zero();
    r.push_zero();
    q.shrink_to_fit();
    r.shrink_to_fit();


    if (neg) {
        negate();

        if (rhs.is_negative()) {
            return {std::move(q), -std::move(r)};
        } else {
            return {-std::move(q), -std::move(r)};
        }
    } else {
        if (rhs.is_negative()) {
            return {-std::move(q), std::move(r)};
        } else {
            return {std::move(q), std::move(r)};
        }
    }
}

big_integer& big_integer::operator/=(big_integer const& rhs) {
    big_integer res;
    std::tie(res, std::ignore) = divide(rhs);
    swap(res);
    return *this;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    big_integer res;
    std::tie(std::ignore, res) = divide(rhs);
    swap(res);
    return *this;
}

big_integer& big_integer::bit_operation(big_integer const& rhs, int_t f(int_t, int_t)) {
    values.reserve(rhs.size());
    while (size() < rhs.size()) {
        values.push_back(get_rest());
    }

    for (size_t i = 0; i < size(); i++) {
        values[i] = (*f)(values[i], rhs.get(i));
    }
    shrink_to_fit();
    return *this;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    return bit_operation(rhs, [](big_integer::int_t a, big_integer::int_t b) { return a & b; });
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    return bit_operation(rhs, [](big_integer::int_t a, big_integer::int_t b) { return a | b; });
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    return bit_operation(rhs, [](big_integer::int_t a, big_integer::int_t b) { return a ^ b; });
}

big_integer& big_integer::operator<<=(int rhs_int) {
    size_t rhs = static_cast<size_t>(rhs_int);
    size_t blocks = rhs / INT_T_BITS;
    size_t in_block = rhs % INT_T_BITS;
    size_t out_block = INT_T_BITS - in_block;

    values.reserve(size() + blocks + 1);
    for (size_t i = 0; i <= blocks; i++) {
        values.push_back(get_rest());
    }

    for (size_t i = size(), j = size() - blocks; j > 0; i--, j--) {
        values[i - 1] = values[j - 1];
        if (i < size()) {
            values[i] |= out_block == INT_T_BITS ? 0 : values[i - 1] >> out_block;
        }
        values[i - 1] <<= in_block;
        if (i != j) {
            values[j - 1] = 0;
        }
    }

    shrink_to_fit();
    return *this;
}

big_integer& big_integer::operator>>=(int rhs_int) {
    size_t rhs = static_cast<size_t>(rhs_int);
    size_t blocks = rhs / INT_T_BITS;
    size_t in_block = rhs % INT_T_BITS;
    size_t out_block = (INT_T_BITS - in_block) % INT_T_BITS;
    int_t rest = get_rest();

    for (size_t i = 0, j = blocks; i < size(); i++, j++) {
        if (i > 0) {
            values[i - 1] |= (get(j) & ((static_cast<int_t>(1) << in_block) - 1)) << out_block;
        }
        values[i] = get(j) >> in_block;
    }
    values.back() |= (rest & ((static_cast<int_t>(1) << in_block) - 1)) << out_block;

    shrink_to_fit();
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    return big_integer(*this).negate();
}

big_integer big_integer::operator~() const {
    return big_integer(*this).negate_bits();
}

big_integer& big_integer::operator++() {
    return *this += 1;
}

big_integer big_integer::operator++(int) {
    big_integer result(*this);
    *this += 1;
    return result;
}

big_integer& big_integer::operator--() {
    return *this -= 1;
}

big_integer big_integer::operator--(int) {
    big_integer result(*this);
    *this -= 1;
    return result;
}

bool operator==(big_integer const& a, big_integer const& b) {
    return a.compare_to(b) == 0;
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return a.compare_to(b) != 0;
}

bool operator<(big_integer const& a, big_integer const& b) {
    return a.compare_to(b) < 0;
}

bool operator>(big_integer const& a, big_integer const& b) {
    return a.compare_to(b) > 0;
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return a.compare_to(b) <= 0;
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return a.compare_to(b) >= 0;
}

std::string to_string(big_integer const& a) {
    std::stringstream ss;
    ss << a;
    return ss.str();
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
    return a >>= b;
}

std::ostream& operator<<(std::ostream& s, big_integer a) {
    if (a == 0) {
        return s << '0';
    }

    if (a.is_negative()) {
        s << '-';
        a.negate();
    }

    std::string ans;
    while (a > 0) {
        big_integer q;
        big_integer::int_t r;
        std::tie(q, r) = a.divide(10);
        a = q;
        a.shrink_to_fit();
        ans.push_back('0' + static_cast<char>(r));
    }

    std::reverse(ans.begin(), ans.end());
    for (auto e : ans) {
        s << e;
    }

    return s;
}

big_integer::int_t big_integer::get(size_t i) const {
    return size() > i ? values[i] : (is_negative() ? INT_T_MAX : 0);
}

size_t big_integer::size() const {
    return values.size();
}

int compare_ints(big_integer::int_t a, big_integer::int_t b) {
    return a < b ? -1 : (a > b ? 1 : 0);
}

int big_integer::compare_to(big_integer const& rhs, size_t offset) const {
    if (is_negative() + rhs.is_negative() == 1) {
        return is_negative() ? -1 : 1;
    }

    for (size_t i = std::max(size(), rhs.size() < offset ? 0 : rhs.size() - offset); i > 0; i--) {
        int ans = compare_ints(get(i - 1), rhs.get(i - 1 + offset));
        if (ans) {
            return ans;
        }
    }

    return 0;
}

int big_integer::compare_to(big_integer const& rhs) const {
    return compare_to(rhs, 0);
}

big_integer& big_integer::negate_bits() {
    for (size_t i = 0; i < size(); i++) {
        values[i] = ~values[i];
    }
    return *this;
}

big_integer& big_integer::negate() {
    return ++negate_bits();
}

bool big_integer::is_negative() const {
    return values.back() >> (INT_T_BITS - 1);
}

void big_integer::shrink_to_fit() {
    int_t rest = get_rest();
    while (size() > 1 && values.back() == rest && is_negative() == (values[size() - 2] >> (INT_T_BITS - 1))) {
        values.pop_back();
    }
}

void big_integer::swap(big_integer &other) {
    std::swap(values, other.values);
}

big_integer::int_t big_integer::get_rest() const {
    return is_negative() ? INT_T_MAX : 0;
}
