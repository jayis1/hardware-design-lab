/*
         * Pantry Warden logger
         * Author: jayis1
         */

        #include <stdio.h>

        #include "logger.h"
        #include "inference.h"

        void logger_init(pw_logger_t *logger)
        {
            logger->count = 0U;
            for (unsigned i = 0; i < PW_LOG_DEPTH; ++i) {
                logger->entries[i].tick = 0U;
                logger->entries[i].state = PW_STATE_STABLE;
                logger->entries[i].health = 0.0f;
                logger->entries[i].spoilage = 0.0f;
                logger->entries[i].pest = 0.0f;
                logger->entries[i].condensation = 0.0f;
            }
        }

        void logger_push(pw_logger_t *logger, unsigned tick, const pw_inference_t *inf)
        {
            const unsigned index = logger->count % PW_LOG_DEPTH;
            logger->entries[index].tick = tick;
            logger->entries[index].state = inf->state;
            logger->entries[index].health = inf->shelf_health_score;
            logger->entries[index].spoilage = inf->spoilage_confidence;
            logger->entries[index].pest = inf->pest_confidence;
            logger->entries[index].condensation = inf->condensation_risk;
            logger->count++;
        }

        void logger_print(const pw_logger_t *logger)
        {
            const unsigned available = logger->count < PW_LOG_DEPTH ? logger->count : PW_LOG_DEPTH;
            puts("---- Pantry Warden event log ----");
            for (unsigned i = 0; i < available; ++i) {
                const pw_log_entry_t *entry = &logger->entries[i];
                printf("LOG tick=%u state=%s health=%.1f spoil=%.1f pest=%.1f cond=%.1f\n",
                       entry->tick,
                       inference_state_name(entry->state),
                       entry->health,
                       entry->spoilage,
                       entry->pest,
                       entry->condensation);
            }
        }
