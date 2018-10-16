#include "date.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <mpi.h>
#include <random>
#include <string>
#include <vector>

#include "matrix.h"

using namespace std;

Matrix<int> compute_neighbors(const Matrix<int>& slice)
{
    auto newslice = slice; // copy the orignal matrix, but we'll just discard the values
    size_t max_x = slice.rows() - 2;
    size_t max_y = slice.cols() - 2;

    for (size_t x = 1; x < max_x; x++) // for each row
    {
        for (size_t y = 1; y < max_y; y++) // for each column
        {
            int sum = slice(x - 1, y - 1) + slice(x - 1, y) + slice(x - 1, y + 1) + slice(x, y + 1) + slice(x + 1, y + 1) + slice(x + 1, y) + slice(x + 1, y - 1) + slice(x, y - 1);

            // PUT THE NEW VALUE OF A CELL
            const int val = slice(x, y);
            int& newval = newslice(x, y);
            if (val == 1 && (sum == 2 || sum == 3)) {
                newval = 1;
            } else if (val == 1) {
                newval = 0;
            } else if (val == 0 && sum == 3) {
                newval = 1;
            } else {
                newval = 0;
            }
        }
    }
    return newslice;
}

void init_slice(Matrix<int>& slice, int rank)
{
    default_random_engine gen(1337 + static_cast<unsigned int>(rank));
    uniform_int_distribution<int> dist(0, 10);

    size_t max_x = slice.rows() - 2;
    size_t max_y = slice.cols() - 2;

    for (size_t x = 1; x <= max_x; ++x) {
        for (size_t y = 1; y <= max_y; ++y) {
            int r = dist(gen);
            r = min(r, 1); // P(r = 0) = 1/10, P(r = 1) = 9/10
            slice(x, y) = r;
        }
    }
}

int main(int argc, char* argv[])
{
    int size, rank;
    MPI_Init(&argc, &argv);

    if (argc != 5) {
        cout << "Usage: " << argv[0] << " plane-dimension procs-x procs-y timesteps\n";
        return 1;
    }
    int64_t plane_dimension = atoll(argv[1]);
    int procs_x = static_cast<int>(atoll(argv[2]));
    int procs_y = static_cast<int>(atoll(argv[3]));
    int64_t timesteps = atoll(argv[4]);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    assert(procs_x * procs_y == size);

    assert(plane_dimension % procs_x == 0);
    assert(plane_dimension % procs_y == 0);

    size_t subplane_x = static_cast<size_t>(plane_dimension / procs_x);
    size_t subplane_y = static_cast<size_t>(plane_dimension / procs_x);
    Matrix<int> slice(subplane_x + 2, subplane_y + 2);
    init_slice(slice, rank);

    int proc_up = (rank < procs_x) ? MPI_PROC_NULL : rank - procs_x;
    int proc_down = (rank + procs_x >= size) ? MPI_PROC_NULL : rank + procs_x;
    int proc_left = (rank % procs_x == 0) ? MPI_PROC_NULL : rank - 1;
    int proc_right = ((rank + 1) % procs_x == 0) ? MPI_PROC_NULL : rank + 1;

    size_t max_col = slice.cols() - 2;
    size_t max_row = slice.rows() - 2;

    for (int64_t step = 0; step < timesteps; ++step) {
        if (rank == 0) {
            std::string s = date::format("[%T] ", std::chrono::system_clock::now());
            cout << s << "Step: t = " << step << endl;
        }
        vector<MPI_Request> vec;
        MPI_Request req;
        for (size_t col = 1; col <= max_col; ++col) {
            MPI_Isend(&slice(1, col), 1, MPI_INT, proc_up, 0, MPI_COMM_WORLD, &req);
            vec.push_back(req);
            MPI_Isend(&slice(max_row, col), 1, MPI_INT, proc_down, 0, MPI_COMM_WORLD, &req);
            vec.push_back(req);

            MPI_Irecv(&slice(0, col), 1, MPI_INT, proc_up, 0, MPI_COMM_WORLD, &req);
            vec.push_back(req);
            MPI_Irecv(&slice(max_row + 1, col), 1, MPI_INT, proc_down, 0, MPI_COMM_WORLD, &req);
            vec.push_back(req);
        }
        for (size_t row = 1; row <= max_row; ++row) {
            MPI_Isend(&slice(row, 1), 1, MPI_INT, proc_left, 0, MPI_COMM_WORLD, &req);
            vec.push_back(req);
            MPI_Isend(&slice(row, max_col), 1, MPI_INT, proc_right, 0, MPI_COMM_WORLD, &req);
            vec.push_back(req);

            MPI_Irecv(&slice(row, 0), 1, MPI_INT, proc_left, 0, MPI_COMM_WORLD, &req);
            vec.push_back(req);
            MPI_Irecv(&slice(row, max_col + 1), 1, MPI_INT, proc_right, 0, MPI_COMM_WORLD, &req);
            vec.push_back(req);
        }

        MPI_Waitall(static_cast<int>(vec.size()), vec.data(), nullptr);

        slice = compute_neighbors(slice);
    }

    int64_t count = 0;
    for (size_t row = 1; row <= max_row; ++row) {
        for (size_t col = 1; col <= max_col; ++col) {
            count += slice(row, col);
        }
    }
    int64_t count_total = 0;

    MPI_Allreduce(&count, &count_total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (rank == 0) {
        cout << count_total << endl;
    }

    MPI_Finalize();
}