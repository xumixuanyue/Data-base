#include "p0_starter.h"
# include<iostream>

using namespace std;

int main()
{
    cout << "--test beginning--" << endl;
    scudb::RowMatrix<int> matr1(5, 6);
    for (int i = 0; i < matr1.GetRowCount(); ++i)
    {
        for (int j = 0; j < matr1.GetColumnCount(); ++j)
        {
            matr1.SetElement(i, j, i * matr1.GetColumnCount() + j);
        }
    }
    cout << "Matrix1£º" << endl;
    scudb::RowMatrixOperations<int>::print(matr1);

    vector<int> tem56(30);
    for (int i = 0; i < 30; i++) {
        tem56[i] = i % 5;
    }
    cout << "Matrix2£º" << endl;
    scudb::RowMatrix<int> matr2(5, 6);
    matr2.FillFrom(tem56);
    scudb::RowMatrixOperations<int>::print(matr2);

    cout << "Matrix3£º" << endl;
    scudb::RowMatrix<int> matr3(6, 5);
    matr3.FillFrom(tem56);
    scudb::RowMatrixOperations<int>::print(matr3);

    vector<int> tem55(25);
    for (int i = 0; i < 25; i++) {
        tem55[i] = i % 6;
    }
    cout << "Matrix4£º" << endl;
    scudb::RowMatrix<int> matr4(5, 5);
    matr4.FillFrom(tem55);
    scudb::RowMatrixOperations<int>::print(matr4);

    cout << "Add Matrix1 and Matrix2 to get Matrix5£º" << endl;
    unique_ptr<scudb::RowMatrix<int>> matr5 = scudb::RowMatrixOperations<int>::Add(&matr1, &matr2);
    scudb::RowMatrixOperations<int>::print(*matr5);

    cout << "Multiplied Matrix2 and Matrix3 to get Matrix6£º" << endl;
    unique_ptr<scudb::RowMatrix<int>> matr6 = scudb::RowMatrixOperations<int>::Multiply(&matr2, &matr3);
    scudb::RowMatrixOperations<int>::print(*matr6);

    cout << "GEMM Matrix2, Matrix3 and Matrix4 to get Matrix7£º" << endl;
    unique_ptr<scudb::RowMatrix<int>> matr7 = scudb::RowMatrixOperations<int>::GEMM(&matr2, &matr3, &matr4);
    scudb::RowMatrixOperations<int>::print(*matr7);

    cout << "--test over--" << endl;
    return 0;
}
