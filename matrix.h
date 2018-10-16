#ifndef MATRIX_H
#define MATRIX_H

#include <vector>

template <typename T>
class Matrix {
public:
    Matrix(size_t rows, size_t cols);
    T& operator()(size_t i, size_t j);
    const T& operator()(size_t i, size_t j) const;
    size_t cols() const { return mCols; }
    size_t rows() const { return mRows; }

private:
    size_t mRows;
    size_t mCols;
    std::vector<T> mData;
};

template <typename T>
Matrix<T>::Matrix(size_t rows, size_t cols)
    : mRows(rows)
    , mCols(cols)
    , mData(rows * cols)
{
}

template <typename T>
T& Matrix<T>::operator()(size_t row, size_t col)
{
    return mData[row * mCols + col];
}

template <typename T>
const T& Matrix<T>::operator()(size_t i, size_t j) const
{
    return mData[i * mCols + j];
}

#endif // MATRIX_H
