//
// Created by anxin on 2020-04-07.
//

#include "Member.h"

//true negative, false positive, false negative, true positive
const int User::ALPHA[4] = {90, 10, 50, 50};

const int Question::BETA[2] = {10, 10};

vector<string> Answer::string_split(const string &s) {
    vector<string> tokens;
    istringstream iss(s);
    copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));
    return tokens;
}

vector<Keyword>
Answer::to_keywords(const unordered_set<string> &words_filter, WordModel &word_model) {
    // replace punc
    vector<Keyword> keywords;
    replace_if(raw_answer.begin(), raw_answer.end(),
               [](const char &c) { return std::ispunct(c); }, ' ');

    // lower alphabetic
    transform(raw_answer.begin(), raw_answer.end(), raw_answer.begin(),
              [](unsigned char c) { return std::tolower(c); });

    auto tokens = string_split(raw_answer);

    // extract keywords
    // only remove stopwords here, may use doman specific dictionary
    // here skip words not in dictionary
    for (auto token: tokens) {
        if (words_filter.count(token) == 1 || !word_model.contain(token)) {
//                    cout << token << endl;
            continue;
        }
        keywords.emplace_back(owner_id, question_id, std::move(token));
    }
    return keywords;
}
vector<Keyword>
Answer::to_keywords() {
    // replace punc
    vector<Keyword> keywords;
    replace_if(raw_answer.begin(), raw_answer.end(),
               [](const char &c) { return std::ispunct(c); }, ' ');

    // lower alphabetic
    transform(raw_answer.begin(), raw_answer.end(), raw_answer.begin(),
              [](unsigned char c) { return std::tolower(c); });

    auto tokens = string_split(raw_answer);
    keywords.reserve(tokens.size());
    for (auto token: tokens) {
        keywords.emplace_back(owner_id, question_id, std::move(token));
    }
    return keywords;
}

void WordModel::load_raw_file(const string &raw_file_path) {
    ifstream word_embedding_file(raw_file_path);
    if (!word_embedding_file.is_open()) {
        cout << "can not open word embedding file" << endl;
        cout << strerror(errno) << endl;
        exit(0);
    }
    string line;
    while (getline(word_embedding_file, line)) {
        istringstream iss(line);
        string word;
        iss >> word;
        WordVec vec;
        float v;
        while (iss >> v) {
            vec.push_back(v);
        }
        // normalize the keyword
        float total = sqrt(hpc::dot_product(vec, vec));
        hpc::vector_mul_inplace(vec, 1 / total);
        model[word] = vec;
    }
    this->dimension = int(model.begin()->second.size());
    word_embedding_file.close();
}

void WordModel::load_model(const string &path) {
    ifstream word_model(path);
    if (!word_model.is_open()) {
        cout << "can not open word model file" << endl;
        cout << strerror(errno) << endl;
        exit(0);
    }
    boost::archive::binary_iarchive ia(word_model);
    ia >> model;
    this->dimension = int(model.begin()->second.size());
    word_model.close();
}

void WordModel::save_model(const string &path) {
    ofstream word_model(path);
    if (!word_model.is_open()) {
        cout << "can not open word model file" << endl;
        cout << strerror(errno) << endl;
        exit(0);
    }
    boost::archive::binary_oarchive oa(word_model);
    oa << model;
    word_model.close();
}
