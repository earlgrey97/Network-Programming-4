#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <event2/event.h>

// cpp lib
#include <iostream>
#include <memory>
#include <string>
#include <grpc++/grpc++.h>
#include "supchild.grpc.pb.h"
#include "supsup.grpc.pb.h"

#define MAXBUFLEN 65535
#define BACKLOG 10

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using supchild::SuperChild;
using supchild::SCRequest;
using supchild::SCResponse;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using supsup::SuperSuper;
using supsup::SSRequest;
using supsup::SSResponse;

using grpc::ClientAsyncResponseReader;//
using grpc::CompletionQueue;//

using grpc::ServerAsyncResponseWriter;//
using grpc::ServerCompletionQueue;//
// ./super 12345 [gRPC port] [child1's ip]:[child1's port] ....
// ./super 12346 [gRPC port] -s [another super's ip]:[another super's port] [child4's ip]:[child4's port] ...

char* myport;
char* grpc_port;
char* super1_pair;

std::string super2_pair;

char* part1;;
char* part2;

int is_super = 0;

pthread_t th_for_cli;
pthread_t th_for_job;

std::string mypart_str;
int child_num;

/* functions */
void* get_file_and_split(void* arg);
void* do_my_job(void* mypart);

char* child_pair[5];
//std::string child_pair1;
//std::string child_pair2;
//std::string child_pair3;
//std::string child_pair4;
//std::string child_pair5;

char child_part1[MAXBUFLEN];
char child_part2[MAXBUFLEN];
char child_part3[MAXBUFLEN];
char child_part4[MAXBUFLEN];
char child_part5[MAXBUFLEN];

struct for_child{
	int my_id;
	//char my_child_part[MAXBUFLEN];
	//int len;
};
std::string child_done1;
std::string child_done2;
std::string child_done3;
std::string child_done4;
std::string child_done5;
int child_done = 0; //number of child done
std::string other_part_done;
std::string my_part_done;
std::string whole_done_str;
int other_done = 0;
int im_done = 0;

int master_flag = 0; //if I'm the one who got request, it's 1

int nodb_cnt = 0; //count for no db from req "6"

int cli_sockfd; //client sockfd

/* ----------SuperChildClient---------- */
class SuperChildClient {
	public:
		SuperChildClient(std::shared_ptr<Channel> channel) : stub_(SuperChild::NewStub(channel)) {}
		std::string SC(const std::string& user, const std::string& reqoptsc, const std::string& childid) {
			SCRequest request;
			
			request.set_screq(user);
			request.set_screqopt(reqoptsc);
			request.set_mychildid(childid);

			SCResponse response;
			ClientContext context;
			//CompletionQueue cq;//
			Status status = stub_->SC(&context, request, &response);
			//Status status;//
			
			//std::unique_ptr<ClientAsyncResponseReader<SCResponse> > rpc(stub_->PrepareAsyncSC(&context, request, &cq));//

			//rpc->StartCall();//

			//rpc->Finish(&response, &status, (void*)1);//
			//void* got_tag;//
			//bool ok = false;//
			//GPR_ASSERT(cq.Next(&got_tag, &ok));//
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
/* ----------SuperSuperClient---------- */
class SuperSuperClient {
	public:
		SuperSuperClient(std::shared_ptr<Channel> channel) : stub_(SuperSuper::NewStub(channel)) {}
		std::string SS(const std::string& reqbysup, const std::string& reqopt) {
			SSRequest request;
			request.set_ssreq(reqbysup);
			request.set_ssreqopt(reqopt);

			SSResponse response;
			ClientContext context;
			//CompletionQueue cq;//
			//Status status;//
			//std::unique_ptr<ClientAsyncResponseReader<SSResponse> > rpc(stub_->PrepareAsyncSS(&context, request, &cq));//
			//rpc->StartCall();//
			//rpc->Finish(&response, &status, (void*)1);//
			//void* got_tag;//
			//bool ok = false;//
			//GPR_ASSERT(cq.Next(&got_tag, &ok));//
			//GPR_ASSERT(got_tag == (void*)1);//
			//GPR_ASSERT(ok);//

			Status status = stub_->SS(&context, request, &response);

			if(status.ok()){
				return response.ssres();
			}else{
				std::cout << status.error_code() << ": " << status.error_message() << std::endl;
				return "RPC failed";
			}
		}
	private:
		std::unique_ptr<SuperSuper::Stub> stub_;
};
/*---------------------------------------------------*/
/*---------------------------------------------------*/
/*---------------------------------------------------*/
/*---------------------------------------------------*/
std::string handle_dbmiss_req(std::string tofind, std::string childid)
{
	std::string db_resp1;
	std::string db_resp2;
	std::string db_resp3;
	std::string db_resp4;
	std::string db_resp5;

	printf("HANDLE DB MISS FOR CHILD!!!!\n");	
	std::string db_respopt;
	nodb_cnt = 0;
	// send DBMISS req to rest of childs
	if((child_num >= 1) && !(childid.compare("0") == 0)){ 
		printf("send this req to child 1\n");
		std::string child_pair1(child_pair[0]);
		SuperChildClient db_super_child1(grpc::CreateChannel(child_pair1, grpc::InsecureChannelCredentials()));
		std::cout << "child_pair1 " << child_pair1 << std::endl;
		std::cout << "tofind " << tofind << std::endl;
		db_resp1 = db_super_child1.SC(tofind, "6", "hey");
		std::cout << "I got this from child 1" << db_resp1 << std::endl;
		if(db_resp1.compare("NODATA") == 0) nodb_cnt++;
		else return db_resp1; //child found it.
	}	
	if(child_num >= 2){
		printf("send this req to child 2\n");
		std::string child_pair2(child_pair[1]);
		SuperChildClient db_super_child2(grpc::CreateChannel(child_pair2, grpc::InsecureChannelCredentials()));
		db_resp2 = db_super_child2.SC(tofind, "6", "hey");
		if(db_resp2.compare("NODATA") == 0) nodb_cnt++;
		else return db_resp2;
	}	
	if(child_num >= 3){
		std::string child_pair3(child_pair[2]);
		SuperChildClient db_super_child3(grpc::CreateChannel(child_pair3, grpc::InsecureChannelCredentials()));
		db_resp3 = db_super_child3.SC(tofind, "6", "hey");
		if(db_resp3.compare("NODATA") == 0) nodb_cnt++;
		else return db_resp3;
	}	
	if(child_num >= 4){
		std::string child_pair4(child_pair[3]);
		SuperChildClient db_super_child4(grpc::CreateChannel(child_pair4, grpc::InsecureChannelCredentials()));
		db_resp4 = db_super_child4.SC(tofind, "6", "hey");
		if(db_resp4.compare("NODATA") == 0) nodb_cnt++;
		else return db_resp4;
	}	
	if(child_num >= 5){
		std::string child_pair5(child_pair[4]);
		SuperChildClient db_super_child5(grpc::CreateChannel(child_pair5, grpc::InsecureChannelCredentials()));
		db_resp5 = db_super_child5.SC(tofind, "6", "hey");
		if(db_resp5.compare("NODATA") == 0) nodb_cnt++;
		else return db_resp5;
	}	
	// if all responses are "7", ask to super as supersuper client
	
	printf("MY CHILDS DOESNT HAVE THE DATA\n");
	if(is_super == 1){ // I'm super1. so, ask to super2
		SuperSuperClient db_super_super2(grpc::CreateChannel(super2_pair, grpc::InsecureChannelCredentials()));
		std::string found2 = db_super_super2.SS(tofind, "8");
		//this must be found
		return found2;		
	}
	else{ // I'm super2, so ask to super1
		printf("I'm super2, so ask to super1\n");
			
		std::string super1_pair_str(super1_pair);
		SuperSuperClient db_super_super1(grpc::CreateChannel(super1_pair_str, grpc::InsecureChannelCredentials()));
		std::string found1 = db_super_super1.SS(tofind, "8");
			//this must be found
		return found1;			
	}
	
	
	//control must not reach here
	//return 0;
}
/*--------------------------------------------------------------------------*/
void merge_all(){
	printf("-------now merge-----------------------------------\n");
	if(child_num >= 1){
		my_part_done.append(child_done1);
	}
	if(child_num >= 2){
		my_part_done.append(child_done2);
	}
	if(child_num >= 3){
		my_part_done.append(child_done3);
	}
	if(child_num >= 4){
		my_part_done.append(child_done4);
	}
	if(child_num >= 5){
		my_part_done.append(child_done5);
	}
	im_done = 1;
	std::cout << "my part done: " << my_part_done << std::endl;
}
/* ---------SuperChildServer----------- */
class DBMISSService final : public SuperChild::Service {
	Status SC(ServerContext* context, const SCRequest* request, SCResponse* response) override{
		std::string screqopt_str = request->screqopt();
		std::string screq_str;
		std::string foundstr;
		std::string result_from_child;
		std::string child_id;

		if(screqopt_str.compare("5") == 0){//DBMISS req from child
			screq_str = request->screq();
			child_id = request->mychildid();
			std::cout << "I got request for finding DBMISS '5'" << screq_str << std::endl;
			foundstr = handle_dbmiss_req(screq_str, child_id);
			// there always should be foundstr
			// set response to child(who requested first)	
			response->set_scres(foundstr);	
		}
		if(screqopt_str.compare("4") == 0){//child is done translating
			printf("child is done translating\n");
			result_from_child = request->screq();
			child_id = request->mychildid();
			std::cout << "result from child" << result_from_child << std::endl;
			std::cout << "child_id" << child_id << std::endl;
			if(child_id.compare("0") == 0){
				child_done1 = result_from_child;
				child_done1.pop_back();
				child_done++;
				std::cout << "child_done1: " << child_done1 << std::endl;
			}
			if(child_id.compare("1") == 0){
				child_done2 = result_from_child;
				child_done2.pop_back();
				child_done++;
				std::cout << "child_done2: " << child_done2 << std::endl;
			}
			if(child_id.compare("2") == 0){
				child_done3 = result_from_child;
				child_done3.pop_back();
				child_done++;
				std::cout << "child_done2: " << child_done2 << std::endl;
			}
			if(child_id.compare("3") == 0){
				child_done4 = result_from_child;
				child_done4.pop_back();
				child_done++;
			}
			if(child_id.compare("4") == 0){
				child_done5 = result_from_child;
				child_done5.pop_back();
				child_done++;
			}
			if(child_done == child_num){
				merge_all();
				im_done = 1;
				if(master_flag == 0){
					// send this result to another super node
					printf("I'm not master...\n");
					if(is_super == 1){// to super 2
						printf(" and I'm super node 1\n");
						SuperSuperClient give_mine1(grpc::CreateChannel(super2_pair, grpc::InsecureChannelCredentials()));
						give_mine1.SS(my_part_done,"10");
					}
					else{// to super 1
						printf(" and I'm super node 2\n");
						std::string super1_pair_str(super1_pair);
						SuperSuperClient give_mine2(grpc::CreateChannel(super1_pair_str, grpc::InsecureChannelCredentials()));
						give_mine2.SS(my_part_done,"10");
					}	
				
				}
			}
					
		}
		//control shouldn't reach here
		printf("what???\n");
		return Status::OK;
	}
};

void RunSCServer() {
	printf("I'm a server listening to childs\n");
	std::string my_grpc_sc("0.0.0.0:");
	std::string grpc_port_str_sc(grpc_port);
	my_grpc_sc.append(grpc_port_str_sc);
	std::cout << "my_grpc: " << my_grpc_sc << std::endl;
	std::string server_address_sc(my_grpc_sc);

	DBMISSService service;
	ServerBuilder builder;
	builder.AddListeningPort(server_address_sc, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());

	std::cout << "SCServer listening on" << server_address_sc << std::endl;
	
	server->Wait();
}

//------------------------------------------------------------------------------

/* ----------SuperSuperServer---------- */
class Super1Service final : public SuperSuper::Service {
	Status SS(ServerContext* context, const SSRequest* request, SSResponse* response) override{
		printf("Server is running????????????\n");
		std::string ssreqopt_str = request->ssreqopt();
		if(ssreqopt_str.compare("1") == 0){
			super2_pair = request->ssreq();
			std::cout << "I got super2_pair: " << super2_pair << std::endl;//
			pthread_create(&th_for_cli, NULL, get_file_and_split, NULL); 
		}
		if(ssreqopt_str.compare("2") == 0){
			mypart_str = request->ssreq();
			std::cout << "I got mypart_str: " << mypart_str << std::endl;//
			
			//do my job
			const char* mypart = mypart_str.c_str();
			pthread_create(&th_for_job, NULL, do_my_job, (void*)mypart);
		}
		if(ssreqopt_str.compare("8") == 0){ //request for DBMISS by another super node
			std::string dbmiss1 = request->ssreq();
			// found_str should always be found
			std::string found_str1 = handle_dbmiss_req(dbmiss1, "notmine");
			response->set_ssres(found_str1);
		}
		if(ssreqopt_str.compare("10") == 0){
			//supernode2 is all done, so merge the whole
			printf("super node 2 is done!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1111111!!!!!!!!\n");
			const char* whole_done1;
			int whole_size;
			char whole_size_str[10];
			other_done = 1;
			while(im_done != 1) ;
			other_part_done = request->ssreq();
			my_part_done.append(other_part_done);
			std::cout << "other_part_done: " << other_part_done << std::endl;
			std::cout << "my_part_done: " << my_part_done << std::endl;
			whole_size = my_part_done.length();
			sprintf(whole_size_str, "%d", whole_size);
			printf("whole size: %d\n", whole_size);
			//whole_done1 = (char*)malloc(whole_size);
			//memset(whole_done1, 0, whole_size);
			whole_done1 = my_part_done.c_str();
			//send to client
			//strcpy(whole_done1, my_part_done.c_str());
			printf("whole_done1: %s\n", my_part_done.c_str());
			printf("whole_done1: %s\n", whole_done1);
			write(cli_sockfd, whole_size_str, strlen(whole_size_str));
			write(cli_sockfd, ":", 1);
			write(cli_sockfd, whole_done1, strlen(whole_done1));
			//free(whole_done1);
		}
		printf("what???\n");
		return Status::OK;
	}
};

class Super2Service final : public SuperSuper::Service {
	Status SS(ServerContext* context, const SSRequest* request, SSResponse* response) override{
		printf("Server is running????????????\n");
		std::string ssreqopt_str = request->ssreqopt();
		if(ssreqopt_str.compare("2") == 0){
			mypart_str = request->ssreq();
			std::cout << "I got mypart_str: " << mypart_str << std::endl;//
			const char* mypart = mypart_str.c_str();
			pthread_create(&th_for_job, NULL, do_my_job, (void*)mypart);
		}
		if(ssreqopt_str.compare("8") == 0){ //request for DBMISS by another super node
			std::string dbmiss2 = request->ssreq();
			// found_str should always be found
			std::string found_str2 = handle_dbmiss_req(dbmiss2, "notmine");
			response->set_ssres(found_str2);
		}
		if(ssreqopt_str.compare("10") == 0){
		//supernode1 is all done, so merge and send to client
			printf("super node 1 is done!!\n");
			const char* whole_done2;
			int whole_size2;
			char whole_size_str2[10];
			other_done = 1;
			while(im_done != 1);
			other_part_done = request->ssreq();
			my_part_done.append(other_part_done);
			whole_size2 = my_part_done.length();
			sprintf(whole_size_str2, "%d", whole_size2);
			//whole_done2 = (char*)malloc(whole_size2);
			//memset(whole_done2, 0, whole_size2);
			whole_done2 = my_part_done.c_str();
			//send to client
			//strcpy(whole_done2, my_part_done.c_str());
			printf("whole_done2: %s\n", whole_done2);
			write(cli_sockfd, whole_size_str2, strlen(whole_size_str2));
			write(cli_sockfd, ":", 1);
			write(cli_sockfd, whole_done2, strlen(whole_done2));
		}
		printf("what???\n");
		return Status::OK;
	}
};

void RunSSServer1() {
	printf("hi i'm super1 grpc  server\n");
	std::string my_grpc("0.0.0.0:");
	std::string grpc_port_str(grpc_port);
	my_grpc.append(grpc_port_str);
	std::cout << "my_grpc: " << my_grpc << std::endl;
	std::string server_address(my_grpc);

	Super1Service service;
	DBMISSService service2;
	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	builder.RegisterService(&service2);
	std::unique_ptr<Server> server(builder.BuildAndStart());

	std::cout << "Server listening on" << server_address << std::endl;
	
	server->Wait();
}

void RunSSServer2() {
	printf("hi i'm super2 grpc  server\n");
	std::string my_grpc("0.0.0.0:");
	std::string grpc_port_str(grpc_port);
	my_grpc.append(grpc_port_str);
	std::cout << "my_grpc: " << my_grpc << std::endl;
	std::string server_address(my_grpc);

	Super2Service service;
	DBMISSService service2;
	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	builder.RegisterService(&service2);
	std::unique_ptr<Server> server(builder.BuildAndStart());

	std::cout << "Server listening on" << server_address << std::endl;
	
	server->Wait();
}

void* get_file_and_split(void* arg)
{
	int my_sockfd;
	struct sockaddr_in cli_addr;
	//int cli_sockfd;
	int cli_len;
	int recv_cnt = 0;
	int recv_bytes = 0;
	char recv_buf[MAXBUFLEN];
	char* final_recv_buf;
	int flag;
	int file_size = 0;
	char file_size_buf[20];
	int read_left;
	char* helper;
	int copy_size;
	
	
	//get file from client
	my_sockfd = socket(AF_INET, SOCK_STREAM, 0);
       	if(my_sockfd < 0){
		fprintf(stderr, "super 1// socket: failed\n");
		exit(1);
	}	
	memset(&cli_addr, 0, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(atoi(myport));
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(my_sockfd, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0){
		fprintf(stderr, "super 1// bind: failed\n");
		exit(1);
	}
	if(listen(my_sockfd, BACKLOG) < 0){
		fprintf(stderr, "super 1// listen: failed\n");
		exit(1);
	}
	cli_len = sizeof(cli_addr);
	cli_sockfd = accept(my_sockfd, (struct sockaddr*)&cli_addr, (socklen_t*)&cli_len);
	if(cli_sockfd < 0){
		fprintf(stderr, "super 1//accept: failed\n");
		exit(1);
	}
	//flag = fcntl(cli_sockfd, F_GETFL, 0);
	//fcntl(cli_sockfd, F_SETFL, flag | O_NONBLOCK);
	final_recv_buf = (char*)malloc(4000000);
	
	recv(cli_sockfd, file_size_buf, 20, 0);
	file_size = atoi(file_size_buf);
	read_left = file_size;
	memset(final_recv_buf, 0, 4000000);
	while(read_left != 0){
		memset(recv_buf, 0, MAXBUFLEN);
		recv_bytes = recv(cli_sockfd, recv_buf, MAXBUFLEN, 0);
		strncpy(final_recv_buf, recv_buf, strlen(recv_buf));
		read_left = file_size - recv_bytes;
		recv_cnt++;
	}
	printf("final_recv_buf: %s %zu\n",final_recv_buf, strlen(final_recv_buf));
	part1 = (char*)malloc(file_size);
	printf("file_size: %d\n",file_size);
	memset(part1, 0, file_size);
	strncpy(part1, final_recv_buf, file_size/2);
	printf("filesize/2: %d\n",(file_size/2));
	helper = part1 + strlen(part1) - 1;
	while(1){
		if(!(isalpha(*helper) || isdigit(*helper))){
			(*helper) = '\0';
			break;
		}
		helper--;
	}
	part2 = (char*)malloc(file_size - strlen(part1));
	printf("part1 len: %zu\n",strlen(part1));
	printf("remaining: %lu\n",file_size-strlen(part1));
	memset(part2, 0, file_size - strlen(part1));
	helper = final_recv_buf + strlen(part1);
	//printf("helper: %s %zu\n",helper, strlen(helper));
	strcpy(part2, helper);
	free(final_recv_buf);
	
	printf("part1: %s len: %zu\n", part1, strlen(part1));	
	printf("part2: %s len: %zu\n", part2, strlen(part2));	
	
	std::string part2_str(part2);
	std::cout << "part 2 str len" << part2_str.length() << std::endl;	
	master_flag = 1;
	if(is_super == 1){
		//send part2 to supernode2
		SuperSuperClient send_part2_to_super2(grpc::CreateChannel(super2_pair, grpc::InsecureChannelCredentials()));
		send_part2_to_super2.SS(part2_str,"2");	
		
		//do my job
		pthread_create(&th_for_job, NULL, do_my_job, (void*)part1);
	}
	else{
		//send part2 to supernode1
		std::string super1_pair_str(super1_pair);
		SuperSuperClient send_part2_to_super1(grpc::CreateChannel(super1_pair_str, grpc::InsecureChannelCredentials()));
		send_part2_to_super1.SS(part2_str,"2");	
	
		//do my job
		pthread_create(&th_for_job, NULL, do_my_job, (void*)part1);
	}
	
	return 0;
	
}
/*-------------------------------------------------------------------*/
void* work_with_child(void* args)
{
	struct for_child* args_ptr = (struct for_child*)args;
	int my_id = args_ptr->my_id;//0 to 4
	//char* my_child_part = child_part[my_id];
	printf("let's work with child nodes: %d\n", my_id);//
	
	///strncpy(my_child_part, args_ptr->my_child_part, strlen(args_ptr->my_child_part));
	//std::string my_child_part_str(my_child_part);

	printf("my id: %d\n", my_id);
	//send part to child node(as grpc client)
	if(my_id == 0){
		printf("here 1\n");
		std::string child_part1_str(child_part1);
		//printf("I'm child_pair[0]: %s with %s\n",child_pair[0], child_part1);
		printf("here 2\n");
		std::string child_pair_str1(child_pair[my_id]);
		//child_pair1 = child_pair_str1;
		printf("here 3\n");
		//std::cout << "child_pair_str1: " << child_pair_str1 << std::endl;
		//std::cout << "child_pair1: " << child_pair1 << std::endl;
		//pthread_mutex_lock(&mutex);
		SuperChildClient super_child1(grpc::CreateChannel(child_pair_str1, grpc::InsecureChannelCredentials()));
		printf("here 4\n");
		//std::cout << "my_child_part_str: " << child_part1_str << std::endl;
		std::cout << "child_part1_str: " << child_part1_str << std::endl;
		super_child1.SC(child_part1_str, "3", "0");
		//pthread_mutex_unlock(&mutex);
		printf("here 5\n");
		//std::cout << "SuperChild receiver: " << response1 << std::endl;
	}
	else if(my_id == 1){
		printf("here 6\n");
		//printf("I'm child_pair[1]: %s with %s\n",child_pair[1], child_part2);
		std::string child_part2_str(child_part2);
		printf("here 7\n");
		std::string child_pair_str2(child_pair[my_id]);
		//child_pair2 = child_pair_str2;
		printf("here 8\n");
		//pthread_mutex_lock(&mutex);
		SuperChildClient super_child2(grpc::CreateChannel(child_pair_str2, grpc::InsecureChannelCredentials()));
		printf("here 9\n");
		std::cout << "child_part2_str: " << child_part2_str << std::endl;
		super_child2.SC(child_part2_str, "3", "1");
		//pthread_mutex_unlock(&mutex);
		//std::cout << "SuperChild receiver: " << response2 << std::endl;
		printf("here 10\n");

	}
	else if(my_id == 2){
		printf("here 11\n");
		std::string child_pair_str3(child_pair[my_id]);
		printf("here 12\n");
		std::string child_part3_str(child_part3);
		//child_pair3 = child_pair_str3;
		printf("here 13\n");
		//pthread_mutex_lock(&mutex);
		SuperChildClient super_child3(grpc::CreateChannel(child_pair_str3, grpc::InsecureChannelCredentials()));
		printf("here 14\n");
		std::cout << "child_part3_str: " << child_part3_str << std::endl;
		super_child3.SC(child_part3_str, "3", "2");
		//pthread_mutex_unlock(&mutex);
		printf("here 15\n");
		//std::cout << "SuperChild receiver: " << response3 << std::endl;
	}
	/*//put response in my_part_done[my_id]
	const char* my_done = response.c_str();
	my_part_done[my_id] = (char*)malloc(strlen(my_done));
	strcpy(my_part_done[my_id], my_done);
	*/
	return 0;
}
/*-------------------------------------------------------------------*/
void* do_my_job(void* mypart)
{
	const char* total_half = (const char*)mypart;
	//char* child_part[child_num];
	int i = 0;
	pthread_t th_for_child[child_num];
	char* helper;
	struct for_child* arg1 = (struct for_child*)malloc(sizeof(struct for_child));
	struct for_child* arg2 = (struct for_child*)malloc(sizeof(struct for_child));
	struct for_child* arg3 = (struct for_child*)malloc(sizeof(struct for_child));
	printf("childnum: %d\n", child_num);
	printf("hi!!!!!!!!!!!!!!!!!!1 %s %zu\n", total_half, strlen(total_half));

	//divide the string for childs
	if(child_num == 1){
		printf("1 child\n");
		memset(child_part1, 0, strlen(total_half));
		strcpy(child_part1, total_half);	
		printf("child_part1: %s\n", child_part1);
		// divide parts to child	
		arg1->my_id = 0;
		/*-------------------------------------*/
		std::string child_part1_str1(child_part1);
		std::string child_pair1_str1(child_pair[0]);
		SuperChildClient super_child1(grpc::CreateChannel(child_pair1_str1, grpc::InsecureChannelCredentials()));
		//std::cout << "child_part1_str: " << child_part1_str << std::endl;
		super_child1.SC(child_part1_str1, "3", "0");
		printf("here 5\n");
		/*------------------------------------*/
		////pthread_create(&th_for_child[0], NULL, work_with_child, (void*)arg1);
		// wait for thread to end
		//pthread_join(th_for_child[0], NULL);
	}
	else if(child_num == 2){
		memset(child_part1, 0, strlen(total_half)/2);
		strncpy(child_part1, total_half, strlen(total_half)/2);
		helper = child_part1 + strlen(child_part1) - 1;
		while(1){
			if(!(isalpha(*helper) || isdigit(*helper))){
				(*helper) = '\0';
				break;
			}
			helper--;
		}
		printf("I divided my part to child 1: %s %zu\n", child_part1, strlen(child_part1));
		memset(child_part2, 0, strlen(total_half)-strlen(child_part1));
		helper = (char*)total_half + strlen(child_part1);
		strcpy(child_part2, helper);
		
		/*-------------------------------------*/
		std::string child_part1_str2(child_part1);
		std::string child_pair1_str2(child_pair[0]);
		SuperChildClient super_child2(grpc::CreateChannel(child_pair1_str2, grpc::InsecureChannelCredentials()));
		//std::cout << "child_part1_str: " << child_part1_str << std::endl;
		super_child2.SC(child_part1_str2, "3", "0");
		printf("here 5\n");
		/*------------------------------------*/
		std::string child_part2_str2(child_part2);
		std::string child_pair2_str2(child_pair[1]);
		SuperChildClient super_child3(grpc::CreateChannel(child_pair2_str2, grpc::InsecureChannelCredentials()));
		super_child3.SC(child_part2_str2, "3", "1");
		/*------------------------------------*/
		// divide parts to childs	
		arg1->my_id = 0;
		printf("child_part1 : %s\n", child_part1);
		//pthread_create(&th_for_child[0], NULL, work_with_child, (void*)arg1);
		arg2->my_id = 1;
		printf("child_part2 : %s\n", child_part2);
		//pthread_create(&th_for_child[1], NULL, work_with_child, (void*)arg2);
		// wait for threads to end
		//pthread_join(th_for_child[0], NULL);
		//pthread_join(th_for_child[1], NULL);
	}
	else if(child_num == 3){
		memset(child_part1, 0, strlen(total_half)/3);
		strncpy(child_part1, total_half, strlen(total_half)/3);
		helper = child_part1 + strlen(child_part1) - 1;
		while(1){
			if(!(isalpha(*helper) || isdigit(*helper))){
				(*helper) = '\0';
				break;
			}
			helper--;
		}
		memset(child_part2, 0, strlen(total_half)/3);
		helper = (char*)total_half + strlen(child_part1);
		printf("helper: %s\n", helper);
		strncpy(child_part2, helper, strlen(total_half)/3);
		helper = child_part2 + strlen(child_part2) - 1;
		while(1){
			if(!(isalpha(*helper) || isdigit(*helper))){
				//(*helper) = '\0';
				break;
			}
			helper--;
		}
		//child_part[2] = (char*)malloc(strlen(total_half)-strlen(child_part[0])-strlen(child_part[1]));
		memset(child_part3, 0, strlen(total_half)-strlen(child_part1)-strlen(child_part2));
		helper = (char*)total_half + strlen(child_part1) + strlen(child_part2);
		strcpy(child_part3, helper);
	
		printf("child part 1: %s\n", child_part1);	
		printf("child part 2: %s\n", child_part2);	
		printf("child part 3: %s\n", child_part3);	
		// divide parts to childs	
		/*-------------------------------------*/
		std::string child_part1_str3(child_part1);
		std::string child_pair1_str3(child_pair[0]);
		SuperChildClient super_child4(grpc::CreateChannel(child_pair1_str3, grpc::InsecureChannelCredentials()));
		//std::cout << "child_part1_str: " << child_part1_str << std::endl;
		super_child4.SC(child_part1_str3, "3", "0");
		printf("here 5\n");
		/*------------------------------------*/
		std::string child_part2_str3(child_part2);
		std::string child_pair2_str3(child_pair[1]);
		SuperChildClient super_child5(grpc::CreateChannel(child_pair2_str3, grpc::InsecureChannelCredentials()));
		super_child5.SC(child_part2_str3, "3", "1");
		/*------------------------------------*/
		std::string child_part3_str3(child_part3);
		std::string child_pair3_str3(child_pair[2]);
		SuperChildClient super_child6(grpc::CreateChannel(child_pair3_str3, grpc::InsecureChannelCredentials()));
		super_child6.SC(child_part3_str3, "3", "1");
		/*------------------------------------*/
		printf("created thread 1\n");
		arg1->my_id = 0;
		//pthread_create(&th_for_child[0], NULL, work_with_child, (void*)arg1);
		printf("created thread 2\n");
		arg2->my_id = 1;
		//pthread_create(&th_for_child[1], NULL, work_with_child, (void*)arg2);
		printf("created thread 3\n");
		arg3->my_id = 2;
		//pthread_create(&th_for_child[2], NULL, work_with_child, (void*)arg3);
		// wait for threads to end
		//pthread_join(th_for_child[0], NULL);
		//pthread_join(th_for_child[1], NULL);
		//pthread_join(th_for_child[2], NULL);
		
	}
	else if(child_num == 4){
		;
	}
	else if(child_num == 5){
		;
	}
	return 0;
}
/*-------------------------------------------------------------------*/
void* sc_server_th_routine(void* arg)
{
	RunSCServer();
	return 0;
}

void* ss_server1_th_routine(void* arg)
{
	RunSSServer1();
	return 0;
}

void* ss_server2_th_routine(void* arg)
{
	RunSSServer2();
	return 0;
}

////////// Super node 1 //////////
void is_super_1(int argc, char* argv[])
{
	printf("is_super_1 function\n");
	pthread_t ss_server1_th;

	pthread_create(&ss_server1_th, NULL, ss_server1_th_routine, NULL);
	pthread_join(ss_server1_th, NULL);
	printf("super 1 job done\n");	
	//std::cout << "super 2 grpc addr is" << super2_pair << std::endl;
	//divide part1 to childs by gRPC
	/*std::string child_pair1_str(child_pair1);
	SuperChildClient super_child(grpc::CreateChannel(child_pair1_str, grpc::InsecureChannelCredentials()));
	std::string part1_str(part1);
	std::string response = super_child.SC(part1_str);
	std::cout << "Super receiver: " << response << std::endl;*/

}

////////// Super node 2 //////////
void is_super_2(int argc, char* argv[])
{
	printf("is_super_2 function\n");
	pthread_t ss_server2_th;
	struct ifreq ifr;
	char ipstr[40];
	int my_socket;

	pthread_create(&ss_server2_th, NULL, ss_server2_th_routine, NULL);
	/*my_socket = socket(AF_INET, SOCK_DGRAM, 0);
	strncpy(ifr.ifr_name, "enp0s3" , IFNAMSIZ);

	if(ioctl(my_socket, SIOCGIFADDR, &ifr) < 0){
		printf("Error");
	}
	else{
		inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2, ipstr, sizeof(struct sockaddr));
		printf("myownipaddr is :%s\n",ipstr);
	}*/
		
	//send my grpc pair to supernode1
	std::string my_grpc_addr = "localhost:";
	std::string super1_pair_str(super1_pair);
	std::string grpc_port_str(grpc_port);
	my_grpc_addr.append(grpc_port_str);
	SuperSuperClient inform_mine(grpc::CreateChannel(super1_pair_str, grpc::InsecureChannelCredentials()));
	inform_mine.SS(my_grpc_addr,"1");	
	
	//run thread to accept, split, and send file	
	pthread_create(&th_for_cli, NULL, get_file_and_split, NULL); 
	
	pthread_join(ss_server2_th, NULL);

	printf("super 2 job done\n");	
}

int main(int argc, char* argv[]) {
    /* Change as you wish */
	
	int i = 0;
	printf("argc: %d\n", argc);

	pthread_t th_server_for_child;

	if(strcmp(argv[3], "-s")){
		printf("set is_super to 1\n");
		is_super = 1;
		// set basics for supernode1
		child_num = argc - 3;
		myport = argv[1];
		grpc_port = argv[2];
		printf("child num: %d!\n",child_num);
		for(i=0; i<child_num; i++){
			printf("here!\n");
			child_pair[i] = argv[3+i];
			printf("here!\n");
		}
	}
	else{
		printf("set is_super to 2\n");
		is_super = 2;
		//set basics for supernode2
		child_num = argc - 5;
		myport = argv[1];
		grpc_port = argv[2];
		super1_pair = argv[4];
		printf("child num: %d!\n",child_num);
		for(i=0; i<child_num; i++){
			child_pair[i] = argv[5+i];
		}
	}
	
	//pthread_create(&th_server_for_child, NULL, sc_server_th_routine, NULL); 
	
	if(is_super == 1) is_super_1(argc, argv);
       	else is_super_2(argc, argv);	
	//pthread_join(th_server_for_child, 0);

	return 0;
}
