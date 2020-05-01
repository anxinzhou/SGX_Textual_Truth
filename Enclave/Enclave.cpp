/*
 * Copyright (C) 2011-2020 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "Enclave_t.h"
#include <vector>
#include <numeric>
#include "Member.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include "ttruth.h"
#include <queue>
#include <cstring>

using namespace std;

void ecall_ttruth(int *question_top_k_user, int question_num, int user_num,
		int top_k, int cluster_size, int vector_size, int cnt) {

	vector<vector<Observation>> question_observations(question_num);

	ocall_print_string("truth discovery start in enclave\n");
	vector<vector<Keyword>> question_keywords;
	for (int question_id = 0; question_id < question_num; question_id++) {
		vector<Observation> observations(user_num, Observation(cluster_size));
		// load solutions for the question
		std::vector<int> user_solution_size(user_num);

		sgx_status_t ret = ocall_query_solution_size(question_id,
				&user_solution_size[0], user_num);
		if (ret != SGX_SUCCESS) {
			throw "can not read solution size";
		}
		int total_solution_size = accumulate(user_solution_size.begin(),
				user_solution_size.end(), 0);
		char *solutions = new char[total_solution_size + 1];
		//extra 1 to store NULL terminator
		ret = ocall_load_question_solutions(question_id, solutions,
				total_solution_size + 1);
		if (ret != SGX_SUCCESS) {
			throw "can not load solution for question";
		}
		vector<Answer> answers;
		int sol_start_loc = 0;
		for (int i = 0; i < user_num; i++) {
			int sol_end_loc = sol_start_loc + user_solution_size[i];
			if (user_solution_size[i] == 0) {
				// dot not provide solution
				continue;
			} else {
				string raw_answer(solutions + sol_start_loc,
						user_solution_size[i]);
				//ocall_print_string(("solution in sgx:"+raw_answer+"\n").c_str());
				answers.emplace_back(i, question_id, std::move(raw_answer));
			}
			sol_start_loc = sol_end_loc;
		}
		delete[] solutions;

		//prepare keywords
		vector<Keyword> keywords;
		for (auto &answer : answers) {
			auto user_kws = answer.to_keywords();
			for (auto &ukw : user_kws) {
				keywords.push_back(std::move(ukw));
			}
		}

		// filter keywords first;
		// get unique keywords
		unordered_set<string> voc;

		for (auto &kw : keywords) {
			if (voc.count(kw.content) == 0)
				voc.insert(kw.content);
		}

		int word_num = voc.size();
		vector<int> words_size;
		int total_word_size = 0;
		vector<string> words;
		for (auto &e : voc) {
			words_size.push_back(e.size());
			total_word_size += e.size();
			words.push_back(e);
			//ocall_print_string((e+"\n").c_str());
		}
		char *raw_words = new char[total_word_size + 1];

		// load words
		int words_start_loc = 0;
		for (int i = 0; i < word_num; i++) {
			int words_end_loc = words_start_loc + words_size[i];
			memcpy(raw_words + words_start_loc, words[i].c_str(),
					words[i].size());
			words_start_loc = words_end_loc;
		}
		if (words_start_loc != total_word_size) {
			throw("word size incorrect");
		}

		raw_words[total_word_size] = '\0';
		vector<int> words_filter_ind(word_num);
		ret = ocall_filter_keywords(raw_words, &words_size[0],
				&words_filter_ind[0], word_num);

		if (ret != SGX_SUCCESS) {
			throw "can not filter words";
		}

		// filter keywords use indicator
		// record the size of words after filtering
		unordered_set<string> to_remove_words;
		words_start_loc = 0;
		vector<int> filter_words_size;
		int filter_words_total_size = 0;
		for (int i = 0; i < word_num; i++) {
			string word(raw_words + words_start_loc, words_size[i]);
			if (words_filter_ind[i] == 0) {
				filter_words_size.push_back(words_size[i]);
				filter_words_total_size += words_size[i];
			} else {
				to_remove_words.insert(std::move(word));
			}
			words_start_loc += words_size[i];
		}
		// filter keywords
		vector<Keyword> tmp;
		for (auto &kw : keywords) {
			if (to_remove_words.count(kw.content) == 0) {
				tmp.push_back(std::move(kw));
			}
		}
		keywords = std::move(tmp);
		// get unique words after filtering
		words_start_loc = 0;
		int filter_word_num = 0;
		int filter_words_start_loc = 0;
		char *filtered_raw_words = new char[filter_words_total_size + 1];
		filtered_raw_words[filter_words_total_size] = '\0';
		for (int i = 0; i < word_num; i++) {
			if (words_filter_ind[i] == 0) {
				string word(raw_words + words_start_loc, words_size[i]);
				memcpy(filtered_raw_words + filter_words_start_loc,
						raw_words + words_start_loc, words_size[i]);
				filter_words_start_loc += words_size[i];
				filter_word_num += 1;
			}
			words_start_loc += words_size[i];
		}

		// load vectors for word
		float *word_vectors = new float[filter_word_num * vector_size];
		ret = ocall_load_words_vectors(filtered_raw_words,
				&filter_words_size[0], word_vectors, filter_word_num,
				filter_word_num * vector_size);
		if (ret != SGX_SUCCESS) {
			throw "can not load word vectors";
		}
		// build word model from words and raw_vectors;
		unordered_map<string, WordVec> dic;
		filter_words_start_loc = 0;
		for (int i = 0; i < filter_word_num; i++) {
			string word(filtered_raw_words + filter_words_start_loc,
					filter_words_size[i]);
			WordVec v(word_vectors + vector_size * i,
					word_vectors + vector_size * (i + 1));
			dic.emplace(std::move(word), std::move(v));
			filter_words_start_loc += filter_words_size[i];
		}
		WordModel word_model(dic, vector_size);
		delete[] filtered_raw_words;
		delete[] raw_words;
		delete[] word_vectors;

		// clustering
		sphere_kmeans(keywords, word_model, cluster_size);

		// update observation
		observation_update(observations, keywords);

		question_keywords.push_back(std::move(keywords));

		question_observations[question_id] = std::move(observations);

	}

	// latent truth discovery
	auto question_truth_indicator = latent_truth_model(question_observations);
	/*for(int i=0;i<question_truth_indicator.size();i++) {
		ocall_print_int_array(&question_truth_indicator[i][0], question_truth_indicator[i].size());

	}*/
	// decide the top_k_score of each question
	// and score of each user

	vector<double> user_score(user_num);

	// calculate score of each usre and get the top_k_user of each question

	auto score_cmp = [](pair<double, int> &a, pair<double, int> &b) {
		return a.first < b.first;
	};
	for (int question_id = 0; question_id < question_num; question_id++) {
		// calculate truth score of each answer
		vector<double> user_answer_score(user_num);
		for (auto &keyword : question_keywords[question_id]) {
			int owner_id = keyword.owner_id;
			int cluster_assignment = keyword.cluster_assignment;
			int indicator =
					question_truth_indicator[question_id][cluster_assignment];
			if (indicator == 1)
				user_answer_score[owner_id] += 1;
		}

		double total_score = std::accumulate(user_answer_score.begin(),
				user_answer_score.end(), 0);
		// normalize score for each answer
		if (total_score == 0)
			throw "total score should not be 0";

		for (int i = 0; i < user_num; i++) {
			auto &score = user_answer_score[i];
			score /= total_score;
			user_score[i] += score;
		}

		// decide the top_k score
		priority_queue<pair<double, int>, vector<pair<double, int>>,
				decltype(score_cmp)> q(score_cmp);

		for (int i = 0; i < user_num; i++) {
			auto score = user_answer_score[i];
			q.emplace(score, i);
		}
		vector<int> top_k_user(top_k);
		for (int i = 0; i < top_k; i++) {
			int user_index = q.top().second;
			top_k_user[i] = user_index;
		}
		memcpy(question_top_k_user + question_id * top_k, &top_k_user[0],
				top_k * sizeof(int));
	}
}

void test() {
	ocall_print_string("test");
	ocall_print_string("should be right");
}