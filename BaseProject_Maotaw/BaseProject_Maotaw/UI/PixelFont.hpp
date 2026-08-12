#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "Core/RendererCore.hpp"

namespace PixelFont {
    using Glyph = std::array<uint8_t, 7>;

    [[nodiscard]] inline Glyph GetGlyph(char c) {
        switch (c) {
        case 'A': return {14,17,17,31,17,17,17};
        case 'B': return {30,17,17,30,17,17,30};
        case 'C': return {15,16,16,16,16,16,15};
        case 'D': return {30,17,17,17,17,17,30};
        case 'E': return {31,16,16,30,16,16,31};
        case 'F': return {31,16,16,30,16,16,16};
        case 'G': return {15,16,16,19,17,17,15};
        case 'H': return {17,17,17,31,17,17,17};
        case 'I': return {31,4,4,4,4,4,31};
        case 'K': return {17,18,20,24,20,18,17};
        case 'L': return {16,16,16,16,16,16,31};
        case 'M': return {17,27,21,21,17,17,17};
        case 'N': return {17,25,21,19,17,17,17};
        case 'O': return {14,17,17,17,17,17,14};
        case 'P': return {30,17,17,30,16,16,16};
        case 'R': return {30,17,17,30,20,18,17};
        case 'S': return {15,16,16,14,1,1,30};
        case 'T': return {31,4,4,4,4,4,4};
        case 'U': return {17,17,17,17,17,17,14};
        case 'V': return {17,17,17,17,17,10,4};
        case 'W': return {17,17,17,21,21,27,17};
        case 'X': return {17,17,10,4,10,17,17};
        case 'Y': return {17,17,10,4,4,4,4};
        case 'Z': return {31,1,2,4,8,16,31};
        case '0': return {14,17,19,21,25,17,14};
        case '1': return {4,12,4,4,4,4,14};
        case '2': return {14,17,1,2,4,8,31};
        case '3': return {30,1,1,14,1,1,30};
        case '4': return {2,6,10,18,31,2,2};
        case '5': return {31,16,16,30,1,1,30};
        case '6': return {14,16,16,30,17,17,14};
        case '7': return {31,1,2,4,8,8,8};
        case '8': return {14,17,17,14,17,17,14};
        case '9': return {14,17,17,15,1,1,14};
        case '+': return {0,4,4,31,4,4,0};
        case '!': return {4,4,4,4,4,0,4};
        case '.': return {0,0,0,0,0,0,4};
        case ':': return {0,4,4,0,4,4,0};
        case '>': return {16,8,4,2,4,8,16};
        case '%': return {17,2,4,8,16,8,17};
        default: return {};
        }
    }

    [[nodiscard]] inline int Width(const char* text) {
        int length = 0;
        while (text[length]) ++length;
        return length ? length * 6 - 1 : 0;
    }

    inline void Draw(int x, int y, const char* text, uint32_t color) {
        if (!RendererCore::Ready()) return;
        RendererCore::MarkDirty(x, y, x + Width(text), y + 7);

        for (int index = 0; text[index]; ++index) {
            const Glyph glyph = GetGlyph(text[index]);
            for (int row = 0; row < 7; ++row)
                for (int column = 0; column < 5; ++column)
                    if (glyph[row] & (1u << (4 - column))) {
                        const int pixelX = x + index * 6 + column;
                        const int pixelY = y + row;
                        if (pixelX >= 0 && pixelX < RendererCore::width &&
                            pixelY >= 0 && pixelY < RendererCore::height) {
                            RendererCore::bits[static_cast<size_t>(pixelY) *
                                               RendererCore::width + pixelX] = color;
                        }
                    }
        }
    }

    inline void Number(int value, char (&buffer)[4]) {
        value = std::clamp(value, 0, 999);
        if (value >= 100) {
            buffer[0] = static_cast<char>('0' + value / 100);
            buffer[1] = static_cast<char>('0' + value / 10 % 10);
            buffer[2] = static_cast<char>('0' + value % 10);
        } else if (value >= 10) {
            buffer[0] = static_cast<char>('0' + value / 10);
            buffer[1] = static_cast<char>('0' + value % 10);
        } else {
            buffer[0] = static_cast<char>('0' + value);
        }
    }

    inline void Decimal(float value, char (&buffer)[4]) {
        const int tenths = std::clamp(
            static_cast<int>(value * 10.0f + 0.5f), 1, 10);
        buffer[0] = static_cast<char>('0' + tenths / 10);
        buffer[1] = '.';
        buffer[2] = static_cast<char>('0' + tenths % 10);
    }
}
