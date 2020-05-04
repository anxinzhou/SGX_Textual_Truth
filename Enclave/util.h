//
// Created by anxin on 2020-02-25.
//

#ifndef TEXTTRUTH_UTIL_H
#define TEXTTRUTH_UTIL_H

#include <sgx_intrin.h>
#include <vector>

namespace myRand {
	static unsigned long next = 1;
	const unsigned int MY_RAND_MAX = 32768;

	/* RAND_MAX assumed to be 32767 */
	inline int myrand_int(void) {
		next = next * 1103515245 + 12345;
		return((unsigned)(next/65536) % 32768);
	}

	inline double myrandom() {
		return (double) myrand_int()/MY_RAND_MAX;
	}

	inline void mysrand(unsigned seed) {
	    next = seed;
	}
}

namespace hpc {
    inline bool array_equal(std::vector<int> &a, std::vector<int> &b);

    float dot_product(const std::vector<float> &p, const std::vector<float> &q);

    float dot_product(float *p, float *q, int length);

    void vector_mul_inplace(std::vector<float> &p, float v);

    // add p+q and store result in p
    void vector_add_inplace(std::vector<float> &p, const std::vector<float> &q);

    // sub p-q and store result in p
    void vector_sub_inplace(std::vector<float> &p, const std::vector<float> &q);

    // sub p-q and store result in p
    std::vector<float> vector_sub(const std::vector<float> &p, const std::vector<float> &q);



    bool vector_cmpeq(std::vector<int> &p, std::vector<int> &q);
}
#endif //TEXTTRUTH_UTIL_H
