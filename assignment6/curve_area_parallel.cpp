#include <mpi.h>
#include <stdlib.h>

#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "core/utils.h"

#define sqr(x) ((x) * (x))
#define DEFAULT_NUMBER_OF_POINTS "1000000000"
#define DEFAULT_A "2"
#define DEFAULT_B "1"
#define DEFAULT_RANDOM_SEED "1"

uint c_const = (uint)RAND_MAX + (uint)1;
inline double get_random_coordinate(uint *random_seed) {
  return ((double)rand_r(random_seed)) /
         c_const;  // thread-safe random number generator
}

unsigned long get_points_in_curve(unsigned long n, uint random_seed, float a,
                                  float b) {
  unsigned long curve_count = 0;
  double x_coord, y_coord;
  for (unsigned long i = 0; i < n; i++) {
    x_coord = ((2.0 * get_random_coordinate(&random_seed)) - 1.0);
    y_coord = ((2.0 * get_random_coordinate(&random_seed)) - 1.0);
    if ((a * sqr(x_coord) + b * sqr(sqr(y_coord))) <= 1.0) curve_count++;
  }
  return curve_count;
}

void curve_area_calculation_parallel(unsigned long n, float a, float b,
                                     uint r_seed, int world_rank,
                                     int world_size) {
  timer local_timer;
  timer global_timer;
  double local_time_taken = 0.0;
  uint random_seed = r_seed;

  // Dividing up n vertices on P processes.
  // Total number of processes is world_size. This process rank is world_rank

  int min_points_per_process = n / world_size;
  int excess_points = n % world_size;
  int points_to_be_generated = 0;
  if (world_rank < excess_points) {
    points_to_be_generated = min_points_per_process + 1;

  } else {
    points_to_be_generated = min_points_per_process;
  }
  // Each process will work on points_to_be_generated and estimate curve_points.

  if (world_rank == 0) global_timer.start();
  local_timer.start();

  unsigned long local_curve_points =
      get_points_in_curve(points_to_be_generated, r_seed + world_rank, a, b);

  //*------------------------------------------------------------------------
  local_time_taken = local_timer.stop();

  // --- synchronization phase start ---
  std::vector<unsigned long> p_local_curve_points(world_size, 0);
  std::vector<double> p_local_time_taken(world_size, 0.0);

  if (world_rank == 0) {
    p_local_curve_points[0] = local_curve_points;
    p_local_time_taken[0] = local_time_taken;

    // get process' curve count
    unsigned long p_curve_count = 0;
    for (int i = 1; i < world_size; i++) {
      MPI_Recv(&p_curve_count, 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      p_local_curve_points[i] = p_curve_count;
    }

    // get process' local times
    double time_taken = 0.0;
    for (int i = 1; i < world_size; i++) {
      MPI_Recv(&time_taken, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      p_local_time_taken[i] = time_taken;
    }
  } else {
    //  send curve points data to root
    MPI_Send(&local_curve_points, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD);
    // send local time taken to root
    MPI_Send(&local_time_taken, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  }
  // --- synchronization phase end -----
  if (world_rank == 0) {
    double global_time_taken = global_timer.stop();
    std::vector<unsigned long> n_points_process(world_size, 0);
    unsigned long global_curve_points = 0;

    // determine the number of points that was assigned to each process and sum
    // up local curve points
    for (int i = 0; i < world_size; i++) {
      if (i < excess_points) {
        n_points_process[i] = min_points_per_process + 1;

      } else {
        n_points_process[i] = min_points_per_process;
      }

      global_curve_points += p_local_curve_points[i];
    }

    std::cout << "rank, points_generated, curve_points, time_taken\n";
    for (int i = 0; i < world_size; i++) {
      std::cout << i << ", " << n_points_process[i] << ", "
                << p_local_curve_points[i] << ", "
                << std::setprecision(TIME_PRECISION) << p_local_time_taken[i]
                << "\n";
    }

    double final_area_value = 4.0 * (double)global_curve_points / (double)n;

    std::cout << "Total points generated : " << n << "\n";
    std::cout << "Total points in curve : " << global_curve_points << "\n";
    std::cout << "Area : " << std::setprecision(VAL_PRECISION)
              << final_area_value << "\n";
    std::cout << "Time taken (in seconds) : "
              << std::setprecision(TIME_PRECISION) << global_time_taken << "\n";
  } else {
    // TODO: print process statistics individually for each thread
  }
}

int main(int argc, char *argv[]) {
  // Initialize command line arguments
  cxxopts::Options options("Curve_area_calculation",
                           "Calculate area inside curve a x^2 + b y ^4 = 1 "
                           "using serial and parallel execution");
  options.add_options(
      "custom", {{"nPoints", "Number of points",
                  cxxopts::value<unsigned long>()->default_value(
                      DEFAULT_NUMBER_OF_POINTS)},
                 {"coeffA", "Coefficient a",
                  cxxopts::value<float>()->default_value(DEFAULT_A)},
                 {"coeffB", "Coefficient b",
                  cxxopts::value<float>()->default_value(DEFAULT_B)},
                 {"rSeed", "Random Seed",
                  cxxopts::value<uint>()->default_value(DEFAULT_RANDOM_SEED)}});
  auto cl_options = options.parse(argc, argv);
  unsigned long n_points = cl_options["nPoints"].as<unsigned long>();
  float a = cl_options["coeffA"].as<float>();
  float b = cl_options["coeffB"].as<float>();
  uint r_seed = cl_options["rSeed"].as<uint>();

  MPI_Init(NULL, NULL);
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Check edge cases on inputs
  if (n_points <= 0) {
    throw std::invalid_argument(
        "The commandline arguments: --nPoints must be at least 1\n");
  }
  if (a < 1 || b < 1) {
    throw std::invalid_argument(
        "The commandline arguments: --coeffA and --coeffB must be at least "
        "1.\n");
  }

  if (world_rank == 0) {
    std::cout << "Number of points : " << n_points << "\n";
    std::cout << "A : " << a << "\n"
              << "B : " << b << "\n";
    std::cout << "Random Seed : " << r_seed << "\n";
  }

  curve_area_calculation_parallel(n_points, a, b, r_seed, world_rank, world_size);

  MPI_Finalize();
  return 0;
}
