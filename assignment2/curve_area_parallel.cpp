#include <stdlib.h>

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "core/utils.h"

#define sqr(x) ((x) * (x))
#define DEFAULT_NUMBER_OF_POINTS "1000000000"
#define DEFAULT_A "2"
#define DEFAULT_B "1"
#define DEFAULT_RANDOM_SEED "1"

uint c_const = (uint)RAND_MAX + (uint)1;
inline double get_random_coordinate(uint *random_seed) {
  // thread-safe random number generator
  return ((double)rand_r(random_seed)) / c_const;
}

void get_points_in_curve(unsigned long n, uint random_seed, float a, float b,
                         unsigned long *curve_count, double *time_taken,
                         std::atomic<unsigned long> *total_count) {
  timer t;
  double x_coord, y_coord;
  unsigned long local_count = 0;
  t.start();

  for (unsigned long i = 0; i < n; i++) {
    x_coord = ((2.0 * get_random_coordinate(&random_seed)) - 1.0);
    y_coord = ((2.0 * get_random_coordinate(&random_seed)) - 1.0);
    if ((a * sqr(x_coord) + b * sqr(sqr(y_coord))) <= 1.0) {
      local_count++;
    }
  }
  *curve_count = local_count;
  *time_taken = t.stop();
  total_count->fetch_add(*curve_count);
}

void curve_area_calculation_parallel(unsigned long n, float a, float b,
                                     uint r_seed, uint T) {
  std::vector<std::thread> threads(T);
  std::vector<unsigned long> local_curve_points(T, 0);
  std::atomic<unsigned long> total_curve_points(0);
  unsigned long n_points = n / T;
  unsigned long remainder = n % T;
  std::vector<unsigned long> n_points_threads(T, 0);
  uint random_seed = r_seed;
  timer main_timer;
  double time_taken = 0.0;
  std::vector<double> local_time_taken(T, 0.0);

  // determine how many points each thread will use
  for (uint i = 0; i < T; i++) {
    if (remainder > 0) {
      n_points_threads[i] = n_points + 1;
      remainder--;
    } else {
      n_points_threads[i] = n_points;
    }
  }

  main_timer.start();

  // Create T threads
  for (uint i = 0; i < T; i++) {
    threads.push_back(std::thread(get_points_in_curve, n_points_threads[i],
                                  random_seed + i, a, b, &local_curve_points[i],
                                  &local_time_taken[i], &total_curve_points));
  }

  // Join threads
  for (std::thread &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  double area_value = 4.0 * (double)total_curve_points / (double)n;

  //*------------------------------------------------------------------------
  time_taken = main_timer.stop();

  std::cout << "thread_id, points_generated, curve_points, time_taken\n";
  for (uint i = 0; i < T; i++) {
    std::cout << i << ", " << n_points_threads[i] << ", "
              << local_curve_points[i] << ", "
              << std::setprecision(TIME_PRECISION) << local_time_taken[i]
              << "\n";
  }
  std::cout << "Total points generated : " << n << "\n";
  std::cout << "Total points in curve : " << total_curve_points << "\n";
  std::cout << "Area : " << std::setprecision(VAL_PRECISION) << area_value
            << "\n";
  std::cout << "Time taken (in seconds) : " << std::setprecision(TIME_PRECISION)
            << time_taken << "\n";
}

int main(int argc, char *argv[]) {
  // Initialize command line arguments
  cxxopts::Options options("Curve_area_calculation",
                           "Calculate area inside curve a x^2 + b y ^4 = 1 "
                           "using serial and parallel execution");
  options.add_options(
      "custom",
      {{"nPoints", "Number of points",
        cxxopts::value<unsigned long>()->default_value(
            DEFAULT_NUMBER_OF_POINTS)},
       {"coeffA", "Coefficient a",
        cxxopts::value<float>()->default_value(DEFAULT_A)},
       {"coeffB", "Coefficient b",
        cxxopts::value<float>()->default_value(DEFAULT_B)},
       {"rSeed", "Random Seed",
        cxxopts::value<uint>()->default_value(DEFAULT_RANDOM_SEED)},
       {"nThreads", "Number of Threads",
        cxxopts::value<uint>()->default_value(DEFAULT_NUMBER_OF_WORKERS)}});
  auto cl_options = options.parse(argc, argv);
  unsigned long n_points = cl_options["nPoints"].as<unsigned long>();
  float a = cl_options["coeffA"].as<float>();
  float b = cl_options["coeffB"].as<float>();
  uint r_seed = cl_options["rSeed"].as<uint>();
  uint n_threads = cl_options["nThreads"].as<uint>();

  // Check edge cases on inputs
  if (n_points <= 0 || n_threads <= 0) {
    throw std::invalid_argument(
        "The commandline arguments: --nPoints and --nThreads "
        "must be at least 1\n");
  }
  if (a < 1 || b < 1) {
    throw std::invalid_argument(
        "The commandline arguments: --coeffA and --coeffB must be at least "
        "1.\n");
  }

  std::cout << "Number of points : " << n_points << "\n";
  std::cout << "Number of threads : " << n_threads << "\n";
  std::cout << "A : " << a << "\n"
            << "B : " << b << "\n";
  std::cout << "Random Seed : " << r_seed << "\n";

  curve_area_calculation_parallel(n_points, a, b, r_seed, n_threads);
  return 0;
}
