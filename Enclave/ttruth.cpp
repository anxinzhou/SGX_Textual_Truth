//
// Created by anxin on 2020-04-07.
//

#include "ttruth.h"

inline float distance_metrics(const WordVec &a, const WordVec &b) {
	return 2 - 2 * hpc::dot_product(a, b);
}

void sphere_kmeans(vector<Keyword> &keywords, WordModel &word_model,
		int cluster_num, int max_iter, double tol) {
	int dimension = 0;

	if (keywords.size() != 0)
		dimension = word_model.dimension;
	// init with kmeans ++
	vector<WordVec> clusters = kmeans_init(keywords, word_model, cluster_num);

	vector<const WordVec*> keyword_vec;
	keyword_vec.reserve(keywords.size());

	for (int i = 0; i < keywords.size(); i++) {
		auto vec = &word_model.get_vec(keywords[i].content);
		//ocall_print_float_array(&vec[0],vec.size());
		if (vec->size() != word_model.dimension) {
			throw "wrong dimension";
		}
		keyword_vec.push_back(vec);
	}

	for (int t = 0; t < max_iter; t++) {
		// e step
		// assign new cluster
		for (int i = 0; i < keywords.size(); i++) {
			auto &keyword = keywords[i];
			float max_prob = INT_MIN;
			int max_index = -1;
			for (int k = 0; k < clusters.size(); k++) {
				float prob = hpc::dot_product(clusters[k], *keyword_vec[i]);

				if (prob > max_prob) {
					max_prob = prob;
					max_index = k;
				}
			}
			keyword.cluster_assignment = max_index;
		}
		// M step
		// update parameters
		vector<WordVec> new_clusters(cluster_num, WordVec(dimension, 0));
		for (int i = 0; i < keywords.size(); i++) {
			auto &keyword = keywords[i];
			int cluster_assignment = keyword.cluster_assignment;
			hpc::vector_add_inplace(new_clusters[cluster_assignment],
					*keyword_vec[i]);
		}

		//normalize mu
		for (int i = 0; i < cluster_num; i++) {
			auto &mu = new_clusters[i];
			float mu_l2 = sqrt(hpc::dot_product(mu, mu));
			if (mu_l2 < 1e-8)
				continue; //
			hpc::vector_mul_inplace(mu, 1.0 / mu_l2);
		}
		// check stop tol
		double cur_tol = 0;
		for (int i = 0; i < cluster_num; i++) {
			WordVec diff = hpc::vector_sub(new_clusters[i], clusters[i]);
			double square_norm = sqrt(hpc::dot_product(diff, diff));
			cur_tol += square_norm;
		}

		if (cur_tol < tol) {
//            cout << "sphere clustering converge at iteration " << t << endl;
			break;
		}

		//set new cluster
		for (int i = 0; i < cluster_num; i++) {
			clusters[i] = std::move(new_clusters[i]);
		}
	}

	// see how many words each cluster have
	/*vector<int> cluster_histogram(cluster_num,0);
	 string countinfo;
	 for(int k=0;k<keywords.size();k++) {
	 cluster_histogram[keywords[k].cluster_assignment]+=1;

	 }

	 for(int k=0;k<cluster_histogram.size();k++) {
	 countinfo+=to_string(cluster_histogram[k])+" ";
	 }
	 countinfo+="\n";
	 ocall_print_string(countinfo.c_str());*/
}

void observation_update(vector<Observation> &observations,
		vector<Keyword> &keywords) {
	for (auto &keyword : keywords) {
		int owner_id = keyword.owner_id;
		int cluster_assignment = keyword.cluster_assignment;
		observations[owner_id][cluster_assignment] = 1;
	}
}

vector<int> get_solution_provide_indicator(vector<Observation> &observations) {
	int user_num = observations.size();
	vector<int> sol_provide_ind(user_num);
	for (int j = 0; j < user_num; j++) {
		int total_observations = std::accumulate(observations[j].begin(),
				observations[j].end(), 0);
		if (total_observations == 0) {
			sol_provide_ind[j] = 0;
		} else {
			sol_provide_ind[j] = 1;
		}
	}
	return sol_provide_ind;
}

vector<vector<int>> latent_truth_model(
		vector<vector<Observation>> &question_observations, int max_iter) {
	//random initialize truth indicator of each question and user prior count
	int question_num = question_observations.size();
	if (question_num == 0)
		throw "question size should be larger than 0";
	int user_num = question_observations[0].size();
	if (user_num == 0)
		throw "user size be larger than 0";
	int key_factor_num = question_observations[0][0].size();
	vector<vector<int>> question_indicator(question_num,
			vector<int>(key_factor_num, 0));
	vector<PriorCount> user_prior_count(user_num, PriorCount(4));
	for (int question_id = 0; question_id < question_num; question_id++) {
		auto &observations = question_observations[question_id];
		// an space efficient way to decide if user has provide solutions for the question
		vector<int> sol_provide_ind = get_solution_provide_indicator(
				observations);
		// update prior count of each user
		for (int j = 0; j < key_factor_num; j++) {
			uint32_t random_num = 0;
			sgx_status_t ret = sgx_read_rand((unsigned char*) &random_num, 2);
			if (ret != SGX_SUCCESS) {
				throw("can not read randomness");
			}
			int indicator = random_num % 2;
			question_indicator[question_id][j] = indicator;
			for (int k = 0; k < user_num; k++) {
				// if user does not provide solution for the question
				if (sol_provide_ind[k] == 0)
					continue;
				int ob = observations[k][j];
				user_prior_count[k][indicator * 2 + ob] += 1;
			}
		}
	}

	// gibbs sampling to infer truth
	for (int t = 0; t < max_iter; t++) {
		// update truth indicator
		for (int question_id = 0; question_id < question_num; question_id++) {
			auto &observations = question_observations[question_id];
			auto &truth_indicator = question_indicator[question_id];
			vector<int> sol_provide_ind = get_solution_provide_indicator(
					observations);
			for (int j = 0; j < key_factor_num; j++) {
				double p[2] = { log(Question::BETA[0]), log(Question::BETA[1]) };
				int old_indicator = truth_indicator[j];
				for (int indicator = 0; indicator <= 1; ++indicator) {

					for (int k = 0; k < user_num; k++) {
						if (sol_provide_ind[k] == 0)
							continue; //user doest not offer observation
						int observation = observations[k][j];
						int alpha_observe = User::get_alpha(indicator,
								observation);
						int alpha_observe0 = User::get_alpha(indicator, 0);
						int alpha_observe1 = User::get_alpha(indicator, 1);
						int prior_count_observe = user_prior_count[k][indicator
								* 2 + observation];

						int prior_count_observe0 = user_prior_count[k][indicator
								* 2];
						int prior_count_observe1 = user_prior_count[k][indicator
								* 2 + 1];
						if (old_indicator == indicator) {
							prior_count_observe -= 1;
						}
						p[indicator] += log(
								(double) (prior_count_observe + alpha_observe)
										/ (alpha_observe0 + alpha_observe1
												+ prior_count_observe0
												+ prior_count_observe1));
					}
				}
				// assign 1 or 0 to truth indicator according to p
				//                truth_indicator[j] = p[0] > p[1] ? 0:1;
				//                 normalize probability, adjust the larger one to log 1
				double diff = 0;
				diff = p[0] > p[1] ? -p[0] : -p[1];
				p[0] += diff;
				p[1] += diff;
				p[0] = exp(p[0]);
				p[1] = exp(p[1]);

				uint32_t random_num = 0;
				sgx_status_t ret = sgx_read_rand((unsigned char*) &random_num,
						2);
				if (ret != SGX_SUCCESS) {
					throw("can not read randomness");
				}

				double threshold = (double) random_num / ((1 << 16) - 1);
				int new_indicator = threshold < p[0] / (p[0] + p[1]) ? 0 : 1;
				if (old_indicator != new_indicator) {
					truth_indicator[j] = new_indicator;
					for (int k = 0; k < user_num; k++) {
						if (sol_provide_ind[k] == 0)
							continue;
						int observation = observations[k][j];
						user_prior_count[k][new_indicator * 2 + observation] +=
								1;
						user_prior_count[k][old_indicator * 2 + observation] -=
								1;
					}
				}
			}
		}
	}
	/*for(int i=0;i<user_num;i++) {
	 ocall_print_int_array(&user_prior_count[i][0], 4);
	 }*/
	return question_indicator;
}

vector<WordVec> kmeans_init(vector<Keyword> &keywords, WordModel &word_model,
		int cluster_num) {
	/*for(auto &kw:keywords){
	 ocall_print_string((kw.content+" ").c_str());
	 }
	 ocall_print_string("\n");*/

	vector<const WordVec*> keyword_vec;
	keyword_vec.reserve(keywords.size());

	for (int i = 0; i < keywords.size(); i++) {
		auto vec = &word_model.get_vec(keywords[i].content);
		//ocall_print_float_array(&vec[0],vec.size());
		if (vec->size() != word_model.dimension) {
			throw "wrong dimension";
		}
		keyword_vec.push_back(vec);
	}

	/*for(int i=0; i<keywords.size();i++) {
	 ocall_print_string((to_string(keyword_vec[i]->size())+"\n").c_str());
	 }*/

	vector<WordVec> clusters;
	uint32_t random_num = 0;
	sgx_status_t ret = sgx_read_rand((unsigned char*) &random_num, 4);
	if (ret != SGX_SUCCESS) {
		throw("can not read randomness");
	}
	int first_index = random_num % keywords.size();
	auto first_cluster = *(keyword_vec[first_index]);
	clusters.push_back(first_cluster);

	vector<float> min_dis_cache(keywords.size(), INT_MAX);
	vector<float> cluster_distance(keywords.size());
	for (int i = 1; i < cluster_num; i++) {
		float total_dis = 0;
		for (int j = 0; j < keywords.size(); j++) {
			float min_dis = min_dis_cache[j];
			auto &kv = *(keyword_vec[j]);
			auto &cv = clusters[i - 1];
			int kv_size = kv.size();
			int cv_size = cv.size();
			if (kv.size() != cv.size() || kv.size() != word_model.dimension)
				throw "wrong size";
			float dis = distance_metrics(kv, cv);
			min_dis = min(min_dis, dis);
			min_dis_cache[j] = min_dis;

			cluster_distance[j] = min_dis;
			total_dis += min_dis;
		}
		// normalize distance
		for (auto &cd : cluster_distance) {
			cd /= total_dis;
		}
		// sample from distribution
		for (int j = 1; j < cluster_distance.size(); j++) {
			cluster_distance[j] += cluster_distance[j - 1];
		}
		cluster_distance[cluster_distance.size() - 1] = 1;
		uint32_t random_num = 0;
		sgx_status_t ret = sgx_read_rand((unsigned char*) &random_num, 2);
		if (ret != SGX_SUCCESS) {
			throw("can not read randomness");
		}
		double prob = (double) random_num / ((1 << 16) - 1);
		int cluster_index = -1;
		if (prob < cluster_distance[0])
			cluster_index = 0;
		else {
			for (int j = 1; j < cluster_distance.size(); j++) {
				if (prob >= cluster_distance[j - 1]
						&& prob < cluster_distance[j]) {
					cluster_index = j;
					break;
				}
			}
		}
//        cout<< cluster_index<<endl;
		clusters.push_back(*keyword_vec[cluster_index]);
	}
	return clusters;
}

vector<int> weighted_sphere_kmeans(vector<string> &kws, vector<int> weight,
		WordModel &word_model, int cluster_num, int max_iter, double tol) {
	int dimension = 0;
	vector<int> cluster_assignment(kws.size());
	dimension = word_model.dimension;

	// init with kmeans ++
	vector<WordVec> clusters = weighted_kmeans_init(kws, weight, word_model,
			cluster_num);

	vector<const WordVec*> keyword_vec;
	keyword_vec.reserve(kws.size());

	for (int i = 0; i < kws.size(); i++) {
		auto vec = &word_model.get_vec(kws[i]);
		//ocall_print_float_array(&vec[0],vec.size());
		if (vec->size() != word_model.dimension) {
			throw "wrong dimension";
		}
		keyword_vec.push_back(vec);
	}

	for (int t=0; t < max_iter; t++) {
		// e step
		// assign new cluster
		for (int i = 0; i < kws.size(); i++) {

			float max_prob = INT_MIN;
			int max_index = -1;
			for (int k = 0; k < clusters.size(); k++) {
				float prob = hpc::dot_product(clusters[k], *keyword_vec[i]);

				if (prob > max_prob) {
					max_prob = prob;
					max_index = k;
				}
			}
			cluster_assignment[i] = max_index;
		}
		// M step
		// update parameters
		vector<WordVec> new_clusters(cluster_num, WordVec(dimension, 0));
		for (int i = 0; i < kws.size(); i++) {
			auto &keyword = kws[i];
			int cluster_ind = cluster_assignment[i];
			auto &new_cluster = new_clusters[cluster_ind];
			auto &kv = *keyword_vec[i];
			for (int k = 0; k < kv.size(); k++) {
				new_cluster[k] += 3 * kv[k];
			}
		}

		//normalize mu
		for (int i = 0; i < cluster_num; i++) {
			auto &mu = new_clusters[i];
			float mu_l2 = sqrt(hpc::dot_product(mu, mu));
			if (mu_l2 < 1e-8)
				continue; //
			hpc::vector_mul_inplace(mu, 1.0 / mu_l2);
		}
		// check stop tol
		double cur_tol = 0;
		for (int i = 0; i < cluster_num; i++) {
			WordVec diff = hpc::vector_sub(new_clusters[i], clusters[i]);
			double square_norm = sqrt(hpc::dot_product(diff, diff));
			cur_tol += square_norm;
		}

		if (cur_tol < tol) {
			//            cout << "sphere clustering converge at iteration " << t << endl;
			break;
		}

		//set new cluster
		for (int i = 0; i < cluster_num; i++) {
			clusters[i] = std::move(new_clusters[i]);
		}

		if (t==max_iter-1) {
			ocall_print_string("kmeans early quit without converge\n");
		}
	}

	// see how many words each cluster have
	/*vector<int> cluster_histogram(cluster_num,0);
	 string countinfo;
	 for(int k=0;k<keywords.size();k++) {
	 cluster_histogram[keywords[k].cluster_assignment]+=1;

	 }

	 for(int k=0;k<cluster_histogram.size();k++) {
	 countinfo+=to_string(cluster_histogram[k])+" ";
	 }
	 countinfo+="\n";
	 ocall_print_string(countinfo.c_str());*/
	return cluster_assignment;
}

vector<WordVec> weighted_kmeans_init(vector<string> &kws, vector<int> weight,
		WordModel &word_model, int cluster_num) {
	vector<const WordVec*> keyword_vec;
	keyword_vec.reserve(kws.size());

	for (int i = 0; i < kws.size(); i++) {
		auto vec = &word_model.get_vec(kws[i]);
		//ocall_print_float_array(&vec[0],vec.size());
		if (vec->size() != word_model.dimension) {
			throw "wrong dimension";
		}
		keyword_vec.push_back(vec);
	}

	/*for(int i=0; i<keywords.size();i++) {
	 ocall_print_string((to_string(keyword_vec[i]->size())+"\n").c_str());
	 }*/

	vector<WordVec> clusters;

	int first_index = myRand::myrand_int() % kws.size();
	auto first_cluster = *(keyword_vec[first_index]);
	clusters.push_back(first_cluster);

	vector<float> min_dis_cache(kws.size(), INT_MAX);
	vector<float> cluster_distance(kws.size());
	for (int i = 1; i < cluster_num; i++) {
		float total_dis = 0;
		for (int j = 0; j < kws.size(); j++) {
			float min_dis = min_dis_cache[j];
			auto &kv = *(keyword_vec[j]);
			auto &cv = clusters[i - 1];
			int kv_size = kv.size();
			int cv_size = cv.size();
			if (kv.size() != cv.size() || kv.size() != word_model.dimension)
				throw "wrong size";
			float dis = distance_metrics(kv, cv);
			min_dis = min(min_dis, dis);
			min_dis_cache[j] = min_dis;
			int w = weight[j];
			cluster_distance[j] = w * min_dis;
			total_dis += cluster_distance[j];
		}
		if(total_dis<1e-10) total_dis = 1e-10;
		// normalize distance
		for (auto &cd : cluster_distance) {
			cd /= total_dis;
		}
		// sample from distribution
		for (int j = 1; j < cluster_distance.size(); j++) {
			cluster_distance[j] += cluster_distance[j - 1];
		}
		cluster_distance[cluster_distance.size() - 1] = 1;

		double prob = myRand::myrandom();
		int cluster_index = -1;
		if (prob <= cluster_distance[0])
			cluster_index = 0;
		else {
			for (int j = 1; j < cluster_distance.size(); j++) {
				if (prob > cluster_distance[j - 1]
						&& prob <= cluster_distance[j]) {
					cluster_index = j;
					break;
				}
			}
		}
		//        cout<< cluster_index<<endl;
		clusters.push_back(*keyword_vec[cluster_index]);
	}
	return clusters;

}
