/*
    File: client_MP6.cpp

    Author: J. Higginbotham
    Department of Computer Science
    Texas A&M University
    Date  : 2016/05/21

    Based on original code by: Dr. R. Bettati, PhD
    Department of Computer Science
    Texas A&M University
    Date  : 2013/01/31

    MP6 for Dr. //Tyagi's
    Ahmed's sections of CSCE 313.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define NUM_REQUEST_THREADS 3
#define NUM_STAT_THREADS 3

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*
    As in MP7 no additional includes are required
    to complete the assignment, but you're welcome to use
    any that you think would help.
*/
/*--------------------------------------------------------------------------*/

#include <cassert>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <assert.h>
#include <fstream>
#include <numeric>
#include <vector>
#include "reqchannel.h"
#include "bounded_buffer.h"

/*
    This next file will need to be written from scratch, along with
    semaphore.h and (if you choose) their corresponding .cpp files.
*/

//#include "bounded_buffer.h"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -----Information Flow-----
   Request Threads(3) -> Request Buffer(1) -> Worker Threads(n) ->
   Response Buffers(3) -> Stat Threads(3) -> Histograms(3)
*/

struct Request {
	std::string name;                    // who the request is for
	BoundedBuffer<std::string> *response_buffer; // where the request goes
};

struct RT_PARAMS {
	BoundedBuffer<Request> *request_buffer; // where to put the requests
	int num_requests;                       // how many of them to add
	Request r;                              // request to add
};

struct WT_PARAMS {
	BoundedBuffer<Request> *request_buffer; // where to get the requests
	RequestChannel *worker_channel;          // channel to make requests over
};

struct ST_PARAMS {
	BoundedBuffer<std::string> *response_buffer; // where to get responses
	std::vector<int> *histogram;                 // where to put them
};

/*
    This class can be used to write to standard output
    in a multithreaded environment. It's primary purpose
    is printing debug messages while multiple threads
    are in execution.
*/
class atomic_standard_output {
    pthread_mutex_t console_lock;
public:
    atomic_standard_output() { pthread_mutex_init(&console_lock, NULL); }
    ~atomic_standard_output() { pthread_mutex_destroy(&console_lock); }
    void print(std::string s){
        pthread_mutex_lock(&console_lock);
        std::cout << s << std::endl;
        pthread_mutex_unlock(&console_lock);
    }
};

atomic_standard_output aso;

/* CONSTANTS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* HELPER FUNCTIONS */
/*--------------------------------------------------------------------------*/

std::string make_histogram(std::string name, std::vector<int> *data) {
    std::string results = "Frequency count for " + name + ":\n";
    for(int i = 0; i < data->size(); ++i) {
        results += std::to_string(i * 10) + "-" + std::to_string((i * 10) + 9) + ": " + std::to_string(data->at(i)) + "\n";
    }
    return results;
}

/*--------------------------------------------------------------------------*/
/* Thread Funcions                                                          */
/*--------------------------------------------------------------------------*/

void* rt_func(void* arg) {
	aso.print("entered request thread");
	
	// handle parameters
	RT_PARAMS p = *(RT_PARAMS *)arg;
	BoundedBuffer<Request> *request_buffer = p.request_buffer;
	int num_requests                       = p.num_requests;
	Request r                              = p.r;

	// push num_request Requests to the buffer
	for(int i=0; i<num_requests; i++) {
		request_buffer->push(r);
		aso.print("pushing " + r.name);
	}
   
   	aso.print("request thread ended");
	return NULL;
}

void* wt_func(void* arg) {
	aso.print("entered worker");
	
	// handle parameters
	WT_PARAMS p = *(WT_PARAMS *)arg;
	BoundedBuffer<Request> *request_buffer = p.request_buffer;
	RequestChannel *worker_channel         = p.worker_channel;

	while(true) {
		// take an item off the buffer
		Request r = request_buffer->pop();

		aso.print("processing \"" + r.name + "\"");

		// quit when we find and item with name "quit"
		if(r.name == "quit") {
			worker_channel->send_request("quit");
			delete worker_channel;
			aso.print("worker thread ended");
			break;
		}

		// ask server for response
		std::string msg = "data " + r.name;
		std::string response = worker_channel->send_request(msg);

		// push request to response buffer
		r.response_buffer->push(response);
	}

	return NULL;
}

void* st_func(void* arg) {
/*
	aso.print("entered stat thread");

	// handle parameters
	ST_PARAMS p = *(ST_PARAMS *)arg;
	BoundedBuffer<std::string> *response_buffer = p.response_buffer;
	std::vector<int> *histogram                 = p.histogram;


	std::string response;

   	while(true) {
		response = response_buffer->pop();
		aso.print(response + " ");

		// quit when we find a quit message
		if(response.compare("quit ") == 0) 
			break;
		
		// update the histogram with the value
		//histogram->at(stoi(response) / 10) += 1;
	}

	aso.print("stat thread ended");
*/

	return NULL;
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
    int n = 10; //default number of requests per "patient"
    int b = 50; //default size of request_buffer
    int w = 10; //default number of worker threads
    bool USE_ALTERNATE_FILE_OUTPUT = false;
    int opt = 0;
    while ((opt = getopt(argc, argv, "n:b:w:m:h")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'm':
                if(atoi(optarg) == 2) USE_ALTERNATE_FILE_OUTPUT = true;
                break;
            case 'h':
            default:
                std::cout << "This program can be invoked with the following flags:" << std::endl;
                std::cout << "-n [int]: number of requests per patient" << std::endl;
                std::cout << "-b [int]: size of request buffer" << std::endl;
                std::cout << "-w [int]: number of worker threads" << std::endl;
                std::cout << "-m 2: use output2.txt instead of output.txt for all file output" << std::endl;
                std::cout << "-h: print this message and quit" << std::endl;
                std::cout << "Example: ./client_solution -n 10000 -b 50 -w 120 -m 2" << std::endl;
                std::cout << "If a given flag is not used, a default value will be given" << std::endl;
                std::cout << "to its corresponding variable. If an illegal option is detected," << std::endl;
                std::cout << "behavior is the same as using the -h flag." << std::endl;
                exit(0);
        }
    }
    
    int pid = fork();
    if(pid == 0) {
        struct timeval start_time;
        struct timeval finish_time;
        int64_t start_usecs;
        int64_t finish_usecs;
        ofstream ofs;
        if(USE_ALTERNATE_FILE_OUTPUT) ofs.open("output2.txt", ios::out | ios::app);
        else ofs.open("output.txt", ios::out | ios::app);
        
        std::cout << "n == " << n << std::endl;
        std::cout << "b == " << b << std::endl;
        std::cout << "w == " << w << std::endl;
        
	//aso.print("client started");
        std::cout << "CLIENT STARTED:" << std::endl;
        std::cout << "Establishing control channel... " << std::flush;
        RequestChannel *chan = new RequestChannel("control", RequestChannel::CLIENT_SIDE);
        std::cout << "done." << std::endl;
        
        /*
            This time you're up a creek.
            What goes in this section of the code?
            Hint: it looks a bit like what went here 
            in MP7, but only a *little* bit.
        */

	BoundedBuffer<Request> request_buffer(b);

	BoundedBuffer<std::string> response_buffer_john(b);
	BoundedBuffer<std::string> response_buffer_jane(b);
	BoundedBuffer<std::string> response_buffer_joe(b);
	
	std::vector<int> histogram_john(10, 0);
	std::vector<int> histogram_jane(10, 0);
	std::vector<int> histogram_joe(10, 0);
	
	/*-------------------------------------------------------------------*/
	/* Request Threads                                                   */
	/*-------------------------------------------------------------------*/

	// request thread ids
	pthread_t rt_ids[3];

	// requests to be pushed to the buffer
	Request reqs[] = {{"John Smith", &response_buffer_john},
	                  {"Jane Smith", &response_buffer_jane},
	         	  {"Joe Smith",  &response_buffer_joe}};
	
	// parameters for request threads
	RT_PARAMS rt_params[] = {{&request_buffer, n, reqs[0]},
	                         {&request_buffer, n, reqs[1]},
				 {&request_buffer, n, reqs[2]}};
	
	// create request threads
	for(int i=0; i<3; i++)
		pthread_create(&rt_ids[i], 0, &rt_func, (void *)&rt_params[i]);

	/*-------------------------------------------------------------------*/
	/* Worker Threads                                                    */
	/*-------------------------------------------------------------------*/
	
	// worker thread ids
	pthread_t wt_ids[w];

	// parameters for workers
	WT_PARAMS wt_params[w];

	// create worker threads
	for(int i=0; i<w; i++) {
		std::string s = chan->send_request("newthread");
		RequestChannel *worker_channel = new RequestChannel(s, RequestChannel::CLIENT_SIDE);
		wt_params[i] = WT_PARAMS{&request_buffer, worker_channel};
		pthread_create(&wt_ids[i], 0, &wt_func, (void *)&wt_params[i]);
	}
	
	/*-------------------------------------------------------------------*/
	/* Statistics Threads                                                */
	/*-------------------------------------------------------------------*/

	// stat thread ids
	pthread_t st_ids[3];

	// parameters for stat threads
	ST_PARAMS st_params[] = {{&response_buffer_john, &histogram_john},
	                         {&response_buffer_jane, &histogram_jane},
				 {&response_buffer_joe,  &histogram_joe}};
	
	// create stat threads
	for(int i=0; i<3; i++)
		pthread_create(&st_ids[i], 0, &st_func, (void *)&st_params[i]);

	/*-------------------------------------------------------------------*/
	/* Join Threads                                                      */
	/*-------------------------------------------------------------------*/

	// join request threads
	for(int i=0; i<3; i++) {
		pthread_join(rt_ids[i], NULL);
		//aso.print("joinied a request thread");
	}

	// push quit messages to workers
	for(int i=0; i<w; i++) 
		request_buffer.push(Request{"quit", NULL});

	// join worker threads
	for(int i=0; i<w; i++)
		pthread_join(wt_ids[i], NULL);
	
	// push quit messages to stat threads
	response_buffer_john.push("quit");
	response_buffer_jane.push("quit");
	response_buffer_joe.push("quit");

	// join stat threads
	for(int i=0; i<3; i++)
		pthread_join(st_ids[i], NULL);

	std::cout << "size of john response buffer: " << response_buffer_john.size() <<std::endl;
	std::cout << "size of jane response buffer: " << response_buffer_jane.size() <<std::endl;
	std::cout << "size of joe response buffer: " << response_buffer_joe.size() << std::endl;

	aso.print("done?");
	
	/*-------------------------------------------------------------------*/
	/* Print Results                                                     */
	/*-------------------------------------------------------------------*/

        ofs.close();
        std::cout << "Sleeping..." << std::endl;
        usleep(10000);
        std::string finale = chan->send_request("quit");
        std::cout << "Finale: " << finale << std::endl;
    }
    else if(pid != 0) execl("dataserver", NULL);
}
