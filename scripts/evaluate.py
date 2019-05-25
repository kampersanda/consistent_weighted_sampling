#!/usr/bin/env python3

import numpy as np
import argparse


def read_data(path):
    lines = [line for line in open(path, 'rt')][2:]
    lines = [line.split(',')[:-1] for line in lines]
    lines = [[int(elem.split(':')[0]) for elem in line] for line in lines]
    return np.array(lines)


def eval_recall(score, groundtruth, R):
    assert(score.shape[0] == groundtruth.shape[0])
    M = score.shape[0]
    recall = 0.0
    for q in range(M):
        for r in range(R):
            if score[q][r] == groundtruth[q][0]:
                recall += 1.0
    return recall / M


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("score")
    parser.add_argument("groundtruth")
    args = parser.parse_args()

    score = read_data(args.score)
    groundtruth = read_data(args.groundtruth)

    top_k = score.shape[1]

    for R in [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000]:
        if R <= top_k:
            recall = eval_recall(score, groundtruth, R)
            print("Recall@" + str(R) + ":\t{0:.3f}".format(recall))
