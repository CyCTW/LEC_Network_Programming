#include<iostream>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>

#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <fstream>
#include <sstream>
#include <sys/stat.h>

using namespace std;

map<string, Aws::String> user_bucket_tb;
int recv_state(int recv_st) {
	if (recv_st <= 0) {
		if (recv_st == 0) 
			cerr << "lose connection.\n";
		else
			cerr << "recv error.\n";
		return -1;
	}
	return 0;
}
inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}
// AWS S3 Function
bool create_bucket(const Aws::String &bucket_name,
    const Aws::S3::Model::BucketLocationConstraint &region = Aws::S3::Model::BucketLocationConstraint::us_east_1)
{
    // Set up the request
    Aws::S3::Model::CreateBucketRequest request;
    request.SetBucket(bucket_name);


    // Is the region other than us-east-1 (N. Virginia)?
    if (region != Aws::S3::Model::BucketLocationConstraint::us_east_1)
    {
        // Specify the region as a location constraint
        Aws::S3::Model::CreateBucketConfiguration bucket_config;
        bucket_config.SetLocationConstraint(region);
        request.SetCreateBucketConfiguration(bucket_config);
    }

    // Create the bucket
    Aws::S3::S3Client s3_client;
    auto outcome = s3_client.CreateBucket(request);
    if (!outcome.IsSuccess())
    {
        auto err = outcome.GetError();
        std::cout << "ERROR: CreateBucket: " << 
            err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
        return false;
    }
    return true;
}
void upload_awsObject(string bucket_name, string post_name) {
	// upload content of this post to bucket
	const Aws::String awsbucket_name(bucket_name.c_str(), bucket_name.size());
	const Aws::String awspost_name(post_name.c_str(), post_name.size());
	if (!file_exists(post_name)) {
		std::cout << "ERROR: NoSuchFile: The specified file does not exist" 
			<< std::endl;
		exit(-1);
	}
	Aws::Client::ClientConfiguration clientConfig;

	Aws::S3::S3Client s3_client(clientConfig);
	Aws::S3::Model::PutObjectRequest object_request;
	
	object_request.SetBucket(awsbucket_name);
	object_request.SetKey(awspost_name);
	const std::shared_ptr<Aws::IOStream> input_data = 
		Aws::MakeShared<Aws::FStream>("SampleAllocationTag", 
						awspost_name.c_str(), 
						std::ios_base::in | std::ios_base::binary);
	object_request.SetBody(input_data);

	// Put object
	auto put_object_outcome = s3_client.PutObject(object_request);
	if (!put_object_outcome.IsSuccess()) {
		auto error = put_object_outcome.GetError();
		std::cout << "ERROR: " << error.GetExceptionName() << ": " 
			<< error.GetMessage() << std::endl;
	}
}
void update_awsObject(string b_name, string p_id, string &comment) {
	const Aws::String bucket_name(b_name.c_str(), b_name.size());
	
	const Aws::String object_name(p_id.c_str(), p_id.size());

	// Set up the request
	Aws::S3::S3Client s3_client;
	Aws::S3::Model::GetObjectRequest object_request;
	object_request.SetBucket(bucket_name);
	object_request.SetKey(object_name);

	// Get the object
	auto get_object_outcome = s3_client.GetObject(object_request);
	if (get_object_outcome.IsSuccess())
	{
		// Get an Aws::IOStream reference to the retrieved file
		auto &retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();

		// Output the first line of the retrieved text file
		// std::cout << "Beginning of file contents:\n";
		char file_data[255] = { 0 };
		bool save = false;
		while(retrieved_file.getline(file_data, 254)) {
			// std::cout << file_data << std::endl;
			string lines = string(file_data).substr(0, 2);
			if (lines == "--") {
				save = true;
				continue;
			}
			if(save) {
				comment += string(file_data);
			}
			memset(file_data, 0, sizeof(file_data));
		}
	}
	else
	{
		auto error = get_object_outcome.GetError();
		std::cout << "ERROR: " << error.GetExceptionName() << ": " 
			<< error.GetMessage() << std::endl;
	}

}
void get_awsObject(string b_name, string p_id, string &buf_str) {
	const Aws::String bucket_name(b_name.c_str(), b_name.size());
	// string p_id = "NP_HW3_" + post_id;
	const Aws::String object_name(p_id.c_str(), p_id.size());

	// Set up the request
	Aws::S3::S3Client s3_client;
	Aws::S3::Model::GetObjectRequest object_request;
	object_request.SetBucket(bucket_name);
	object_request.SetKey(object_name);

	// Get the object
	auto get_object_outcome = s3_client.GetObject(object_request);
	if (get_object_outcome.IsSuccess())
	{
		// Get an Aws::IOStream reference to the retrieved file
		auto &retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();

		// Output the first line of the retrieved text file
		// std::cout << "Beginning of file contents:\n";
		char file_data[255] = { 0 };
		buf_str += "--\n";
		while(retrieved_file.getline(file_data, 254)) {
			// std::cout << file_data << std::endl;
			buf_str += string(file_data);
			buf_str += '\n';
			memset(file_data, 0, sizeof(file_data));
		}
		// buf_str += "% ";
	}
	else
	{
		auto error = get_object_outcome.GetError();
		std::cout << "ERROR: " << error.GetExceptionName() << ": " 
			<< error.GetMessage() << std::endl;
	}
}

void delete_awsObject(string b_name, string p_id) {
	
	const Aws::String bucket_name(b_name.c_str(), b_name.size());
	const Aws::String key_name(p_id.c_str(), p_id.size());

	std::cout << "Deleting" << key_name << " from S3 bucket: " <<
		bucket_name << std::endl;

	// snippet-start:[s3.cpp.delete_object.code]
	Aws::S3::S3Client s3_client;

	Aws::S3::Model::DeleteObjectRequest object_request;
	object_request.WithBucket(bucket_name).WithKey(key_name);

	auto delete_object_outcome = s3_client.DeleteObject(object_request);

	if (delete_object_outcome.IsSuccess())
	{
		std::cout << "Done!" << std::endl;
	}
	else
	{
		std::cout << "DeleteObject error: " <<
			delete_object_outcome.GetError().GetExceptionName() << " " <<
			delete_object_outcome.GetError().GetMessage() << std::endl;
	}
	// snippet-end:[s3.cpp.delete_object.code]
	
	// store metadata in server
	
}


void find_br(string &content) {
	int	pos = 0;
	while(1) {
		pos = content.find("<br>", pos);
		if (pos != string::npos) {
			content.replace(pos, 4, "\n");
		}
		else
			break;					
	}
}


int main(int argc, char const **argv) {
	if(argc != 2) {
		cerr<<"Wrong format. Should be " << argv[0] <<  "{PORT}.\n";
		return -1;
	}
	int portnum = atoi(argv[1]);
	
	struct sockaddr_in serv_addr;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		cerr << "error opening socket.\n";
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portnum);
	// store IP in addr
	if (inet_pton(AF_INET, "140.113.123.236", &serv_addr.sin_addr) <= 0) {
		cerr << "Invalid Address\n";
		return -1;
	}
	// connetcion
	if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		cerr << "Error on connecting";
		return -1;
	}

	char buff[1024];
	memset(buff, 0, sizeof(buff));
	int recv_st = recv(sockfd, buff, 1023, 0);
	if (recv_state(recv_st) == -1) return -1;

	string buf_str = string(buff);
	cout << buf_str;
	int postId = 0;

	while(true) {
		
		// get user input
		string input, tp;

		char c =cin.get();
		if(c!='\n'){
			input += c;
			getline(cin, tp);
			input += tp;
		}
		else{
			input = "#";
		}

		stringstream ss(input);
		string ret, tmp;
		vector<string> argu;
		while(ss >> tmp) {
			argu.push_back(tmp);		
		}
		char buf[1024];
		memset(buf, 0, sizeof(buf));

		// string buf_str = string(buf);
		// cout << buf_str;

		// empty input
		if(argu.empty()) {
			send(sockfd, "#", 1, 0);
			continue;
		}
		// send siginal
		send(sockfd, input.c_str(), input.size(), 0);
		int recv_st = recv(sockfd, buf, 1023, 0);
		
		if (recv_state(recv_st) == -1) return -1;

		string buf_str = string(buf);
		int key_idx = 898;

		Aws::SDKOptions options;
		Aws::InitAPI(options);
		{
		if( argu[0] == "register") {

			if (buf_str.substr(0, 8) == "Register") {
				// create new bucket in S3
				string b_name = argu[1];
				transform(b_name.begin(),b_name.end(),b_name.begin(),::tolower);
				
				b_name += to_string(key_idx);
				key_idx++;
				b_name = "0616225nphw3" + b_name;
				// cout << b_name << '\n';
				const Aws::String bucket_name(b_name.c_str(), b_name.size());

				user_bucket_tb[argu[1]] = bucket_name;
 
				if (!create_bucket(bucket_name)) {
					cerr << "Create bucket error!\n";
				}
								
				// store metadata in server
				send(sockfd, bucket_name.c_str(), bucket_name.size(), 0);
				recv_st = recv(sockfd, buf, 1023, 0);
				if (recv_state(recv_st) == -1) return -1;

			}
		}
		else if( argu[0] == "login") {
			if (buf_str.substr(0, 7) == "Welcome") {
				// login with its Amazon S3 bucket
			}
		}
		else if( argu[0] == "create-post") {
			if (buf_str.substr(0, 6) == "Create") {
				// get bucket name
				char c_buf[256];
				memset(c_buf, 0, sizeof(c_buf));
				send(sockfd, "#", 1, 0);
				recv(sockfd, c_buf, 255, 0);
				string tpp = string(c_buf);
				
				stringstream ssm(tpp);
				string bucket_name, post_id;
				int p_id;
				ssm >> bucket_name >> post_id;

				// write file first
				ofstream f;
				string post_name = "NP_HW3_POST_" + post_id;

				// cout << post_name << '\n';
				// cout << bucket_name << '\n';			
				f.open(post_name);

				// deal with content
				int pos = input.find("--content");
				string content = input.substr(pos+10);
				find_br(content);
				content += "\n--\n";
				f << content;
				f.close();
				upload_awsObject(bucket_name, post_name);
				
			}
		}
		else if( argu[0] == "read") {
			if (buf_str.substr(0, 6) == "Author") {
				//  get content of the post in S3 using metadata
				char c_buf[256];
				memset(c_buf, 0, sizeof(c_buf));
				send(sockfd, "#", 1, 0);
				recv(sockfd, c_buf, 255, 0);
				string b_name = string(c_buf);
				string post_name = "NP_HW3_POST_" + argu[1];
				get_awsObject(b_name, post_name, buf_str);
				
			}
			buf_str += "% ";
		}
		else if( argu[0] == "update-post") {
			if (buf_str.substr(0, 6) == "Update") {
				// update content on S3

				if (argu[2] == "--content") {
					// get bucket name
					char c_buf[256];
					memset(c_buf, 0, sizeof(c_buf));
					send(sockfd, "#", 1, 0);
					recv(sockfd, c_buf, 255, 0);
					string bucket_name = string(c_buf);
					string post_id = argu[1];
					string post_name = "NP_HW3_POST_" + post_id;
					string comment;
					// get comment 
					update_awsObject(bucket_name, post_name, comment);

					// write file first
					ofstream f;

					cout << post_name << '\n';
					cout << bucket_name << '\n';
					f.open(post_name);

					// deal with content
					int pos = input.find("--content");
					string content = input.substr(pos+10);
					
					find_br(content);
					content += "\n--\n";
					
					f << content;
					f << comment;
					f.close();

					upload_awsObject(bucket_name, post_name);
				}

			}
		
		}
		else if( argu[0] == "comment") {
			if (buf_str.substr(0, 7) == "Comment") {
				// get bucket name
				char c_buf[256];
				memset(c_buf, 0, sizeof(c_buf));
				send(sockfd, "#", 1, 0);
				recv(sockfd, c_buf, 255, 0);
				string bucket_name;
				string user_name;
				string msg = string(c_buf);
				stringstream ss;
				ss << string(c_buf);
				ss >> user_name >> bucket_name;
				string post_id = argu[1];

				// write file first
				ofstream f;
				string post_name = "NP_HW3_POST_" + post_id;

				cout << post_name << '\n';
				cout << bucket_name << '\n';
				f.open(post_name, std::ios_base::app);

				// deal with content
				int pos = input.find(argu[2]);
				string content = user_name + ": " + input.substr(pos);
				find_br(content);
				f << '\n';
				f << content;
				f.close();
				upload_awsObject(bucket_name, post_name);
			}
		}
		else if( argu[0] == "delete-post") {
			if (buf_str.substr(0, 6) == "Delete") {
				// delete the post object on S3
				// get bucket name
				char c_buf[256];
				memset(c_buf, 0, sizeof(c_buf));
				send(sockfd, "#", 1, 0);
				recv(sockfd, c_buf, 255, 0);
				string b_name = string(c_buf);
				string post_name = "NP_HW3_POST_" + argu[1];
				// delete content of this post to bucket
				delete_awsObject(b_name, post_name);
			}
		}
		else if (argu[0] == "mail-to"){
			if (buf_str.substr(0, 4) == "Sent") {
				// get bucket name
				char c_buf[256];
				memset(c_buf, 0, sizeof(c_buf));
				send(sockfd, "#", 1, 0);
				recv(sockfd, c_buf, 255, 0);
				string tpp = string(c_buf);
				
				stringstream ssm(tpp);
				string bucket_name, mail_id;
				int p_id;
				ssm >> bucket_name >> mail_id;

				// write file first
				ofstream f;
				string post_name = "NP_HW3_MAIL_" + mail_id;

				// cout << post_name << '\n';
				// cout << bucket_name << '\n';			
				f.open(post_name);

				// deal with content
				int pos = input.find("--content");
				string content = input.substr(pos+10);
				find_br(content);
				// content += "--\n";
				f << content;
				f << "\n";
				f.close();
				upload_awsObject(bucket_name, post_name);

			}
		}
		else if( argu[0] == "retr-mail") {
			if (buf_str.substr(0,1) == "S") {
				char c_buf[256];
				memset(c_buf, 0, sizeof(c_buf));
				send(sockfd, "#", 1, 0);
				recv(sockfd, c_buf, 255, 0);

				string bucket_name = string(c_buf);
				string mail_name = "NP_HW3_MAIL_" + argu[1];
				get_awsObject(bucket_name, mail_name, buf_str);
			}
			buf_str += "% ";
			
		}
		else if( argu[0] == "delete-mail") {
			if(buf_str.substr(0, 1) == "M") {
				char c_buf[256];
				memset(c_buf, 0, sizeof(c_buf));
				send(sockfd, "#", 1, 0);
				recv(sockfd, c_buf, 255, 0);
				string b_name = string(c_buf);
				string mail_name = "NP_HW3_MAIL_" + argu[1];
				// delete content of this post to bucket
				delete_awsObject(b_name, mail_name);
			}
			
		}
		
		
		}
		Aws::ShutdownAPI(options);
		cout << buf_str;

		// new command



	}



}
