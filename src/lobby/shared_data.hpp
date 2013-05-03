#pragma once
#ifndef SHARED_DATA_HPP_INCLUDED
#define SHARED_DATA_HPP_INCLUDED

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <json_spirit.h>
#include <iostream>
#include <map>
#include <vector>

#include "connection_fwd.hpp"
#include "queue.hpp"
#include "sqlite_wrapper.hpp"

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace game_server
{
	typedef boost::shared_ptr<queue::queue<json_spirit::mValue> > client_message_queue_ptr;
	
	// Reload time in seconds. 
	const int last_seen_counter_reload_value = 5 * 60;

	struct client_info
	{
		client_info()
			: session_id(-1), is_human(false), signed_in(false)
		{}
		client_info(int sid, bool human, const std::string& slt)
			: session_id(sid), is_human(human), salt(slt), signed_in(false),
			counter(0), last_seen_count(last_seen_counter_reload_value)
		{
			msg_q = client_message_queue_ptr(new queue::queue<json_spirit::mValue>);
		}
		int session_id;
		std::string salt;
		bool is_human;
		bool signed_in;
		// Message queue for the client.
		client_message_queue_ptr msg_q;
		// Saved connection for sending deferred replies on.
		http::server::connection_ptr conn;
		// Counter for tracking deferred replies.
		int counter;
		// Counter decremented every second, after five minutes of no activity we expire sessions.
		int last_seen_count;
	};

	struct server_info
	{
		size_t min_players;
		size_t min_humans;
		size_t max_players;
		std::string name;
		std::string display_name;
		json_spirit::mObject other;
		bool has_bots;

		std::string server_address;
		std::string server_port;
	};

	typedef std::map<std::string, client_info> client_map;

	struct game_info 
	{
		bool started;
		int bot_count;
		size_t max_players;
		std::string name;
		std::vector<std::string> clients;
		std::vector<std::string> bot_types;
	};

	typedef std::map<int, game_info> game_list;
	class shared_data
	{
	public:
		enum action 
		{
			send_salt,
			user_not_found,
			password_failed,
			login_success,
			bad_session_id,
		};
		shared_data()
		{}
		shared_data(sqlite::sqlite_wrapper_ptr db_ptr, boost::shared_ptr<sql::Connection> conn)
			: db_ptr_(db_ptr), conn_(conn)
		{
		}
		virtual ~shared_data()
		{}
		boost::tuple<action, client_info> process_user(const std::string& uname, 
			const std::string& phash, 
			int session_id);
		bool sign_off(const std::string& uname, int session_id);
		bool check_user_and_session(const std::string& uname, int session_id);
		void check_add_client(const std::string& user, client_info& ci);
		void check_add_game(int gid, const game_info& gi);
		void get_user_list(json_spirit::mArray* users);
		void get_games_list(json_spirit::mArray* games);
		void add_server(const server_info& si);
		bool create_game(const std::string& user, const std::string& game_type, size_t max_players, int* game_id);
		void remove_game(const std::string& user, int game_id);
		static int make_session_id();
		client_message_queue_ptr get_message_queue(const std::string& user);
		void set_waiting_connection(const std::string& user, http::server::connection_ptr conn);
		void process_waiting_connections();
		bool post_message_to_client(const std::string& user, const json_spirit::mValue& val);
		void post_message_to_all_clients(const json_spirit::mValue& val);
		bool post_message_to_game_clients(int game_id, const json_spirit::mValue& val);
		bool check_game_and_client(int game_id, const std::string& user, const std::string& user_to_add);
		void update_last_seen_count(const std::string& user);
#ifdef BOOST_NO_CXX11_NULLPTR
		bool check_client_in_games(const std::string& user, int* game_id = NULL);
#else
		bool check_client_in_games(const std::string& user, int* game_id = nullptr);
#endif
		bool is_user_in_game(const std::string& user, int game_id) const;
		const game_info* get_game_info(int game_id) const;
		int get_user_session_id(const std::string& user) const;
		bool is_user_in_any_games(const std::string& user, int* game_id) const;

		bool is_user_in_database(const std::string& user) const;
		bool add_user_to_database(const std::string& user, 
			const std::string& password, 
			const std::string& email,
			const std::string& avatar);
		bool get_user_from_database(const std::string& user, 
			std::string& password, 
			std::string& email,
			std::string& avatar);
		bool get_user_data(const std::string& user, json_spirit::mValue& obj) const;
		bool set_user_data(const std::string& user, const json_spirit::mValue& obj);
	private:
		sqlite::sqlite_wrapper_ptr db_ptr_;
		boost::shared_ptr<sql::Connection> conn_;

		// This is now the list of games that the lobby has created.
		game_list games_;
		// List of clients the lobby knows about.
		client_map clients_;
		std::vector<server_info> servers_;

		// List of games from server.
		game_list server_games_;

		mutable boost::recursive_mutex guard_;

		shared_data(shared_data&);
	};
}

#endif
