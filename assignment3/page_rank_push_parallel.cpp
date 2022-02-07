#include <stdlib.h>

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "core/graph.h"
#include "core/utils.h"

#ifdef USE_INT
#define INIT_PAGE_RANK 100000
#define EPSILON 1000
#define PAGE_RANK(x) (15000 + (5 * x) / 6)
#define CHANGE_IN_PAGE_RANK(x, y) std::abs(x - y)
typedef int64_t PageRankType;
#else
#define INIT_PAGE_RANK 1.0
#define EPSILON 0.01
#define DAMPING 0.85
#define PAGE_RANK(x) (1 - DAMPING + DAMPING * x)
#define CHANGE_IN_PAGE_RANK(x, y) std::fabs(x - y)
typedef double PageRankType;
#endif

void getPageRank(Graph &g, uint tid, int max_iters, uintV start, uintV end,
                 PageRankType *pr_curr_global, PageRankType *pr_next_global,
                 double *time_taken, CustomBarrier *barrier,
                 std::vector<std::mutex> &pr_next_mutex) {
  timer t;

  t.start();
  for (int iter = 0; iter < max_iters; iter++) {
    // for each vertex 'u', process all its outNeighbors 'v'
    for (uintV u = start; u <= end; u++) {
      uintE out_degree = g.vertices_[u].getOutDegree();
      PageRankType quotient = (pr_curr_global[u] / (PageRankType)out_degree);
      for (uintE i = 0; i < out_degree; i++) {
        uintV v = g.vertices_[u].getOutNeighbor(i);
        pr_next_mutex[v].lock();
        pr_next_global[v] += quotient;
        pr_next_mutex[v].unlock();
      }
    }
    barrier->wait();
    for (uintV v = start; v <= end; v++) {
      // reset pr_curr for the next iteration
      pr_curr_global[v] = PAGE_RANK(pr_next_global[v]);
      pr_next_global[v] = 0.0;
    }
    barrier->wait();
  }
  *time_taken = t.stop();
}

void pageRankParallel(Graph &g, int max_iters, uint n_threads) {
  uintV n = g.n_;

  PageRankType *pr_curr = new PageRankType[n];
  PageRankType *pr_next = new PageRankType[n];

  for (uintV i = 0; i < n; i++) {
    pr_curr[i] = INIT_PAGE_RANK;
    pr_next[i] = 0.0;
  }

  std::vector<std::thread> threads(n_threads);
  std::vector<uintV> start_vertex(n_threads, 0);
  std::vector<uintV> end_vertex(n_threads, 0);
  uintV min_vertices_for_each_thread = n / n_threads;
  uintV excess_vertices = n % n_threads;
  uintV cur_Vertex = 0;

  // determine number of verticies for each thread
  for (uint i = 0; i < n_threads; i++) {
    start_vertex[i] = cur_Vertex;
    if (excess_vertices > 0) {
      end_vertex[i] = cur_Vertex + min_vertices_for_each_thread;
      excess_vertices--;
    } else {
      end_vertex[i] = cur_Vertex + min_vertices_for_each_thread - 1;
    }
    cur_Vertex = end_vertex[i] + 1;
  }

  std::vector<double> local_time_taken(n_threads, 0.0);
  CustomBarrier barrier(n_threads);
  std::vector<std::mutex> pr_next_mutex(n);

  // Push based pagerank
  timer t1;
  double time_taken = 0.0;
  // Create threads and distribute the work across T threads
  // -------------------------------------------------------------------
  t1.start();
  for (uint i = 0; i < n_threads; i++) {
    threads.push_back(std::thread(getPageRank, std::ref(g), i, max_iters,
                                  start_vertex[i], end_vertex[i], pr_curr,
                                  pr_next, &local_time_taken[i], &barrier,
                                  std::ref(pr_next_mutex)));
  }

  for (std::thread &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
  time_taken = t1.stop();
  // -------------------------------------------------------------------
  std::cout << "thread_id, time_taken" << std::endl;
  // std::cout << "0, " << time_taken << std::endl;
  // Print the above statistics for each thread
  // Example output for 2 threads:
  // thread_id, time_taken
  // 0, 0.12
  // 1, 0.12
  for (uint i = 0; i < n_threads; i++) {
    std::cout << i << ", " << local_time_taken[i] << std::endl;
  }

  PageRankType sum_of_page_ranks = 0;
  for (uintV u = 0; u < n; u++) {
    sum_of_page_ranks += pr_curr[u];
  }
  std::cout << "Sum of page ranks : " << sum_of_page_ranks << "\n";
  std::cout << "Time taken (in seconds) : " << time_taken << "\n";
  delete[] pr_curr;
  delete[] pr_next;
}

int main(int argc, char *argv[]) {
  cxxopts::Options options(
      "page_rank_push",
      "Calculate page_rank using serial and parallel execution");
  options.add_options(
      "",
      {
          {"nThreads", "Number of Threads",
           cxxopts::value<uint>()->default_value(DEFAULT_NUMBER_OF_THREADS)},
          {"nIterations", "Maximum number of iterations",
           cxxopts::value<uint>()->default_value(DEFAULT_MAX_ITER)},
          {"inputFile", "Input graph file path",
           cxxopts::value<std::string>()->default_value(
               "/scratch/input_graphs/roadNet-CA")},
      });

  auto cl_options = options.parse(argc, argv);
  uint n_threads = cl_options["nThreads"].as<uint>();
  uint max_iterations = cl_options["nIterations"].as<uint>();
  std::string input_file_path = cl_options["inputFile"].as<std::string>();

  // Check edge cases on inputs
  if (n_threads <= 0 || max_iterations <= 0) {
    throw std::invalid_argument(
        "The commandline arguments: --n_threads and --max_iterations "
        "must be at least 1\n");
  }

#ifdef USE_INT
  std::cout << "Using INT" << std::endl;
#else
  std::cout << "Using DOUBLE" << std::endl;
#endif
  std::cout << std::fixed;
  std::cout << "Number of Threads : " << n_threads << std::endl;
  std::cout << "Number of Iterations: " << max_iterations << std::endl;

  Graph g;
  std::cout << "Reading graph\n";
  g.readGraphFromBinary<int>(input_file_path);
  std::cout << "Created graph\n";
  pageRankParallel(g, max_iterations, n_threads);

  return 0;
}
