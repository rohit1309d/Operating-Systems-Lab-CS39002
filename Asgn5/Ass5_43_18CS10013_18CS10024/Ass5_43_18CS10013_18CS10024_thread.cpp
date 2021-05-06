#include <pthread.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <chrono> 
#include <signal.h>
using namespace std;
using namespace std::chrono; 
#define max_size 8

int tot_jobs,job_created,job_completed;

pthread_mutex_t m;

struct job{
	pid_t process_id;
	int producer_numb;
	int priority;
	int comp_time;
	int job_id;
};

job create_job(int p_numb,pid_t p_id)
{
	job new_job;
	new_job.process_id=p_id;
	new_job.producer_numb=p_numb;
	new_job.priority=rand()%10 + 1;
	new_job.comp_time=rand()%4 + 1;
	new_job.job_id = rand()*rand() % 100000;
	return new_job;
}


struct Comparepriority {
    bool operator()(job const& one, job const& two)
    {
     
        return one.priority<two.priority;
    }
};

priority_queue<job,vector<job>,Comparepriority> buffer;

void* producer(void* input) {

	while(true) {
		
		int ran = rand()%4;
		sleep(ran);

		pthread_mutex_lock(&m);
		

		pid_t id = getpid();
		long num = (long)input;

		if (buffer.size() < max_size)
		{
			
			if(tot_jobs > job_created){

				job p_job = create_job(num,id);	
			    buffer.push(p_job);	

			   cout << "producer : " << p_job.producer_numb << ", producer pid : " << p_job.process_id << ", priority : " << p_job.priority << ", compute time : " << p_job.comp_time << endl;
		        
		        job_created++;

			}
			else 
			{
				
				pthread_mutex_unlock(&m);
				break;
			}
		}

        pthread_mutex_unlock(&m);
   }

   return (void*)1;
}

void* consumer(void *input) {

	while(true) {

		int ran = rand() % 4;
		sleep(ran);

		if(tot_jobs <= job_completed)
		{
			break;
		}
		
		pthread_mutex_lock(&m);
         
		if (buffer.size() != 0)
		{
			
	        pid_t id = getpid();
			long num = (long)input;

			if(tot_jobs > job_completed){

				job p_job = buffer.top();
				buffer.pop();
				
				cout << "consumer : " << num << " consumer pid : " << id << ", producer : " << p_job.producer_numb << ", producer pid : " << p_job.process_id << ", priority : " << p_job.priority << ", compute time : " << p_job.comp_time << endl;
				job_completed++;
		        sleep(p_job.comp_time);			
		    }
		    else 
			{
				pthread_mutex_unlock(&m);
				break;
			}		
		}

		pthread_mutex_unlock(&m);
	}

	return (void*)1;
}

int main()
{
	pthread_mutex_init(&m, NULL);

	int np,nc;
	cout << "Enter the number of producers : ";
	cin >> np;
	cout << "Enter the number of consumers : ";
	cin >> nc;
	cout << "Enter the total number of jobs : ";
	cin >> tot_jobs;

	pthread_t producers[np];
	pthread_t consumers[nc];

	job_created = 0;
	job_completed = 0;

	int i;
    auto start = high_resolution_clock::now(); 

	for(i = 0; i < np; i++)
		pthread_create(&producers[i], NULL, producer, (void *)i);

	for(i = 0; i < nc; i++)
		pthread_create(&consumers[i], NULL, consumer, (void *)i);

	for (i = 0; i < np; i++)
		pthread_join(producers[i], NULL);

	for (i = 0; i < nc; i++)
		pthread_join(consumers[i], NULL);


	auto stop = high_resolution_clock::now(); 
    auto duration = duration_cast<microseconds>(stop - start); 
    cout << "Time taken : " << duration.count() << " microseconds" << endl; 
  

	for (i = 0; i < np+nc; i++)
		pthread_kill(pthread_self(), 0);

	return 0;
}



