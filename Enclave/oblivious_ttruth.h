//
// Created by anxin on 2020/4/11.
//

#ifndef TEXTTRUTH_OBLIVIOUS_TTRUTH_H
#define TEXTTRUTH_OBLIVIOUS_TTRUTH_H

#include "Enclave_t.h"
#include <sgx_trts.h>
#include "Member.h"
#include <random>
#include "util.h"
#include "oblivious_primitive.h"
#include <iostream>
#include <climits>


void oblivious_ttruth(vector<Question> &questions, vector<User> &users,const unordered_set<string> &words_filter ,WordModel &word_model, int top_k=5);
void oblivious_observation_update(vector<Observation> &observations, vector<Keyword> &keywords);
void oblivious_sphere_kmeans(vector<Keyword> &keywords, WordModel &word_model,int cluster_num, int max_iter=20, double tol= 1e-12);

vector<vector<int>> oblivious_latent_truth_model(vector<vector<Observation>> &question_observations, int max_iter=50);
// helper function
vector<WordVec> oblivious_kmeans_init(vector<Keyword> &keywords,WordModel &word_model, int cluster_num);


#endif //TEXTTRUTH_OBLIVIOUS_TTRUTH_H
