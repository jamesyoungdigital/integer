/*
integer
An Arbitrary Precision Integer Type
by Jason Lee @ calccrypto at gmail.com

With much help from:
    Auston Sterling - Initial debugging and coding help and FFT multiplication
    Corbin @ Code Review (StackExchange) - wrote a sizeable chunk of code and suggestions
    Keith Nicholas  @ Code Review (StackExchange)
    ROBOKITTY @ Code Review (StackExchange)
    Winston Ewert @ Code Review (StackExchange) - suggested many improvements

This is an implementation of an arbitrary precision integer
container. The actual limit of how large the integers can
be is std::deque <Z>().max_size() * sizeof(Z) * 8 bits, which
should be enormous. Most of the basic operators are implemented,
although their outputs might not necessarily be the same output
as a standard version of that operator. Anything involving
pointers and addresses should be taken care of by C++.

Data is stored in big-endian, so _value[0] is the most
significant digit, and _value[_value.size() - 1] is the
least significant DIGIT.

Negative values are stored as their positive _value,
with a bool that says the _value is negative.

NOTE: Build with the newest compiler. Some functions are only
      supported in the latest versions of C++ compilers and
      standards.

NOTE: Base256 strings are assumed to be positive when read into
      integer. Use operator-() to negate the value.

NOTE: Multiple algorithms for subtraction, multiplication, and
      division have been implemented and commented out. They
      should all work, but are there for educational purposes.
      If one is faster than another on different systems, by
      all means change which algorithm is used. Just make sure
      all related functions are changed as well.

NOTE: All algorithms operate on positive values only. The
      operators deal with the signs.

NOTE: Changing the internal representation to a std::string
      makes integer run slower than using a std::deque <uint8_t>
*/

#include <cmath> //For fft sin, cos, M_PI, and floor
#include <cstdint>
#include <deque>
#include <list>
#include <iostream>
#include <stdexcept>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265359
#endif

#ifndef __INTEGER__
#define __INTEGER__

class integer{
    public:
        typedef uint8_t                           DIGIT;            // Should be unsigned integer type. Change bitsize to reduce wasted bits/increase speed
        typedef uint64_t                          DOUBLE_DIGIT;     // sizeof(DOUBLE_DIGIT) must be larger than sizeof(DIGIT)

        typedef std::deque <DIGIT>                REP;              // internal representation of values
        typedef REP::size_type                    REP_SIZE_T;       // size type of internal representation

    private:
        static constexpr DIGIT NEG1 = -1;                           // value with all bits ON - will only work for unsigned integer types, where -1 == 111...1
        static constexpr DIGIT OCTETS = sizeof(DIGIT);              // number of octets per DIGIT
        static constexpr DIGIT BITS = OCTETS << 3;                  // number of bits per DIGIT; hardcode this if DIGIT is not standard int type
        static constexpr DIGIT HIGH_BIT = 1 << (BITS - 1);          // highest bit of DIGIT (uint8_t -> 128)

        // Member variables
        bool _sign;                                                 // false = positive, true = negative
        REP _value;                                                 // holds the actual _value in REP

        template <typename Z> void setFromZ(Z val, const uint16_t & bits){
            _value.clear();
            _sign = false;

            if (val < 0){
                _sign = true;
                val = -val;
            }

            while (val){
                _value.push_front(val & NEG1);
                val >>= BITS;
            }

            trim();
        }

        void trim();                                                // remove 0 digits from top of deque to save memory

    public:
        // Constructors
        integer();
        integer(const REP & rhs, const bool & sign = false);
        integer(const integer & rhs);

        // Constructors for Numerical Input
        integer(const bool & b);
        integer(const char & c);
        integer(const uint8_t & val);
        integer(const uint16_t & val);
        integer(const uint32_t & val);
        integer(const uint64_t & val);
        integer(const int8_t & val);
        integer(const int16_t & val);
        integer(const int32_t & val);
        integer(const int64_t & val);

        // Special Constructor for Strings
        integer(const std::string & val, const uint16_t & base);

        // Use this to construct integers with other types that have pointers/iterators to their beginning and end
        //  - all inputs are treated as positive values
        //  - each element in the container is treated as one digit in the original format of the value
        //    so values lareger than 255 might construct bad values
        //
        //      Written by Corbin
        //      Modified by me
        //
        template <typename Iterator> integer(Iterator start, const Iterator& end, const uint16_t & base) : integer() {
            bool s = false;
            if (start == end){
                return;
            }

            if ((2 <= base) && (base <= 10)){
                while (start != end){
                    if (!std::isdigit(*start)){
                        throw std::runtime_error("Error: Non-digit found");
                    }
                    *this += (*this * base) + (*start - '0');
                    ++start;
                }
            }
            else if (base == 16){
                while (start != end){
                    *this <<= 4;
                    if (std::isxdigit(*start) || std::isalpha(*start)){
                        if (std::isdigit(*start))
                            *this += *start - '0';                  //0-9
                        else if (std::islower(*start))
                            *this += *start - 'a';                  //a-f
                        else if (std::isupper(*start))
                            *this += *start - 'A';                  //A-F
                        else
                            throw std::runtime_error("Error: Character not between 'a' and 'f' found");
                    }
                    else{
                        throw std::runtime_error("Error: Non-alphanumeric character found");
                    }
                    ++start;
                }
            }
            else if (base == 256){
                while (start != end){
                    _value.push_back(*start);
                    ++start;
                }
            }
            else{
                throw std::runtime_error("Error: Bad base provided: " + integer(base).str());
            }

            _sign = s;
            trim();
        }

    public:

    public:
        //  RHS input args only

        // Assignment Operator
        const integer & operator=(const integer & val);
        const integer & operator=(const bool & rhs);
        const integer & operator=(const char & rhs);
        const integer & operator=(const uint8_t & val);
        const integer & operator=(const uint16_t & val);
        const integer & operator=(const uint32_t & val);
        const integer & operator=(const uint64_t & val);
        const integer & operator=(const int8_t & val);
        const integer & operator=(const int16_t & val);
        const integer & operator=(const int32_t & val);
        const integer & operator=(const int64_t & val);

        // Typecast Operators
        operator bool() const;
        operator char() const;
        operator uint8_t() const;
        operator uint16_t() const;
        operator uint32_t() const;
        operator uint64_t() const;
        operator int8_t() const;
        operator int16_t() const;
        operator int32_t() const;
        operator int64_t() const;

        // Bitwise Operators
        integer operator&(const integer & rhs) const;
            integer operator&(const bool & rhs) const;
            integer operator&(const char & rhs) const;
            integer operator&(const uint8_t & rhs) const;
            integer operator&(const uint16_t & rhs) const;
            integer operator&(const uint32_t & rhs) const;
            integer operator&(const uint64_t & rhs) const;
            integer operator&(const int8_t & rhs) const;
            integer operator&(const int16_t & rhs) const;
            integer operator&(const int32_t & rhs) const;
            integer operator&(const int64_t & rhs) const;

        integer operator&=(const integer & rhs);
            integer operator&=(const bool & rhs);
            integer operator&=(const char & rhs);
            integer operator&=(const uint8_t & rhs);
            integer operator&=(const uint16_t & rhs);
            integer operator&=(const uint32_t & rhs);
            integer operator&=(const uint64_t & rhs);
            integer operator&=(const int8_t & rhs);
            integer operator&=(const int16_t & rhs);
            integer operator&=(const int32_t & rhs);
            integer operator&=(const int64_t & rhs);

        integer operator|(const integer & rhs) const;
            integer operator|(const bool & rhs) const;
            integer operator|(const char & rhs) const;
            integer operator|(const uint8_t & rhs) const;
            integer operator|(const uint16_t & rhs) const;
            integer operator|(const uint32_t & rhs) const;
            integer operator|(const uint64_t & rhs) const;
            integer operator|(const int8_t & rhs) const;
            integer operator|(const int16_t & rhs) const;
            integer operator|(const int32_t & rhs) const;
            integer operator|(const int64_t & rhs) const;

        integer operator|=(const integer & rhs);
            integer operator|=(const bool & rhs);
            integer operator|=(const char & rhs);
            integer operator|=(const uint8_t & rhs);
            integer operator|=(const uint16_t & rhs);
            integer operator|=(const uint32_t & rhs);
            integer operator|=(const uint64_t & rhs);
            integer operator|=(const int8_t & rhs);
            integer operator|=(const int16_t & rhs);
            integer operator|=(const int32_t & rhs);
            integer operator|=(const int64_t & rhs);

        integer operator^(const integer & rhs) const;
            integer operator^(const bool & rhs) const;
            integer operator^(const char & rhs) const;
            integer operator^(const uint8_t & rhs) const;
            integer operator^(const uint16_t & rhs) const;
            integer operator^(const uint32_t & rhs) const;
            integer operator^(const uint64_t & rhs) const;
            integer operator^(const int8_t & rhs) const;
            integer operator^(const int16_t & rhs) const;
            integer operator^(const int32_t & rhs) const;
            integer operator^(const int64_t & rhs) const;

        integer operator^=(const integer & rhs);
            integer operator^=(const bool & rhs);
            integer operator^=(const char & rhs);
            integer operator^=(const uint8_t & rhs);
            integer operator^=(const uint16_t & rhs);
            integer operator^=(const uint32_t & rhs);
            integer operator^=(const uint64_t & rhs);
            integer operator^=(const int8_t & rhs);
            integer operator^=(const int16_t & rhs);
            integer operator^=(const int32_t & rhs);
            integer operator^=(const int64_t & rhs);

        integer operator~();

        // Bitshift Operators
        // left bitshift. sign is maintained
        integer operator<<(const integer & shift) const;
            integer operator<<(const bool & rhs) const;
            integer operator<<(const char & rhs) const;
            integer operator<<(const uint8_t & rhs) const;
            integer operator<<(const uint16_t & rhs) const;
            integer operator<<(const uint32_t & rhs) const;
            integer operator<<(const uint64_t & rhs) const;
            integer operator<<(const int8_t & rhs) const;
            integer operator<<(const int16_t & rhs) const;
            integer operator<<(const int32_t & rhs) const;
            integer operator<<(const int64_t & rhs) const;

        integer operator<<=(const integer & shift);
            integer operator<<=(const bool & rhs);
            integer operator<<=(const char & rhs);
            integer operator<<=(const uint8_t & rhs);
            integer operator<<=(const uint16_t & rhs);
            integer operator<<=(const uint32_t & rhs);
            integer operator<<=(const uint64_t & rhs);
            integer operator<<=(const int8_t & rhs);
            integer operator<<=(const int16_t & rhs);
            integer operator<<=(const int32_t & rhs);
            integer operator<<=(const int64_t & rhs);

        // right bitshift. sign is maintained
        integer operator>>(const integer & shift) const;
            integer operator>>(const bool & rhs) const;
            integer operator>>(const char & rhs) const;
            integer operator>>(const uint8_t & rhs) const;
            integer operator>>(const uint16_t & rhs) const;
            integer operator>>(const uint32_t & rhs) const;
            integer operator>>(const uint64_t & rhs) const;
            integer operator>>(const int8_t & rhs) const;
            integer operator>>(const int16_t & rhs) const;
            integer operator>>(const int32_t & rhs) const;
            integer operator>>(const int64_t & rhs) const;

        integer operator>>=(const integer & shift);
            integer operator>>=(const bool & rhs);
            integer operator>>=(const char & rhs);
            integer operator>>=(const uint8_t & rhs);
            integer operator>>=(const uint16_t & rhs);
            integer operator>>=(const uint32_t & rhs);
            integer operator>>=(const uint64_t & rhs);
            integer operator>>=(const int8_t & rhs);
            integer operator>>=(const int16_t & rhs);
            integer operator>>=(const int32_t & rhs);
            integer operator>>=(const int64_t & rhs);

        // Logical Operators
        bool operator!();

        // Comparison Operators
        bool operator==(const integer & rhs) const;
            bool operator==(const bool & rhs) const;
            bool operator==(const char & rhs) const;
            bool operator==(const uint8_t & rhs) const;
            bool operator==(const uint16_t & rhs) const;
            bool operator==(const uint32_t & rhs) const;
            bool operator==(const uint64_t & rhs) const;
            bool operator==(const int8_t & rhs) const;
            bool operator==(const int16_t & rhs) const;
            bool operator==(const int32_t & rhs) const;
            bool operator==(const int64_t & rhs) const;

        bool operator!=(const integer & rhs) const;
            bool operator!=(const bool & rhs) const;
            bool operator!=(const char & rhs) const;
            bool operator!=(const uint8_t & rhs) const;
            bool operator!=(const uint16_t & rhs) const;
            bool operator!=(const uint32_t & rhs) const;
            bool operator!=(const uint64_t & rhs) const;
            bool operator!=(const int8_t & rhs) const;
            bool operator!=(const int16_t & rhs) const;
            bool operator!=(const int32_t & rhs) const;
            bool operator!=(const int64_t & rhs) const;

    private:
        // operator> not considering signs
        bool gt(const integer & lhs, const integer & rhs) const;

    public:
        bool operator>(const integer & rhs) const;
            bool operator>(const bool & rhs) const;
            bool operator>(const char & rhs) const;
            bool operator>(const uint8_t & rhs) const;
            bool operator>(const uint16_t & rhs) const;
            bool operator>(const uint32_t & rhs) const;
            bool operator>(const uint64_t & rhs) const;
            bool operator>(const int8_t & rhs) const;
            bool operator>(const int16_t & rhs) const;
            bool operator>(const int32_t & rhs) const;
            bool operator>(const int64_t & rhs) const;

        bool operator>=(const integer & rhs) const;
            bool operator>=(const bool & rhs) const;
            bool operator>=(const char & rhs) const;
            bool operator>=(const uint8_t & rhs) const;
            bool operator>=(const uint16_t & rhs) const;
            bool operator>=(const uint32_t & rhs) const;
            bool operator>=(const uint64_t & rhs) const;
            bool operator>=(const int8_t & rhs) const;
            bool operator>=(const int16_t & rhs) const;
            bool operator>=(const int32_t & rhs) const;
            bool operator>=(const int64_t & rhs) const;

    private:
        // operator< not considering signs
        bool lt(const integer & lhs, const integer & rhs) const;

    public:
        bool operator<(const integer & rhs) const;
            bool operator<(const bool & rhs) const;
            bool operator<(const char & rhs) const;
            bool operator<(const uint8_t & rhs) const;
            bool operator<(const uint16_t & rhs) const;
            bool operator<(const uint32_t & rhs) const;
            bool operator<(const uint64_t & rhs) const;
            bool operator<(const int8_t & rhs) const;
            bool operator<(const int16_t & rhs) const;
            bool operator<(const int32_t & rhs) const;
            bool operator<(const int64_t & rhs) const;

        bool operator<=(const integer & rhs) const;
            bool operator<=(const bool & rhs) const;
            bool operator<=(const char & rhs) const;
            bool operator<=(const uint8_t & rhs) const;
            bool operator<=(const uint16_t & rhs) const;
            bool operator<=(const uint32_t & rhs) const;
            bool operator<=(const uint64_t & rhs) const;
            bool operator<=(const int8_t & rhs) const;
            bool operator<=(const int16_t & rhs) const;
            bool operator<=(const int32_t & rhs) const;
            bool operator<=(const int64_t & rhs) const;

    private:
        // Arithmetic Operators
        integer add(const integer & lhs, const integer & rhs) const;

    public:
        integer operator+(const integer & rhs) const;
            integer operator+(const bool & rhs) const;
            integer operator+(const char & rhs) const;
            integer operator+(const uint8_t & rhs) const;
            integer operator+(const uint16_t & rhs) const;
            integer operator+(const uint32_t & rhs) const;
            integer operator+(const uint64_t & rhs) const;
            integer operator+(const int8_t & rhs) const;
            integer operator+(const int16_t & rhs) const;
            integer operator+(const int32_t & rhs) const;
            integer operator+(const int64_t & rhs) const;

        integer operator+=(const integer & rhs);
            integer operator+=(const bool & rhs);
            integer operator+=(const char & rhs);
            integer operator+=(const uint8_t & rhs);
            integer operator+=(const uint16_t & rhs);
            integer operator+=(const uint32_t & rhs);
            integer operator+=(const uint64_t & rhs);
            integer operator+=(const int8_t & rhs);
            integer operator+=(const int16_t & rhs);
            integer operator+=(const int32_t & rhs);
            integer operator+=(const int64_t & rhs);

    private:
        // Subtraction as done by hand
        // lhs must be larger than rhs
        integer long_sub(const integer & lhs, const integer & rhs) const;

       // Two's Complement Subtraction
       integer two_comp_sub(const integer & lhs, const integer & rhs) const;

        integer sub(const integer & lhs, const integer & rhs) const;

    public:
        integer operator-(const integer & rhs) const;
            integer operator-(const bool & rhs) const;
            integer operator-(const char & rhs) const;
            integer operator-(const uint8_t & rhs) const;
            integer operator-(const uint16_t & rhs) const;
            integer operator-(const uint32_t & rhs) const;
            integer operator-(const uint64_t & rhs) const;
            integer operator-(const int8_t & rhs) const;
            integer operator-(const int16_t & rhs) const;
            integer operator-(const int32_t & rhs) const;
            integer operator-(const int64_t & rhs) const;

        integer operator-=(const integer & rhs);
            integer operator-=(const bool & rhs);
            integer operator-=(const char & rhs);
            integer operator-=(const uint8_t & rhs);
            integer operator-=(const uint16_t & rhs);
            integer operator-=(const uint32_t & rhs);
            integer operator-=(const uint64_t & rhs);
            integer operator-=(const int8_t & rhs);
            integer operator-=(const int16_t & rhs);
            integer operator-=(const int32_t & rhs);
            integer operator-=(const int64_t & rhs);

    private:
        // // Peasant Multiplication
        // integer peasant(const integer & lhs, const integer & rhs) const;

        // // Recurseive Peasant Algorithm
        // integer recursive_peasant(const integer & lhs, const integer & rhs) const;

        // // Recursive Multiplication
        // integer recursive_mult(const integer & lhs, const integer & rhs) const;

        // // Karatsuba Algorithm O(n-log2(3) = n - 1.585)
        // // The Peasant Multiplication function is needed if Karatsuba is used.
        // // Thanks to kjo @ stackoverflow for fixing up my original Karatsuba Algorithm implementation
        // // which I then converted to C++ and made a few changes.
        // // http://stackoverflow.com/questions/7058838/karatsuba-algorithm-too-much-recursion
        // integer karatsuba(const integer & lhs, const integer & rhs, integer bm = 0x1000000U) const;

        // // // Toom-Cook multiplication
        // // // as described at http://en.wikipedia.org/wiki/Toom%E2%80%93Cook_multiplications
        // // // The peasant function is needed if karatsuba is used.
        // // // This implementation is a bit weird. In the pointwise Multiplcation step, using
        // // // operator* and long_mult works, but everything else fails.
        // // // It's also kind of slow.
        // // integer toom_cook_3(integer m, integer n, integer bm = 0x1000000U);

        // // Long multiplication
        // integer long_mult(const integer & lhs, const integer & rhs) const;

        //Private FFT helper function
        int fft(std::deque<double>& data, bool dir = true) const;

        // FFT-based multiplication
        //Based on the convolution theorem which states that the Fourier
        //transform of a convolution is the pointwise product of their
        //Fourier transforms.
        integer fft_mult(const integer& lhs, const integer& rhs) const;

    public:
        integer operator*(const integer & rhs) const;
            integer operator*(const bool & rhs) const;
            integer operator*(const char & rhs) const;
            integer operator*(const uint8_t & rhs) const;
            integer operator*(const uint16_t & rhs) const;
            integer operator*(const uint32_t & rhs) const;
            integer operator*(const uint64_t & rhs) const;
            integer operator*(const int8_t & rhs) const;
            integer operator*(const int16_t & rhs) const;
            integer operator*(const int32_t & rhs) const;
            integer operator*(const int64_t & rhs) const;

        integer operator*=(const integer & rhs);
            integer operator*=(const bool & rhs);
            integer operator*=(const char & rhs);
            integer operator*=(const uint8_t & rhs);
            integer operator*=(const uint16_t & rhs);
            integer operator*=(const uint32_t & rhs);
            integer operator*=(const uint64_t & rhs);
            integer operator*=(const int8_t & rhs);
            integer operator*=(const int16_t & rhs);
            integer operator*=(const int32_t & rhs);
            integer operator*=(const int64_t & rhs);

    private:
        // Naive Division: keep subtracting until lhs == 0
        std::pair <integer, integer> naive_div(const integer & lhs, const integer & rhs) const;

        // Long Division returning both quotient and remainder
        std::pair <integer, integer> long_div(const integer & lhs, const integer & rhs) const;

        // Recursive Division that returns both the quotient and remainder
        // Recursion took up way too much memory
        std::pair <integer, integer> recursive_divmod(const integer & lhs, const integer & rhs) const;

        // Non-Recursive version of above algorithm
        std::pair <integer, integer> non_recursive_divmod(const integer & lhs, const integer & rhs) const;

        // division ignoring signs
        std::pair <integer, integer> dm(const integer & lhs, const integer & rhs) const;

    public:
        integer operator/(const integer & rhs) const;
            integer operator/(const bool & rhs) const;
            integer operator/(const char & rhs) const;
            integer operator/(const uint8_t & rhs) const;
            integer operator/(const uint16_t & rhs) const;
            integer operator/(const uint32_t & rhs) const;
            integer operator/(const uint64_t & rhs) const;
            integer operator/(const int8_t & rhs) const;
            integer operator/(const int16_t & rhs) const;
            integer operator/(const int32_t & rhs) const;
            integer operator/(const int64_t & rhs) const;

        integer operator/=(const integer & rhs);
            integer operator/=(const bool & rhs);
            integer operator/=(const char & rhs);
            integer operator/=(const uint8_t & rhs);
            integer operator/=(const uint16_t & rhs);
            integer operator/=(const uint32_t & rhs);
            integer operator/=(const uint64_t & rhs);
            integer operator/=(const int8_t & rhs);
            integer operator/=(const int16_t & rhs);
            integer operator/=(const int32_t & rhs);
            integer operator/=(const int64_t & rhs);

        integer operator%(const integer & rhs) const;
            integer operator%(const bool & rhs) const;
            integer operator%(const char & rhs) const;
            integer operator%(const uint8_t & rhs) const;
            integer operator%(const uint16_t & rhs) const;
            integer operator%(const uint32_t & rhs) const;
            integer operator%(const uint64_t & rhs) const;
            integer operator%(const int8_t & rhs) const;
            integer operator%(const int16_t & rhs) const;
            integer operator%(const int32_t & rhs) const;
            integer operator%(const int64_t & rhs) const;

        integer operator%=(const integer & rhs);
            integer operator%=(const bool & rhs);
            integer operator%=(const char & rhs);
            integer operator%=(const uint8_t & rhs);
            integer operator%=(const uint16_t & rhs);
            integer operator%=(const uint32_t & rhs);
            integer operator%=(const uint64_t & rhs);
            integer operator%=(const int8_t & rhs);
            integer operator%=(const int16_t & rhs);
            integer operator%=(const int32_t & rhs);
            integer operator%=(const int64_t & rhs);

        // Increment Operator
        const integer & operator++();
        integer operator++(int);

        // Decrement Operator
        const integer & operator--();
        integer operator--(int);

        // Nothing done since promotion doesn't work here
        integer operator+() const;

        // Flip Sign
        integer operator-() const;

        // get private values
        bool sign() const;

        // get number of bits
        REP_SIZE_T bits() const;

        // get number of bytes
        REP_SIZE_T bytes() const;

        // get number of digits
        REP_SIZE_T digits() const;

        // get internal data
        REP data() const;

        // Miscellaneous Functions
        integer twos_complement(unsigned int b = 0) const;

        // returns positive _value of *this
        integer abs() const;

        // returns floor(log_b(*this))
        template <typename Z>
        integer log(Z b) const{
            integer count = 0;
            integer x = *this;
            while (x){
                x /= b;
                count++;
            }
            return count;
        }

        // raises *this to exponent exp
        template <typename Z>
        integer pow(Z exp) const {
            if (exp < 0)
                return 0;
            integer result = 1;
            // take advantage of optimized integer * 10
            if (*this == 10){
                for(Z x = 0; x < exp; x++)
                    result *= *this;
                return result;
            }
            integer b = *this;
            while (exp){
                if (exp & 1)
                    result *= b;
                exp >>= 1;
                b *= b;
            }
            return result;
        }

        // fills an integer with 1s
        void fill(const uint64_t & b);

        // get bit, where 0 is the lsb and bits() - 1 is the msb
        bool operator[](const unsigned int & b) const;

        // Output _value as a string in bases 2 to 16, and 256
        std::string str(const uint16_t & base = 10, const unsigned int & length = 1) const;
};

// lhs not of type integer

// Bitwise Operators
integer operator&(const bool & lhs, const integer & rhs);
integer operator&(const char & lhs, const integer & rhs);
integer operator&(const uint8_t & lhs, const integer & rhs);
integer operator&(const uint16_t & lhs, const integer & rhs);
integer operator&(const uint32_t & lhs, const integer & rhs);
integer operator&(const uint64_t & lhs, const integer & rhs);
integer operator&(const int8_t & lhs, const integer & rhs);
integer operator&(const int16_t & lhs, const integer & rhs);
integer operator&(const int32_t & lhs, const integer & rhs);
integer operator&(const int64_t & lhs, const integer & rhs);

bool operator&=(bool & lhs, const integer & rhs);
char operator&=(char & lhs, const integer & rhs);
uint8_t operator&=(uint8_t & lhs, const integer & rhs);
uint16_t operator&=(uint16_t & lhs, const integer & rhs);
uint32_t operator&=(uint32_t & lhs, const integer & rhs);
uint64_t operator&=(uint64_t & lhs, const integer & rhs);
int8_t operator&=(int8_t & lhs, const integer & rhs);
int16_t operator&=(int16_t & lhs, const integer & rhs);
int32_t operator&=(int32_t & lhs, const integer & rhs);
int64_t operator&=(int64_t & lhs, const integer & rhs);

integer operator|(const bool & lhs, const integer & rhs);
integer operator|(const char & lhs, const integer & rhs);
integer operator|(const uint8_t & lhs, const integer & rhs);
integer operator|(const uint16_t & lhs, const integer & rhs);
integer operator|(const uint32_t & lhs, const integer & rhs);
integer operator|(const uint64_t & lhs, const integer & rhs);
integer operator|(const int8_t & lhs, const integer & rhs);
integer operator|(const int16_t & lhs, const integer & rhs);
integer operator|(const int32_t & lhs, const integer & rhs);
integer operator|(const int64_t & lhs, const integer & rhs);

bool operator|=(bool & lhs, const integer & rhs);
char operator|=(char & lhs, const integer & rhs);
uint8_t operator|=(uint8_t & lhs, const integer & rhs);
uint16_t operator|=(uint16_t & lhs, const integer & rhs);
uint32_t operator|=(uint32_t & lhs, const integer & rhs);
uint64_t operator|=(uint64_t & lhs, const integer & rhs);
int8_t operator|=(int8_t & lhs, const integer & rhs);
int16_t operator|=(int16_t & lhs, const integer & rhs);
int32_t operator|=(int32_t & lhs, const integer & rhs);
int64_t operator|=(int64_t & lhs, const integer & rhs);

integer operator^(const bool & lhs, const integer & rhs);
integer operator^(const char & lhs, const integer & rhs);
integer operator^(const uint8_t & lhs, const integer & rhs);
integer operator^(const uint16_t & lhs, const integer & rhs);
integer operator^(const uint32_t & lhs, const integer & rhs);
integer operator^(const uint64_t & lhs, const integer & rhs);
integer operator^(const int8_t & lhs, const integer & rhs);
integer operator^(const int16_t & lhs, const integer & rhs);
integer operator^(const int32_t & lhs, const integer & rhs);
integer operator^(const int64_t & lhs, const integer & rhs);

bool operator^=(bool & lhs, const integer & rhs);
char operator^=(char & lhs, const integer & rhs);
uint8_t operator^=(uint8_t & lhs, const integer & rhs);
uint16_t operator^=(uint16_t & lhs, const integer & rhs);
uint32_t operator^=(uint32_t & lhs, const integer & rhs);
uint64_t operator^=(uint64_t & lhs, const integer & rhs);
int8_t operator^=(int8_t & lhs, const integer & rhs);
int16_t operator^=(int16_t & lhs, const integer & rhs);
int32_t operator^=(int32_t & lhs, const integer & rhs);
int64_t operator^=(int64_t & lhs, const integer & rhs);

// Bitshift operators
integer operator<<(const bool & lhs, const integer & rhs);
integer operator<<(const char & lhs, const integer & rhs);
integer operator<<(const uint8_t & lhs, const integer & rhs);
integer operator<<(const uint16_t & lhs, const integer & rhs);
integer operator<<(const uint32_t & lhs, const integer & rhs);
integer operator<<(const uint64_t & lhs, const integer & rhs);
integer operator<<(const int8_t & lhs, const integer & rhs);
integer operator<<(const int16_t & lhs, const integer & rhs);
integer operator<<(const int32_t & lhs, const integer & rhs);
integer operator<<(const int64_t & lhs, const integer & rhs);

bool operator<<=(bool & lhs, const integer & rhs);
char operator<<=(char & lhs, const integer & rhs);
uint8_t operator<<=(uint8_t & lhs, const integer & rhs);
uint16_t operator<<=(uint16_t & lhs, const integer & rhs);
uint32_t operator<<=(uint32_t & lhs, const integer & rhs);
uint64_t operator<<=(uint64_t & lhs, const integer & rhs);
int8_t operator<<=(int8_t & lhs, const integer & rhs);
int16_t operator<<=(int16_t & lhs, const integer & rhs);
int32_t operator<<=(int32_t & lhs, const integer & rhs);
int64_t operator<<=(int64_t & lhs, const integer & rhs);

integer operator>>(const bool & lhs, const integer & rhs);
integer operator>>(const char & lhs, const integer & rhs);
integer operator>>(const uint8_t & lhs, const integer & rhs);
integer operator>>(const uint16_t & lhs, const integer & rhs);
integer operator>>(const uint32_t & lhs, const integer & rhs);
integer operator>>(const uint64_t & lhs, const integer & rhs);
integer operator>>(const int8_t & lhs, const integer & rhs);
integer operator>>(const int16_t & lhs, const integer & rhs);
integer operator>>(const int32_t & lhs, const integer & rhs);
integer operator>>(const int64_t & lhs, const integer & rhs);

bool operator>>=(bool & lhs, const integer & rhs);
char operator>>=(char & lhs, const integer & rhs);
uint8_t operator>>=(uint8_t & lhs, const integer & rhs);
uint16_t operator>>=(uint16_t & lhs, const integer & rhs);
uint32_t operator>>=(uint32_t & lhs, const integer & rhs);
uint64_t operator>>=(uint64_t & lhs, const integer & rhs);
int8_t operator>>=(int8_t & lhs, const integer & rhs);
int16_t operator>>=(int16_t & lhs, const integer & rhs);
int32_t operator>>=(int32_t & lhs, const integer & rhs);
int64_t operator>>=(int64_t & lhs, const integer & rhs);

// Comparison Operators
bool operator==(bool & lhs, const integer & rhs);
bool operator==(char & lhs, const integer & rhs);
bool operator==(uint8_t & lhs, const integer & rhs);
bool operator==(uint16_t & lhs, const integer & rhs);
bool operator==(uint32_t & lhs, const integer & rhs);
bool operator==(uint64_t & lhs, const integer & rhs);
bool operator==(int8_t & lhs, const integer & rhs);
bool operator==(int16_t & lhs, const integer & rhs);
bool operator==(int32_t & lhs, const integer & rhs);
bool operator==(int64_t & lhs, const integer & rhs);

bool operator!=(bool & lhs, const integer & rhs);
bool operator!=(char & lhs, const integer & rhs);
bool operator!=(uint8_t & lhs, const integer & rhs);
bool operator!=(uint16_t & lhs, const integer & rhs);
bool operator!=(uint32_t & lhs, const integer & rhs);
bool operator!=(uint64_t & lhs, const integer & rhs);
bool operator!=(int8_t & lhs, const integer & rhs);
bool operator!=(int16_t & lhs, const integer & rhs);
bool operator!=(int32_t & lhs, const integer & rhs);
bool operator!=(int64_t & lhs, const integer & rhs);

bool operator>(const bool & lhs, const integer & rhs);
bool operator>(const char & lhs, const integer & rhs);
bool operator>(const uint8_t & lhs, const integer & rhs);
bool operator>(const uint16_t & lhs, const integer & rhs);
bool operator>(const uint32_t & lhs, const integer & rhs);
bool operator>(const uint64_t & lhs, const integer & rhs);
bool operator>(const int8_t & lhs, const integer & rhs);
bool operator>(const int16_t & lhs, const integer & rhs);
bool operator>(const int32_t & lhs, const integer & rhs);
bool operator>(const int64_t & lhs, const integer & rhs);

bool operator>=(bool & lhs, const integer & rhs);
bool operator>=(char & lhs, const integer & rhs);
bool operator>=(uint8_t & lhs, const integer & rhs);
bool operator>=(uint16_t & lhs, const integer & rhs);
bool operator>=(uint32_t & lhs, const integer & rhs);
bool operator>=(uint64_t & lhs, const integer & rhs);
bool operator>=(int8_t & lhs, const integer & rhs);
bool operator>=(int16_t & lhs, const integer & rhs);
bool operator>=(int32_t & lhs, const integer & rhs);
bool operator>=(int64_t & lhs, const integer & rhs);

bool operator<(const bool & lhs, const integer & rhs);
bool operator<(const char & lhs, const integer & rhs);
bool operator<(const uint8_t & lhs, const integer & rhs);
bool operator<(const uint16_t & lhs, const integer & rhs);
bool operator<(const uint32_t & lhs, const integer & rhs);
bool operator<(const uint64_t & lhs, const integer & rhs);
bool operator<(const int8_t & lhs, const integer & rhs);
bool operator<(const int16_t & lhs, const integer & rhs);
bool operator<(const int32_t & lhs, const integer & rhs);
bool operator<(const int64_t & lhs, const integer & rhs);

bool operator<=(bool & lhs, const integer & rhs);
bool operator<=(char & lhs, const integer & rhs);
bool operator<=(uint8_t & lhs, const integer & rhs);
bool operator<=(uint16_t & lhs, const integer & rhs);
bool operator<=(uint32_t & lhs, const integer & rhs);
bool operator<=(uint64_t & lhs, const integer & rhs);
bool operator<=(int8_t & lhs, const integer & rhs);
bool operator<=(int16_t & lhs, const integer & rhs);
bool operator<=(int32_t & lhs, const integer & rhs);
bool operator<=(int64_t & lhs, const integer & rhs);

// Arithmetic Operators
integer operator+(const bool & lhs, const integer & rhs);
integer operator+(const char & lhs, const integer & rhs);
integer operator+(const uint8_t & lhs, const integer & rhs);
integer operator+(const uint16_t & lhs, const integer & rhs);
integer operator+(const uint32_t & lhs, const integer & rhs);
integer operator+(const uint64_t & lhs, const integer & rhs);
integer operator+(const int8_t & lhs, const integer & rhs);
integer operator+(const int16_t & lhs, const integer & rhs);
integer operator+(const int32_t & lhs, const integer & rhs);
integer operator+(const int64_t & lhs, const integer & rhs);

bool operator+=(bool & lhs, const integer & rhs);
char operator+=(char & lhs, const integer & rhs);
uint8_t operator+=(uint8_t & lhs, const integer & rhs);
uint16_t operator+=(uint16_t & lhs, const integer & rhs);
uint32_t operator+=(uint32_t & lhs, const integer & rhs);
uint64_t operator+=(uint64_t & lhs, const integer & rhs);
int8_t operator+=(int8_t & lhs, const integer & rhs);
int16_t operator+=(int16_t & lhs, const integer & rhs);
int32_t operator+=(int32_t & lhs, const integer & rhs);
int64_t operator+=(int64_t & lhs, const integer & rhs);

integer operator-(const bool & lhs, const integer & rhs);
integer operator-(const char & lhs, const integer & rhs);
integer operator-(const uint8_t & lhs, const integer & rhs);
integer operator-(const uint16_t & lhs, const integer & rhs);
integer operator-(const uint32_t & lhs, const integer & rhs);
integer operator-(const uint64_t & lhs, const integer & rhs);
integer operator-(const int8_t & lhs, const integer & rhs);
integer operator-(const int16_t & lhs, const integer & rhs);
integer operator-(const int32_t & lhs, const integer & rhs);
integer operator-(const int64_t & lhs, const integer & rhs);

bool operator-=(bool & lhs, const integer & rhs);
char operator-=(char & lhs, const integer & rhs);
uint8_t operator-=(uint8_t & lhs, const integer & rhs);
uint16_t operator-=(uint16_t & lhs, const integer & rhs);
uint32_t operator-=(uint32_t & lhs, const integer & rhs);
uint64_t operator-=(uint64_t & lhs, const integer & rhs);
int8_t operator-=(int8_t & lhs, const integer & rhs);
int16_t operator-=(int16_t & lhs, const integer & rhs);
int32_t operator-=(int32_t & lhs, const integer & rhs);
int64_t operator-=(int64_t & lhs, const integer & rhs);

integer operator*(const bool & lhs, const integer & rhs);
integer operator*(const char & lhs, const integer & rhs);
integer operator*(const uint8_t & lhs, const integer & rhs);
integer operator*(const uint16_t & lhs, const integer & rhs);
integer operator*(const uint32_t & lhs, const integer & rhs);
integer operator*(const uint64_t & lhs, const integer & rhs);
integer operator*(const int8_t & lhs, const integer & rhs);
integer operator*(const int16_t & lhs, const integer & rhs);
integer operator*(const int32_t & lhs, const integer & rhs);
integer operator*(const int64_t & lhs, const integer & rhs);

bool operator*=(bool & lhs, const integer & rhs);
char operator*=(char & lhs, const integer & rhs);
uint8_t operator*=(uint8_t & lhs, const integer & rhs);
uint16_t operator*=(uint16_t & lhs, const integer & rhs);
uint32_t operator*=(uint32_t & lhs, const integer & rhs);
uint64_t operator*=(uint64_t & lhs, const integer & rhs);
int8_t operator*=(int8_t & lhs, const integer & rhs);
int16_t operator*=(int16_t & lhs, const integer & rhs);
int32_t operator*=(int32_t & lhs, const integer & rhs);
int64_t operator*=(int64_t & lhs, const integer & rhs);

integer operator/(const bool & lhs, const integer & rhs);
integer operator/(const char & lhs, const integer & rhs);
integer operator/(const uint8_t & lhs, const integer & rhs);
integer operator/(const uint16_t & lhs, const integer & rhs);
integer operator/(const uint32_t & lhs, const integer & rhs);
integer operator/(const uint64_t & lhs, const integer & rhs);
integer operator/(const int8_t & lhs, const integer & rhs);
integer operator/(const int16_t & lhs, const integer & rhs);
integer operator/(const int32_t & lhs, const integer & rhs);
integer operator/(const int64_t & lhs, const integer & rhs);

bool operator/=(bool & lhs, const integer & rhs);
char operator/=(char & lhs, const integer & rhs);
uint8_t operator/=(uint8_t & lhs, const integer & rhs);
uint16_t operator/=(uint16_t & lhs, const integer & rhs);
uint32_t operator/=(uint32_t & lhs, const integer & rhs);
uint64_t operator/=(uint64_t & lhs, const integer & rhs);
int8_t operator/=(int8_t & lhs, const integer & rhs);
int16_t operator/=(int16_t & lhs, const integer & rhs);
int32_t operator/=(int32_t & lhs, const integer & rhs);
int64_t operator/=(int64_t & lhs, const integer & rhs);

integer operator%(const bool & lhs, const integer & rhs);
integer operator%(const char & lhs, const integer & rhs);
integer operator%(const uint8_t & lhs, const integer & rhs);
integer operator%(const uint16_t & lhs, const integer & rhs);
integer operator%(const uint32_t & lhs, const integer & rhs);
integer operator%(const uint64_t & lhs, const integer & rhs);
integer operator%(const int8_t & lhs, const integer & rhs);
integer operator%(const int16_t & lhs, const integer & rhs);
integer operator%(const int32_t & lhs, const integer & rhs);
integer operator%(const int64_t & lhs, const integer & rhs);

bool operator%=(bool & lhs, const integer & rhs);
char operator%=(char & lhs, const integer & rhs);
uint8_t operator%=(uint8_t & lhs, const integer & rhs);
uint16_t operator%=(uint16_t & lhs, const integer & rhs);
uint32_t operator%=(uint32_t & lhs, const integer & rhs);
uint64_t operator%=(uint64_t & lhs, const integer & rhs);
int8_t operator%=(int8_t & lhs, const integer & rhs);
int16_t operator%=(int16_t & lhs, const integer & rhs);
int32_t operator%=(int32_t & lhs, const integer & rhs);
int64_t operator%=(int64_t & lhs, const integer & rhs);

// IO Operators
std::ostream & operator<<(std::ostream & stream, const integer & rhs);
std::istream & operator>>(std::istream & stream, integer & rhs);

// Special functions
std::string makebin(const integer & value, const unsigned int & size = 1);
std::string makehex(const integer & value, const unsigned int & size = 1);
std::string makeascii(const integer & value, const unsigned int & size = 1);
integer abs(const integer & value);

template <typename Z> integer log(integer x, Z REP){
    return x.log(REP);
}

template <typename Z> integer pow(integer REP, Z exp){
    return REP.pow(exp);
}

#endif // INTEGER_H
