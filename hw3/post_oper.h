#include "source.h"

string create_board(int sockfd, vector<string> &argu) {
	string ret;
	if (argu.size() != 2) {
		ret += create_board_format;
		// send(sockfd, create_board_format.c_str(), create_board_format.size(), 0);
		return ret;
	}
	
	string user_name;
    if ( !check_online(sockfd, user_name, ret) ) {
    	return ret;
	}
	
	string name = argu[1];
	string find_board = "INSERT INTO BOARDS (NAME, MODERATOR) VALUES ('" + name + "', '" + user_name + "');"; 
	int st = sqlite3_exec(db, find_board.c_str(), DB_check_enter, 0, 0);
	
	if (st != SQLITE_OK) {
		// already used
		string wan = "Board already exist.\n";
		ret += wan;
		// send(sockfd, wan.c_str(), wan.size(), 0);
	}
	else {
		string wan = "Create board successfully.\n";
		ret += wan;
		
		// send(sockfd, wan.c_str(), wan.size(), 0);
	}


	return ret;
}

string create_post(int sockfd, vector<string> &argu) {
	string ret;
	string board_name, title, content;
	bool format = false;
	int content_idx = 1e9;
	bool title_finish = false;
	for(int i=1; i<argu.size(); i++) {
		if (i==1) { 
			board_name = argu[i]; continue;
		}

		if (i==2 && (argu[i] != "--title")) {
			ret += create_post_format;
			// send(sockfd, create_post_format.c_str(), create_post_format.size(), 0);
			return ret;
		}
		
		if (argu[i] == "--content") {
			format = true; 
			content_idx = i;
			title_finish = true;
			continue;
		}
		// collect title
		if (i > 2 && !title_finish) {
			if (i > 3)
				title += ' ';
			title += argu[i];
		}

		// collect content
		if (i > content_idx) {
			if (i > content_idx+1)
				content += ' ';

			// check <br>
			size_t pos = 0;
			while(1) {
				pos = argu[i].find("<br>", pos);
				if (pos != string::npos) {
					argu[i].replace(pos, 4, "\n");
				}
				else	
					break;
			}
			content += argu[i];
		}
	}
	if (!format){
		ret += create_post_format;
		// send(sockfd, create_post_format.c_str(), create_post_format.size(), 0);
		return ret;
	}
	string user_name;
	// check login
	if ( !check_online(sockfd, user_name, ret) )
        return ret;

	// deal with time
	auto now = chrono::system_clock::now();
	auto in_time_t = chrono::system_clock::to_time_t(now);

	stringstream ss;
	string date_;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
	ss >> date_;

	string find_board = "SELECT * FROM BOARDS WHERE NAME='" + board_name + "';";
	bool enter = false;
	sqlite3_exec(db, find_board.c_str(), DB_check_enter, (void*)(&enter), 0);
	if ( !enter ) {
		// can't find
		string wan = "Board does not exist.\n";
		ret += wan;
		// send(sockfd, wan.c_str(), wan.size(), 0);
	}
	else {
		string add_post = "INSERT INTO POSTS (BOARD_NAME, TITLE, AUTHOR, DATE)"\
						  "VALUES ('" + board_name + "', '" + title + "', '" + user_name + "', '" + date_ + "');";
		sqlite3_exec(db, add_post.c_str(), DB_check_enter, 0, 0);
		postid_name[POST_ID] = user_name;

		POST_ID++;

		string wan = "Create post successfully.\n";
		ret += wan;
		// send(sockfd, wan.c_str(), wan.size(), 0);
	}
	return ret;
}

// don't have to login 
string list_board(int sockfd, vector<string> &argu) {
	// Question: ##' ', how to deal with? 
	string ret;
	string key;
	if (argu.size() > 2){
		ret += list_board_format;
		// send(sockfd, list_board_format.c_str(), list_board_format.size(), 0);
		return ret;
	}

	if (argu.size() == 2) {
		if (argu[1].substr(0, 2) == "##") {
			key = argu[1].substr(2);
		}
		else {
			ret += list_board_format;
			// send(sockfd, list_board_format.c_str(), list_board_format.size(), 0);
			return ret;
		}
	}
	// TODO: list board
	if( key.empty() ) {
		string list_all_boards = "SELECT * FROM BOARDS;";
		string indexes = "Index     Name      Moderator \n";
		ret += indexes;
		// pair<int, bool > fd_enter = {sockfd, false};
		bool enter = false;
		// send(sockfd, indexes.c_str(), indexes.size(), 0);
		sqlite3_exec(db, list_all_boards.c_str(), DB_list_data, (void*)&enter, 0);
		ret += glo_ret;
		glo_ret.clear();
	}
	else {
		// advanced search
		string list_key_boards = "SELECT * FROM BOARDS WHERE NAME LIKE '%" + key + "%';";
		// pair<int, bool> fd_enter = {sockfd, false};

		string indexes = "ID        Title     Author    Date      \n";
		ret += indexes;
		// pair<int, bool > fd_enter = {sockfd, false};

		// send(sockfd, indexes.c_str(), indexes.size(), 0);
		bool enter = false;
		sqlite3_exec(db, list_key_boards.c_str(), DB_list_data, (void*)&enter, 0);
		ret += glo_ret;
		glo_ret.clear();
	}
	return ret;

}

// don't have to login 
string list_post(int sockfd, vector<string> &argu) {
	string key, ret;
	string board_name;

	if (argu.size() > 3 || argu.size() < 2) {
		ret += list_board_format;
		// send(sockfd, list_board_format.c_str(), list_board_format.size(), 0);
		return ret;
	}
	
	if (argu.size() == 3) {
		if(argu[2].substr(0, 2) == "##") {
			key = argu[2].substr(2);
		}
		else {
			ret += list_board_format;
			// send(sockfd, list_board_format.c_str(), list_board_format.size(), 0);	
			return ret;
		}
	}
	board_name = argu[1];

	// pair<int, bool > fd_enter = {sockfd, false};
	bool enter = false;
	// Question: key have space?
	string indexes = "ID        Title     Author    Date      \n";
	ret += indexes;
	if (key.empty()) {
		string list_key_posts = "SELECT POST_ID, TITLE, AUTHOR, DATE FROM POSTS WHERE BOARD_NAME='" + board_name + "';";

		sqlite3_exec(db, list_key_posts.c_str(), DB_list_data, (void*)&enter, 0);
		ret += glo_ret;
		glo_ret.clear();
	}
	else {
		string list_key_posts = "SELECT POST_ID, TITLE, AUTHOR, DATE FROM POSTS WHERE BOARD_NAME='" + board_name + "' AND TITLE LIKE '%" + key + "%';";

		sqlite3_exec(db, list_key_posts.c_str(), DB_list_data, (void*)&enter, 0);
		ret += glo_ret;
		glo_ret.clear();
	}
	if(enter == false) {
		string no_board = "Board does not exist.\n";
		ret += no_board;
		// send(sockfd, no_board.c_str(), no_board.size(), 0);
	}
	return ret;
}

// don't have to login 
string read_(int sockfd, vector<string> &argu) {
	string ret;
	if( argu.size() != 2){
		ret += read_format;
		// send(sockfd, read_format.c_str(), read_format.size(), 0);
		return ret;
	}
	string post_id = argu[1];
	string read_post = "SELECT AUTHOR, TITLE, DATE FROM POSTS WHERE POST_ID=" + post_id + ";";
	// pair<int, bool> fd_enter = {sockfd, false};
	pair<int, bool > fd_enter = {0, false};
	
	sqlite3_exec(db, read_post.c_str(), DB_read_posts, (void*)&fd_enter, 0);
	ret += glo_ret;
	glo_ret.clear();
	if (fd_enter.second == false) {
		string no_posts = "Post does not exist.\n";
		ret += no_posts;
		// send(sockfd, no_posts.c_str(), no_posts.size(), 0);
	}
	return ret;
}

string delete_post(int sockfd, vector<string> &argu) {
	string ret;
	if (argu.size() != 2) {
		ret += delete_post_format;
		// send(sockfd, delete_post_format.c_str(), delete_post_format.size(), 0);
		return ret;
	}
	string post_id = argu[1];
	string user_name;
	if (online.find(sockfd) != online.end()) {
		user_name = online[sockfd];
	}
	else {
		ret += not_login;
		// send(sockfd, not_login.c_str(), not_login.size(), 0);
		return ret;
	}
	
	string check_autho = "SELECT * FROM POSTS WHERE POST_ID=" + post_id + ";";
	bool enter = false;
	sqlite3_exec(db, check_autho.c_str(), DB_check_enter, (void*)&enter, 0);

	if(enter) {
		string check_post = "SELECT * FROM POSTS WHERE POST_ID=" + post_id + " AND AUTHOR='" + user_name + "';";
		bool enter2 = false;
		sqlite3_exec(db, check_post.c_str(), DB_check_enter, (void*)&enter2, 0);
		if (!enter2) {
			string no_autho = "Not the post owner\n";
			ret += no_autho;
			// send(sockfd, no_autho.c_str(), no_autho.size(), 0);
		}
		else {
			string del_post = "DELETE FROM POSTS WHERE POST_ID=" + post_id + " AND AUTHOR='" + user_name + "';";
			sqlite3_exec(db, del_post.c_str(), DB_check_enter, 0, 0);

			string succ = "Delete successfully\n";
			ret += succ;
			// send(sockfd, succ.c_str(), succ.size(), 0);
		}		
	}
	else {
		string not_enter = "Post does not exist.\n";
		ret += not_enter;
		// send(sockfd, not_enter.c_str(), not_enter.size(), 0);
	}
	return ret;
}

string update_post(int sockfd, vector<string> &argu) {
	//Question: can <new> be space?
	string item, ret;
	if (argu.size() < 4) {
		ret += update_post_format;
		// send(sockfd, update_post_format.c_str(), update_post_format.size(), 0);
		return ret;
	}
	else{
		item = argu[2];
		if(item != "--title" && item != "--content") {
			ret += update_post_format;
			// send(sockfd, update_post_format.c_str(), update_post_format.size(), 0);
			return ret;
		}
		else {
			if(item == "--title") item = "TITLE";
			else if(item == "--content") item= "CONTENT";
		}
	}
	string post_id = argu[1];
	string new_content;
	// may have <br>
	for(int i=3; i<argu.size(); i++) {
		if (i>3)
			new_content += ' ';
		// check <br>
		size_t pos = 0;
		while(1) {
			pos = argu[i].find("<br>", pos);
			if (pos != string::npos) {
				argu[i].replace(pos, 4, "\n");
			}
			else	
				break;
		}
		new_content += argu[i];
	}

	string user_name;
	if ( !check_online(sockfd, user_name, ret) )
        return ret;
	
	string check_autho = "SELECT * FROM POSTS WHERE POST_ID=" + post_id + ";";
	bool enter_inner = false;
	sqlite3_exec(db, check_autho.c_str(), DB_check_enter, (void*)&enter_inner, 0);

	if(enter_inner) {
		string check_upd = "SELECT * FROM POSTS WHERE POST_ID=" + post_id + " AND AUTHOR='" + user_name + "';";
		bool enter3 = false;
		sqlite3_exec(db, check_upd.c_str(), DB_check_enter, (void*)&enter3, 0);
		if (enter3) {
			if (item == "TITLE") {
				string upd_post = "UPDATE POSTS SET "+ item + " = '" + new_content + "' WHERE POST_ID=" + post_id + " AND AUTHOR='" + user_name + "';";
				sqlite3_exec(db, upd_post.c_str(), DB_check_enter, 0, 0);
			}
			string succ = "Update successfully\n";
			ret += succ;
			// send(sockfd, succ.c_str(), succ.size(), 0);
		}
		else {
			string no_autho = "Not the post owner\n";
			ret += no_autho;
			// send(sockfd, no_autho.c_str(), no_autho.size(), 0);
		}
	}
	else {
		string not_enter = "Post does not exist.\n";
		ret += not_enter;
		// send(sockfd, not_enter.c_str(), not_enter.size(), 0);
	}
	return ret;

}
// comment have no <br>
// Question: not print "index"
string comment(int sockfd, vector<string> &argu) {
	string ret;
	if (argu.size() < 3) {
		ret += comment_format;
		// send(sockfd, comment_format.c_str(), comment_format.size(), 0);
		return ret;
	}
	string user_name;

	if ( !check_online(sockfd, user_name, ret) )
        return ret;

	string post_id = argu[1];
	string check_exist = "SELECT AUTHOR FROM POSTS WHERE POST_ID=" + post_id + ";";
	bool enter = false;
	sqlite3_exec(db, check_exist.c_str(), DB_check_enter, (void*)&enter, 0);
	// post_user = post_username;

	if (!enter) {
		string not_exist = "Post does not exist.\n";
		ret += not_exist;
		// send(sockfd, not_exist.c_str(), not_exist.size(), 0);
		return ret;
	}

	//Question comment have many space ex: hi!      hi!
	string comment;

	for(int i=2; i<argu.size(); i++) {
		if (i > 2)
			comment += ' ';
		comment += argu[i];
	}
	comment = user_name + ": " + comment + "\n";
	// TODO: ADD new comment 
	string add_new_comment = "UPDATE POSTS SET COMMENT = COMMENT || '" + comment + "' WHERE POST_ID=" + post_id +";";
	sqlite3_exec(db, add_new_comment.c_str(), DB_check_enter, 0, 0);

	string succ_upd = "Comment successfully\n";
	ret += succ_upd;
	// send(sockfd, succ_upd.c_str(), succ_upd.size(), 0);
	return ret;
}