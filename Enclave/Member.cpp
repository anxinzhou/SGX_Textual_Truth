//
// Created by anxin on 2020-04-07.
//

#include "Member.h"

//true negative, false positive, false negative, true positive
const int User::ALPHA[4] = { 90, 10, 50, 50 };

const int Question::BETA[2] = { 10, 10 };

vector<string> Answer::string_split(const string &s) {
	vector<string> tokens;
	// skip header space
	int i = 0;
	int s_size = int(s.size());
	while (i != s_size) {
		while (i != s_size && s[i] == ' ')
			i++; // skip header space;
		int end_loc = s.find(' ', i);
		if (end_loc == string::npos) {
			// no space any more
			end_loc = int(s.size());
		}
		tokens.push_back(s.substr(i, end_loc - i));
		i = end_loc;
	}
	return tokens;
}

vector<Keyword> Answer::to_keywords(const unordered_set<string> &words_filter,
		WordModel &word_model) {
	// replace punc and lower alphabetic
	vector<Keyword> keywords;
	for (auto &c : raw_answer) {
		if (std::ispunct(c)) {
			c = ' ';
			continue;
		}
		c = std::tolower(c);
	}

	auto tokens = string_split(raw_answer);

	// extract keywords
	// only remove stopwords here, may use doman specific dictionary
	// here skip words not in dictionary
	for (auto token : tokens) {
		if (words_filter.count(token) == 1 || !word_model.contain(token)) {
//                    cout << token << endl;
			continue;
		}
		keywords.emplace_back(owner_id, question_id, std::move(token));
	}
	return keywords;
}
vector<Keyword> Answer::to_keywords() {
	// replace punc
	vector<Keyword> keywords;
	for (auto &c : raw_answer) {
		if (std::ispunct(c)) {
			c = ' ';
			continue;
		}
		c = std::tolower(c);
	}
	auto tokens = string_split(raw_answer);
	keywords.reserve(tokens.size());
	for (auto token : tokens) {
		keywords.emplace_back(owner_id, question_id, std::move(token));
	}
	return keywords;
}

