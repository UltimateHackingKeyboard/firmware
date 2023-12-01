#ifndef _DOUBLE_BUFFER_HPP_
#define _DOUBLE_BUFFER_HPP_

#include <array>
#include <atomic>
#include <cstring>
#include <c2usb.hpp>

template <typename T>
class double_buffer
{
    struct aligned_storage
    {
        C2USB_USB_TRANSFER_ALIGN(T, m_){};
    };

  public:
    constexpr double_buffer() {}

    T& left() { return buffers_[select_.load()].m_; }
    const T& left() const { return buffers_[select_.load()].m_; }
    T& right() { return buffers_[1 - select_.load()].m_; }
    const T& right() const { return buffers_[1 - select_.load()].m_; }

    void swap_sides() { select_.fetch_xor(1); }

    void reset() { buffers_ = {}; }

    bool differs() const { return std::memcmp(&left(), &right(), sizeof(T)) != 0; }

  private:
    std::array<aligned_storage, 2> buffers_{};
    std::atomic_uint8_t select_{};
};

#endif // _DOUBLE_BUFFER_HPP_