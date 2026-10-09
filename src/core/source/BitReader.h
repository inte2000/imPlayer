#ifndef BIT_READER_H
#define BIT_READER_H

class CBitReader {
public:
    CBitReader(const uint8_t* data, uint32_t size) : data_(data), size_(size * 8), pos_(0) {}

    uint32_t ReadBits(int n) {
        if (n <= 0 || pos_ + n > size_)
            throw std::runtime_error("Read beyond buffer");

        uint32_t result = 0;
        for (int i = 0; i < n; ++i) {
            int bytePos = (pos_ + i) / 8;
            int bitPos  = 7 - ((pos_ + i) % 8);
            uint8_t bit = (data_[bytePos] >> bitPos) & 1;
            result = (result << 1) | bit;
        }
        pos_ += n;
        return result;
    }

    void SkipBits(int n) { pos_ += n; }

private:
    const uint8_t* data_;
    uint32_t size_;
    uint32_t pos_;
};

#endif //BIT_READER_H
