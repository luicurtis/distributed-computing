#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=slow

srun /home/cwlui/cmpt431/assignment3/page_rank_pull_parallel --nThreads 8 --nIterations 10 --inputFile /scratch/input_graphs/roadNet-CA