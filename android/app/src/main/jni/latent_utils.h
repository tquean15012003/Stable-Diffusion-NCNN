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
    void addSentenceToStore(const std::string& sentence, const ncnn::Mat& mat);

    // Prints the matrix details
    void printMatInfo(const ncnn::Mat& mat, const char* tag);

    // Finds the most similar matrix in the store to a given matrix and returns the associated sentence
    std::string findMostSimilarSentence(const ncnn::Mat& mat);

private:
    std::unordered_map<std::string, ncnn::Mat> sentenceStore;

    // Calculates cosine similarity between two matrices
    float cosineSimilarity(const ncnn::Mat& mat1, const ncnn::Mat& mat2);
};

#endif //ANDROID_LATENT_UTILS_H
