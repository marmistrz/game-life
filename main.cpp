#include "date.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <mpi.h>
#include <random>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#include "matrix.h"

using namespace std;

Matrix<int> compute_neighbors(const Matrix<int>& slice)
{
    auto newslice = slice; // copy the orignal matrix, but we'll just discard the values
    size_t max_col = slice.rows() - 2;
    size_t max_row = slice.cols() - 2;

    for (size_t x = 1; x <= max_col; x++) // for each row
    {
        for (size_t y = 1; y <= max_row; y++) // for each column
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

void init_slice(Matrix<int>& slice, size_t offset_row, size_t offset_col)
{
    constexpr size_t MOD = 91;

    size_t max_row = slice.rows() - 2;
    size_t max_col = slice.cols() - 2;

    for (size_t row = 1; row <= max_row; ++row) {
        for (size_t col = 1; col <= max_col; ++col) {
            size_t real_row = offset_row + row;
            size_t real_col = offset_col + col;
            auto mr = real_row % MOD;
            auto mc = real_col % MOD;
            auto r = ((mr + 1) * (mr + mc)) % MOD;
            slice(row, col) = static_cast<int>(r < 20);
        }
    }
}

void check_condition(bool cond, const char* const msg)
{
    if (!cond) {
        cerr << "Error: " << msg << "\n";
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    int size, rank;
    MPI_Init(&argc, &argv);

    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " procs-row procs-col plane-dimension timesteps\n";
        return 1;
    }
    int64_t plane_dimension = atoll(argv[3]);
    int procs_row = static_cast<int>(atoll(argv[1]));
    int procs_col = static_cast<int>(atoll(argv[2]));
    int64_t timesteps = atoll(argv[4]);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        check_condition(procs_col * procs_row == size, "the process grid size doesn't match the number of processes!");
        check_condition(plane_dimension % procs_col == 0, "the number of processes in a column should divide the plane dimension!");
        check_condition(plane_dimension % procs_row == 0, "the number of processes in a row should divide the plane dimension!");
    }

    size_t subplane_rowsize = static_cast<size_t>(plane_dimension / procs_row);
    size_t subplane_colsize = static_cast<size_t>(plane_dimension / procs_col);
    // the number of rows is the size of each column, the same for the number of rows
    Matrix<int> slice(subplane_colsize + 2, subplane_rowsize + 2);

    auto my_row = static_cast<size_t>(rank / procs_row);
    auto my_col = static_cast<size_t>(rank % procs_row);

    //cout << rank << ":" << my_row << " " << my_col << " " << subplane_rowsize << " " << subplane_colsize << endl;

    init_slice(slice, my_row * subplane_colsize, my_col * subplane_rowsize);

    int proc_up = (rank < procs_row) ? MPI_PROC_NULL : rank - procs_row;
    int proc_down = (rank + procs_row >= size) ? MPI_PROC_NULL : rank + procs_row;
    int proc_left = (rank % procs_row == 0) ? MPI_PROC_NULL : rank - 1;
    int proc_right = ((rank + 1) % procs_row == 0) ? MPI_PROC_NULL : rank + 1;

    size_t max_col = slice.cols() - 2;
    size_t max_row = slice.rows() - 2;

    vector<MPI_Request> vec;
    auto start = std::chrono::high_resolution_clock::now();
    if (rank == 0) {
        {
            std::string s = date::format("[%T] ", start);
            cout << s << "Starting computation" << endl;
        }
    }

    for (int64_t step = 0; step < timesteps; ++step) {
        vec.clear();
        if (rank == 0) {
            std::string s = date::format("[%T] ", std::chrono::system_clock::now());
            cout << s << "Step: t = " << step << endl;
        }
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
        vec.clear();

        // Now we need to synchronize the corners
        MPI_Isend(&slice(1, 0), 1, MPI_INT, proc_up, 0, MPI_COMM_WORLD, &req);
        vec.push_back(req);
        MPI_Isend(&slice(1, max_col + 1), 1, MPI_INT, proc_up, 0, MPI_COMM_WORLD, &req);
        vec.push_back(req);
        MPI_Isend(&slice(max_row, 0), 1, MPI_INT, proc_down, 0, MPI_COMM_WORLD, &req);
        vec.push_back(req);
        MPI_Isend(&slice(max_row, max_col + 1), 1, MPI_INT, proc_down, 0, MPI_COMM_WORLD, &req);
        vec.push_back(req);

        MPI_Irecv(&slice(0, 0), 1, MPI_INT, proc_up, 0, MPI_COMM_WORLD, &req);
        vec.push_back(req);
        MPI_Irecv(&slice(0, max_col + 1), 1, MPI_INT, proc_up, 0, MPI_COMM_WORLD, &req);
        vec.push_back(req);
        MPI_Irecv(&slice(max_row + 1, 0), 1, MPI_INT, proc_down, 0, MPI_COMM_WORLD, &req);
        vec.push_back(req);
        MPI_Irecv(&slice(max_row + 1, max_col + 1), 1, MPI_INT, proc_down, 0, MPI_COMM_WORLD, &req);
        vec.push_back(req);

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
        filesystem::create_directory("output");
        ofstream outfile;
        outfile.open("output/game-life-output.txt");
        outfile << count_total << "\n";
    }

    if (rank == 0) {
        auto end = std::chrono::high_resolution_clock::now();
        {
            std::string s = date::format("[%T] ", end);
            auto elapsed = chrono::duration_cast<chrono::seconds>(end - start);
            cout << s << "Finished computation. Elapsed time: " << elapsed.count() << "s" << endl;
        }
    }
    MPI_Finalize();
    return 0;
}
