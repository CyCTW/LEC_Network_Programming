#include "source.h"

// funciton: mail_to, list_mail, retr_mail, delete-mail;

string mail_to(int sockfd, vector<string> &argu) {
	string ret;
	if (argu.size() < 6 ) {
		ret += mail_to_format;
		// send(sockfd, comment_format.c_str(), comment_format.size(), 0);
		return ret;
	}
	if (argu[2] != "--subject") {
		ret += mail_to_format;
		return ret;
	}

	string user_name;
    if ( !check_online(sockfd, user_name, ret) ) {
        return ret;
    }

	string recv_user = argu[1];
	string subject;
	for(int i=3; i < argu.size(); i++) {
		if(argu[i] == "--content")
			break;
		if (i > 3)
			subject += ' ';
		subject += argu[i];
	}

	string check_user = "SELECT * FROM USERS WHERE USERNAME='" + recv_user + "';";
	bool enter = false;
	sqlite3_exec(db, check_user.c_str(), DB_check_enter, (void*)&enter, 0);
	if(!enter) {
		ret += recv_user;
		ret += " does not exist.\n";
		return ret;
	}
	auto now = chrono::system_clock::now();
	auto in_time_t = chrono::system_clock::to_time_t(now);

	stringstream ss;
	string date_;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
	ss >> date_;

	user_mailid[recv_user]++;
	int mail_id = user_mailid[recv_user];
	string store_maildata = "INSERT INTO MAILS (TO_USER, MAILID, SUBJECT, FROM_USER, DATE)"\
							"VALUES ('" + recv_user + "', '" + to_string(mail_id) + "', '" + subject + "', '" + user_name +"', '" + date_ + "');";
	sqlite3_exec(db, store_maildata.c_str(), 0, 0, 0);

	ret += "Sent successfully.\n";
	return ret;
}

string list_mail(int sockfd, vector<string> &argu) {
	string ret;
	if (argu.size() != 1) {
		ret += list_mail_format;
		// send(sockfd, comment_format.c_str(), comment_format.size(), 0);
		return ret;
	}
	
	string user_name;
	if ( !check_online(sockfd, user_name, ret) )
		return ret;
	string indexes = "ID        Subject   From      Date      \n";
	ret += indexes;
	string list_mails = "SELECT MAILID, SUBJECT, FROM_USER, DATE FROM MAILS WHERE TO_USER='" + user_name + "';";
	bool enter = false;
	sqlite3_exec(db, list_mails.c_str(), DB_list_data, (void*)&enter, 0);

	ret += glo_ret;
	glo_ret.clear();
	return ret;
}

string retr_mail(int sockfd, vector<string> &argu) {
	string ret;
	if (argu.size() != 2) {
		ret += retr_mail_format;
		// send(sockfd, comment_format.c_str(), comment_format.size(), 0);
		return ret;
	}

	string user_name;
	if ( !check_online(sockfd, user_name, ret) )
		return ret;
	cout << "username is: " << user_name << '\n';
	string mail_id = argu[1];
	string read_mail = "SELECT SUBJECT, FROM_USER, DATE FROM MAILS WHERE MAILID=" + mail_id + " AND TO_USER='" + user_name + "';";
	// pair<int, bool> fd_enter = {sockfd, false};
	pair<int, bool > fd_enter = {1, false};
	
	sqlite3_exec(db, read_mail.c_str(), DB_read_posts, (void*)&fd_enter, 0);
	ret += glo_ret;
	glo_ret.clear();
	if (fd_enter.second == false) {
		string no_posts = "No such mail.\n";
		ret += no_posts;
	}
	return ret;
}

string delete_mail(int sockfd, vector<string> &argu) {
	string ret;
	if (argu.size() != 2) {
		ret += delete_mail_format;
		return ret;
	}
	string user_name;
	if ( !check_online(sockfd, user_name, ret) )
		return ret;

	string mail_id = argu[1];
	string check_mail = "SELECT * FROM MAILS WHERE MAILID=" + mail_id + " AND TO_USER='" + user_name + "';"; 
	bool enter = false;
	sqlite3_exec(db, check_mail.c_str(), DB_check_enter, (void*)&enter, 0);
	
	if (!enter) {
		ret += "No such mail\n";
		return ret;
	}
	else {
    	user_mailid[user_name]--;
		string delete_mail = "DELETE FROM MAILS WHERE MAILID=" + mail_id + " AND TO_USER='" + user_name + "';";
		sqlite3_exec(db, delete_mail.c_str(), 0, 0, 0);
        for(int i=0; i<user_mailid[user_name]; i++) {
            string update_id = "UPDATE MAILS SET MAILID=" + to_string(i+1) + " WHERE TO_USER= '" + user_name + "' LIMIT 1 OFFSET " + to_string(i) +";";
            sqlite3_exec(db, update_id.c_str(), 0, 0, 0);
        }
		ret += "Mail deleted.\n";
		return ret;
	}
}
