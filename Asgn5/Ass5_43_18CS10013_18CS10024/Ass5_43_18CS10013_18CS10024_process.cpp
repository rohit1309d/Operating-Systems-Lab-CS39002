#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <bits/stdc++.h>
#include <chrono> 
#include <signal.h>
#include <pthread.h>
using namespace std;
using namespace std::chrono; 
#define max_size 8

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

struct share_item{
	int tot_jobs;
	int job_created = 0;
	int job_completed = 0;
	pthread_mutex_t m;
	job buffer[max_size+10];
	int count = 0;
};


void producer(int i, share_item* share1){
	while(true) {
			
		int ran = rand()%4;
		sleep(ran);

		pthread_mutex_lock(&(share1->m));

		if (share1->count < max_size)
		{			
			pid_t id = getpid();
			
			if(share1->tot_jobs > share1->job_created){


				job p_job = create_job(i+1,id);	

				int i = 0;
				
				if(share1->count != max_size){
			      
			      if(share1->count == 0){
			         share1->buffer[share1->count++] = p_job;        
			      }else{
			         
			         for(i = share1->count - 1; i >= 0; i-- ){
			            if(p_job.priority < share1->buffer[i].priority){
			               share1->buffer[i+1] = share1->buffer[i];
			            }else{
			               break;
			            }            
			         }  
						
			         share1->buffer[i+1] = p_job;
			         share1->count++;
			      }
			   }

			    cout << "producer : " << p_job.producer_numb << ", producer pid : " << p_job.process_id << ", priority : " << p_job.priority << ", compute time : " << p_job.comp_time << endl;
		        
		        share1->job_created++;

			}
		}

	    pthread_mutex_unlock(&(share1->m));
	}	
}

void consumer(int i, share_item* share1){
	while(true) {

		int ran = rand() % 4;
		sleep(ran);

		pthread_mutex_lock(&(share1->m));

		if ((share1->count) != 0)
		{			
	        pid_t id = getpid();

			if(share1->tot_jobs > share1->job_completed){

				job p_job = share1->buffer[--(share1->count)];
				
				cout << "consumer : " << i+1 << " consumer pid : " << id << ", producer : " << p_job.producer_numb << ", producer pid : " << p_job.process_id << ", priority : " << p_job.priority << ", compute time : " << p_job.comp_time << endl;
				share1->job_completed++;
		        sleep(p_job.comp_time);			
		    }
		    else 
			{
				pthread_mutex_unlock(&(share1->m));
				break;
			}		
		}

		pthread_mutex_unlock(&(share1->m));
	}
}

int main()
{
	share_item *share;
	int shmid;
	key_t key = 341;
	shmid = shmget(IPC_PRIVATE, sizeof(share_item), 0666|IPC_CREAT);
	share = (share_item *) shmat(shmid, 0, 0);
	pthread_mutexattr_t atr;
	pthread_mutexattr_init(&atr);
	pthread_mutexattr_setpshared(&atr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&(share->m), &atr);

	int np,nc;
	cout << "Enter the number of producers : ";
	cin >> np;
	cout << "Enter the number of consumers : ";
	cin >> nc;
	cout << "Enter the total number of jobs : ";
	cin >> share->tot_jobs;

	pid_t producers[np] = {0};
	pid_t consumers[nc] = {0};

	int i;

	auto start = high_resolution_clock::now();

	for(i = 0; i < np; i++){
		if ((producers[i] = fork()) == 0)
		{
			producer(i, share);
		}
	}

	for(i = 0; i < np; i++){
		if ((consumers[i] = fork()) == 0)
		{
			consumer(i, share);
		}
	}

	while(share->job_created != share->job_completed || share->job_completed != share->tot_jobs);

	shmdt(share);
	shmctl(shmid, IPC_RMID, NULL);

	auto stop = high_resolution_clock::now(); 
	auto duration = duration_cast<microseconds>(stop - start); 
    cout << "Time taken : " << duration.count() << " microseconds" << endl; 
  

	for (int i = 0; i < np; ++i)
	{
		kill(producers[i], SIGTERM);
	}

	for (int i = 0; i < nc; ++i)
	{
		kill(consumers[i], SIGTERM);
	}

	return 0;

}



