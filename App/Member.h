//
// Created by anxin on 2020-04-07.
//

#ifndef TEXTTRUTH_MEMBER_H
#define TEXTTRUTH_MEMBER_H

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "boost/filesystem.hpp"
#include "math.h"
#include "util.h"

using namespace std;
using PriorCount = vector<int>;
using Observation = vector<int>;
using WordVec = vector<float>;

class WordModel {
public:
    unordered_map<string, WordVec> model;
    int dimension;

    WordModel() = default;

    WordModel(unordered_map<string, WordVec> &m, int dim) : model(m), dimension(dim) {};

    WordModel(unordered_map<string, WordVec> &&m, int dim) : model(m), dimension(dim) {};

    bool contain(string &word) {
        return model.count(word) != 0;
    }

    const WordVec &get_vec(string &word) {
        return model.at(word);
    }

    void load_raw_file(const string &path);

    void load_model(const string &path);

    void save_model(const string &path);
};

class Keyword {
public:
    int question_id;
    int cluster_assignment;
    int owner_id;
    string content;
    int shuffle_tag;

    explicit Keyword() = default;

    explicit Keyword(int oid, int qid, const string &cnt) :
		question_id(qid),
            owner_id(oid),
            content(cnt) {};

    explicit Keyword(int oid, int qid, string &&cnt):
		question_id(qid),
		            owner_id(oid),
		            content(cnt) {};

};

class Answer {
public:
    int question_id;
    int owner_id;
    string raw_answer;
    float truth_score;

    explicit Answer(int oid, int qid, string &as) : question_id(qid), owner_id(oid),
                                                                         raw_answer(as) { truth_score = 0; };

    explicit Answer(int oid, int qid, string &&as) : question_id(qid),owner_id(oid),
                                                                          raw_answer(as) { truth_score = 0; };

    vector<Keyword>
    to_keywords(const unordered_set<string> &words_filter, WordModel &word_model);

    vector<Keyword>
    to_keywords();

    static vector<string> string_split(const string &s);

};

class Question {
public:
    string content;
    int key_factor_number;
    vector<int> truth_indicator;
    static const int BETA[2];

    vector<Answer> top_k_results;

    Question(string &cnt, int kfn) : content(cnt), key_factor_number(kfn),
                                                       truth_indicator(vector<int>(kfn)) {};

    Question(string &&cnt, int kfn) : content(cnt), key_factor_number(kfn),
                                                        truth_indicator(vector<int>(kfn)) {};

    void set_key_factor_number(int num) {
        this->key_factor_number = num;
    }

    void set_truth_indicator(int factor_index, int indicator) {
        truth_indicator[factor_index] = indicator;
    }

    int get_truth_indicator(int factor_index) {
        return truth_indicator[factor_index];
    }
};

class User {
public:
    PriorCount priorCount;
    unordered_map<int, Answer> answers;
    unordered_map<int, Observation> observations;
    //false negative, false positive, true negative, true positive
    static const int ALPHA[4];

    float truth_score;

    User() {
        priorCount = PriorCount(4);
        truth_score = 0;
    };

//    User(PriorCount & priorCount, Observation &observation):priorCount(priorCount),observation(observation){};
    void update_prior_count(int key_factor_indicator, int observation, int num) {
        priorCount[key_factor_indicator * 2 + observation] += num;
    }

    void clear_prior_count() {
        int *start = &priorCount[0];
        memset(start, 0, sizeof(int) * 4);
    }

    int get_prior_count(int key_factor_indicator, int observation) {
        return priorCount[key_factor_indicator * 2 + observation];
    }

    void update_answer(Question &question, Answer &answer) {
        answers.emplace(answer.question_id, answer);
        initialize_observation(answer.question_id, question.key_factor_number);
    }

    void update_answer(Question &question, Answer &&answer) {
        answers.emplace(answer.question_id, std::move(answer));
        initialize_observation(answer.question_id, question.key_factor_number);
    }

    static int get_alpha(int key_factor_indicator, int observation) {
        return ALPHA[key_factor_indicator * 2 + observation];
    }

private:
    void initialize_observation(int question_id, int cluster_num) {
        observations.emplace(question_id, Observation(cluster_num));
    }
};

#endif //TEXTTRUTH_MEMBER_H


