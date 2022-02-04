#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=slow
#SBATCH --nodelist=cs-cloud-02

srun /home/cwlui/cmpt431/assignment3/page_rank_push_parallel_atomic --nThreads 8 --nIterations 10 --inputFile /scratch/input_graphs/roadNet-CA