#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=05:00
#SBATCH --mem=2G
#SBATCH --partition=slow

srun /home/cwlui/cmpt431/assignment5/one_lock_blocking_queue_throughput --n_producers 4 --n_consumers 4 --seconds 30 --init_allocator 100000000