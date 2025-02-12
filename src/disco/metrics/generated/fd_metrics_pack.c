/* THIS FILE IS GENERATED BY gen_metrics.py. DO NOT HAND EDIT. */
#include "fd_metrics_pack.h"

const fd_metrics_meta_t FD_METRICS_PACK[FD_METRICS_PACK_TOTAL] = {
    DECLARE_METRIC_HISTOGRAM_SECONDS( PACK_SCHEDULE_MICROBLOCK_DURATION_SECONDS ),
    DECLARE_METRIC_HISTOGRAM_SECONDS( PACK_NO_SCHED_MICROBLOCK_DURATION_SECONDS ),
    DECLARE_METRIC_HISTOGRAM_SECONDS( PACK_INSERT_TRANSACTION_DURATION_SECONDS ),
    DECLARE_METRIC_HISTOGRAM_SECONDS( PACK_COMPLETE_MICROBLOCK_DURATION_SECONDS ),
    DECLARE_METRIC_HISTOGRAM_NONE( PACK_TOTAL_TRANSACTIONS_PER_MICROBLOCK_COUNT ),
    DECLARE_METRIC_HISTOGRAM_NONE( PACK_VOTES_PER_MICROBLOCK_COUNT ),
    DECLARE_METRIC( PACK_NORMAL_TRANSACTION_RECEIVED, COUNTER ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, BUNDLE_BLACKLIST ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, WRITE_SYSVAR ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, ESTIMATION_FAIL ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, DUPLICATE_ACCOUNT ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, TOO_MANY_ACCOUNTS ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, TOO_LARGE ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, EXPIRED ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, ADDR_LUT ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, UNAFFORDABLE ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, DUPLICATE ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, PRIORITY ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, NONVOTE_ADD ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, VOTE_ADD ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, NONVOTE_REPLACE ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_INSERTED, COUNTER, PACK_TXN_INSERT_RETURN, VOTE_REPLACE ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, NO_TXN_NO_BANK_NO_LEADER_NO_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, TXN_NO_BANK_NO_LEADER_NO_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, NO_TXN_BANK_NO_LEADER_NO_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, TXN_BANK_NO_LEADER_NO_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, NO_TXN_NO_BANK_LEADER_NO_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, TXN_NO_BANK_LEADER_NO_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, NO_TXN_BANK_LEADER_NO_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, TXN_BANK_LEADER_NO_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, NO_TXN_NO_BANK_NO_LEADER_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, TXN_NO_BANK_NO_LEADER_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, NO_TXN_BANK_NO_LEADER_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, TXN_BANK_NO_LEADER_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, NO_TXN_NO_BANK_LEADER_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, TXN_NO_BANK_LEADER_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, NO_TXN_BANK_LEADER_MICROBLOCK ),
    DECLARE_METRIC_ENUM( PACK_METRIC_TIMING, COUNTER, PACK_TIMING_STATE, TXN_BANK_LEADER_MICROBLOCK ),
    DECLARE_METRIC( PACK_TRANSACTION_DROPPED_FROM_EXTRA, COUNTER ),
    DECLARE_METRIC( PACK_TRANSACTION_INSERTED_TO_EXTRA, COUNTER ),
    DECLARE_METRIC( PACK_TRANSACTION_INSERTED_FROM_EXTRA, COUNTER ),
    DECLARE_METRIC( PACK_TRANSACTION_EXPIRED, COUNTER ),
    DECLARE_METRIC_ENUM( PACK_AVAILABLE_TRANSACTIONS, GAUGE, AVAIL_TXN_TYPE, ALL ),
    DECLARE_METRIC_ENUM( PACK_AVAILABLE_TRANSACTIONS, GAUGE, AVAIL_TXN_TYPE, REGULAR ),
    DECLARE_METRIC_ENUM( PACK_AVAILABLE_TRANSACTIONS, GAUGE, AVAIL_TXN_TYPE, VOTES ),
    DECLARE_METRIC_ENUM( PACK_AVAILABLE_TRANSACTIONS, GAUGE, AVAIL_TXN_TYPE, CONFLICTING ),
    DECLARE_METRIC_ENUM( PACK_AVAILABLE_TRANSACTIONS, GAUGE, AVAIL_TXN_TYPE, BUNDLES ),
    DECLARE_METRIC( PACK_PENDING_TRANSACTIONS_HEAP_SIZE, GAUGE ),
    DECLARE_METRIC( PACK_SMALLEST_PENDING_TRANSACTION, GAUGE ),
    DECLARE_METRIC( PACK_MICROBLOCK_PER_BLOCK_LIMIT, COUNTER ),
    DECLARE_METRIC( PACK_DATA_PER_BLOCK_LIMIT, COUNTER ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_SCHEDULE, COUNTER, PACK_TXN_SCHEDULE, TAKEN ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_SCHEDULE, COUNTER, PACK_TXN_SCHEDULE, CU_LIMIT ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_SCHEDULE, COUNTER, PACK_TXN_SCHEDULE, FAST_PATH ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_SCHEDULE, COUNTER, PACK_TXN_SCHEDULE, BYTE_LIMIT ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_SCHEDULE, COUNTER, PACK_TXN_SCHEDULE, WRITE_COST ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_SCHEDULE, COUNTER, PACK_TXN_SCHEDULE, SLOW_PATH ),
    DECLARE_METRIC_ENUM( PACK_TRANSACTION_SCHEDULE, COUNTER, PACK_TXN_SCHEDULE, DEFER_SKIP ),
    DECLARE_METRIC( PACK_CUS_CONSUMED_IN_BLOCK, GAUGE ),
    DECLARE_METRIC_HISTOGRAM_NONE( PACK_CUS_SCHEDULED ),
    DECLARE_METRIC_HISTOGRAM_NONE( PACK_CUS_REBATED ),
    DECLARE_METRIC_HISTOGRAM_NONE( PACK_CUS_NET ),
    DECLARE_METRIC( PACK_DELETE_MISSED, COUNTER ),
    DECLARE_METRIC( PACK_DELETE_HIT, COUNTER ),
};
