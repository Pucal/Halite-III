#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include <functional>
#include <unordered_set>

#include "GameEvent.hpp"
#include "Location.hpp"
#include "Player.hpp"

namespace hlt {

class Halite;

class Map;

class Store;

/** Base transaction class independent of commands. */
class BaseTransaction {
protected:
    std::function<void(GameEvent)> callback;       /**< The game event callback. */
    Store &store;                                  /**< The game store. */
    Map &map;                                      /**< The game map. */

public:
    std::unordered_set<Player::id_type> offenders;         /**< Players that prevent transaction from committing. */
    std::unordered_set<Entity::id_type> changed_entities;  /**< Remaining entities that have been acted upon. */
    std::unordered_set<Location> changed_cells;            /**< Cells that have been updated. */

    /**
     * Construct BaseTransaction from Store and Map.
     * @param store The Store.
     * @param map The Map.
     */
    explicit BaseTransaction(Store &store, Map &map) : store(store), map(map) {}

    /**
     * Set a callback for GameEvents generated during the transaction commit.
     * @param callback The callback to set.
     */
    virtual void set_callback(std::function<void(GameEvent)> callback) = 0;

    /**
     * Check if the transaction may be committed without actually committing.
     * @return False if the transaction may not be committed.
     */
    virtual bool check() = 0;

    /** If the transaction may be committed, commit the transaction. */
    virtual void commit() = 0;

    /** Virtual destructor. */
    virtual ~BaseTransaction() = default;
};

/**
 * Transactions that execute a series of player commands atomically.
 * @tparam Command The type of the command.
 */
template<class Command>
class Transaction : public BaseTransaction {
protected:
    using Commands = std::vector<std::reference_wrapper<const Command>>; /**< The type of the player command list. */
    std::unordered_map<Player::id_type, Commands> commands;              /**< The stored commands per player. */
public:
    using BaseTransaction::BaseTransaction;

    /**
     * Add a command to the transaction.
     * @param player The player executing the command.
     * @param command The command to be executed.
     */
    void add_command(Player &player, const Command &command) {
        commands[player.id].emplace_back(command);
    }

    /**
     * Set a callback for GameEvents generated during the transaction commit.
     * @param callback The callback to set.
     */
    void set_callback(std::function<void(GameEvent)> callback) override { this->callback = callback; }

    /** Virtual destructor. */
    ~Transaction() override = default;
};

class DumpCommand;

class ConstructCommand;

class MoveCommand;

class SpawnCommand;

/** Transaction for DumpCommand. */
class DumpTransaction final : public Transaction<DumpCommand> {
public:
    using Transaction::Transaction;

    /**
     * Check if the transaction may be committed without actually committing.
     * @return False if the transaction may not be committed.
     */
    bool check() override;

    /** If the transaction may be committed, commit the transaction. */
    void commit() override;

};

/** Transaction for ConstructCommand. */
class ConstructTransaction final : public Transaction<ConstructCommand> {
public:
    using Transaction::Transaction;

    /**
     * Check if the transaction may be committed without actually committing.
     * @return False if the transaction may not be committed.
     */
    bool check() override;

    /** If the transaction may be committed, commit the transaction. */
    void commit() override;
};

/** Transaction for MoveCommand. */
class MoveTransaction final : public Transaction<MoveCommand> {
public:
    using Transaction::Transaction;

    /**
     * Check if the transaction may be committed without actually committing.
     * @return False if the transaction may not be committed.
     */
    bool check() override;

    /** If the transaction may be committed, commit the transaction. */
    void commit() override;
};

/** Transaction for SpawnCommand. */
class SpawnTransaction final : public Transaction<SpawnCommand> {
public:
    using Transaction::Transaction;

    /**
     * Check if the transaction may be committed without actually committing.
     * @return False if the transaction may not be committed.
     */
    bool check() override;

    /** If the transaction may be committed, commit the transaction. */
    void commit() override;
};

/** Transaction for all commands. */
class CommandTransaction final : public BaseTransaction {
    /** Count of occurrences per player-entity pair, to catch duplicates. */
    std::unordered_map<Player, std::unordered_map<Entity::id_type, int>> occurrences;
    /** Total expenses per player. */
    std::unordered_map<Player, energy_type> expenses;

public:
    DumpTransaction dump_transaction;           /**< The DumpCommand transaction. */
    ConstructTransaction construct_transaction; /**< The ConstructCommand transaction. */
    MoveTransaction move_transaction;           /**< The MoveCommand transaction. */
    SpawnTransaction spawn_transaction;         /**< The SpawnCommand transaction. */

    /** All of the transactions, in commit order. */
    std::vector<std::reference_wrapper<BaseTransaction>> all_transactions
            {dump_transaction, construct_transaction, move_transaction, spawn_transaction};

    /**
     * Add a DumpCommand to the transaction.
     * @param player The player executing the command.
     * @param command The command.
     */
    void add_command(Player &player, const DumpCommand &command);

    /**
     * Add a ConstructCommand to the transaction.
     * @param player The player executing the command.
     * @param command The command.
     */
    void add_command(Player &player, const ConstructCommand &command);

    /**
     * Add a MoveCommand to the transaction.
     * @param player The player executing the command.
     * @param command The command.
     */
    void add_command(Player &player, const MoveCommand &command);

    /**
     * Add a SpawnCommand to the transaction.
     * @param player The player executing the command.
     * @param command The command.
     */
    void add_command(Player &player, const SpawnCommand &command);

    /**
     * Check if the transaction may be committed without actually committing.
     * @return False if the transaction may not be committed.
     */
    bool check() override;

    /** If the transaction may be committed, commit the transaction. */
    void commit() override;

    /**
     * Set a callback for GameEvents generated during the transaction commit.
     * @param callback The callback to set.
     */
    void set_callback(std::function<void(GameEvent)> callback) override;

    /**
     * Construct CommandTransaction from Store and map.
     * @param store The Store.
     * @param map The Map.
     */
    explicit CommandTransaction(Store &store, Map &map) :
            BaseTransaction(store, map),
            dump_transaction(store, map),
            construct_transaction(store, map),
            move_transaction(store, map),
            spawn_transaction(store, map) {}
};

}

#endif // TRANSACTION_HPP