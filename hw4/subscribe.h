
#include "kafka_consumer.h"

using namespace std;

// , name, keyword
string subscribe_format = "usage: subscribe --board <board-name> --keyword <keyword>\n";
string already_sub = "Already subscribed\n";

string subscribe_(vector<string> &argu) {
    string ret;
    if (argu.size() != 5) {
        return subscribe_format;
    }
    else if (argu[1] == "--board" && argu[3] != "--keyword") {
        return subscribe_format;
    }
    else if (argu[1] == "--author" && argu[3] != "--keyword") {
        return subscribe_format;
    }
    else if (argu[1] != "--board" && argu[1] != "--author") {
        return subscribe_format;
    }

    if (argu[1] == "--board") {
        string board_name = argu[2];
        string keyword = argu[4];
        
        if ( !sub_list["Board"].count(board_name) ) {
            sub_list["Board"][board_name] = {};
            kafka_sub(1, board_name );
            sleep(1);
        }
        vector<string> &tmp = sub_list["Board"][board_name];
        for(auto &v : tmp) {
            if (v == keyword) {
                return already_sub;
            }
        }
        tmp.push_back(keyword);

    }
    else if(argu[1] == "--author") {
        string author = argu[2];
        string keyword = argu[4];
        
        if (!sub_list["Author"].count(author) ) {
            sub_list["Author"][author] = {};
            kafka_sub(1, author );
            sleep(1);
        }
        
        vector<string> &tmp = sub_list["Author"][author];
        for(auto &v : tmp) {
            if (v == keyword) {
                return already_sub;
            }
        }
        tmp.push_back(keyword);
    }
    else {
        return subscribe_format;
    }
    return string("Subscribed successfully.\n");
}

string unsubscribe_(vector<string> &argu) {
    string ret;
    string user_name;
    if (argu.size() != 3) {
        return subscribe_format;
    }
    else if (argu[1] != "--board" && argu[1] != "--author") {
        return subscribe_format;
    }
    string unsub = argu[2];


    bool succ = false;
    if (argu[1] == "--board") {
        for(auto &name : sub_list["Board"]) {
            if (unsub == name.first) {
                sub_list["Board"].erase(unsub);
                kafka_sub(0, unsub);

                succ = true;
                break;
            }
        }
    }
    else if (argu[1] == "--author") {
        for(auto & name : sub_list["Author"]) {
            if (unsub == name.first) {
                sub_list["Author"].erase(unsub);
                kafka_sub(0, unsub );

                succ = true;
                break;
            }
        }
    }
    if (!succ) {
        return string("You haven't subscribed.\n");
    }
    else {
        return string("Unsubscribed successfully.\n");
    }
}

string list_sub(vector<string> &argu) {
    string ret;

    for(auto &sub : sub_list) {
        if (sub.second.empty())
            continue;
        // cout << sub.first << ": ";
        ret += (sub.first + ": ");
        for(auto &name: sub.second) {
            // cout << name.first << ": ";
            ret += (name.first + ": ");
            for(auto &v : name.second) {
                // cout << v << ' ';
                ret += (v + ", ");
            }
            // cout << "; ";
            ret += "; ";
        }
        ret += '\n';
        // cout << '\n';
    }
    return ret;
}