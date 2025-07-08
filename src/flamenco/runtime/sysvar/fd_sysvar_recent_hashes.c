#include "fd_sysvar_recent_hashes.h"
#include "../context/fd_exec_slot_ctx.h"
#include "../fd_system_ids.h"
#include "fd_sysvar_cache.h"

/* Skips fd_types encoding preflight checks and directly serializes the
   blockhash queue into a buffer representing account data for the
   recent blockhashes sysvar. */

static void
encode_rbh_from_blockhash_queue( fd_exec_slot_ctx_t * slot_ctx,
                                 uchar                enc[ FD_SYSVAR_RECENT_HASHES_BINCODE_SZ ] ) {
  fd_block_hash_queue_global_t const * bhq = fd_bank_block_hash_queue_query( slot_ctx->bank );

  fd_hash_hash_age_pair_t_mapnode_t * ages_pool = fd_block_hash_queue_ages_pool_join( bhq );
  fd_hash_hash_age_pair_t_mapnode_t * ages_root = fd_block_hash_queue_ages_root_join( bhq );

  ulong queue_sz   = fd_hash_hash_age_pair_t_map_size( ages_pool, ages_root );
  ulong hashes_len = fd_ulong_min( queue_sz, FD_SYSVAR_RECENT_HASHES_CAP );
  fd_memcpy( enc, &hashes_len, sizeof(ulong) );
  enc += sizeof(ulong);

  /* Iterate over blockhash queue and encode the recent blockhashes.
     We can do direct memcpying and avoid redundant checks from fd_types
     encoders since the enc buffer is already sized out to the
     worst-case bound. */
  fd_hash_hash_age_pair_t_mapnode_t const * nn;
  for( fd_hash_hash_age_pair_t_mapnode_t const * n = fd_hash_hash_age_pair_t_map_minimum_const( ages_pool, ages_root ); n; n = nn ) {
    nn = fd_hash_hash_age_pair_t_map_successor_const( ages_pool, n );
    ulong enc_idx = bhq->last_hash_index - n->elem.val.hash_index;
    if( enc_idx>=hashes_len ) {
      continue;
    }
    fd_hash_t hash = n->elem.key;
    ulong     lps  = n->elem.val.fee_calculator.lamports_per_signature;

    fd_memcpy( enc + enc_idx * (FD_HASH_FOOTPRINT + sizeof(ulong)), &hash, FD_HASH_FOOTPRINT );
    fd_memcpy( enc + enc_idx * (FD_HASH_FOOTPRINT + sizeof(ulong)) + sizeof(fd_hash_t), &lps, sizeof(ulong) );
  }
}

void
fd_sysvar_recent_hashes_init( fd_exec_slot_ctx_t * slot_ctx ) {

  ulong sz_max = 0UL;
  uchar * data = fd_sysvar_cache_data_modify_prepare( slot_ctx, &fd_sysvar_recent_block_hashes_id, NULL, &sz_max );
  if( FD_UNLIKELY( !data ) ) FD_LOG_ERR(( "fd_sysvar_cache_data_modify_prepare(recent_block_hashes) failed" ));
  FD_TEST( sz_max>=FD_SYSVAR_RECENT_HASHES_BINCODE_SZ );
  fd_memset( data, 0, FD_SYSVAR_RECENT_HASHES_BINCODE_SZ );
  fd_sysvar_cache_data_modify_commit( slot_ctx, &fd_sysvar_recent_block_hashes_id, FD_SYSVAR_RECENT_HASHES_BINCODE_SZ );

}

// https://github.com/anza-xyz/agave/blob/e8750ba574d9ac7b72e944bc1227dc7372e3a490/accounts-db/src/blockhash_queue.rs#L113
static void
register_blockhash( fd_exec_slot_ctx_t * slot_ctx, fd_hash_t const * hash ) {

  fd_block_hash_queue_global_t *      bhq       = fd_bank_block_hash_queue_modify( slot_ctx->bank );
  fd_hash_hash_age_pair_t_mapnode_t * ages_pool = fd_block_hash_queue_ages_pool_join( bhq );
  fd_hash_hash_age_pair_t_mapnode_t * ages_root = fd_block_hash_queue_ages_root_join( bhq );
  bhq->last_hash_index++;
  if( fd_hash_hash_age_pair_t_map_size( ages_pool, ages_root ) >= bhq->max_age ) {
    fd_hash_hash_age_pair_t_mapnode_t * nn;
    for( fd_hash_hash_age_pair_t_mapnode_t * n = fd_hash_hash_age_pair_t_map_minimum( ages_pool, ages_root ); n; n = nn ) {
      nn = fd_hash_hash_age_pair_t_map_successor( ages_pool, n );
      /* NOTE: Yes, this check is incorrect. It should be >= which caps the blockhash queue at max_age
         entries, but instead max_age + 1 entries are allowed to exist in the queue at once. This mimics
         Agave to stay conformant with their implementation.
         https://github.com/anza-xyz/agave/blob/e8750ba574d9ac7b72e944bc1227dc7372e3a490/accounts-db/src/blockhash_queue.rs#L109 */
      if( bhq->last_hash_index - n->elem.val.hash_index > bhq->max_age ) {
        fd_hash_hash_age_pair_t_map_remove( ages_pool, &ages_root, n );
        fd_hash_hash_age_pair_t_map_release( ages_pool, n );
      }
    }
  }

  fd_hash_hash_age_pair_t_mapnode_t * node = fd_hash_hash_age_pair_t_map_acquire( ages_pool );
  node->elem = (fd_hash_hash_age_pair_t){
    .key = *hash,
    .val = (fd_hash_age_t){ .hash_index = bhq->last_hash_index, .fee_calculator = (fd_fee_calculator_t){ .lamports_per_signature = fd_bank_lamports_per_signature_get( slot_ctx->bank ) }, .timestamp = (ulong)fd_log_wallclock() }
  };
  // https://github.com/anza-xyz/agave/blob/e8750ba574d9ac7b72e944bc1227dc7372e3a490/accounts-db/src/blockhash_queue.rs#L121-L128
  fd_hash_hash_age_pair_t_map_insert( ages_pool, &ages_root, node );
  // https://github.com/anza-xyz/agave/blob/e8750ba574d9ac7b72e944bc1227dc7372e3a490/accounts-db/src/blockhash_queue.rs#L130
  fd_hash_t * last_hash = fd_block_hash_queue_last_hash_join( bhq );
  fd_memcpy( last_hash, hash, sizeof(fd_hash_t) );

  fd_block_hash_queue_ages_pool_update( bhq, ages_pool );
  fd_block_hash_queue_ages_root_update( bhq, ages_root );
}

void
fd_sysvar_recent_hashes_update( fd_exec_slot_ctx_t * slot_ctx ) {

  /* Add PoH hash to bank blockhash queue */

  fd_block_hash_queue_global_t * bhq = fd_bank_block_hash_queue_modify( slot_ctx->bank );
  if( FD_UNLIKELY( !bhq ) ) FD_LOG_ERR(( "Blockhash queue sysvar is invalid, cannot update" ));

  register_blockhash( slot_ctx, fd_bank_poh_query( slot_ctx->bank ) );

  /* Update sysvar account with latest 150 hashes */

  fd_sysvar_cache_t * sysvar_cache = slot_ctx->sysvar_cache;
  if( FD_UNLIKELY( !fd_sysvar_cache_flags_exists( sysvar_cache->slot_hashes.flags ) ) ) {
    fd_sysvar_recent_hashes_init( slot_ctx );
  }

  ulong sz_max = 0UL;
  uchar * data = fd_sysvar_cache_data_modify_prepare( slot_ctx, &fd_sysvar_recent_block_hashes_id, NULL, &sz_max );
  if( FD_UNLIKELY( !data ) ) FD_LOG_ERR(( "fd_sysvar_cache_data_modify_prepare(recent_block_hashes) failed" ));
  FD_TEST( sz_max>=FD_SYSVAR_RECENT_HASHES_BINCODE_SZ );
  encode_rbh_from_blockhash_queue( slot_ctx, data );
  fd_sysvar_cache_data_modify_commit( slot_ctx, &fd_sysvar_recent_block_hashes_id, FD_SYSVAR_RECENT_HASHES_BINCODE_SZ );
}
