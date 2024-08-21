//
// Created by harry.tran on 13/7/24.
//
#include "latent_utils.h"
#include <android/log.h>
#include <cmath>
#include "net.h"

// Adds a sentence and its latent matrix to the store
void LatentUtils::add_sentence_to_store(const std::string& sentence, const ncnn::Mat& mat) {
    sentence_store[sentence] = mat.clone();
}

ncnn::Mat LatentUtils::get_sentence_store_item(const std::string& sentence) {
    auto it = result_store.find(sentence);
    return it->second.clone();
}

void LatentUtils::add_result_to_store(const std::string& sentence, const ncnn::Mat& mat) {
    result_store[sentence] = mat.clone();
}

ncnn::Mat LatentUtils::get_result_store_item(const std::string& sentence) {
    auto it = result_store.find(sentence);
    return it->second.clone();
}

void LatentUtils::add_denoised_to_store(const std::string &sentence, const ncnn::Mat &mat) {
    denoised_store[sentence] = mat.clone();
}

ncnn::Mat LatentUtils::get_denoised_item(const std::string &sentence) {
    auto it = denoised_store.find(sentence);
    return it->second.clone();
}

void LatentUtils::print_all() {
    for (const auto& pair : sentence_store) {
        __android_log_print(ANDROID_LOG_INFO, "MatStorage", "Key: %s", pair.first.c_str());
        print_mat_info(pair.second);
    }
}

// Prints the matrix details
void LatentUtils::print_mat_info(const ncnn::Mat& mat) {
    __android_log_print(ANDROID_LOG_INFO, "Mat Info", "Mat dims: %d, w: %d, h: %d, c: %d", mat.dims, mat.w, mat.h, mat.c);
    for (int c = 0; c < mat.c; c++) {
        const float* ptr = mat.channel(c);
        for (int h = 0; h < mat.h; h++) {
            std::string row_values;
            for (int w = 0; w < mat.w; w++) {
                row_values += std::to_string(ptr[w]) + " ";
            }
            __android_log_print(ANDROID_LOG_INFO, "Mat Info", "Channel %d, Row %d: %s", c, h, row_values.c_str());
            ptr += mat.w;
        }
    }
}

bool LatentUtils::exists(const std::string& key)  {
    return sentence_store.find(key) != sentence_store.end();
}

// Finds the most similar matrix in the store to a given matrix and returns the associated sentence
std::pair<std::string, float> LatentUtils::find_most_similar_sentence(const ncnn::Mat& mat) {
    float max_similarity = -1.0f;
    std::string most_similar_sentence;
    float best_similarity = -1.0f;

    for (const auto& pair : sentence_store) {
        float similarity = cosine_similarity(mat, pair.second);
        if (similarity > best_similarity) {
            best_similarity = similarity;
            most_similar_sentence = pair.first;
        }
    }
    return std::make_pair(most_similar_sentence, best_similarity);
}

// Calculates cosine similarity between two matrices
float LatentUtils::cosine_similarity(const ncnn::Mat& mat1, const ncnn::Mat& mat2) {
    // Ensure both matrices have the same dimensions
    if (mat1.dims != mat2.dims || mat1.w != mat2.w || mat1.h != mat2.h || mat1.c != mat2.c) {
        return 0.0f; // Return 0 if dimensions do not match
    }

    float dot_product = 0.0f;
    float magnitude1 = 0.0f;
    float magnitude2 = 0.0f;

    for (int c = 0; c < mat1.c; c++) {
        const float* ptr1 = mat1.channel(c);
        const float* ptr2 = mat2.channel(c);
        for (int h = 0; h < mat1.h; h++) {
            for (int w = 0; w < mat1.w; w++) {
                float val1 = ptr1[w];
                float val2 = ptr2[w];
                dot_product += val1 * val2;
                magnitude1 += val1 * val1;
                magnitude2 += val2 * val2;
            }
            ptr1 += mat1.w;
            ptr2 += mat2.w;
        }
    }

    magnitude1 = std::sqrt(magnitude1);
    magnitude2 = std::sqrt(magnitude2);

    if (magnitude1 == 0.0f || magnitude2 == 0.0f) {
        return 0.0f; // Avoid division by zero
    }

    return dot_product / (magnitude1 * magnitude2);
}

LatentUtils latent_utils;
