#pragma once

#include <QPointF>
#include <QVector2D>
#include <array>
#include <cassert>
#include <cmath>
#include <complex>
#include <cstddef>
#include <optional>

/**
 * @brief The affine 3D Matrix for 2D space
 */
template<typename T>
    requires std::is_arithmetic_v<T>
class Matrix
{
    template <typename P>
    friend class MatrixPrivate;

public:
    /**
     * @brief identity - create identity matrix
     *  | 1  0  0 |
     *  | 0  1  0 |
     *  | 0  0  1 |
     * @return identity matrix
     */
    static constexpr Matrix identity()
    {
        return Data { I, O, O, O, I, O, O, O, I };
    }

    /**
     * @brief scale - create scale matrix
     *  | f  0  0 |
     *  | 0  f  0 |
     *  | 0  0  1 |
     * @param f - vertical and horizontal scale factor
     * @return Matrix with scale
     */
    static constexpr Matrix scale(T f)
    {
        return scale(f, f);
    }

    /**
     * @brief scale - create scale matrix
     *  | x  0  0 |
     *  | 0  y  0 |
     *  | 0  0  1 |
     * @param x - horizontal scale factor
     * @param y - vertical scale factor
     * @return Matrix with scale
     */
    static constexpr Matrix scale(T x, T y)
    {
        return Data { x, O, O, O, y, O, O, O, I };
    }

    /**
     * @brief translate - create translate matrix
     *  | 1  0  x |
     *  | 0  1  y |
     *  | 0  0  1 |
     * @param x - horizontal translation
     * @param y - vertical translation
     * @return Matrix with translation
     */
    static constexpr Matrix translate(T x, T y)
    {
        return Data { I, O, x, O, I, y, O, O, I };
    }

    static constexpr Matrix translate(const QPointF &offset)
        requires std::is_same<T, qreal>::value
    {
        return translate(offset.x(), offset.y());
    }

    /**
     * @brief rotateRad - create translate matrix from radians
     *  | cos(θ) -sin(θ)   0 |
     *  | sin(θ)  cos(θ)   0 |
     *  |   0       0      1 |
     * @param rad
     * @return Matrix with rotation
     */
    static Matrix rotate(const std::complex<T>& rotor)
    {
        return Data { rotor.real(), -rotor.imag(), O, rotor.imag(), rotor.real(), O, O, O, I };
    }

    QVector2D scale() const { return {m_data[Indices::ScaleX], m_data[Indices::ScaleY]}; }

    const auto& scaleX() const
    {
        return m_data[Indices::ScaleX];
    }

    const auto& scaleY() const
    {
        return m_data[Indices::ScaleY];
    }

    QPointF translation() const { return {m_data[Indices::TransX], m_data[Indices::TransY]}; }

    std::complex<T> rotation() const
    {
        assert(m_data[Indices::ScaleY]);
        assert(m_data[Indices::SkewY]);
        return { m_data[Indices::ScaleX] / m_data[Indices::ScaleY], -m_data[Indices::SkewX] / m_data[Indices::SkewY] };
    }

    /**
     * @brief operator* - multiply `this` with `rhs`
     * ![alt text](https://d138zd1ktt9iqe.cloudfront.net/media/seo_landing_files/multiplication-of-matrices-of-order-3-x-3-1627879219.png)
     * @note opration is not commutative
     * @param rhs - right matrix
     * @return this * rhs
     */
    constexpr Matrix operator*(const Matrix& rhs) const
    {
        const auto& l = m_data;
        const auto& r = rhs.m_data;

        return Data {
            (l[0] * r[0] + l[1] * r[3] + l[2] * r[6]),
            (l[0] * r[1] + l[1] * r[4] + l[2] * r[7]),
            (l[0] * r[2] + l[1] * r[5] + l[2] * r[8]),

            (l[3] * r[0] + l[4] * r[3] + l[5] * r[6]),
            (l[3] * r[1] + l[4] * r[4] + l[5] * r[7]),
            (l[3] * r[2] + l[4] * r[5] + l[5] * r[8]),

            (l[6] * r[0] + l[7] * r[3] + l[8] * r[6]),
            (l[6] * r[1] + l[7] * r[4] + l[8] * r[7]),
            (l[6] * r[2] + l[7] * r[5] + l[8] * r[8])
        };
    };

    /**
     * @brief operator* - simple 3D vec-mat multiplication
     * @param rhs - vector to transform
     * @return transformed vector
     */
    constexpr QVector3D operator*(const QVector3D &rhs) const
    {
        const auto x = rhs.x();
        const auto y = rhs.y();
        const auto z = rhs.z();
        return Vec<T, 3>(
            (a() * x + b() * y + c() * z),
            (d() * x + e() * y + f() * z),
            (g() * x + h() * y + i() * z));
    };

    /**
     * @brief applyAffine
     * @return vector (x, y) multiplied by matrix
     *               | A B C |
     * @param this - | D E F |
     *               | G H I |
     *              | x |
     * @param rhs - | y |
     *              | 1 |
     *
     *                       |A B C| |x|                               Ax+By+C   Dx+Ey+F
     * @return Matrix * pt = |D E F| |y| = |Ax+By+C Dx+Ey+F Gx+Hy+I| = ------- , -------
     *                       |G H I| |1|                               Gx+Hy+I   Gx+Hy+I
     */
    constexpr QPointF applyAffine(const QPointF &rhs) const
        requires std::is_same<T, qreal>::value
    {
        const auto result = *this * Vec<T, 3>(rhs.x(), rhs.y(), I);
        return ScreenPoint(result.x() / result.z(), result.y() / result.z());
    }

    //constexpr QRectF applyAffine(const QRectF& rhs) const
    //{
    //    QPointF transformed[] = {
    //        applyAffine(rhs.topLeft()),
    //        applyAffine(rhs.topRight()),
    //        applyAffine(rhs.bottomRight()),
    //        applyAffine(rhs.bottomLeft())
    //    };
    //    return QRectF::aabb(Range { transformed, transformed + sizeof(transformed) / sizeof(transformed[0]) });
    //}

    /**
     * @brief applyAffineZeroTranslation
     * @return vector (x, y) multiplied by matrix, treating matrix translation as zero.
     *               | A B 0 |
     * @param this - | D E 0 |
     *               | G H I |
     *              | x |
     * @param rhs - | y |
     *              | 1 |
     *
     *            |A B 0| |x|                            Ax+By     Dx+Ey
     * @return -  |D E 0| |y| = |Ax+By Dx+Ey Gx+Hy+I| = ------- , -------
     *            |G H I| |1|                           Gx+Hy+I   Gx+Hy+I
     */
    constexpr QPointF applyAffineZeroTranslation(const QPointF &rhs) const
    {
        const auto result = Matrix({ a(), b(), O, d(), e(), O, g(), h(), i() }) * Vec<T, 3>(rhs.x(), rhs.y(), I);
        return ScreenPoint(result.x() / result.z(), result.y() / result.z());
    }

    /**
     * @brief apply only affine scale to size object
     */
    constexpr QSizeF applyScale(const QSizeF &rhs) const
    {
        return ScreenSize(rhs.width() * a(), rhs.height() * e());
    }

    /**
     * @brief operator* - affine 2D vec-mat multiplication
     * @param rhs - point to transform
     * @return transformed point
     */
    constexpr QPointF operator*(const QPointF &rhs) const { return applyAffine(rhs); };

    /**
     * @brief operator* - affine 2D vec-mat multiplication
     * @param rhs - size to transform
     * @return transformed size
     */
    constexpr QSizeF operator*(const QSizeF &rhs) const { return applyScale(rhs); }

    /**
     * @brief operator* - affine 2D vec-mat multiplication
     * @param rhs - rect to transform
     * @return transformed rect
     */
    constexpr QRectF operator*(const QRectF &rhs) const { return applyAffine(rhs); }

    /**
     * @brief operator~ - invert matrix
     * @return inverted matrix if this is invertable else nullopt
     */
    std::optional<Matrix> operator~() const
    {
        const auto dd = det();
        if (dd == 0) {
            return std::nullopt;
        }
        const auto t = transposed();
        return Matrix { Data {
            det2x2(t.template minor<0, 0>()) / dd,
            -det2x2(t.template minor<1, 0>()) / dd,
            det2x2(t.template minor<2, 0>()) / dd,
            -det2x2(t.template minor<0, 1>()) / dd,
            det2x2(t.template minor<1, 1>()) / dd,
            -det2x2(t.template minor<2, 1>()) / dd,
            det2x2(t.template minor<0, 2>()) / dd,
            -det2x2(t.template minor<1, 2>()) / dd,
            det2x2(t.template minor<2, 2>()) / dd } };
    }

    /**
     * @brief transposed
     * @return returns matrix reflected about the main diagonal
     */
    Matrix transposed() const
    {
        return Matrix({ a(), d(), g(), b(), e(), h(), c(), f(), i() });
    }

    template <std::size_t i, std::size_t j>
    constexpr std::array<T, 4> minor() const
    {
        static_assert(i < SideLen && j < SideLen, "wrong bounds for minor matrix");
        std::size_t pos = 0;
        std::array<T, 4> result;
        for (std::size_t y = 0; y < SideLen; ++y) {
            for (std::size_t x = 0; x < SideLen; ++x) {
                if (x != i && y != j) {
                    result[pos++] = m_data[x + y * SideLen];
                }
            }
        }
        return result;
    }

    /**
     * @brief det2x2 - determinant of 2x2 matrix
     * @return
     */
    constexpr static T det2x2(const std::array<T, 4>& data)
    {
        const auto a = data[0];
        const auto b = data[1];
        const auto c = data[2];
        const auto d = data[3];
        return a * d - b * c;
    }

    /**
     * @brief det - determinant
     * @return
     */
    constexpr T det() const
    {
        return a() * e() * i() + b() * f() * g() + c() * d() * h() - c() * e() * g() - b() * d() * i() - a() * f() * h();
    }

    auto operator<=>(const Matrix&) const = default;

    // clang-format off
    constexpr const auto& a() const { return m_data[0]; }
    constexpr const auto& b() const { return m_data[1]; }
    constexpr const auto& c() const { return m_data[2]; }
    constexpr const auto& d() const { return m_data[3]; }
    constexpr const auto& e() const { return m_data[4]; }
    constexpr const auto& f() const { return m_data[5]; }
    constexpr const auto& g() const { return m_data[6]; }
    constexpr const auto& h() const { return m_data[7]; }
    constexpr const auto& i() const { return m_data[8]; }
    // clang-format on

    friend std::ostream& operator<<(std::ostream& stream, const Matrix& m)
    {
        return stream << "[ " << m.m_data[0]
                      << ", " << m.m_data[1]
                      << ", " << m.m_data[2]
                      << ", " << m.m_data[3]
                      << ", " << m.m_data[4]
                      << ", " << m.m_data[5]
                      << ", " << m.m_data[6]
                      << ", " << m.m_data[7]
                      << ", " << m.m_data[8]
                      << " ]";
    }

private:
    using Data = std::array<T, 9>;
    static constexpr std::size_t SideLen = 3;

    // TODO make implementation for nZ, Real, PBU, ...
    static constexpr T I = T { 1 };
    static constexpr T O = T { 0 };

    struct Indices
    {
        static constexpr std::size_t ScaleX = 0; //!< horizontal scale factor
        static constexpr std::size_t SkewX = 1; //!< horizontal skew factor
        static constexpr std::size_t TransX = 2; //!< horizontal translation
        static constexpr std::size_t SkewY = 3; //!< vertical skew factor
        static constexpr std::size_t ScaleY = 4; //!< vertical scale factor
        static constexpr std::size_t TransY = 5; //!< vertical translation
        static constexpr std::size_t Persp0 = 6; //!< input x perspective factor
        static constexpr std::size_t Persp1 = 7; //!< input y perspective factor
        static constexpr std::size_t Persp2 = 8; //!< perspective bias
    };

    constexpr Matrix(const Data& d)
        : m_data(d)
    {
    }

private:
    Data m_data;
};
