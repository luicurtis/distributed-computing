#!/bin/bash
#
#SBATCH --cpus-per-task=4
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=slow
#SBATCH --nodelist=cs-cloud-02

srun /home/cwlui/cmpt431/assignment4/page_rank_push_parallel_atomic --nThreads 4 --nIterations 100 --inputFile /scratch/input_graphs/roadNet-CA --strategy 3 --granularity 100