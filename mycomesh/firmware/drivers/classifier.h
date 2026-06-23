/*
 * classifier.h — Activity state classifier header
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef MYCOMESH_CLASSIFIER_H
#define MYCOMESH_CLASSIFIER_H

#include "board.h"
#include <stdint.h>

/* Initialize the classifier with default prototype templates. */
void classifier_init(void);

/* Classify an epoch of fungal activity into one of 5 states:
 *   CLASS_IDLE, CLASS_FORAGE, CLASS_TRANSPORT, CLASS_STRESS, CLASS_COMPETE
 * Uses template matching on the epoch feature vector. */
uint8_t classifier_classify(const epoch_summary_t *epoch);

/* Get the human-readable label for a class. */
const char *classifier_label(uint8_t class_label);

/* Update prototype templates (for adaptive learning). */
void classifier_update_prototype(uint8_t class_label,
                                 const epoch_summary_t *epoch);

#endif /* MYCOMESH_CLASSIFIER_H */