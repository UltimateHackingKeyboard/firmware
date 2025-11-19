#ifndef _DOUBLE_BUFFER_HPP_
#define _DOUBLE_BUFFER_HPP_

#include <array>
#include <atomic>
#include <c2usb.hpp>
#include <cstring>

template <typename T>
class double_buffer {
    struct aligned_storage {
        C2USB_USB_TRANSFER_ALIGN(T, m_){};
        constexpr bool operator==(const aligned_storage &other) const = default;
        constexpr bool operator!=(const aligned_storage &other) const = default;
    };

  public:
    constexpr double_buffer() {}

    T &operator[](uint8_t i) { return buffers_[i].m_; }

    const T &operator[](uint8_t i) const { return buffers_[i].m_; }

    uint8_t active_side(std::memory_order mo = std::memory_order::seq_cst) const
    {
        return select_.load(mo);
    }

    uint8_t inactive_side(std::memory_order mo = std::memory_order::seq_cst) const
    {
        return 1 - select_.load(mo);
    }

    template <typename TArg>
    uint8_t indexof(TArg *ptr) const
    {
        auto oneptr = reinterpret_cast<std::uintptr_t>(&buffers_[1]);
        auto dataptr = reinterpret_cast<std::uintptr_t>(ptr);
        return (dataptr < oneptr) ? 0 : 1;
    }

    void swap_sides(std::memory_order mo = std::memory_order::seq_cst) { select_.fetch_xor(1, mo); }

    bool compare_swap(uint8_t i, std::memory_order mo = std::memory_order::seq_cst)
    {
        return select_.compare_exchange_strong(i, 1 - i, mo);
    }

    bool compare_swap_copy(uint8_t i, std::memory_order mo = std::memory_order::seq_cst)
    {
        if (compare_swap(i, mo)) {
            buffers_[1 - i] = buffers_[i];
            return true;
        }
        return false;
    }

    void reset() { buffers_ = {}; }

    // bool differs() const { return std::memcmp(&buffers_[0] != &buffers_[1], sizeof(T)) != 0; }
    bool differs() const { return buffers_[0] != buffers_[1]; }

  private:
    std::array<aligned_storage, 2> buffers_{};
    std::atomic_uint8_t select_{};
};

#endif // _DOUBLE_BUFFER_HPP_