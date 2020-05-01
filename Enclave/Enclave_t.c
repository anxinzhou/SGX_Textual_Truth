#include "Enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */
#include "sgx_lfence.h" /* for sgx_lfence */

#include <errno.h>
#include <mbusafecrt.h> /* for memcpy_s etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_ENCLAVE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_within_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define ADD_ASSIGN_OVERFLOW(a, b) (	\
	((a) += (b)) < (b)	\
)


typedef struct ms_ecall_ttruth_t {
	int* ms_question_top_k_user;
	int ms_question_num;
	int ms_user_num;
	int ms_top_k;
	int ms_cluster_size;
	int ms_vector_size;
	int ms_cnt;
} ms_ecall_ttruth_t;

typedef struct ms_ecall_oblivious_ttruth_t {
	int* ms_question_top_k_user;
	int ms_question_num;
	int ms_user_num;
	int ms_top_k;
	int ms_cluster_size;
	int ms_vector_size;
	int ms_cnt;
} ms_ecall_oblivious_ttruth_t;

typedef struct ms_ocall_query_solution_size_t {
	int ms_question_id;
	int* ms_user_solution_size;
	int ms_user_num;
} ms_ocall_query_solution_size_t;

typedef struct ms_ocall_load_question_solutions_t {
	int ms_question_id;
	char* ms_solutions;
	int ms_len;
} ms_ocall_load_question_solutions_t;

typedef struct ms_ocall_load_words_vectors_t {
	char* ms_raw_words;
	int* ms_words_size;
	float* ms_vectors;
	int ms_word_num;
	int ms_vec_len;
} ms_ocall_load_words_vectors_t;

typedef struct ms_ocall_filter_keywords_t {
	char* ms_raw_words;
	int* ms_words_size;
	int* ms_words_filter_ind;
	int ms_word_num;
} ms_ocall_filter_keywords_t;

typedef struct ms_ocall_print_string_t {
	const char* ms_string;
} ms_ocall_print_string_t;

typedef struct ms_ocall_print_float_array_t {
	float* ms_arr;
	int ms_len;
} ms_ocall_print_float_array_t;

typedef struct ms_ocall_print_int_array_t {
	int* ms_arr;
	int ms_len;
} ms_ocall_print_int_array_t;

static sgx_status_t SGX_CDECL sgx_ecall_ttruth(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_ttruth_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_ttruth_t* ms = SGX_CAST(ms_ecall_ttruth_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_question_top_k_user = ms->ms_question_top_k_user;
	int _tmp_cnt = ms->ms_cnt;
	size_t _len_question_top_k_user = _tmp_cnt * sizeof(int);
	int* _in_question_top_k_user = NULL;

	if (sizeof(*_tmp_question_top_k_user) != 0 &&
		(size_t)_tmp_cnt > (SIZE_MAX / sizeof(*_tmp_question_top_k_user))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_question_top_k_user, _len_question_top_k_user);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_question_top_k_user != NULL && _len_question_top_k_user != 0) {
		if ( _len_question_top_k_user % sizeof(*_tmp_question_top_k_user) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_question_top_k_user = (int*)malloc(_len_question_top_k_user)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_question_top_k_user, 0, _len_question_top_k_user);
	}

	ecall_ttruth(_in_question_top_k_user, ms->ms_question_num, ms->ms_user_num, ms->ms_top_k, ms->ms_cluster_size, ms->ms_vector_size, _tmp_cnt);
	if (_in_question_top_k_user) {
		if (memcpy_s(_tmp_question_top_k_user, _len_question_top_k_user, _in_question_top_k_user, _len_question_top_k_user)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_question_top_k_user) free(_in_question_top_k_user);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_oblivious_ttruth(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_oblivious_ttruth_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_oblivious_ttruth_t* ms = SGX_CAST(ms_ecall_oblivious_ttruth_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_question_top_k_user = ms->ms_question_top_k_user;
	int _tmp_cnt = ms->ms_cnt;
	size_t _len_question_top_k_user = _tmp_cnt * sizeof(int);
	int* _in_question_top_k_user = NULL;

	if (sizeof(*_tmp_question_top_k_user) != 0 &&
		(size_t)_tmp_cnt > (SIZE_MAX / sizeof(*_tmp_question_top_k_user))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_question_top_k_user, _len_question_top_k_user);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_question_top_k_user != NULL && _len_question_top_k_user != 0) {
		if ( _len_question_top_k_user % sizeof(*_tmp_question_top_k_user) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_question_top_k_user = (int*)malloc(_len_question_top_k_user)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_question_top_k_user, 0, _len_question_top_k_user);
	}

	ecall_oblivious_ttruth(_in_question_top_k_user, ms->ms_question_num, ms->ms_user_num, ms->ms_top_k, ms->ms_cluster_size, ms->ms_vector_size, _tmp_cnt);
	if (_in_question_top_k_user) {
		if (memcpy_s(_tmp_question_top_k_user, _len_question_top_k_user, _in_question_top_k_user, _len_question_top_k_user)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_question_top_k_user) free(_in_question_top_k_user);
	return status;
}

static sgx_status_t SGX_CDECL sgx_test(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	test();
	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[3];
} g_ecall_table = {
	3,
	{
		{(void*)(uintptr_t)sgx_ecall_ttruth, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_oblivious_ttruth, 0, 0},
		{(void*)(uintptr_t)sgx_test, 0, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[7][3];
} g_dyn_entry_table = {
	7,
	{
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL ocall_query_solution_size(int question_id, int* user_solution_size, int user_num)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_user_solution_size = user_num * sizeof(int);

	ms_ocall_query_solution_size_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_query_solution_size_t);
	void *__tmp = NULL;

	void *__tmp_user_solution_size = NULL;

	CHECK_ENCLAVE_POINTER(user_solution_size, _len_user_solution_size);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (user_solution_size != NULL) ? _len_user_solution_size : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_query_solution_size_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_query_solution_size_t));
	ocalloc_size -= sizeof(ms_ocall_query_solution_size_t);

	ms->ms_question_id = question_id;
	if (user_solution_size != NULL) {
		ms->ms_user_solution_size = (int*)__tmp;
		__tmp_user_solution_size = __tmp;
		if (_len_user_solution_size % sizeof(*user_solution_size) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset(__tmp_user_solution_size, 0, _len_user_solution_size);
		__tmp = (void *)((size_t)__tmp + _len_user_solution_size);
		ocalloc_size -= _len_user_solution_size;
	} else {
		ms->ms_user_solution_size = NULL;
	}
	
	ms->ms_user_num = user_num;
	status = sgx_ocall(0, ms);

	if (status == SGX_SUCCESS) {
		if (user_solution_size) {
			if (memcpy_s((void*)user_solution_size, _len_user_solution_size, __tmp_user_solution_size, _len_user_solution_size)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_load_question_solutions(int question_id, char* solutions, int len)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_solutions = len * sizeof(char);

	ms_ocall_load_question_solutions_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_load_question_solutions_t);
	void *__tmp = NULL;

	void *__tmp_solutions = NULL;

	CHECK_ENCLAVE_POINTER(solutions, _len_solutions);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (solutions != NULL) ? _len_solutions : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_load_question_solutions_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_load_question_solutions_t));
	ocalloc_size -= sizeof(ms_ocall_load_question_solutions_t);

	ms->ms_question_id = question_id;
	if (solutions != NULL) {
		ms->ms_solutions = (char*)__tmp;
		__tmp_solutions = __tmp;
		if (_len_solutions % sizeof(*solutions) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset(__tmp_solutions, 0, _len_solutions);
		__tmp = (void *)((size_t)__tmp + _len_solutions);
		ocalloc_size -= _len_solutions;
	} else {
		ms->ms_solutions = NULL;
	}
	
	ms->ms_len = len;
	status = sgx_ocall(1, ms);

	if (status == SGX_SUCCESS) {
		if (solutions) {
			if (memcpy_s((void*)solutions, _len_solutions, __tmp_solutions, _len_solutions)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_load_words_vectors(char* raw_words, int* words_size, float* vectors, int word_num, int vec_len)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_raw_words = raw_words ? strlen(raw_words) + 1 : 0;
	size_t _len_words_size = word_num * sizeof(int);
	size_t _len_vectors = vec_len * sizeof(float);

	ms_ocall_load_words_vectors_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_load_words_vectors_t);
	void *__tmp = NULL;

	void *__tmp_vectors = NULL;

	CHECK_ENCLAVE_POINTER(raw_words, _len_raw_words);
	CHECK_ENCLAVE_POINTER(words_size, _len_words_size);
	CHECK_ENCLAVE_POINTER(vectors, _len_vectors);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (raw_words != NULL) ? _len_raw_words : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (words_size != NULL) ? _len_words_size : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (vectors != NULL) ? _len_vectors : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_load_words_vectors_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_load_words_vectors_t));
	ocalloc_size -= sizeof(ms_ocall_load_words_vectors_t);

	if (raw_words != NULL) {
		ms->ms_raw_words = (char*)__tmp;
		if (_len_raw_words % sizeof(*raw_words) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, raw_words, _len_raw_words)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_raw_words);
		ocalloc_size -= _len_raw_words;
	} else {
		ms->ms_raw_words = NULL;
	}
	
	if (words_size != NULL) {
		ms->ms_words_size = (int*)__tmp;
		if (_len_words_size % sizeof(*words_size) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, words_size, _len_words_size)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_words_size);
		ocalloc_size -= _len_words_size;
	} else {
		ms->ms_words_size = NULL;
	}
	
	if (vectors != NULL) {
		ms->ms_vectors = (float*)__tmp;
		__tmp_vectors = __tmp;
		if (_len_vectors % sizeof(*vectors) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset(__tmp_vectors, 0, _len_vectors);
		__tmp = (void *)((size_t)__tmp + _len_vectors);
		ocalloc_size -= _len_vectors;
	} else {
		ms->ms_vectors = NULL;
	}
	
	ms->ms_word_num = word_num;
	ms->ms_vec_len = vec_len;
	status = sgx_ocall(2, ms);

	if (status == SGX_SUCCESS) {
		if (vectors) {
			if (memcpy_s((void*)vectors, _len_vectors, __tmp_vectors, _len_vectors)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_filter_keywords(char* raw_words, int* words_size, int* words_filter_ind, int word_num)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_raw_words = raw_words ? strlen(raw_words) + 1 : 0;
	size_t _len_words_size = word_num * sizeof(int);
	size_t _len_words_filter_ind = word_num * sizeof(int);

	ms_ocall_filter_keywords_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_filter_keywords_t);
	void *__tmp = NULL;

	void *__tmp_words_filter_ind = NULL;

	CHECK_ENCLAVE_POINTER(raw_words, _len_raw_words);
	CHECK_ENCLAVE_POINTER(words_size, _len_words_size);
	CHECK_ENCLAVE_POINTER(words_filter_ind, _len_words_filter_ind);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (raw_words != NULL) ? _len_raw_words : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (words_size != NULL) ? _len_words_size : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (words_filter_ind != NULL) ? _len_words_filter_ind : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_filter_keywords_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_filter_keywords_t));
	ocalloc_size -= sizeof(ms_ocall_filter_keywords_t);

	if (raw_words != NULL) {
		ms->ms_raw_words = (char*)__tmp;
		if (_len_raw_words % sizeof(*raw_words) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, raw_words, _len_raw_words)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_raw_words);
		ocalloc_size -= _len_raw_words;
	} else {
		ms->ms_raw_words = NULL;
	}
	
	if (words_size != NULL) {
		ms->ms_words_size = (int*)__tmp;
		if (_len_words_size % sizeof(*words_size) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, words_size, _len_words_size)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_words_size);
		ocalloc_size -= _len_words_size;
	} else {
		ms->ms_words_size = NULL;
	}
	
	if (words_filter_ind != NULL) {
		ms->ms_words_filter_ind = (int*)__tmp;
		__tmp_words_filter_ind = __tmp;
		if (_len_words_filter_ind % sizeof(*words_filter_ind) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset(__tmp_words_filter_ind, 0, _len_words_filter_ind);
		__tmp = (void *)((size_t)__tmp + _len_words_filter_ind);
		ocalloc_size -= _len_words_filter_ind;
	} else {
		ms->ms_words_filter_ind = NULL;
	}
	
	ms->ms_word_num = word_num;
	status = sgx_ocall(3, ms);

	if (status == SGX_SUCCESS) {
		if (words_filter_ind) {
			if (memcpy_s((void*)words_filter_ind, _len_words_filter_ind, __tmp_words_filter_ind, _len_words_filter_ind)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_print_string(const char* string)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_string = string ? strlen(string) + 1 : 0;

	ms_ocall_print_string_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_print_string_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(string, _len_string);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (string != NULL) ? _len_string : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_print_string_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_print_string_t));
	ocalloc_size -= sizeof(ms_ocall_print_string_t);

	if (string != NULL) {
		ms->ms_string = (const char*)__tmp;
		if (_len_string % sizeof(*string) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, string, _len_string)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_string);
		ocalloc_size -= _len_string;
	} else {
		ms->ms_string = NULL;
	}
	
	status = sgx_ocall(4, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_print_float_array(float* arr, int len)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_arr = len * sizeof(float);

	ms_ocall_print_float_array_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_print_float_array_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(arr, _len_arr);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (arr != NULL) ? _len_arr : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_print_float_array_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_print_float_array_t));
	ocalloc_size -= sizeof(ms_ocall_print_float_array_t);

	if (arr != NULL) {
		ms->ms_arr = (float*)__tmp;
		if (_len_arr % sizeof(*arr) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, arr, _len_arr)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_arr);
		ocalloc_size -= _len_arr;
	} else {
		ms->ms_arr = NULL;
	}
	
	ms->ms_len = len;
	status = sgx_ocall(5, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_print_int_array(int* arr, int len)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_arr = len * sizeof(int);

	ms_ocall_print_int_array_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_print_int_array_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(arr, _len_arr);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (arr != NULL) ? _len_arr : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_print_int_array_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_print_int_array_t));
	ocalloc_size -= sizeof(ms_ocall_print_int_array_t);

	if (arr != NULL) {
		ms->ms_arr = (int*)__tmp;
		if (_len_arr % sizeof(*arr) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, arr, _len_arr)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_arr);
		ocalloc_size -= _len_arr;
	} else {
		ms->ms_arr = NULL;
	}
	
	ms->ms_len = len;
	status = sgx_ocall(6, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

