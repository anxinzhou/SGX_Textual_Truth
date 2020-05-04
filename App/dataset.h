//
// Created by anxin on 2020/4/9.
//

#ifndef TEXTTRUTH_DATASET_H
#define TEXTTRUTH_DATASET_H

#include "Member.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "boost/filesystem.hpp"

extern unordered_set<string> words_filter;

class AnswerGradingData {
public:
    vector<Question> questions;
    vector<vector<Answer>> answers;
    vector<vector<float>> scores;
    vector<int> question_per_exam;
    static const int key_factor_number;
    AnswerGradingData() {
        question_per_exam = vector<int> {
            7, 7, 7, 7,  4, 7, 7, 7, 7, 7, 10, 10
        };
    }
    void load_dataset(const string &dir_path);

private:
    void load_question(const string &question_file_path);

    void load_answers(const string &answer_file_path);

    void load_scores(const string &score_file_path);
};

class SimulateData {
public:

	vector<vector<Answer>> answers;
	static const int key_factor_number;
	explicit SimulateData(int worker_num, int question_num, int answer_word_num, vector<string> &words_space);
	explicit SimulateData(int worker_num, int question_num, int answer_word_num, int answer_keyword_num,vector<string> &keywords_space, vector<string>&fake_words_space);
};



#endif //TEXTTRUTH_DATASET_H

