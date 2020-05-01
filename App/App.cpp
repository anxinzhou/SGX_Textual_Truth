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

// App.cpp : Define the entry point for the console application.
//
#include <string.h>
#include <assert.h>
#include <fstream>
#include <thread>
#include <iostream>

#include "Enclave_u.h"
#include "sgx_urts.h"
#include "sgx_tseal.h"

#include "dataset.h"
#include "Member.h"
#include <boost/serialization/string.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <chrono>

#define ENCLAVE_NAME "enclave.signed.so"

// Global data
sgx_enclave_id_t global_eid = 0;
vector<vector<Answer>> questions_answers;
char **question_solutions;
const int VECTOR_SIZE = 100;
WordModel word_model;

typedef struct _sgx_errlist_t {
	sgx_status_t err;
	const char *msg;
	const char *sug; /* Suggestion */
} sgx_errlist_t;

using namespace std;
using namespace std::chrono;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] =
		{ { SGX_ERROR_UNEXPECTED, "Unexpected error occurred.",
		NULL }, { SGX_ERROR_INVALID_PARAMETER, "Invalid parameter.",
		NULL }, { SGX_ERROR_OUT_OF_MEMORY, "Out of memory.",
		NULL }, { SGX_ERROR_ENCLAVE_LOST, "Power transition occurred.",
				"Please refer to the sample \"PowerTransition\" for details." },
				{ SGX_ERROR_INVALID_ENCLAVE, "Invalid enclave image.",
				NULL }, { SGX_ERROR_INVALID_ENCLAVE_ID,
						"Invalid enclave identification.",
						NULL }, { SGX_ERROR_INVALID_SIGNATURE,
						"Invalid enclave signature.",
						NULL }, { SGX_ERROR_OUT_OF_EPC, "Out of EPC memory.",
				NULL },
				{ SGX_ERROR_NO_DEVICE, "Invalid SGX device.",
						"Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards." },
				{ SGX_ERROR_MEMORY_MAP_CONFLICT, "Memory map conflicted.",
				NULL }, { SGX_ERROR_INVALID_METADATA,
						"Invalid enclave metadata.",
						NULL }, { SGX_ERROR_DEVICE_BUSY, "SGX device was busy.",
				NULL }, { SGX_ERROR_INVALID_VERSION,
						"Enclave version was invalid.",
						NULL }, { SGX_ERROR_INVALID_ATTRIBUTE,
						"Enclave was not authorized.",
						NULL }, { SGX_ERROR_ENCLAVE_FILE_ACCESS,
						"Can't open enclave file.",
						NULL }, };

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret) {
	size_t idx = 0;
	size_t ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

	for (idx = 0; idx < ttl; idx++) {
		if (ret == sgx_errlist[idx].err) {
			if (NULL != sgx_errlist[idx].sug)
				printf("Info: %s\n", sgx_errlist[idx].sug);
			printf("Error: %s\n", sgx_errlist[idx].msg);
			break;
		}
	}

	if (idx == ttl)
		printf(
				"Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n",
				ret);
}

void ocall_query_solution_size(int question_id, int *user_solution_size,
		int user_num) {
	auto &answers = questions_answers[question_id];
	for (int i = 0; i < user_num; i++) {
		int answer_size = int(answers.size());
		if (i >= answer_size) {
			user_solution_size[i] = 0;
		} else {
			user_solution_size[i] = int(answers[i].raw_answer.size());
		}
	}
}

void ocall_load_question_solutions(int question_id, char *solutions, int len) {
	//cout<<"solutions:"<<question_solutions[question_id]<<endl;
	strcpy(solutions, question_solutions[question_id]);
}

void ocall_filter_keywords(char *raw_words, int *words_size,
		int *words_filter_ind, int word_num) {
	int start_loc = 0;
	for (int i = 0; i < word_num; i++) {
		int word_size = words_size[i];
		int end_loc = start_loc + word_size;
		string word(raw_words + start_loc, word_size);
		//cout<<word<<endl;
		if (AnswerGradingData::words_filter.count(word) == 1
				|| !word_model.contain(word)) {
			words_filter_ind[i] = 1;
		} else {
			words_filter_ind[i] = 0;
		}
		start_loc = end_loc;
	}
}

void ocall_load_words_vectors(char *raw_words, int *words_size, float *vectors,
		int word_num, int vec_len) {
	int dimension = word_model.dimension;
	int start_loc = 0;
	for (int i = 0; i < word_num; i++) {
		int word_size = words_size[i];
		int end_loc = start_loc + word_size;
		string word(raw_words + start_loc, word_size);
		auto vec = word_model.get_vec(word);
		memcpy(vectors+i*dimension, &vec[0], dimension * sizeof(float));
		start_loc = end_loc;
	}
}

/* OCall functions */
void ocall_print_string(const char *str) {
	/* Proxy/Bridge will check the length and null-terminate
	 * the input string to prevent buffer overflow.
	 */
	printf("%s", str);
}

void ocall_print_int_array(int *arr, int len) {
	for(int i=0;i<len;i++){
		cout<<arr[i]<<" ";
	}
	cout<<endl;
}

void ocall_print_float_array(float *arr, int len) {
	for(int i=0;i<len;i++){
		cout<<arr[i]<<" ";
	}
	cout<<endl;
}

void test_texttruth(bool oblivious = false) {

	// load dataset answer grading
	const string dataset_path = "answer_grading";
	const string model_path = "wordvec/glove27B100d";
	AnswerGradingData answer_grade_data;
	answer_grade_data.load_dataset(dataset_path);

    /*word_model.load_raw_file("./pre-trained_model/glove.twitter.27B.100d.txt");
    word_model.save_model(model_path);
    return;*/
	auto start = high_resolution_clock::now();
	word_model.load_model(model_path);
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	std::cout << "load model time " << (double) duration.count() / 1000000.0
			<< "s" << endl;
	cout << "model size " << word_model.model.size() << endl;

	// set user
	questions_answers = std::move(answer_grade_data.answers);

	auto &questions = answer_grade_data.questions;
	auto &baseline_scores = answer_grade_data.scores;
	auto &question_per_exam = answer_grade_data.question_per_exam;

	int question_num = int(questions.size());
	question_solutions = new char*[question_num];

	for (int i = 0; i < question_num; i++) {
		int answer_size = 0;
		auto &answers = questions_answers[i];
		for (auto &answer : answers) {
			answer_size += answer.raw_answer.size();
		}
		auto &solutions = question_solutions[i];
		solutions = new char[answer_size + 1];
		int start_loc = 0;
		for (auto &answer : answers) {
			const char *raw_content = answer.raw_answer.c_str();
			memcpy(solutions + start_loc, raw_content,
					answer.raw_answer.size());
			start_loc += answer.raw_answer.size();
		}
		solutions[start_loc] = '\0';
	}

	// decide user number
	int user_num = INT_MIN;
	for (auto &answer : questions_answers) {
		int answer_num = int(answer.size());
		if (answer_num > user_num) {
			user_num = answer_num;
		}
	}
	if (user_num < 0) {
		cout << "empty dataset" << endl;
		exit(-1);
	}

	// benchmark texttruth
	for (int top_k = 1; top_k <=1; top_k += 1) {
		cout << "top " << top_k << endl;
		int *question_top_k_user = new int[question_num * top_k];
		start = high_resolution_clock::now();
		// get top_k_user for each question from truth discovery
		if (!oblivious) {
			//test(global_eid);
			cout<<"non oblivious truth top "<<top_k<<endl;
			sgx_status_t ret = ecall_ttruth(global_eid, question_top_k_user,
					question_num, user_num, top_k,
					AnswerGradingData::key_factor_number, VECTOR_SIZE,
					question_num * top_k);

			if (ret != SGX_SUCCESS) {
				print_error_message(ret);
				throw("truth discover fail");
			}
		} else {
			cout<<"oblivious truth top "<<top_k<<endl;

			sgx_status_t ret = ecall_oblivious_ttruth(global_eid, question_top_k_user,
								question_num, user_num, top_k,
								AnswerGradingData::key_factor_number, VECTOR_SIZE,
								question_num * top_k);

						if (ret != SGX_SUCCESS) {
							print_error_message(ret);
							throw("truth discover fail");
						}
		}

		stop = high_resolution_clock::now();
		duration = duration_cast<microseconds>(stop - start);
		std::cout << "truth discovery time "
				<< (double) duration.count() / 1000000.0 << "s" << endl;

		// show the average score for each exam
		vector<double> exam_score(question_per_exam.size(), 0);
		int count = 0;
		int exam_id = 0;
		for (int question_id = 0; question_id < question_num; question_id++) {
			if (count == question_per_exam[exam_id]) {
				count = 0;
				exam_id += 1;
			}
			vector<int> top_k_user(question_top_k_user+question_id * top_k,
					question_top_k_user+(question_id + 1) * top_k);

			for (auto user_index : top_k_user) {
				double score = baseline_scores[question_id][user_index];
				exam_score[exam_id] += score;
			}
			count += 1;
		}

		// average exam_score;
		int exam_num = int(exam_score.size());
		for (int i = 0; i < exam_num; i++) {
			exam_score[i] /= top_k * question_per_exam[i];
			cout << "exam " << i + 1 << " avg score: " << exam_score[i] << endl;
		}

		delete[] question_top_k_user;
	}
	delete[] question_solutions;
	for (int i = 0; i < question_num; i++) {
		delete[] question_solutions[i];
	}
}

int main(int argc, char *argv[]) {

	(void) argc, (void) argv;

	sgx_status_t ret = SGX_SUCCESS;

	// load the enclave
	// Debug: set the 2nd parameter to 1 which indicates the enclave are launched in debug mode
	ret = sgx_create_enclave(ENCLAVE_NAME, SGX_DEBUG_FLAG, NULL, NULL,
			&global_eid,
			NULL);
	if (ret != SGX_SUCCESS) {
		print_error_message(ret);
		return -1;
	}
	bool oblivious = true;
	test_texttruth(oblivious);
	// Destroy the enclave
	sgx_destroy_enclave(global_eid);

	cout << "Enter a character before exit ..." << endl;
	getchar();
	return 0;
}

