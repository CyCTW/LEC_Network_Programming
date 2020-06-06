#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>

#include <aws/s3/model/Object.h>
#include <aws/s3/model/Bucket.h>

int main(int argc, char** argv)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        // snippet-start:[s3.cpp.list_buckets.code]
        Aws::S3::S3Client s3_client;
        auto outcome = s3_client.ListBuckets();
        
        

        if (outcome.IsSuccess())
        {
            std::cout << "Your Amazon S3 buckets:" << std::endl;

            Aws::Vector<Aws::S3::Model::Bucket> bucket_list =
                outcome.GetResult().GetBuckets();

            // Delete buckets
            const Aws::String user_region = (argc >= 3) ? argv[2] : "us-east-1";
            Aws::Client::ClientConfiguration config;
            config.region = user_region;
            Aws::S3::S3Client s3_client(config);

            Aws::S3::Model::DeleteBucketRequest bucket_request;
            for (auto const &bucket : bucket_list)
            {
                Aws::S3::Model::ListObjectsRequest objects_request;
                objects_request.WithBucket(bucket.GetName());

                auto list_objects_outcome = s3_client.ListObjects(objects_request);

                if (list_objects_outcome.IsSuccess())
                {
                    Aws::Vector<Aws::S3::Model::Object> object_list =
                        list_objects_outcome.GetResult().GetContents();

                    for (auto const &s3_object : object_list)
                    {
                        Aws::S3::Model::DeleteObjectRequest object_request;
                        object_request.WithBucket(bucket.GetName()).WithKey(s3_object.GetKey());

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
                        // std::cout << "* " << s3_object.GetKey() << std::endl;
                    }
                }
                else
                {
                    std::cout << "ListObjects error: " <<
                        list_objects_outcome.GetError().GetExceptionName() << " " <<
                        list_objects_outcome.GetError().GetMessage() << std::endl;
                }
                // Delete objects

                bucket_request.SetBucket(bucket.GetName());
                auto outcome = s3_client.DeleteBucket(bucket_request);
                if (outcome.IsSuccess())
                {
                    std::cout << "Done!" << std::endl;
                }
                else
                {
                    std::cout << "DeleteBucket error: "
                        << outcome.GetError().GetExceptionName() << " - "
                        << outcome.GetError().GetMessage() << std::endl;
                }
                    // std::cout << "  * " << bucket.GetName() << std::endl;
            }
        }
        else
        {
            std::cout << "ListBuckets error: "
                << outcome.GetError().GetExceptionName() << " - "
                << outcome.GetError().GetMessage() << std::endl;
        }
        // snippet-end:[s3.cpp.list_buckets.code]
    }
    Aws::ShutdownAPI(options);
}