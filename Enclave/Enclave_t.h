#ifndef ENCLAVE_T_H__
#define ENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

void ecall_ttruth(int* question_top_k_user, int question_num, int user_num, int top_k, int cluster_size, int vector_size, int cnt);
void ecall_oblivious_ttruth(int* question_top_k_user, int question_num, int user_num, int top_k, int cluster_size, int vector_size, int cnt);
void test(void);

sgx_status_t SGX_CDECL ocall_query_solution_size(int question_id, int* user_solution_size, int user_num);
sgx_status_t SGX_CDECL ocall_load_question_solutions(int question_id, char* solutions, int len);
sgx_status_t SGX_CDECL ocall_load_words_vectors(char* raw_words, int* words_size, float* vectors, int word_num, int vec_len);
sgx_status_t SGX_CDECL ocall_filter_keywords(char* raw_words, int* words_size, int* words_filter_ind, int word_num);
sgx_status_t SGX_CDECL ocall_print_string(const char* string);
sgx_status_t SGX_CDECL ocall_print_float_array(float* arr, int len);
sgx_status_t SGX_CDECL ocall_print_int_array(int* arr, int len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
