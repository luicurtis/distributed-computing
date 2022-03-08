#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=05:00
#SBATCH --mem=2G
#SBATCH --partition=slow

srun /home/cwlui/cmpt431/assignment5/two_lock_queue_correctness --n_producers 2 --n_consumers 2 --inputFile inputs/rand_10M