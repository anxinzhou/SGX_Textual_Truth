#include "Enclave_u.h"
#include <errno.h>

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

static sgx_status_t SGX_CDECL Enclave_ocall_query_solution_size(void* pms)
{
	ms_ocall_query_solution_size_t* ms = SGX_CAST(ms_ocall_query_solution_size_t*, pms);
	ocall_query_solution_size(ms->ms_question_id, ms->ms_user_solution_size, ms->ms_user_num);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_load_question_solutions(void* pms)
{
	ms_ocall_load_question_solutions_t* ms = SGX_CAST(ms_ocall_load_question_solutions_t*, pms);
	ocall_load_question_solutions(ms->ms_question_id, ms->ms_solutions, ms->ms_len);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_load_words_vectors(void* pms)
{
	ms_ocall_load_words_vectors_t* ms = SGX_CAST(ms_ocall_load_words_vectors_t*, pms);
	ocall_load_words_vectors(ms->ms_raw_words, ms->ms_words_size, ms->ms_vectors, ms->ms_word_num, ms->ms_vec_len);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_filter_keywords(void* pms)
{
	ms_ocall_filter_keywords_t* ms = SGX_CAST(ms_ocall_filter_keywords_t*, pms);
	ocall_filter_keywords(ms->ms_raw_words, ms->ms_words_size, ms->ms_words_filter_ind, ms->ms_word_num);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_print_string(void* pms)
{
	ms_ocall_print_string_t* ms = SGX_CAST(ms_ocall_print_string_t*, pms);
	ocall_print_string(ms->ms_string);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_print_float_array(void* pms)
{
	ms_ocall_print_float_array_t* ms = SGX_CAST(ms_ocall_print_float_array_t*, pms);
	ocall_print_float_array(ms->ms_arr, ms->ms_len);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_print_int_array(void* pms)
{
	ms_ocall_print_int_array_t* ms = SGX_CAST(ms_ocall_print_int_array_t*, pms);
	ocall_print_int_array(ms->ms_arr, ms->ms_len);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[7];
} ocall_table_Enclave = {
	7,
	{
		(void*)Enclave_ocall_query_solution_size,
		(void*)Enclave_ocall_load_question_solutions,
		(void*)Enclave_ocall_load_words_vectors,
		(void*)Enclave_ocall_filter_keywords,
		(void*)Enclave_ocall_print_string,
		(void*)Enclave_ocall_print_float_array,
		(void*)Enclave_ocall_print_int_array,
	}
};
sgx_status_t ecall_ttruth(sgx_enclave_id_t eid, int* question_top_k_user, int question_num, int user_num, int top_k, int cluster_size, int vector_size, int cnt)
{
	sgx_status_t status;
	ms_ecall_ttruth_t ms;
	ms.ms_question_top_k_user = question_top_k_user;
	ms.ms_question_num = question_num;
	ms.ms_user_num = user_num;
	ms.ms_top_k = top_k;
	ms.ms_cluster_size = cluster_size;
	ms.ms_vector_size = vector_size;
	ms.ms_cnt = cnt;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_oblivious_ttruth(sgx_enclave_id_t eid, int* question_top_k_user, int question_num, int user_num, int top_k, int cluster_size, int vector_size, int cnt)
{
	sgx_status_t status;
	ms_ecall_oblivious_ttruth_t ms;
	ms.ms_question_top_k_user = question_top_k_user;
	ms.ms_question_num = question_num;
	ms.ms_user_num = user_num;
	ms.ms_top_k = top_k;
	ms.ms_cluster_size = cluster_size;
	ms.ms_vector_size = vector_size;
	ms.ms_cnt = cnt;
	status = sgx_ecall(eid, 1, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t test(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 2, &ocall_table_Enclave, NULL);
	return status;
}

