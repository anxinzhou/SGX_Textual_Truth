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
#include "oblivious_primitive.h"
#include "oblivious_ttruth.h"
#include <omp.h>

const int THREAD_NUM = 8;

using namespace std;

void ecall_ttruth(char **question_solutions, int **question_user_sol_size,
		float **question_vecs, int *question_top_k_user, int question_num,
		int user_num, int top_k, int cluster_size, int vector_size, int cnt) {
	omp_set_dynamic(0);     // Explicitly disable dynamic teams
	omp_set_num_threads(THREAD_NUM); // Use 4 threads for all consecutive parallel regions

	vector<vector<Observation>> question_observations(question_num);

	//ocall_print_string("truth discovery start in enclave\n");
	vector<vector<Keyword>> question_keywords(question_num);
	// parallel question id for large question num
	//#pragma omp parallel for
	for (int question_id = 0; question_id < question_num; question_id++) {
		vector<Observation> observations(user_num, Observation(cluster_size));
		// load solutions for the question
		int *user_solution_size = question_user_sol_size[question_id];

		int total_solution_size = 0;
		for (int i = 0; i < user_num; i++) {
			total_solution_size += user_solution_size[i];
		}

		char *solutions = question_solutions[question_id];
		//extra 1 to store NULL terminator

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

		//prepare keywords
		vector<Keyword> keywords;
		for (auto &answer : answers) {
			auto user_kws = answer.to_keywords();
			for (auto &ukw : user_kws) {
				//if(ukw.question_id == -1) throw "unexpected state";
				keywords.push_back(std::move(ukw));
			}
		}

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
		sgx_status_t ret = ocall_filter_keywords(question_id, raw_words,
				&words_size[0], &words_filter_ind[0], word_num);

		if (ret != SGX_SUCCESS) {
			throw "can not filter words";
		}

		// filter keywords use indicator
		// record the size of words after filtering
		unordered_set<string> to_remove_words;
		words_start_loc = 0;
		vector<string> after_filter_words;
		for (int i = 0; i < word_num; i++) {
			string word(raw_words + words_start_loc, words_size[i]);
			if (words_filter_ind[i] == 0) {
				after_filter_words.push_back(std::move(word));
			} else {
				to_remove_words.insert(std::move(word));
			}
			words_start_loc += words_size[i];
		}
		delete[] raw_words;
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

		float *word_vectors = question_vecs[question_id];

		unordered_map<string, WordVec> dic;
		for (int i = 0; i < after_filter_words.size(); i++) {
			auto &word = after_filter_words[i];
			WordVec v(word_vectors + vector_size * i,
					word_vectors + vector_size * (i + 1));
			dic.emplace(std::move(word), std::move(v));

		}

		WordModel word_model(dic, vector_size);

		// clustering
		// prepare weighted words
		unordered_map<string, int> kw_counter;
		for (auto &kw : keywords) {
			kw_counter[kw.content] += 1;
		}
		vector<string> kws;
		vector<int> weight;
		for (auto &p : kw_counter) {
			kws.push_back(std::move(p.first));
			weight.push_back(p.second);
		}
		auto cluster_assignment = parallel_weighted_sphere_kmeans(kws, weight,
				word_model, cluster_size);
		unordered_map<string, int> kw_cluster_ind;
		for (int i = 0; i < kws.size(); i++) {
			kw_cluster_ind.emplace(
					std::make_pair(std::move(kws[i]), cluster_assignment[i]));
		}
		// assign cluster to keywords
		for (auto &kw : keywords) {
			kw.cluster_assignment = kw_cluster_ind[kw.content];
		}
		// old version
		//sphere_kmeans(keywords, word_model, cluster_size);

		// update observation
		observation_update(observations, keywords);

		question_keywords[question_id] = std::move(keywords);

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
	// parallel question id for large question num
	#pragma omp parallel for
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
		if (total_score < 1e-6)
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
			q.pop();
		}
		memcpy(question_top_k_user + question_id * top_k, &top_k_user[0],
				top_k * sizeof(int));
	}
}

void ecall_oblivious_ttruth(char **question_solutions,
		int **question_user_sol_size, float **question_vecs,
		int *question_top_k_user, int question_num, int user_num, int top_k,
		int cluster_size, int vector_size, int cnt, double epsilon,
		double delta) {
	omp_set_dynamic(0);     // Explicitly disable dynamic teams
	omp_set_num_threads(THREAD_NUM); // Use 4 threads for all consecutive parallel regions
	vector<vector<Observation>> question_observations(question_num);

	//ocall_print_string("truth discovery start in enclave\n");
	vector<vector<Keyword>> question_keywords(question_num);
	// parallel question id for large question num
	//#pragma omp parallel for
	for (int question_id = 0; question_id < question_num; question_id++) {
		vector<Observation> observations(user_num, Observation(cluster_size));
		// load solutions for the question
		int *user_solution_size = question_user_sol_size[question_id];

		int total_solution_size = 0;
		for (int i = 0; i < user_num; i++) {
			total_solution_size += user_solution_size[i];
		}

		char *solutions = question_solutions[question_id];
		//extra 1 to store NULL terminator
		memcpy(solutions, question_solutions[question_id], total_solution_size);
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

		//prepare keywords
		vector<Keyword> keywords;
		for (auto &answer : answers) {
			auto user_kws = answer.to_keywords();
			for (auto &ukw : user_kws) {
				//if(ukw.question_id == -1) throw "unexpected state";
				keywords.push_back(std::move(ukw));
			}
		}
		//ocall_print_string((to_string(keywords.size())+"\n").c_str());
		// insert oblivious part here
		// pad keywords to same length
		keywords_padding(keywords);
		//ocall_print_string((to_string(keywords.size())+"\n").c_str());

		//for(auto &kw: keywords) {
		//ocall_print_string((to_string(kw.content.size())+"\n").c_str());
		//}
		unordered_set<string> test_voc;
		for (auto &kw : keywords) {
			if (test_voc.count(kw.content) == 0) {
				test_voc.insert(kw.content);
			}
		}
		//ocall_print_string((to_string(test_voc.size())+"\n").c_str());

		// keywords space decide
		auto padded_vocabulary = oblivious_vocabulary_decide(keywords);

		// oblivious dummy words addition

		//ocall_print_string((to_string(padded_vocabulary.size())+"\n").c_str());

		//ocall_print_string((to_string(keywords.size())+"\n").c_str());

		oblivious_dummy_words_addition(padded_vocabulary, keywords, epsilon,
				delta);
		//ocall_print_string((to_string(keywords.size())+"\n").c_str());

		// remove padding
		keywords_remove_padding(keywords);

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
		sgx_status_t ret = ocall_filter_keywords(question_id, raw_words,
				&words_size[0], &words_filter_ind[0], word_num);

		if (ret != SGX_SUCCESS) {
			throw "can not filter words";
		}

		// filter keywords use indicator
		// record the size of words after filtering
		unordered_set<string> to_remove_words;
		words_start_loc = 0;
		vector<string> after_filter_words;
		for (int i = 0; i < word_num; i++) {
			string word(raw_words + words_start_loc, words_size[i]);
			if (words_filter_ind[i] == 0) {
				after_filter_words.push_back(std::move(word));
			} else {
				to_remove_words.insert(std::move(word));
			}
			words_start_loc += words_size[i];
		}
		delete[] raw_words;
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

		float *word_vectors = question_vecs[question_id];

		unordered_map<string, WordVec> dic;
		for (int i = 0; i < after_filter_words.size(); i++) {
			auto &word = after_filter_words[i];
			WordVec v(word_vectors + vector_size * i,
					word_vectors + vector_size * (i + 1));
			dic.emplace(std::move(word), std::move(v));

		}

		WordModel word_model(dic, vector_size);

		// clustering
		// prepare weighted words
		unordered_map<string, int> kw_counter;
		for (auto &kw : keywords) {
			if (kw_counter.count(kw.content) == 0) {
				kw_counter[kw.content] = 1 * (kw.owner_id != -1);
			} else {
				kw_counter[kw.content] += 1 * (kw.owner_id != -1);
			}
		}
		vector<string> kws;
		vector<int> weight;
		for (auto &p : kw_counter) {
			kws.push_back(std::move(p.first));
			weight.push_back(p.second);
		}
		auto cluster_assignment = parallel_weighted_sphere_kmeans(kws, weight,
				word_model, cluster_size);
		unordered_map<string, int> kw_cluster_ind;
		for (int i = 0; i < kws.size(); i++) {
			kw_cluster_ind.emplace(
					std::make_pair(std::move(kws[i]), cluster_assignment[i]));
		}
		// assign cluster to keywords
		for (auto &kw : keywords) {
			kw.cluster_assignment = kw_cluster_ind[kw.content];
		}
		// old version
		//oblivious_sphere_kmeans(keywords, word_model, cluster_size);

		// remove dummy words
		//shuffle to hide how many counts dummy for each word
		keywords_padding(keywords);
		oblivious_shuffle(keywords);
		{
			vector<Keyword> tmp;
			for (auto &keyword : keywords) {
				if (keyword.owner_id == -1)
					continue;
				tmp.push_back(std::move(keyword));
			}
			keywords = std::move(tmp);
		}
		keywords_remove_padding(keywords);
		// update observation
		oblivious_observation_update(observations, keywords);

		question_keywords[question_id] = std::move(keywords);

		question_observations[question_id] = std::move(observations);

	}

	// latent truth discovery
	auto question_truth_indicator = oblivious_latent_truth_model(
			question_observations);
	/*for(int i=0;i<question_truth_indicator.size();i++) {
	 ocall_print_int_array(&question_truth_indicator[i][0], question_truth_indicator[i].size());

	 }*/
	// decide the top_k_score of each question
	// and score of each user
	vector<double> user_score(user_num);

	// calculate score of each usre and get the top_k_user of each question
	#pragma omp parallel for
	for (int question_id = 0; question_id < question_num; question_id++) {
		// calculate truth score of each answer
		vector<double> user_answer_score(user_num);
		for (auto &keyword : question_keywords[question_id]) {
			int owner_id = keyword.owner_id;
			int cluster_assignment = keyword.cluster_assignment;
			auto &truth_indicator = question_truth_indicator[question_id];
			int indicator = 0;
			// obliviously get indicator
			for (int k = 0; k < truth_indicator.size(); k++) {
				int ind = truth_indicator[k];
				indicator = oblivious_assign_CMOV(k == cluster_assignment, ind,
						indicator);
			}

			user_answer_score[owner_id] += 1 * (indicator == 1);
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

		vector<pair<double, int>> q;
		for (int i = 0; i < user_num; i++) {
			auto score = user_answer_score[i];
			q.emplace_back(score, i);
		}
		oblivious_sort(q, 1);
		vector<int> top_k_user(top_k);
		for (int i = 0; i < top_k; i++) {
			int user_index = q[i].second;
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
