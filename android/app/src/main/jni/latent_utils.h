//
// Created by harry.tran on 13/7/24.
//

#ifndef ANDROID_LATENT_UTILS_H
#define ANDROID_LATENT_UTILS_H

#include <unordered_map>
#include <vector>
#include <string>
#include "net.h"

class LatentUtils {
public:
    // Adds a sentence and its latent matrix to the store
    void add_sentence_to_store(const std::string& sentence, const ncnn::Mat& mat);
    ncnn::Mat get_sentence_store_item(const std::string& sentence);
    void add_result_to_store(const std::string& sentence, const ncnn::Mat& mat);
    ncnn::Mat get_result_store_item(const std::string& sentence);

    void add_denoised_to_store(const std::string& sentence, const ncnn::Mat& mat);
    ncnn::Mat get_denoised_item(const std::string& sentence);

    bool exists(const std::string& key);

    // Prints the matrix details
    void print_mat_info(const ncnn::Mat& mat);
    void print_all();

    // Finds the most similar matrix in the store to a given matrix and returns the associated sentence
    std::pair<std::string, float> find_most_similar_sentence(const ncnn::Mat& mat);

private:
    std::unordered_map<std::string, ncnn::Mat> sentence_store;
    std::unordered_map<std::string, ncnn::Mat> denoised_store;
    std::unordered_map<std::string, ncnn::Mat> result_store;

    // Calculates cosine similarity between two matrices
    float cosine_similarity(const ncnn::Mat& mat1, const ncnn::Mat& mat2);
};

extern  LatentUtils latent_utils;

#endif //ANDROID_LATENT_UTILS_H
