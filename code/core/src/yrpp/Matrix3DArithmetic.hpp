// Internal arithmetic for the original Matrix3D/Quaternion module.
// x87 control word 0x0E7F: double operations and float stores truncate.
#pragma once
#include <bit>
#include <cmath>
#include <cstdint>
#include "yrpp/Matrix3D.h"
namespace {
inline float matrix_store_float(double value) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
    // clang-format off
    float result;
    __asm fld value
    __asm fstp result
    return result;
// clang-format on
#elif defined(__i386__) || defined(__x86_64__)
    float result;
    __asm__ volatile("fldl %1; fstps %0" : "=m"(result) : "m"(value) : "st");
    return result;
#else
    const float result = float(value);
    return std::abs(double(result)) > std::abs(value) ? std::bit_cast<float>(std::bit_cast<std::uint32_t>(result) - 1u)
                                                      : result;
#endif
}
inline double matrix_add(double a, double b) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
    // clang-format off
    double result;
    __asm fld a
    __asm fadd b
    __asm fstp result
    return result;
// clang-format on
#elif defined(__i386__) || defined(__x86_64__)
    double result;
    __asm__ volatile("fldl %1; faddl %2; fstpl %0" : "=m"(result) : "m"(a), "m"(b) : "st");
    return result;
#else
    const double sum = a + b, virtual_b = sum - a;
    const double error = (a - (sum - virtual_b)) + (b - virtual_b);
    return (sum > 0 && error < 0) || (sum < 0 && error > 0) ? std::nextafter(sum, 0.0) : sum;
#endif
}
inline double matrix_multiply(double a, double b) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
    // clang-format off
    double result;
    __asm fld a
    __asm fmul b
    __asm fstp result
    return result;
// clang-format on
#elif defined(__i386__) || defined(__x86_64__)
    double result;
    __asm__ volatile("fldl %1; fmull %2; fstpl %0" : "=m"(result) : "m"(a), "m"(b) : "st");
    return result;
#else
    const double product = a * b, error = std::fma(a, b, -product);
    return (product > 0 && error < 0) || (product < 0 && error > 0) ? std::nextafter(product, 0.0) : product;
#endif
}
inline double matrix_divide(double a, double b) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
    // clang-format off
    double result;
    __asm fld a
    __asm fdiv b
    __asm fstp result
    return result;
// clang-format on
#elif defined(__i386__) || defined(__x86_64__)
    double result;
    __asm__ volatile("fldl %1; fdivl %2; fstpl %0" : "=m"(result) : "m"(a), "m"(b) : "st");
    return result;
#else
    const double result = a / b, error = std::fma(-result, b, a) * std::copysign(1.0, b);
    return (result > 0 && error < 0) || (result < 0 && error > 0) ? std::nextafter(result, 0.0) : result;
#endif
}
inline Matrix3D drawing_matrix_product(const Matrix3D& a, const Matrix3D& b) noexcept {
    Matrix3D out;
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 4; ++x) {
            double value = matrix_add(double(a.row[y][2]) * b.row[2][x], double(a.row[y][1]) * b.row[1][x]);
            value = matrix_add(value, double(a.row[y][0]) * b.row[0][x]);
            out.row[y][x] = matrix_store_float(x == 3 ? matrix_add(value, a.row[y][3]) : value);
        }
    return out;
}
inline void drawing_translate_axis(Matrix3D& m, unsigned axis, float value) noexcept {
    for (unsigned row = 0; row < 3; ++row)
        m.row[row][3] = matrix_store_float(matrix_add(double(value) * m.row[row][axis], m.row[row][3]));
}
inline void drawing_translate(Matrix3D& m, const Vector3D<float>& p) noexcept {
    const double z =
        matrix_add(matrix_add(matrix_add(double(p.X) * m.row[2][0], m.row[2][3]), double(p.Y) * m.row[2][1]),
                   double(p.Z) * m.row[2][2]);
    drawing_translate_axis(m, 0, p.X);
    drawing_translate_axis(m, 1, p.Y);
    drawing_translate_axis(m, 2, p.Z);
    m.row[2][3] = matrix_store_float(z);
}
}
