#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include "supchild.grpc.pb.h"
#include "assign4.grpc.pb.h"

// ./child 50051 supernode's ip:supernode's gRPC port DB's ip:DB'sport

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using supchild::SuperChild;
using supchild::SCRequest;
using supchild::SCResponse;

using grpc::Channel;
using grpc::ClientContext;
using assign4::Database;
using assign4::Request;
using assign4::Response;

using grpc::ClientAsyncResponseReader;//
using grpc::CompletionQueue;//

using grpc::ServerAsyncResponseWriter;//
using grpc::ServerCompletionQueue;//

std::string result_file;
char* myport;
char* super_pair;
char* db_pair;
int done_flag = 0;
pthread_t th_for_parsing;
std::string dbmiss_req;
std::string whole_from_super;

std::string myid;

class DatabaseClient{
	public:
		DatabaseClient(std::shared_ptr<Channel> channel) 
			: stub_(Database::NewStub(channel)) {}
		
					std::string AccessDB(const std::string& totrans){
			Request request;
			request.set_req(totrans);

			Response response;
			ClientContext context;

			Status status = stub_->AccessDB(&context, request, &response);

			if(status.ok()){
				return response.res();
			}else{
				std::cout << status.error_code() << ": " << status.error_message() << std::endl;
				return "RPC failed";
			}
		}
	
	private:
		std::unique_ptr<Database::Stub> stub_;

};
//-------------------------------------------------------------------------
class ChildSuperClient {
	public:
		ChildSuperClient(std::shared_ptr<Channel> channel) : stub_(SuperChild::NewStub(channel)) {}
		std::string SC(const std::string& csreq, const std::string& reqoptcs, const std::string& childid){
			SCRequest request;

			request.set_screq(csreq);
			request.set_screqopt(reqoptcs);
			request.set_mychildid(childid);

			SCResponse response;
			ClientContext context;
				
			//CompletionQueue cq;//
			//Status status;//
			Status status = stub_->SC(&context, request, &response);

			//std::unique_ptr<ClientAsyncResponseReader<SCResponse> > rpc(stub_->PrepareAsyncSC(&context, request, &cq));//
			//rpc->StartCall();//
			//rpc->Finish(&response, &status, (void*)1);//
			//void* got_tag;//
			//bool ok = false;//
			//GPR_ASSERT(cq.Next(&got_tag, &ok));//
			//GPR_ASSERT(got_tag == (void*)1);//
			//GPR_ASSERT(ok);//

			if(status.ok()){
				return response.scres();
			}else{
				std::cout << status.error_code() << ": " << status.error_message() << std::endl;
				return "RPC failed";
			}
		}
	private:
		std::unique_ptr<SuperChild::Stub> stub_;
};
//-------------------------------------------------------------------------
std::string do_translate(std::string word_buf, std::string req_opt){
	std::string db_pair_str(db_pair);

	//usleep(1000);
	printf("do translate!!!\n");
	//pthread_mutex_lock(&tr_mutex);
	DatabaseClient getdb(grpc::CreateChannel(db_pair_str, grpc::InsecureChannelCredentials()));
	std::string totrans(word_buf);
	std::string response = getdb.AccessDB(totrans);
	const char* resppp = response.c_str();
	
	//if no key(resp == 0), ask to super node (DB MISS)
	if((*resppp) == 0){
		if(req_opt.compare("1") == 0){//it was dbmiss handling word 
			return "NODATA";
		}
		else{//just ordinary word
			printf("DBMISS!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		
			ChildSuperClient req_dbmiss(grpc::CreateChannel(super_pair, grpc::InsecureChannelCredentials()));
			std::cout << "myid: " << myid << std::endl;//	
			std::string dbmiss_response = req_dbmiss.SC(totrans, "5", myid);
			std::cout << "dbmiss answer: " << dbmiss_response << std::endl;//	
		//there should be found dbmiss_response
			return dbmiss_response;	
		}
	}
	// else, there's existing value
	std::cout << "child received: " << response << response.length() << std::endl;	
	//pthread_mutex_unlock(&tr_mutex);
	return response;
}

void* do_parsing(void* resp){
	std::string word_buf;
	char* helper = (char*)resp;
	int word_flag = 0;
	int byte_cnt = 0;
	int resp_len = strlen((const char*)resp);
	std::string word_done;
	char* checker = (char*)resp;
	//printf("what I got: %s / resp_len: %d\n", resp, resp_len);
	
	// check resp
	/*while(1){
		if((*checker) == 0) break;
		printf("checker: %d ", (*checker));
		resp_len++;
		checker++;
	}*/

	printf("resp_len: %d\n", resp_len);
	while(byte_cnt <= resp_len){
		printf("byte cnt: %d, resp_len: %d\n",byte_cnt, resp_len);
		word_buf.clear();
		if(isalpha(*helper) || isdigit(*helper)) word_flag = 1;
		else word_flag = 0;

		if(word_flag == 1){
			while(isalpha(*helper) || isdigit(*helper)){
				if((*helper) == 0){
					word_buf += (*helper);
					byte_cnt++;
				       	break;
				}
				word_buf += (*helper);
				helper++;
				byte_cnt++;
			}
			std::cout << "word_buf is " << word_buf << std::endl; 
			word_done = do_translate(word_buf, "0");
			result_file.append(word_done);
		}
		else{
			while(!(isalpha(*helper) || isdigit(*helper))){
				//printf("helper: %d\n",(*helper));
				if((*helper) == 0){
					word_buf += (*helper);
					byte_cnt++;
				       	break;
				}
				word_buf += (*helper);
				helper++;
				byte_cnt++;
			}
			std::cout << "word_buf is " << word_buf << std::endl; 
			result_file.append(word_buf);
		}
	}
	done_flag = 1;
	printf("I'm done, so send my part with 4\n");
	std::cout << "result_file: " << result_file << std::endl;
	// send request "4" with result file to super node
	std::string super_pair_str(super_pair);
	printf("here 1111111111\n");
	std::cout << "super_pair_str " << super_pair_str << std::endl;
	ChildSuperClient givefile(grpc::CreateChannel(super_pair_str, grpc::InsecureChannelCredentials()));
	printf("here 222222222\n");
	std::cout << "myid " << myid << std::endl;
	givefile.SC(result_file, "4", myid);
	printf("here 33333333\n");
	
	return 0;
}

class SuperChildServiceImpl final : public SuperChild::Service{
	Status SC(ServerContext* context, const SCRequest* request, SCResponse* response) override {
		// get request option from super node
		printf("server is running????????????????????????????\n");	
		std::string screqopt_str = request->screqopt();
		std::cout << "req opt is: " << screqopt_str << std::endl;
		// get request from super node
		const char* resp;
		if(screqopt_str.compare("3") == 0){
			whole_from_super = request->screq();
			myid = request->mychildid();
			std::cout << "from super len: " << whole_from_super.length() << std::endl;
			resp = whole_from_super.c_str();
			printf("I got file from super node!! %s %zu\n", resp, strlen(resp));
			pthread_create(&th_for_parsing, NULL, do_parsing, (void*)resp);
			response->set_scres("OK");
			return Status::OK;
		}
		if(screqopt_str.compare("6") == 0){ //ask this word to DB server
			printf("I got dbmiss req from super node!!!!!!!!!!!!!!!!!!!!\n");
			//pthread_mutex_lock(&tr_mutex);
			dbmiss_req = request->screq();
			//std::string db_pair_str(db_pair);
			//DatabaseClient askdbmiss(grpc::CreateChannel(db_pair_str, grpc::InsecureChannelCredentials()));
			//std::string dbmiss_resp = askdbmiss.AccessDB(dbmiss_req);
			//const char* respp = dbmiss_resp.c_str();
			//my db doesn't have it, so send "NODATA"
			//if((*respp) == 0){	
				//response->set_scres("NODATA");
				//return Status::OK;
			//}
			//else{//mydb has it, so send the data found
				//response->set_scres(dbmiss_resp);
				//return Status::OK;
			//}
			std::string dbmiss_answer = do_translate(dbmiss_req, "1");
			response->set_scres(dbmiss_answer);
			//pthread_mutex_unlock(&tr_mutex);
			return Status::OK;
		}

		return Status::OK;
		//if(done_flag == 1){//I'm done translating, so now send Status::OK
		  //     	response->set_scres(result_file);
		//	return Status::OK;
		//}
	}
};

/*class SCServerImpl final{
	public:
		~SCServerImpl(){
			server_->Shutdown();
			cq_->Shutdown();
		}
		void Run(){
			std::string my = "0.0.0.0:";
			std::string myport_str(myport);
			my.append(myport_str);
			std::string server_address(my);
			
			ServerBuilder builder;
			builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
			builder.RegisterService(&service_);
			cq_ = builder.AddCompletionQueue();
			server_ = builder.BuildAndStart();
			std::cout << "Server listening on " << server_address << std::endl;

			HandleRpcs();
		}

	private:
		class CallData {
			public:
				CallData(SuperChild::AsyncService* service, ServerCompletionQueue* cq) : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
					Proceed();
				}
				void Proceed(){
					if(status_ == CREATE) {
						status_ = PROCESS;
						service_->RequestSC(&ctx_, &request_, &responder_, cq_, cq_, this);
					}
					else if(status_ == PROCESS) {
						new CallData(service_, cq_);
						////////////////////////////////////
						////////////////////////////////////
						status_ = FINISH;
						responder_.Finish(response_, Status::OK, this);
					}
					else{
						GPR_ASSERT(status_ == FINISH);

						delete this;
					}
				}
			private:
				SuperChild::AsyncService* service_;
				ServerCompletionQueue* cq_;
				ServerContext ctx_;
				SCRequest request_;
				SCResponse response_;
				ServerAsyncResponseWriter<SCResponse> responder_;
				enum CallStatus{CREATE, PROCESS, FINISH};
				CallStatus status_;
		};

		void HandleRpcs() {
			new CallData(&service_, cq_.get());
			void* tag;
			bool ok;

			while(true){
				GPR_ASSERT(cq_->Next(&tag, &ok));
				GPR_ASSERT(ok);
				static_cast<CallData*>(tag)->Proceed();
			}
		}

		std::unique_ptr<ServerCompletionQueue> cq_;
		SuperChild::AsyncService service_;
		std::unique_ptr<Server> server_;
};*/

void RunServer(){
	std::string mine = "0.0.0.0:";
	std::string myport_str(myport);
	mine.append(myport_str);
	std::string server_address(mine);
	SuperChildServiceImpl service;
	ServerBuilder builder;

	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	
	std::unique_ptr<Server> server(builder.BuildAndStart());

	server->Wait();
}

void* sc_server_th_routine(void* arg){
	RunServer();
	return 0;
}

int main(int argc, char* argv[]) {
    /* Change as you wish */
	pthread_t server_th;
	myport = argv[1];
	super_pair = argv[2];
	db_pair = argv[3];
	
	pthread_create(&server_th, NULL, sc_server_th_routine, NULL);
	pthread_join(server_th, NULL);
	//RunServer(argc, argv);
	printf("here???\n");
    	return 0;
}
