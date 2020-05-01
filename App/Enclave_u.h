#ifndef ENCLAVE_U_H__
#define ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OCALL_QUERY_SOLUTION_SIZE_DEFINED__
#define OCALL_QUERY_SOLUTION_SIZE_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_query_solution_size, (int question_id, int* user_solution_size, int user_num));
#endif
#ifndef OCALL_LOAD_QUESTION_SOLUTIONS_DEFINED__
#define OCALL_LOAD_QUESTION_SOLUTIONS_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_load_question_solutions, (int question_id, char* solutions, int len));
#endif
#ifndef OCALL_LOAD_WORDS_VECTORS_DEFINED__
#define OCALL_LOAD_WORDS_VECTORS_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_load_words_vectors, (char* raw_words, int* words_size, float* vectors, int word_num, int vec_len));
#endif
#ifndef OCALL_FILTER_KEYWORDS_DEFINED__
#define OCALL_FILTER_KEYWORDS_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_filter_keywords, (char* raw_words, int* words_size, int* words_filter_ind, int word_num));
#endif
#ifndef OCALL_PRINT_STRING_DEFINED__
#define OCALL_PRINT_STRING_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print_string, (const char* string));
#endif
#ifndef OCALL_PRINT_FLOAT_ARRAY_DEFINED__
#define OCALL_PRINT_FLOAT_ARRAY_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print_float_array, (float* arr, int len));
#endif
#ifndef OCALL_PRINT_INT_ARRAY_DEFINED__
#define OCALL_PRINT_INT_ARRAY_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print_int_array, (int* arr, int len));
#endif

sgx_status_t ecall_ttruth(sgx_enclave_id_t eid, int* question_top_k_user, int question_num, int user_num, int top_k, int cluster_size, int vector_size, int cnt);
sgx_status_t ecall_oblivious_ttruth(sgx_enclave_id_t eid, int* question_top_k_user, int question_num, int user_num, int top_k, int cluster_size, int vector_size, int cnt);
sgx_status_t test(sgx_enclave_id_t eid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
