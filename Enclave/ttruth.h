//
// Created by anxin on 2020-04-07.
//

#ifndef TEXTTRUTH_TTRUTH_H
#define TEXTTRUTH_TTRUTH_H

#include "Enclave_t.h"
#include "Member.h"
#include <random>
#include "util.h"
#include <iostream>
#include <sgx_trts.h>
#include <queue>
#include <climits>

void observation_update(vector<Observation> &observations, vector<Keyword> &keywords);

// define two  clustering method
//void hard_movMF(vector<Keyword> &keywords, WordModel &word_model, int cluster_num, int max_iter=100, double tol = 1e-12);
void sphere_kmeans(vector<Keyword> &keywords, WordModel &word_model,int cluster_num, int max_iter=100, double tol= 1e-12);

vector<vector<int>> latent_truth_model(vector<vector<Observation>> &question_observations, int max_iter=100);

// helper function
vector<WordVec> kmeans_init(vector<Keyword> &keywords,WordModel &word_model, int cluster_num);
#endif //TEXTTRUTH_TTRUTH_H

