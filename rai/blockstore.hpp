#pragma once

#include <rai/common.hpp>
#include <rai/node/lmdb.hpp>

namespace rai
{
class block_store;
template <typename T, typename U>
class store_iterator_impl
{
public:
	store_iterator_impl (MDB_txn * transaction_a, MDB_dbi db_a);
	store_iterator_impl (std::nullptr_t);
	store_iterator_impl (MDB_txn * transaction_a, MDB_dbi db_a, MDB_val const & val_a);
	store_iterator_impl (rai::store_iterator_impl<T, U> && other_a);
	store_iterator_impl (rai::store_iterator_impl<T, U> const &) = delete;
	~store_iterator_impl ();
	rai::store_iterator_impl<T, U> & operator++ ();
	rai::store_iterator_impl<T, U> & operator= (rai::store_iterator_impl<T, U> && other_a);
	rai::store_iterator_impl<T, U> & operator= (rai::store_iterator_impl<T, U> const &) = delete;
	std::pair<rai::mdb_val, rai::mdb_val> * operator-> ();
	bool operator== (rai::store_iterator_impl<T, U> const & other_a) const;
	bool operator!= (rai::store_iterator_impl<T, U> const & other_a) const;
	void next_dup ();
	void clear ();
	MDB_cursor * cursor;
	std::pair<rai::mdb_val, rai::mdb_val> current;
};
/**
 * Iterates the key/value pairs of a transaction
 */
template <typename T, typename U>
class store_iterator
{
	friend class rai::block_store;
public:
	store_iterator (std::nullptr_t) :
	impl (new rai::store_iterator_impl<T, U> (nullptr))
	{
	}
	store_iterator (std::unique_ptr<rai::store_iterator_impl<T, U>> impl_a) :
	impl (std::move (impl_a))
	{
		assert (impl != nullptr);
	}
	store_iterator (rai::store_iterator<T, U> && other_a) :
	impl (std::move (other_a.impl))
	{
	}
	rai::store_iterator<T, U> & operator++ ()
	{
		++*impl;
		current.first = static_cast<T> (impl->current.first);
		current.second = static_cast<U> (impl->current.second);
		return *this;
	}
	rai::store_iterator<T, U> & operator= (rai::store_iterator<T, U> && other_a)
	{
		impl = std::move (other_a.impl);
		current = other_a.current;
		return *this;
	}
	rai::store_iterator<T, U> & operator= (rai::store_iterator<T, U> const &) = delete;
	std::pair<T, U> * operator-> ()
	{
		return &current;
	}
	bool operator== (rai::store_iterator<T, U> const & other_a) const
	{
		return *impl == *other_a.impl;
	}
	bool operator!= (rai::store_iterator<T, U> const & other_a) const
	{
		return !(*this == other_a);
	}
private:
	std::pair<T, U> current;
	std::unique_ptr<rai::store_iterator_impl<T, U>> impl;
};

class block_predecessor_set;
/**
 * Manages block storage and iteration
 */
class block_store
{
	friend class rai::block_predecessor_set;
public:
	block_store (bool &, boost::filesystem::path const &, int lmdb_max_dbs = 128);

	void initialize (MDB_txn *, rai::genesis const &);
	void block_put (MDB_txn *, rai::block_hash const &, rai::block const &, rai::block_hash const & = rai::block_hash (0));
	rai::block_hash block_successor (MDB_txn *, rai::block_hash const &);
	void block_successor_clear (MDB_txn *, rai::block_hash const &);
	std::unique_ptr<rai::block> block_get (MDB_txn *, rai::block_hash const &);
	std::unique_ptr<rai::block> block_random (MDB_txn *);
	void block_del (MDB_txn *, rai::block_hash const &);
	bool block_exists (MDB_txn *, rai::block_hash const &);
	rai::block_counts block_count (MDB_txn *);
	bool root_exists (MDB_txn *, rai::uint256_union const &);

	void frontier_put (MDB_txn *, rai::block_hash const &, rai::account const &);
	rai::account frontier_get (MDB_txn *, rai::block_hash const &);
	void frontier_del (MDB_txn *, rai::block_hash const &);

	void account_put (MDB_txn *, rai::account const &, rai::account_info const &);
	bool account_get (MDB_txn *, rai::account const &, rai::account_info &);
	void account_del (MDB_txn *, rai::account const &);
	bool account_exists (MDB_txn *, rai::account const &);
	size_t account_count (MDB_txn *);
	rai::store_iterator<rai::account, rai::account_info> latest_begin (MDB_txn *, rai::account const &);
	rai::store_iterator<rai::account, rai::account_info> latest_begin (MDB_txn *);
	rai::store_iterator<rai::account, rai::account_info> latest_end ();

	void pending_put (MDB_txn *, rai::pending_key const &, rai::pending_info const &);
	void pending_del (MDB_txn *, rai::pending_key const &);
	bool pending_get (MDB_txn *, rai::pending_key const &, rai::pending_info &);
	bool pending_exists (MDB_txn *, rai::pending_key const &);
	rai::store_iterator<rai::pending_key, rai::pending_info> pending_begin (MDB_txn *, rai::pending_key const &);
	rai::store_iterator<rai::pending_key, rai::pending_info> pending_begin (MDB_txn *);
	rai::store_iterator<rai::pending_key, rai::pending_info> pending_end ();

	void block_info_put (MDB_txn *, rai::block_hash const &, rai::block_info const &);
	void block_info_del (MDB_txn *, rai::block_hash const &);
	bool block_info_get (MDB_txn *, rai::block_hash const &, rai::block_info &);
	bool block_info_exists (MDB_txn *, rai::block_hash const &);
	rai::store_iterator<rai::block_hash, rai::block_info> block_info_begin (MDB_txn *, rai::block_hash const &);
	rai::store_iterator<rai::block_hash, rai::block_info> block_info_begin (MDB_txn *);
	rai::store_iterator<rai::block_hash, rai::block_info> block_info_end ();
	rai::uint128_t block_balance (MDB_txn *, rai::block_hash const &);
	static size_t const block_info_max = 32;

	rai::uint128_t representation_get (MDB_txn *, rai::account const &);
	void representation_put (MDB_txn *, rai::account const &, rai::uint128_t const &);
	void representation_add (MDB_txn *, rai::account const &, rai::uint128_t const &);
	rai::store_iterator<rai::account, rai::uint128_union> representation_begin (MDB_txn *);
	rai::store_iterator<rai::account, rai::uint128_union> representation_end ();

	void unchecked_clear (MDB_txn *);
	void unchecked_put (MDB_txn *, rai::block_hash const &, std::shared_ptr<rai::block> const &);
	std::vector<std::shared_ptr<rai::block>> unchecked_get (MDB_txn *, rai::block_hash const &);
	void unchecked_del (MDB_txn *, rai::block_hash const &, std::shared_ptr<rai::block>);
	rai::store_iterator<rai::block_hash, std::shared_ptr<rai::block>> unchecked_begin (MDB_txn *);
	rai::store_iterator<rai::block_hash, std::shared_ptr<rai::block>> unchecked_begin (MDB_txn *, rai::block_hash const &);
	rai::store_iterator<rai::block_hash, std::shared_ptr<rai::block>> unchecked_end ();
	size_t unchecked_count (MDB_txn *);
	std::unordered_multimap<rai::block_hash, std::shared_ptr<rai::block>> unchecked_cache;

	void checksum_put (MDB_txn *, uint64_t, uint8_t, rai::checksum const &);
	bool checksum_get (MDB_txn *, uint64_t, uint8_t, rai::checksum &);
	void checksum_del (MDB_txn *, uint64_t, uint8_t);

	// Return latest vote for an account from store
	std::shared_ptr<rai::vote> vote_get (MDB_txn *, rai::account const &);
	// Populate vote with the next sequence number
	std::shared_ptr<rai::vote> vote_generate (MDB_txn *, rai::account const &, rai::raw_key const &, std::shared_ptr<rai::block>);
	// Return either vote or the stored vote with a higher sequence number
	std::shared_ptr<rai::vote> vote_max (MDB_txn *, std::shared_ptr<rai::vote>);
	// Return latest vote for an account considering the vote cache
	std::shared_ptr<rai::vote> vote_current (MDB_txn *, rai::account const &);
	void flush (MDB_txn *);
	rai::store_iterator<rai::account, std::shared_ptr<rai::vote>> vote_begin (MDB_txn *);
	rai::store_iterator<rai::account, std::shared_ptr<rai::vote>> vote_end ();
	std::mutex cache_mutex;
	std::unordered_map<rai::account, std::shared_ptr<rai::vote>> vote_cache;

	void version_put (MDB_txn *, int);
	int version_get (MDB_txn *);
	void do_upgrades (MDB_txn *);
	void upgrade_v1_to_v2 (MDB_txn *);
	void upgrade_v2_to_v3 (MDB_txn *);
	void upgrade_v3_to_v4 (MDB_txn *);
	void upgrade_v4_to_v5 (MDB_txn *);
	void upgrade_v5_to_v6 (MDB_txn *);
	void upgrade_v6_to_v7 (MDB_txn *);
	void upgrade_v7_to_v8 (MDB_txn *);
	void upgrade_v8_to_v9 (MDB_txn *);
	void upgrade_v9_to_v10 (MDB_txn *);
	void upgrade_v10_to_v11 (MDB_txn *);

	// Requires a write transaction
	rai::raw_key get_node_id (MDB_txn *);

	/** Deletes the node ID from the store */
	void delete_node_id (MDB_txn *);

	rai::mdb_env environment;

	/**
	 * Maps head block to owning account
	 * rai::block_hash -> rai::account
	 */
	MDB_dbi frontiers;

	/**
	 * Maps account to account information, head, rep, open, balance, timestamp and block count.
	 * rai::account -> rai::block_hash, rai::block_hash, rai::block_hash, rai::amount, uint64_t, uint64_t
	 */
	MDB_dbi accounts;

	/**
	 * Maps block hash to send block.
	 * rai::block_hash -> rai::send_block
	 */
	MDB_dbi send_blocks;

	/**
	 * Maps block hash to receive block.
	 * rai::block_hash -> rai::receive_block
	 */
	MDB_dbi receive_blocks;

	/**
	 * Maps block hash to open block.
	 * rai::block_hash -> rai::open_block
	 */
	MDB_dbi open_blocks;

	/**
	 * Maps block hash to change block.
	 * rai::block_hash -> rai::change_block
	 */
	MDB_dbi change_blocks;

	/**
	 * Maps block hash to state block.
	 * rai::block_hash -> rai::state_block
	 */
	MDB_dbi state_blocks;

	/**
	 * Maps (destination account, pending block) to (source account, amount).
	 * rai::account, rai::block_hash -> rai::account, rai::amount
	 */
	MDB_dbi pending;

	/**
	 * Maps block hash to account and balance.
	 * block_hash -> rai::account, rai::amount
	 */
	MDB_dbi blocks_info;

	/**
	 * Representative weights.
	 * rai::account -> rai::uint128_t
	 */
	MDB_dbi representation;

	/**
	 * Unchecked bootstrap blocks.
	 * rai::block_hash -> rai::block
	 */
	MDB_dbi unchecked;

	/**
	 * Mapping of region to checksum.
	 * (uint56_t, uint8_t) -> rai::block_hash
	 */
	MDB_dbi checksum;

	/**
	 * Highest vote observed for account.
	 * rai::account -> uint64_t
	 */
	MDB_dbi vote;

	/**
	 * Meta information about block store, such as versions.
	 * rai::uint256_union (arbitrary key) -> blob
	 */
	MDB_dbi meta;
private:
	MDB_dbi block_database (rai::block_type);
	std::unique_ptr<rai::block> block_random (MDB_txn *, MDB_dbi);
	MDB_val block_raw_get (MDB_txn *, rai::block_hash const &, rai::block_type &);
	void block_raw_put (MDB_txn *, MDB_dbi, rai::block_hash const &, MDB_val);
	void clear (MDB_dbi);
};
}
