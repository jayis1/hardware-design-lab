/*
 * classifier.h — Edge AI classifier public interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */
#ifndef SOUNDLOOM_CLASSIFIER_H
#define SOUNDLOOM_CLASSIFIER_H

#include <stdint.h>

#define CLS_FRAMES_LOCAL  8
#define CLS_MEL_BINS_LOCAL 40

void cls_init(void);
int  cls_infer(const float *mel_features, int num_frames, int num_mel, float *probs);
int  cls_classify_event(const float *features, int n, float *confidence);
int  cls_is_pest(int class_id);
int  cls_is_compaction(int class_id);
int  cls_is_biological(int class_id);
const char* cls_name(int id);

#endif /* SOUNDLOOM_CLASSIFIER_H */