//
// Created by harry.tran on 13/7/24.
//
#include "latent_utils.h"
#include <android/log.h>
#include <cmath>
#include "net.h"

// Adds a sentence and its latent matrix to the store
void LatentUtils::addSentenceToStore(const std::string& sentence, const ncnn::Mat& mat) {
    sentenceStore[sentence] = mat;
}

// Prints the matrix details
void LatentUtils::printMatInfo(const ncnn::Mat& mat, const char* tag) {
    __android_log_print(ANDROID_LOG_INFO, tag, "Mat dims: %d, w: %d, h: %d, c: %d", mat.dims, mat.w, mat.h, mat.c);
}

// Finds the most similar matrix in the store to a given matrix and returns the associated sentence
std::string LatentUtils::findMostSimilarSentence(const ncnn::Mat& mat) {
    float maxSimilarity = -1.0f;
    std::string mostSimilarSentence;

    for (const auto& pair : sentenceStore) {
        float similarity = cosineSimilarity(mat, pair.second);
        if (similarity > maxSimilarity) {
            maxSimilarity = similarity;
            mostSimilarSentence = pair.first;
        }
    }
    __android_log_print(ANDROID_LOG_INFO, "SD", "Max cosine similarity %f", maxSimilarity);

    return mostSimilarSentence;
}

// Calculates cosine similarity between two matrices
float LatentUtils::cosineSimilarity(const ncnn::Mat& mat1, const ncnn::Mat& mat2) {
    if (mat1.w != mat2.w || mat1.h != mat2.h || mat1.c != mat2.c) {
        return -1.0f; // Return -1.0f if dimensions do not match
    }

    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;

    for (int q = 0; q < mat1.c; q++) {
        const float* ptr1 = mat1.channel(q);
        const float* ptr2 = mat2.channel(q);
        for (int i = 0; i < mat1.w * mat1.h; i++) {
            dotProduct += ptr1[i] * ptr2[i];
            normA += ptr1[i] * ptr1[i];
            normB += ptr2[i] * ptr2[i];
        }
    }

    return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
}
