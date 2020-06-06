#include <librdkafka/rdkafka.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unordered_map>
#include <vector>
std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string> > >  sub_list;

using namespace std;

const char *topics[200];           /* Argument: list of topics to subscribe to */
vector<string> Topics;
int topic_cnt = 0;           /* Number of topics to subscribe to */
bool stop_polling = 0;

rd_kafka_t *rk;          /* Consumer instance handle */


static int is_printable (void *buf, size_t size) {
        const char* buf2 = (const char*)buf;
        size_t i;

        for (i = 0 ; i < size ; i++)
                if (!isprint((int)buf2[i]))
                        return 0;

        return 1;
}
void poll() {

    while (!stop_polling) {
        rd_kafka_message_t *rkm;

        rkm = rd_kafka_consumer_poll(rk, 100);
        if (!rkm)
                continue; /* Timeout: no message within 100ms,
                            *  try again. This short timeout allows
                            *  checking for `run` at frequent intervals.
                            */

        /* consumer_poll() will return either a proper message
            * or a consumer error (rkm->err is set). */
        if (rkm->err) {
                /* Consumer errors are generally to be considered
                    * informational as the consumer will automatically
                    * try to recover from all types of errors. */
                // fprintf(stderr,
                //         "%% Consumer error: %s\n",
                //         rd_kafka_message_errstr(rkm));
                rd_kafka_message_destroy(rkm);
                continue;
        }
        int offset = rkm->offset;
        /* Proper message. */
        // printf("Message on %s [%"PRId32"] at offset %"PRId64":\n",
        //         rd_kafka_topic_name(rkm->rkt), rkm->partition,
        //         rkm->offset);

        /* Print the message key. */
        // if (rkm->key && is_printable(rkm->key, rkm->key_len))
        //         printf(" Key: %.*s\n",
        //                 (int)rkm->key_len, (const char *)rkm->key);
        // else if (rkm->key)
        //         printf(" Key: (%d bytes)\n", (int)rkm->key_len);

        // /* Print the message value/payload. */
        // if (rkm->payload && is_printable(rkm->payload, rkm->len))
        //         printf(" Value: %.*s\n",
        //                 (int)rkm->len, (const char *)rkm->payload);
        // else if (rkm->key)
        //         printf(" Value: (%d bytes)\n", (int)rkm->len);

        string topic_name = string(rd_kafka_topic_name(rkm->rkt));
        string val = string((const char*)rkm->payload);
        string board_name, keyword, user_name;
        auto pos = val.find("#");
        if (pos != string::npos){
            board_name = val.substr(0, pos);
            auto pos2 = val.find("#", pos+1);
            if (pos2 != string::npos) {
                keyword = val.substr(pos+1, pos2-(pos+1));
                user_name = val.substr(pos2+1);
            }
            else {
                // cout << "ERROR on message!\n";
            }
        }
        else {
            // cout << "ERROR on message!\n";
        }
        // cout << "board: " << board_name << '\n';
        // cout << "title: " << keyword << '\n';
        // cout << "username: " << user_name << '\n';

        bool succ = false;
        for(auto &sub : sub_list) {
            // cout << sub.first << ": ";
            for(auto &name : sub.second) {
                if (topic_name != name.first)
                    continue;
                // cout << name.first << ": ";
                for(auto &v : name.second) {
                    if ( keyword.find(v) != std::string::npos){
                        cout << "*[" << board_name << "] ";
                        cout << keyword << " - by " <<  user_name << "*\n% ";

                        // printf("[%s] ", rd_kafka_topic_name(rkm->rkt));
                        // printf("%.*s - by %s*\n%%", (int)rkm->len, (const char *)rkm->payload);
                        succ = true;
                        break;
                    }
                }
                if(succ) break;
            }
            if(succ) break;
        }

        rd_kafka_message_destroy(rkm);
    }
}

void kafka_sub(bool sub, string new_topic) {
        rd_kafka_topic_partition_list_t *subscription; /* Subscribed topics */
        rd_kafka_resp_err_t err; /* librdkafka API error code */
        
        if (sub) {
            // sub
            // topics[topic_cnt] = new_topic;
            Topics.push_back(new_topic);
            topic_cnt++;
        }
        else {
            // unsub
            char* prev;
            for(int i=0; i<topic_cnt; i++) {
                string tp = Topics[i];
                if (tp == new_topic) {
                    Topics[i] = "jfkdlsafjdsa";
                }
            }
        }
        // cout << "Sub Topics: \n";
        for(int i=0; i<topic_cnt; i++) {
            topics[i] = Topics[i].c_str();
            // cout << string(topics[i]) << '\n';
        }
        // cout << '\n';

        subscription = rd_kafka_topic_partition_list_new(topic_cnt);
        for (int i = 0 ; i < topic_cnt ; i++)
                rd_kafka_topic_partition_list_add(subscription,
                                                        topics[i],
                                                        /* the partition is ignored
                                                        * by subscribe() */
                                                        RD_KAFKA_PARTITION_UA);

        /* Subscribe to the list of topics */
        err = rd_kafka_subscribe(rk, subscription);
        if (err) {
                fprintf(stderr,
                        "%% Failed to subscribe to %d topics: %s\n",
                        subscription->cnt, rd_kafka_err2str(err));
                rd_kafka_topic_partition_list_destroy(subscription);
                rd_kafka_destroy(rk);
                return;
        }

        // fprintf(stderr,
        //         "%% Subscribed to %d topic(s), "
        //         "waiting for rebalance and messages...\n",
        //         subscription->cnt);

        rd_kafka_topic_partition_list_destroy(subscription);
}

void kafka_init(string groupId) {
    rd_kafka_conf_t *conf;   /* Temporary configuration object */
    rd_kafka_resp_err_t err; /* librdkafka API error code */
    char errstr[512];        /* librdkafka API error reporting buffer */
    const char *brokers;     /* Argument: broker list */
    const char *groupid;     /* Argument: Consumer group id */
    
    rd_kafka_topic_partition_list_t *subscription; /* Subscribed topics */
    int i;
    sub_list["Board"] = {};
    sub_list["Author"] = {};

    brokers   = "localhost:9092";
    groupid   = groupId.c_str();
    // cout << "Groupid: " << groupId << '\n';
    /*
        * Create Kafka client configuration place-holder
        */
    conf = rd_kafka_conf_new();

    /* Set bootstrap broker(s) as a comma-separated list of
        * host or host:port (default port 9092).
        * librdkafka will use the bootstrap brokers to acquire the full
        * set of brokers from the cluster. */
    if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers,
                            errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            fprintf(stderr, "%s\n", errstr);
            rd_kafka_conf_destroy(conf);
            return;
    }

    /* Set the consumer group id.
        * All consumers sharing the same group id will join the same
        * group, and the subscribed topic' partitions will be assigned
        * according to the partition.assignment.strategy
        * (consumer config property) to the consumers in the group. */
    if (rd_kafka_conf_set(conf, "group.id", groupid,
                            errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            fprintf(stderr, "%s\n", errstr);
            rd_kafka_conf_destroy(conf);
            return;
    }

    /* If there is no previously committed offset for a partition
        * the auto.offset.reset strategy will be used to decide where
        * in the partition to start fetching messages.
        * By setting this to earliest the consumer will read all messages
        * in the partition if there was no previously committed offset. */
    if (rd_kafka_conf_set(conf, "auto.offset.reset", "earliest",
                            errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            fprintf(stderr, "%s\n", errstr);
            rd_kafka_conf_destroy(conf);
            return;
    }

    /*
        * Create consumer instance.
        *
        * NOTE: rd_kafka_new() takes ownership of the conf object
        *       and the application must not reference it again after
        *       this call.
        */
    rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
    if (!rk) {
            fprintf(stderr,
                    "%% Failed to create new consumer: %s\n", errstr);
            return;
    }

    conf = NULL; /* Configuration object is now owned, and freed,
                    * by the rd_kafka_t instance. */


    /* Redirect all messages from per-partition queues to
        * the main queue so that messages can be consumed with one
        * call from all assigned partitions.
        *
        * The alternative is to poll the main queue (for events)
        * and each partition queue separately, which requires setting
        * up a rebalance callback and keeping track of the assignment:
        * but that is more complex and typically not recommended. */
    rd_kafka_poll_set_consumer(rk);
    // kafka_sub(1, "jj");
}