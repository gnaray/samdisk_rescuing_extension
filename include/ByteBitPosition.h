#ifndef ByteBitPositionH
#define ByteBitPositionH
//---------------------------------------------------------------------------

#include <climits>
#include <cstddef>
#include <cstdint>
#include <tuple>

constexpr size_t UINT8_T_BIT_SIZE = sizeof(uint8_t) * CHAR_BIT;

class ByteBitPosition
{
public:
    constexpr ByteBitPosition()
    {
        *this = 0;
    }

    constexpr ByteBitPosition(size_t byte_bit_position)
    {
        *this = byte_bit_position;
    }

    constexpr inline size_t BytePosition() const
	{
		return m_byte_position;
	}

    constexpr uint8_t BitPosition() const
	{
		return m_bit_position;
	}

    constexpr size_t TotalBitPosition() const
	{
        return m_byte_position * UINT8_T_BIT_SIZE + m_bit_position;
	}
/*  // Too many ambigous operator overloads...
    constexpr operator size_t() const
    {
        return TotalBitPosition();
    }
*/
//	auto operator<=>(const ByteBitPosition&) const = default;

    constexpr friend bool operator==(const ByteBitPosition& lhs, const ByteBitPosition& rhs)
    {
        return std::tie(lhs.m_byte_position, lhs.m_byte_position)
            == std::tie(rhs.m_byte_position, rhs.m_byte_position); // keep the same order.
    }

    constexpr friend bool operator<(const ByteBitPosition& lhs, const ByteBitPosition& rhs)
    {
        return std::tie(lhs.m_byte_position, lhs.m_byte_position)
            < std::tie(rhs.m_byte_position, rhs.m_byte_position); // keep the same order.
    }

    constexpr ByteBitPosition& operator=(size_t totalBitPosition)
	{
        m_bit_position = totalBitPosition % UINT8_T_BIT_SIZE;
        m_byte_position = totalBitPosition / UINT8_T_BIT_SIZE;
		return *this;
	}

    constexpr ByteBitPosition& operator+=(const ByteBitPosition& rhs)
	{
		*this = TotalBitPosition() + rhs.TotalBitPosition();
		return *this;
	}

    constexpr ByteBitPosition& operator+=(size_t bits)
	{
		*this = TotalBitPosition() + bits;
		return *this;
	}

	// Friends defined inside class body are inline and are hidden from non-ADL lookup.
	// Passing lhs by value helps optimize chained a+b+c,
	// otherwise both parameters may be const references.
    constexpr friend ByteBitPosition operator+(ByteBitPosition lhs,
		const ByteBitPosition& rhs)
    {
        lhs += rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }

    constexpr ByteBitPosition& operator++()
	{
        if (m_bit_position < UINT8_T_BIT_SIZE - 1)
			++m_bit_position;
		else
		{
			m_bit_position = 0;
			m_byte_position++;
		}
		return *this;
	}

    constexpr ByteBitPosition operator++(int)
	{
		ByteBitPosition temp = *this;
		++*this;

		return temp;
	}

    constexpr ByteBitPosition& operator-=(const ByteBitPosition& rhs)
	{
		*this = TotalBitPosition() - rhs.TotalBitPosition();
		return *this;
	}

    constexpr ByteBitPosition& operator-=(size_t bits)
	{
		*this = TotalBitPosition() - bits;
		return *this;
	}

	// Friends defined inside class body are inline and are hidden from non-ADL lookup.
	// Passing lhs by value helps optimize chained a+b+c,
	// otherwise both parameters may be const references.
    constexpr friend ByteBitPosition operator-(ByteBitPosition lhs,
		const ByteBitPosition& rhs)
    {
        lhs -= rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }

    constexpr ByteBitPosition& operator--()
	{
		if (m_bit_position > 0)
			--m_bit_position;
		else
		{
            m_bit_position = UINT8_T_BIT_SIZE - 1;
			m_byte_position--;
		}
		return *this;
	}

    constexpr ByteBitPosition operator--(int)
	{
		ByteBitPosition temp = *this;
		--*this;

		return temp;
	}

    constexpr ByteBitPosition& operator*=(const size_t multiplier)
    {
        *this = TotalBitPosition() * multiplier;
        return *this;
    }

    // Friends defined inside class body are inline and are hidden from non-ADL lookup.
    // Passing lhs by value helps optimize chained a*b*c,
    // otherwise both parameters may be const references.
    constexpr friend ByteBitPosition operator*(ByteBitPosition lhs,
        const size_t multiplier)
    {
        lhs *= multiplier; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }

    constexpr ByteBitPosition& PreAddBytes(size_t bytes)
	{
		m_byte_position += bytes;
		return *this;
	}

    constexpr ByteBitPosition PostAddBytes(size_t bytes)
	{
		ByteBitPosition temp = *this;
        this->PreAddBytes(bytes);

		return temp;
	}

    constexpr ByteBitPosition AddBytes(size_t bytes)
    {
        ByteBitPosition lhs = *this;
        lhs.PreAddBytes(bytes);
        return lhs;
    }

    constexpr ByteBitPosition& PreSubBytes(size_t bytes)
    {
        m_byte_position -= bytes;
        return *this;
    }

    constexpr ByteBitPosition PostSubBytes(size_t bytes)
    {
        ByteBitPosition temp = *this;
        this->PreSubBytes(bytes);

        return temp;
    }

    constexpr ByteBitPosition SubBytes(size_t bytes)
    {
        ByteBitPosition lhs = *this;
        lhs.PreSubBytes(bytes);
        return lhs;
    }

private:
    // Order is important for relational (e.g. spaceship) operator.
    size_t m_byte_position = 0;
	uint8_t m_bit_position = 0;
};

constexpr bool operator!=(const ByteBitPosition& lhs, const ByteBitPosition& rhs)
{
    return !(lhs == rhs);
}

constexpr inline bool operator> (const ByteBitPosition& lhs, const ByteBitPosition& rhs) { return rhs < lhs; }
constexpr inline bool operator<=(const ByteBitPosition& lhs, const ByteBitPosition& rhs) { return !(lhs > rhs); }
constexpr inline bool operator>=(const ByteBitPosition& lhs, const ByteBitPosition& rhs) { return !(lhs < rhs); }

//---------------------------------------------------------------------------
#endif
