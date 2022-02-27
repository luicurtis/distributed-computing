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

class DynamicMapping {
 public:
  uint k;
  uintV n;
  uint num_of_threads;
  std::atomic<uint> threads_done;
  std::atomic<uintV> next_vertex;

  DynamicMapping()
      : k(1), n(0), num_of_threads(1), threads_done(0), next_vertex(0) {}
  DynamicMapping(uint k, uintV n, uint n_threads) {
    this->k = k;
    this->n = n;
    num_of_threads = n_threads;
    threads_done = 0;
    next_vertex = 0;
  }
  uintV getNextVertexToBeProcessed() {
    uintV cur_next = next_vertex.fetch_add(k);
    if (cur_next >= n) {
      uint cur_threads = threads_done.fetch_add(1);
      if (cur_threads + 1 == num_of_threads) {
        threads_done = 0;
        next_vertex = 0;
      }
      return -1;
    } else {
      return cur_next;
    }
  }
};

void getPageRankStatic(Graph &g, uint tid, int max_iters,
                       std::vector<uintV> assigned_vertex,
                       std::vector<std::atomic<PageRankType>> &pr_curr_global,
                       std::vector<std::atomic<PageRankType>> &pr_next_global,
                       double *total_time_taken, double *barrier1_time,
                       double *barrier2_time, CustomBarrier *barrier) {
  timer t;
  timer b1;
  timer b2;
  double b1_time = 0.0;
  double b2_time = 0.0;
  int n = assigned_vertex.size();

  t.start();
  for (int iter = 0; iter < max_iters; iter++) {
    // for each vertex 'u', process all its outNeighbors 'v'
    for (int i = 0; i < n; i++) {
      uintV u = assigned_vertex[i];
      uintE out_degree = g.vertices_[u].getOutDegree();
      PageRankType quotient = (pr_curr_global[u] / (PageRankType)out_degree);
      for (uintE i = 0; i < out_degree; i++) {
        uintV v = g.vertices_[u].getOutNeighbor(i);
        PageRankType cur_val = pr_next_global[v];
        bool cas_res = false;
        while (cas_res == false) {
          cur_val = pr_next_global[v];
          cas_res = pr_next_global[v].compare_exchange_weak(cur_val,
                                                            cur_val + quotient);
        }
      }
    }

    b1.start();
    barrier->wait();
    b1_time += b1.stop();

    for (int i = 0; i < n; i++) {
      uintV v = assigned_vertex[i];
      // reset pr_curr for the next iteration
      pr_curr_global[v] = PAGE_RANK(pr_next_global[v]);
      pr_next_global[v] = 0.0;
    }
    b2.start();
    barrier->wait();
    b2_time += b2.stop();
  }
  *total_time_taken = t.stop();
  *barrier1_time = b1_time;
  *barrier2_time = b2_time;
}

void getPageRankDynamic(Graph &g, uint tid, int max_iters, uint k,
                        uintV *vertices_processed, uintE *edges_processed,
                        std::vector<std::atomic<PageRankType>> &pr_curr_global,
                        std::vector<std::atomic<PageRankType>> &pr_next_global,
                        double *total_time_taken, double *barrier1_time,
                        double *barrier2_time, double *getNextVertex_time,
                        CustomBarrier *barrier, DynamicMapping *dm) {
  timer t;
  timer b1;
  timer b2;
  timer get_vertex;
  double b1_time = 0.0;
  double b2_time = 0.0;
  double get_vertex_time = 0.0;
  uintV v_processed = 0;
  uintE e_processed = 0;
  uintV n = g.n_;

  t.start();
  for (int iter = 0; iter < max_iters; iter++) {
    std::vector<uintV> iter_vertices;
    while (true) {
      get_vertex.start();
      uintV u = dm->getNextVertexToBeProcessed();
      get_vertex_time += get_vertex.stop();
      if (u == -1) break;
      iter_vertices.push_back(u);
      for (uint j = 0; j < k; j++) {
        uintE out_degree = g.vertices_[u].getOutDegree();
        e_processed += out_degree;
        for (uintE i = 0; i < out_degree; i++) {
          uintV v = g.vertices_[u].getOutNeighbor(i);
          PageRankType cur_val = pr_next_global[v];
          PageRankType quotient =
              (pr_curr_global[u] / (PageRankType)out_degree);
          bool cas_res = false;
          while (cas_res == false) {
            cur_val = pr_next_global[v];
            cas_res = pr_next_global[v].compare_exchange_weak(
                cur_val, cur_val + quotient);
          }
        }
        u++;
        if (u >= n) break;
      }
    }

    b1.start();
    barrier->wait();
    b1_time += b1.stop();

    for (uintV start : iter_vertices) {
      uintV v = start;
      for (uint i = 0; i < k; i++) {
        v_processed++;
        // reset pr_curr for the next iteration
        pr_curr_global[v] = PAGE_RANK(pr_next_global[v]);
        pr_next_global[v] = 0.0;
        v++;
        if (v >= n) break;
      }
    }

    b2.start();
    barrier->wait();
    b2_time += b2.stop();
  }

  *total_time_taken = t.stop();
  *barrier1_time = b1_time;
  *barrier2_time = b2_time;
  *vertices_processed = v_processed;
  *edges_processed = e_processed;
  *getNextVertex_time = get_vertex_time;
}

void printStats(uintV n, uint n_threads,
                std::vector<std::atomic<PageRankType>> &pr_curr,
                std::vector<uintV> vertices_processed,
                std::vector<uintE> assigned_edges,
                std::vector<double> barrier1_time,
                std::vector<double> barrier2_time,
                std::vector<double> getNextVertex_time,
                std::vector<double> local_time_taken, double time_taken) {
  std::cout << "thread_id, num_vertices, num_edges, barrier1_time, "
               "barrier2_time, getNextVertex_time, total_time"
            << std::endl;

  for (uint i = 0; i < n_threads; i++) {
    std::cout << i << ", " << vertices_processed[i] << ", " << assigned_edges[i]
              << ", " << barrier1_time[i] << ", " << barrier2_time[i] << ", "
              << getNextVertex_time[i] << ", " << local_time_taken[i]
              << std::endl;
  }

  PageRankType sum_of_page_ranks = 0;
  for (uintV u = 0; u < n; u++) {
    sum_of_page_ranks += pr_curr[u];
  }
  std::cout << "Sum of page ranks : " << sum_of_page_ranks << "\n";
  std::cout << "Time taken (in seconds) : " << time_taken << "\n";
}

void strategy1(Graph &g, int max_iters, uint n_threads) {
  uintV n = g.n_;
  std::vector<std::atomic<PageRankType>> pr_curr(n);
  std::vector<std::atomic<PageRankType>> pr_next(n);

  for (uintV i = 0; i < n; i++) {
    pr_curr[i] = INIT_PAGE_RANK;
    pr_next[i] = 0.0;
  }

  std::vector<std::thread> threads(n_threads);
  std::vector<std::vector<uintV>> assigned_vertex(n_threads,
                                                  std::vector<uintV>());
  std::vector<uintE> edges_processed(n_threads, 0);
  uintV min_vertices_for_each_thread = n / n_threads;
  uintV excess_vertices = n % n_threads;
  uintV start_vertex = 0;
  std::vector<double> local_time_taken(n_threads, 0.0);
  std::vector<double> barrier1_time(n_threads, 0.0);
  std::vector<double> barrier2_time(n_threads, 0.0);
  CustomBarrier barrier(n_threads);
  timer t1;
  double time_taken = 0.0;

  // start timing from allocating vertices and edges
  // -------------------------------------------------------------------
  t1.start();
  // determine number of verticies for each thread
  for (uint i = 0; i < n_threads; i++) {
    if (excess_vertices > 0) {
      for (uintV v = start_vertex;
           v <= start_vertex + min_vertices_for_each_thread; v++) {
        assigned_vertex[i].push_back(v);
        uintE out_degree = g.vertices_[v].getOutDegree();
        edges_processed[i] += out_degree;
      }
      excess_vertices--;
      start_vertex = start_vertex + min_vertices_for_each_thread + 1;
    } else {
      for (uintV v = start_vertex;
           v <= start_vertex + min_vertices_for_each_thread - 1; v++) {
        assigned_vertex[i].push_back(v);
        uintE out_degree = g.vertices_[v].getOutDegree();
        edges_processed[i] += out_degree;
      }
      start_vertex = start_vertex + min_vertices_for_each_thread;
    }
  }

  // Create threads and distribute the work across T threads
  for (uint i = 0; i < n_threads; i++) {
    threads.push_back(std::thread(
        getPageRankStatic, std::ref(g), i, max_iters, assigned_vertex[i],
        std::ref(pr_curr), std::ref(pr_next), &local_time_taken[i],
        &barrier1_time[i], &barrier2_time[i], &barrier));
  }

  for (std::thread &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
  time_taken = t1.stop();
  // -------------------------------------------------------------------
  std::vector<double> getNextVertex_time(n_threads, 0.0);
  std::vector<uintV> vertices_processed;
  for (uint i = 0; i < n_threads; i++) {
    // NOTE: since static mapping goes over each vertex and edge max_iters
    // amount of times need to multiple total number by max_iters
    vertices_processed.push_back(assigned_vertex[i].size() * max_iters);
    edges_processed[i] *= max_iters;
  }
  printStats(n, n_threads, std::ref(pr_curr), vertices_processed,
             edges_processed, barrier1_time, barrier2_time, getNextVertex_time,
             local_time_taken, time_taken);
}

void strategy2(Graph &g, int max_iters, uint n_threads) {
  uintV n = g.n_;
  uintE m = g.m_;
  std::vector<std::atomic<PageRankType>> pr_curr(n);
  std::vector<std::atomic<PageRankType>> pr_next(n);

  for (uintV i = 0; i < n; i++) {
    pr_curr[i] = INIT_PAGE_RANK;
    pr_next[i] = 0.0;
  }

  std::vector<std::thread> threads(n_threads);
  std::vector<std::vector<uintV>> assigned_vertex(n_threads,
                                                  std::vector<uintV>());
  std::vector<uintE> edges_processed(n_threads, 0);
  int edges_per_graph = m / n_threads;
  int total_assigned_edges = 0;
  int curr_vertex = 0;
  std::vector<double> local_time_taken(n_threads, 0.0);
  std::vector<double> barrier1_time(n_threads, 0.0);
  std::vector<double> barrier2_time(n_threads, 0.0);
  CustomBarrier barrier(n_threads);
  timer t1;
  double time_taken = 0.0;

  // -------------------------------------------------------------------
  t1.start();
  // assign vertices based on out-degree
  // Each thread gets assigned vertices until the total assigned edges is >=
  // (thread_id+1) * m/n_threads
  for (int i = 0; i < n_threads; i++) {
    int curr_assigned_edges = 0;
    while (total_assigned_edges < ((i + 1) * edges_per_graph) &&
           curr_vertex < n) {
      assigned_vertex[i].push_back(curr_vertex);
      uintE out_degree = g.vertices_[curr_vertex].getOutDegree();
      total_assigned_edges += out_degree;
      curr_assigned_edges += out_degree;
      curr_vertex++;
    }
    edges_processed[i] = curr_assigned_edges;
  }
  // Assign any left over vertices to the last thread
  while (curr_vertex < n) {
    assigned_vertex[n_threads - 1].push_back(curr_vertex);
    uintE out_degree = g.vertices_[curr_vertex].getOutDegree();
    edges_processed[n_threads - 1] += out_degree;
    curr_vertex++;
  }

  // Create threads and distribute the work across T threads
  for (uint i = 0; i < n_threads; i++) {
    threads.push_back(std::thread(
        getPageRankStatic, std::ref(g), i, max_iters, assigned_vertex[i],
        std::ref(pr_curr), std::ref(pr_next), &local_time_taken[i],
        &barrier1_time[i], &barrier2_time[i], &barrier));
  }

  for (std::thread &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
  time_taken = t1.stop();

  // -------------------------------------------------------------------
  std::vector<double> getNextVertex_time(n_threads, 0.0);
  std::vector<uintV> vertices_processed;
  for (uint i = 0; i < n_threads; i++) {
    // NOTE: since static mapping goes over each vertex and edge max_iters
    // amount of times need to multiple total number by max_iters
    vertices_processed.push_back(assigned_vertex[i].size() * max_iters);
    edges_processed[i] *= max_iters;
  }
  printStats(n, n_threads, std::ref(pr_curr), vertices_processed,
             edges_processed, barrier1_time, barrier2_time, getNextVertex_time,
             local_time_taken, time_taken);
}

void strategy3(Graph &g, int max_iters, uint n_threads, uint k) {
  uintV n = g.n_;
  uintE m = g.m_;
  std::vector<std::atomic<PageRankType>> pr_curr(n);
  std::vector<std::atomic<PageRankType>> pr_next(n);

  for (uintV i = 0; i < n; i++) {
    pr_curr[i] = INIT_PAGE_RANK;
    pr_next[i] = 0.0;
  }
  std::vector<std::thread> threads(n_threads);
  std::vector<uintV> vertices_processed(n_threads, 0);
  std::vector<uintE> edges_processed(n_threads, 0);
  std::vector<double> local_time_taken(n_threads, 0.0);
  std::vector<double> barrier1_time(n_threads, 0.0);
  std::vector<double> barrier2_time(n_threads, 0.0);
  std::vector<double> getNextVertex_time(n_threads, 0.0);
  CustomBarrier barrier(n_threads);
  DynamicMapping dm(k, n, n_threads);

  timer t1;
  double time_taken = 0.0;

  // Create threads and distribute the work across T threads
  // -------------------------------------------------------------------
  t1.start();
  for (uint i = 0; i < n_threads; i++) {
    threads.push_back(std::thread(
        getPageRankDynamic, std::ref(g), i, max_iters, k,
        &vertices_processed[i], &edges_processed[i], std::ref(pr_curr),
        std::ref(pr_next), &local_time_taken[i], &barrier1_time[i],
        &barrier2_time[i], &getNextVertex_time[i], &barrier, &dm));
  }

  for (std::thread &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
  time_taken = t1.stop();
  // -------------------------------------------------------------------

  printStats(n, n_threads, pr_curr, vertices_processed, edges_processed,
             barrier1_time, barrier2_time, getNextVertex_time, local_time_taken,
             time_taken);
}

void strategy4(Graph &g, int max_iters, uint n_threads, uint k) {
  strategy3(g, max_iters, n_threads, k);
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
          {"strategy", "Task decomposition and mapping strategy",
           cxxopts::value<uint>()->default_value("1")},
          {"granularity", "Vertex Decomposition Granularity",
           cxxopts::value<uint>()->default_value("1")},
      });

  auto cl_options = options.parse(argc, argv);
  uint n_threads = cl_options["nThreads"].as<uint>();
  uint max_iterations = cl_options["nIterations"].as<uint>();
  std::string input_file_path = cl_options["inputFile"].as<std::string>();
  uint strategy = cl_options["strategy"].as<uint>();
  uint k = cl_options["granularity"].as<uint>();

  // Check edge cases on inputs
  if (n_threads <= 0 || max_iterations <= 0) {
    throw std::invalid_argument(
        "The commandline arguments: --n_threads and --max_iterations "
        "must be at least 1\n");
  }

  if (strategy <= 0 || strategy > 4) {
    throw std::invalid_argument(
        "The commandline arguments: --strategy only accepts values 1, 2, 3, "
        "and 4\n");
  }

  if (k <= 0) {
    throw std::invalid_argument(
        "The commandline argument: --granularity must be a positive integer "
        "value\n");
  }

#ifdef USE_INT
  std::cout << "Using INT" << std::endl;
#else
  std::cout << "Using DOUBLE" << std::endl;
#endif
  std::cout << std::fixed;
  std::cout << "Number of Threads : " << n_threads << std::endl;
  std::cout << "Strategy : " << strategy << std::endl;
  std::cout << "Granularity : " << k << std::endl;
  std::cout << "Iterations : " << max_iterations << std::endl;

  Graph g;
  std::cout << "Reading graph\n";
  g.readGraphFromBinary<int>(input_file_path);
  std::cout << "Created graph\n";

  switch (strategy) {
    case 1:
      strategy1(g, max_iterations, n_threads);
      break;
    case 2:
      strategy2(g, max_iterations, n_threads);
      break;
    case 3:
      strategy3(g, max_iterations, n_threads, 1);
      break;
    case 4:
      strategy4(g, max_iterations, n_threads, k);
      break;
    default:
      strategy1(g, max_iterations, n_threads);
  }

  return 0;
}
