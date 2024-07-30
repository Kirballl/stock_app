#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <mutex>
#include <cstdint> 
#include <vector>

#include "trade_market_protocol.pb.h"

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include "bcrypt.h"

/**
 * @brief Database working interface
 * This interface defines methods for interacting with the database,
 * including authentication operations, saving and loading orders,
 * managing clients balances and working with the history of quotations.
 */
class IDatabase {
public:
    virtual ~IDatabase() = default;

    /**
     * @brief User authentication and management operations
     * 
     * These methods handle user-related operations in the database.
     * 
     * @param username The username of the user
     * @param password The password of the user (for add_user and authenticate_user)
     * 
     * @return For is_user_exists and authenticate_user: 
     *         true if the operation is successful, false otherwise
     * 
     * @note add_user does not return a value. It throws an exception if the operation fails.
     */
    //@{
    virtual bool is_user_exists(const std::string& username) = 0;
    virtual void add_user(const std::string& username, const std::string& password) = 0;
    virtual bool authenticate_user(const std::string& username, const std::string& password) = 0;
    //@}

    /**
     * @brief Database operations for orders, client balances, and quotes
     *
     * These methods handle various database operations related to user management and trading activities.
     * They work with Protocol Buffer messages defined in the Serialize package.
     *
     * @param order A Serialize::TradeOrder message containing order details
     * @param type Serialize::TradeOrder::TradeType (BUY or SELL)
     * @param client_balance A Serialize::ClientBalance message with updated balance information
     * @param completion_timestamp The timestamp (int64_t) when an order was completed
     * @param number The number of items to load (for completed orders and quote history) when server started
     * @param quote A Serialize::Quote message containing market quote information
     *
     * @return 
     * - For load_active_orders_from_db: A vector of active trade orders
     * - For load_clients_balances_from_db: A vector of client balances
     * - For load_last_completed_orders: A vector of the last completed trade orders
     * - For load_quote_history: A vector of historical market quotes
     *
     * @note Save operations (save_active_order_to_db, update_actual_client_balance_in_db, 
     *       save_completed_order_to_db, save_qoute_to_db) do not return values. 
     *       They may throw exceptions if the operation fails.
     */
    //@{
    virtual void save_active_order_to_db(const Serialize::TradeOrder& order) = 0;
    virtual std::vector<Serialize::TradeOrder> load_active_orders_from_db(Serialize::TradeOrder::TradeType type) = 0;

    virtual void update_actual_client_balance_in_db(const Serialize::ClientBalance& client_balance) = 0;
    virtual std::vector<Serialize::ClientBalance> load_clients_balances_from_db() = 0;

    virtual void save_completed_order_to_db(const Serialize::TradeOrder& order, int64_t completion_timestamp) = 0;
    virtual std::vector<Serialize::TradeOrder> load_last_completed_orders(int number) = 0;

    virtual void save_qoute_to_db(const Serialize::Quote& qoute) = 0;
    virtual std::vector<Serialize::Quote> load_quote_history(int number) = 0;
    //@}

    /**
     * @brief When the server is started, data from the database is loaded into the server RAM.
     *        When the server is stopped, data is loaded from RAM into the database.
     *        Therefore, in order to save correctly when the server is stopped.
     *        (!)Method of clearing active orders tables has been implemented for simplification, 
     *        which may change during the process of matching buy and sell orders.
     */
    virtual void truncate_active_orders_table() = 0;
};

/**
 * @brief Implementation of the Database interface for working with PostgreSQL.
 */
class Database : public IDatabase {
public:
    Database(const std::string& connection_info);

    bool is_user_exists(const std::string& username) override;
    void add_user(const std::string& username, const std::string& password) override;
    bool authenticate_user(const std::string& username, const std::string& password) override;

    /**
     * @brief Used in core. As soon as the order is completed, it is saved to database.
     *        When the server starts, it loads orders from the database.
     */
    //@{
    void save_active_order_to_db(const Serialize::TradeOrder& order) override;
    std::vector<Serialize::TradeOrder> load_active_orders_from_db(Serialize::TradeOrder::TradeType type) override;
    //@}

    /**
     * @brief client_data_manager operations. When matching orders, these steps happen in sequence.
     */
    //@{
    void update_actual_client_balance_in_db(const Serialize::ClientBalance& client_balance) override;
    std::vector<Serialize::ClientBalance> load_clients_balances_from_db() override;

    void save_completed_order_to_db(const Serialize::TradeOrder& order, int64_t completion_timestamp) override;
    std::vector<Serialize::TradeOrder> load_last_completed_orders(int number) override;

    void save_qoute_to_db(const Serialize::Quote& qoute) override;
    std::vector<Serialize::Quote> load_quote_history(int number) override;
    //@}

    void truncate_active_orders_table() override;

private:
    pqxx::connection connection_;
    mutable std::mutex mutex_;

    static const char* CREATE_USERS_TABLE;
    static const char* CREATE_ACTIVE_BUY_ORDERS_TABLE;
    static const char* CREATE_ACTIVE_SELL_ORDERS_TABLE;
    static const char* CREATE_CLIENTS_BALANCES_TABLE;
    static const char* CREATE_COMPLETED_ORDERS_TABLE;
    static const char* CREATE_QUOTE_HISTORY_TABLE;
};

#endif // DATABASE_HPP
