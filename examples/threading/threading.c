#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

	struct thread_data* thread_func_args = (struct thread_data *) thread_param;
	
	int result = 0;
	int second_result = 0;
	thread_func_args->thread_complete_success =false;
	
	usleep((thread_func_args->wait_to_obtain_ms));
	//mutex_lock_result = pthread_mutex_lock(thread_func_args->mutex);
	result = pthread_mutex_lock(thread_func_args->mutex);
	
	if (result == 0)
	{
		usleep((thread_func_args->wait_to_release_ms));
		second_result = pthread_mutex_unlock(thread_func_args->mutex);
		if (second_result == 0)
		{
		  thread_func_args->thread_complete_success =true;
		}
	}
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
	//pthread_t p1;
	//pthead_mutex_t *mutex;
	//int wait_to_obtain_ms;
	//int wait_to_release_ms;
	
	struct thread_data* data = (struct thread_data*)malloc(sizeof(struct thread_data));
	
	data->mutex = mutex;
	data->wait_to_obtain_ms = wait_to_obtain_ms;
	data->wait_to_release_ms = wait_to_release_ms;
	data->p1 = thread;
	
	int success = 0;
	
	success = pthread_create(thread, NULL, threadfunc, data);
	
	if (success != 0)
	{
		//free(data);
		return false;
	}
	else
	{
		return true;
	}

}

