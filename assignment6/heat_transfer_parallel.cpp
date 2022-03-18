#include <mpi.h>
#include <stdlib.h>

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "core/utils.h"

#define DEFAULT_GRID_SIZE "1000"
#define DEFAULT_CX "1"
#define DEFAULT_CY "1"
#define DEFAULT_TIME_STEPS "1000"
#define DEFAULT_MIDDLE_TEMP "600"

class TemperatureArray {
 private:
  uint size;
  uint step;
  double Cx;
  double Cy;
  double *CurrArray;
  double *PrevArray;
  void assign(double *A, uint x, uint y, double newvalue) {
    A[x * size + y] = newvalue;
  };
  double read(double *A, uint x, uint y) { return A[x * size + y]; };

 public:
  TemperatureArray(uint input_size, double iCx, double iCy,
                   double init_temp) {  // create array of dimension sizexsize
    size = input_size;
    Cx = iCx;
    Cy = iCy;
    step = 0;
    CurrArray = (double *)malloc(size * size * sizeof(double));
    PrevArray = (double *)malloc(size * size * sizeof(double));
    for (uint i = 0; i < size; i++)
      for (uint j = 0; j < size; j++) {
        if ((i > size / 3) && (i < 2 * size / 3) && (j > size / 3) &&
            (j < 2 * size / 3)) {
          assign(PrevArray, i, j, init_temp);
          assign(CurrArray, i, j, init_temp);
        } else {
          assign(PrevArray, i, j, 0);
          assign(CurrArray, i, j, 0);
        }
      }
  };

  ~TemperatureArray() {
    free(PrevArray);
    free(CurrArray);
  };

  void IncrementStepCount() { step++; };

  uint ReadStepCount() { return (step); };

  void ComputeNewTemp(uint x, uint y) {
    if ((x > 0) && (x < size - 1) && (y > 0) && (y < size - 1))
      assign(CurrArray, x, y,
             read(PrevArray, x, y) +
                 Cx * (read(PrevArray, x - 1, y) + read(PrevArray, x + 1, y) -
                       2 * read(PrevArray, x, y)) +
                 Cy * (read(PrevArray, x, y - 1) + read(PrevArray, x, y + 1) -
                       2 * read(PrevArray, x, y)));
  };

  void SwapArrays() {
    double *temp = PrevArray;
    PrevArray = CurrArray;
    CurrArray = temp;
  };

  double temp(uint x, uint y) { return read(CurrArray, x, y); };

  void write(uint x, uint y, double newvalue) {
    assign(CurrArray, x, y, newvalue);
  };
};

inline void heat_transfer_calculation(uint size, uint start, uint end,
                                      double *time_taken, TemperatureArray *T,
                                      uint steps, int world_rank,
                                      int world_size) {
  timer t1;
  t1.start();
  uint stepcount;

  // Create datatype for 1 column of temp array
  // MPI_Datatype col;
  // MPI_Type_vector(size, 1, size, MPI_DOUBLE, &col);
  // MPI_Type_commit(&col);

  for (stepcount = 1; stepcount <= steps; stepcount++) {
    // Compute the Temperature Array values Curr[][] in the slice allocated to
    // this process from Prev[][]
    for (uint x = start; x <= end; x++) {
      for (uint y = 0; y < size; y++) {
        T->ComputeNewTemp(x, y);
      }
    }
    // --- synchronization: Send and Receive boundary columns from neighbors
    // Even processes communicate with right proces first
    // Odd  processes communicate with left process first
    double new_temp = 0.0;
    double send_temp = 0.0;
    if (world_rank % 2 == 0) {            // even rank
      if (world_rank < world_size - 1) {  // not last process
        // Send my column "end" to the right process world_rank+1
        for (int i = 0; i < size; i++) {
          send_temp = T->temp(end, i);
          MPI_Send(&send_temp, 1, MPI_DOUBLE, world_rank + 1, world_rank,
                   MPI_COMM_WORLD);
        }

        // Receive column "end+1" from the right process world_rank+1, populate
        // local Curr Array
        for (int i = 0; i < size; i++) {
          MPI_Recv(&new_temp, 1, MPI_DOUBLE, world_rank + 1, world_rank + 1,
                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          T->write(end + 1, i, new_temp);
        }
      }
      if (world_rank > 0) {  // not first process
        // Receive column "start-1" from the left process world_rank-1, populate
        // local Curr Array
        for (int i = 0; i < size; i++) {
          MPI_Recv(&new_temp, 1, MPI_DOUBLE, world_rank - 1, world_rank - 1,
                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          T->write(start - 1, i, new_temp);
        }

        // Send my column "start" to the left process world_rank-1
        for (int i = 0; i < size; i++) {
          send_temp = T->temp(start, i);
          MPI_Send(&send_temp, 1, MPI_DOUBLE, world_rank - 1, world_rank,
                   MPI_COMM_WORLD);
        }
      }
    }                        // even rank
    else {                   // odd rank
      if (world_rank > 0) {  // not first process
        // Receive column "start-1" from the left process world_rank-1, populate
        // local Curr Array
        for (int i = 0; i < size; i++) {
          MPI_Recv(&new_temp, 1, MPI_DOUBLE, world_rank - 1, world_rank - 1,
                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          T->write(start - 1, i, new_temp);
        }

        // Send my column "start" to the left process world_rank-1
        for (int i = 0; i < size; i++) {
          send_temp = T->temp(start, i);
          MPI_Send(&send_temp, 1, MPI_DOUBLE, world_rank - 1, world_rank,
                   MPI_COMM_WORLD);
        }
      }
      if (world_rank < world_size - 1) {  // not last process
        // Send my column "end" to the right process world_rank+1
        for (int i = 0; i < size; i++) {
          send_temp = T->temp(end, i);
          MPI_Send(&send_temp, 1, MPI_DOUBLE, world_rank + 1, world_rank,
                   MPI_COMM_WORLD);
        }

        // Receive column "end+1" from the right process world_rank+1, populate
        // local Curr Array
        for (int i = 0; i < size; i++) {
          MPI_Recv(&new_temp, 1, MPI_DOUBLE, world_rank + 1, world_rank + 1,
                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          T->write(end + 1, i, new_temp);
        }
      }
    }  // odd rank
    // --- synchronization end -----
    T->SwapArrays();
    T->IncrementStepCount();
  }  // end of current step

  *time_taken = t1.stop();
}

void heat_transfer_calculation_parallel(uint size, TemperatureArray *T,
                                        uint steps, int world_rank,
                                        int world_size) {
  timer global_timer;
  double local_time_taken = 0.0;
  double global_time_taken = 0.0;
  uint startx = 0;
  uint endx = size - 1;

  int min_columns = size / world_size;
  int excess_columns = size % world_size;
  if (world_rank < excess_columns) {
    startx = world_rank * (min_columns + 1);
    endx = startx + min_columns;
  } else {
    startx = (excess_columns * (min_columns + 1)) +
             ((world_rank - excess_columns) * min_columns);
    endx = startx + min_columns - 1;
  }

  if (world_rank == 0) global_timer.start();
  //*------------------------------------------------------------------------

  heat_transfer_calculation(size, startx, endx, &local_time_taken, T, steps,
                            world_rank, world_size);

  // --- synchronization: Take turns printing output starting with root and
  // ending with root process
  int turn;
  if (world_rank != 0) {
    MPI_Recv(&turn, 1, MPI_INT, world_rank - 1, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
  } else {
    turn = 1;
  }

  // Print these statistics for each process
  printf("%d, %d, %d, %.*g\n", world_rank, startx, endx, TIME_PRECISION,
         local_time_taken);

  // print the temperature of positions that the process covered
  uint step = size / 6;
  uint position = 0;
  for (uint x = 0; x < 6; x++) {
    if (position >= startx && position <= endx) {
      // std::cout << "Temp[" << position << "," << position  << "]=" <<
      // T->temp(position, position) << "\n";
      printf("Temp[%d,%d]=%.*g\n", position, position, 5,
             T->temp(position, position));
    }
    position += step;
  }
  // Print temparature at select boundary points;
  printf("Temp[%d,%d]=%.*g\n", endx, endx, 5, T->temp(endx, endx));

  MPI_Send(&turn, 1, MPI_INT, (world_rank + 1) % world_size, 0, MPI_COMM_WORLD);

  // Now root process can receive from the last process. This makes sure that at
  // least one MPI_Send is initialized before all MPI_Recvs (again, to prevent
  // deadlock)
  if (world_rank == 0) {
    MPI_Recv(&turn, 1, MPI_INT, world_size - 1, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    global_time_taken = global_timer.stop();
    printf("Time taken (in seconds) : %.*g\n", TIME_PRECISION,
           global_time_taken);
  }
  // --- synchronization end -----
  //*------------------------------------------------------------------------
}

int main(int argc, char *argv[]) {
  // Initialize command line arguments
  cxxopts::Options options(
      "Heat_transfer_calculation",
      "Model heat transfer in a grid using serial and parallel execution");
  options.add_options(
      "custom", {{"gSize", "Grid Size",
                  cxxopts::value<uint>()->default_value(DEFAULT_GRID_SIZE)},
                 {"mTemp", "Temperature in middle of array",
                  cxxopts::value<double>()->default_value(DEFAULT_MIDDLE_TEMP)},
                 {"iCX", "Coefficient of horizontal heat transfer",
                  cxxopts::value<double>()->default_value(DEFAULT_CX)},
                 {"iCY", "Coefficient of vertical heat transfer",
                  cxxopts::value<double>()->default_value(DEFAULT_CY)},
                 {"tSteps", "Time Steps",
                  cxxopts::value<uint>()->default_value(DEFAULT_TIME_STEPS)}});
  auto cl_options = options.parse(argc, argv);
  uint grid_size = cl_options["gSize"].as<uint>();
  double init_temp = cl_options["mTemp"].as<double>();
  double Cx = cl_options["iCX"].as<double>();
  double Cy = cl_options["iCY"].as<double>();
  uint steps = cl_options["tSteps"].as<uint>();

  MPI_Init(NULL, NULL);
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // check for valid input
  if (grid_size <= 0 || steps <= 0) {
    throw std::invalid_argument(
        "The commandline arguments: --gSize and --tSteps must be "
        "at least 1.\n");
  }

  if (world_rank == 0) {
    std::cout << "Number of processes : " << world_size << "\n";
    std::cout << "Grid Size : " << grid_size << "x" << grid_size << "\n";
    std::cout << "Cx : " << Cx << "\n"
              << "Cy : " << Cy << "\n";
    std::cout << "Temperature in the middle of grid : " << init_temp << "\n";
    std::cout << "Time Steps : " << steps << "\n";

    std::cout << "Initializing Temperature Array..."
              << "\n";
    std::cout << "rank, start_column, end_column, time_taken\n";
  }

  TemperatureArray *T = new TemperatureArray(grid_size, Cx, Cy, init_temp);
  if (!T) {
    std::cout << "Cannot Initialize Temperature Array...Terminating"
              << "\n";
    return 2;
  }
  heat_transfer_calculation_parallel(grid_size, T, steps, world_rank,
                                     world_size);

  MPI_Finalize();
  delete T;
  return 0;
}
