#!/bin/bash
#
#SBATCH --cpus-per-task=4
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=slow
#SBATCH --nodelist=cs-cloud-02

srun /home/cwlui/cmpt431/assignment4/page_rank_pull_parallel --nThreads 4 --nIterations 1 --inputFile /scratch/input_graphs/sx-stackoverflow-a2q --strategy 1 --granularity 100