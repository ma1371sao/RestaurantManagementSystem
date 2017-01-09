/*
Shijie Ma
11/28/2016
*/
#include<pthread.h>
#include<time.h>
#include<semaphore.h>
#include<unistd.h>
#include<condition_variable>
#include<vector>
#include<chrono>
#include<queue>
#include<mutex>
#include<fstream>
#include<unordered_map>
#include<iostream>
using namespace std;
ifstream in("input.txt");
sem_t semaphore_machine, semaphore_dinningtable, semaphore_Cook_behavior;
int beginning_time, numberOfdiner, numberOftable, numberOfCook_behavior;
mutex mutexMachine;
mutex mutexTable;
mutex mutexCook_behavior;
queue<int> Cook_behavior_queue;
queue<int> table_queue;
struct Diner {
	int arrive_time;
	int num_burger;
	int num_fries;
	int is_coke;
};
struct diner_parameter {
	Diner temp;
	int diner_idx;
};
struct cook_parameter {
	mutex &synchronize_mutex;
	int Cook_behavior_idx;
	int diner_index;
	int num_burger;
	int num_fries;
	int is_coke;
};
unordered_map<string, int> resource;
int fh;
int fmin;

void* Cook_behavior(void* thread_arg) {
	cook_parameter* param = (cook_parameter*)thread_arg;
	int Cook_behavior_idx = param->Cook_behavior_idx, diner_index = param->diner_index, num_burger = param->num_burger, num_fries = param->num_fries, is_coke = param->is_coke;
	auto current_time = time((time_t*)NULL) - beginning_time;
	int h = current_time / 60;
	int min = current_time % 60;
	if (h < 10)	cout << "0";
	cout << h << ":";
	if (min < 10)	cout << "0";
	cout << min;
	cout << " - Cook " << Cook_behavior_idx << " processes Diner " << diner_index + 1 << "'s order." << endl;
	int count = 0;
	if (num_burger > 0) count += 1;
	if (num_fries > 0) count += 1;
	if (is_coke > 0) count += 1;
	while (count > 0) {
		sem_wait(&semaphore_machine);
		mutexMachine.lock();
		if (resource["burger"]) {
			if (num_burger > 0) {
				resource["burger"] = 0;
				mutexMachine.unlock();
				while (num_burger > 0) {
					auto current_time = time((time_t*)NULL) - beginning_time;
					int h = current_time / 60;
					int min = current_time % 60;
					if (h < 10)	cout << "0";
					cout << h << ":";
					if (min < 10)	cout << "0";
					cout << min;
					cout << " - Cook " << Cook_behavior_idx << " uses the burger machine." << endl;
					sleep(5);
					num_burger--;
				}
				mutexMachine.lock();
				resource["burger"] = 1;
				mutexMachine.unlock();
				count -= 1;
				num_burger = 0;
			}
			else
				mutexMachine.unlock();
		}
		if (resource["fries"] && num_fries > 0) {
			if (num_fries > 0) {
				resource["fries"] = 0;
				mutexMachine.unlock();
				while (num_fries) {
					auto current_time = time((time_t*)NULL) - beginning_time;
					int h = current_time / 60;
					int min = current_time % 60;
					if (h < 10)	cout << "0";
					cout << h << ":";
					if (min < 10)	cout << "0";
					cout << min;
					cout << " - Cook " << Cook_behavior_idx << " uses the fries machine." << endl;
					sleep(3);
					num_fries--;
				}
				mutexMachine.lock();
				resource["fries"] = 1;
				mutexMachine.unlock();
				count -= 1;
				num_fries = 0;
			}
			else
				mutexMachine.unlock();
		}
		if (resource["coke"]) {
			if (is_coke) {
				resource["coke"] = 0;
				mutexMachine.unlock();
				auto current_time = time((time_t*)NULL) - beginning_time;
				int h = current_time / 60;
				int min = current_time % 60;
				if (h < 10)	cout << "0";
				cout << h << ":";
				if (min < 10)	cout << "0";
				cout << min;
				cout << " - Cook " << Cook_behavior_idx << " uses the coke machine." << endl;
				sleep(is_coke * 1);
				mutexMachine.lock();
				resource["coke"] = 1;
				mutexMachine.unlock();
				count -= 1;
				is_coke = 0;
			}
			else
				mutexMachine.unlock();
		}
		sem_post(&semaphore_machine);
	}
	pthread_exit(NULL);
}

void* Diner_behavior(void* pthread_arg) {
	diner_parameter* par = (diner_parameter*)pthread_arg;
	int diner_idx = par->diner_idx;
	Diner temp = par->temp;
	vector<bool> order = vector<bool>(3, true);
	sleep(temp.arrive_time);
	auto current_time = time((time_t*)NULL) - beginning_time;
	int h = current_time / 60;
	int min = current_time % 60;
	if (h < 10)	cout << "0";
	cout << h << ":";
	if (min < 10)	cout << "0";
	cout << min;
	cout << " - Diner " << diner_idx + 1 << " arrives." << endl;
	sem_wait(&semaphore_dinningtable);
	mutexTable.lock();
	auto table = table_queue.front();
	table_queue.pop();
	mutexTable.unlock();
	current_time = time((time_t*)NULL) - beginning_time;
	h = current_time / 60;
	min = current_time % 60;
	if (h < 10)	cout << "0";
	cout << h << ":";
	if (min < 10)	cout << "0";
	cout << min;
	cout << " - Diner " << diner_idx + 1 << " is seated on table " << table << "." << endl;
	mutex synchronize_mutex;
	sem_wait(&semaphore_Cook_behavior);
	mutexCook_behavior.lock();
	int Cook_behavior_idx = Cook_behavior_queue.front();
	Cook_behavior_queue.pop();
	mutexCook_behavior.unlock();
	cook_parameter param_Cook_behavior = { synchronize_mutex, Cook_behavior_idx, diner_idx, temp.num_burger, temp.num_fries, temp.is_coke };
	pthread_t Cook_behavior_thread;
	int rc = pthread_create(&Cook_behavior_thread, NULL, Cook_behavior, (void*)&param_Cook_behavior);
	if (rc) {
		cout << "cannot create the Cook thread" << endl;
		exit(-1);
	}
	pthread_join(Cook_behavior_thread, NULL);
	mutexCook_behavior.lock();
	Cook_behavior_queue.push(Cook_behavior_idx);
	mutexCook_behavior.unlock();
	sem_post(&semaphore_Cook_behavior);
	current_time = time((time_t*)NULL) - beginning_time;
	h = current_time / 60;
	min = current_time % 60;
	if (h < 10)	cout << "0";
	cout << h << ":";
	if (min < 10)	cout << "0";
	cout << min;
	cout << " - Diner " << diner_idx + 1 << "'s order is ready. Diner " << diner_idx + 1 << " starts eating." << endl;
	sleep(30);	//eat
	current_time = time((time_t*)NULL) - beginning_time;
	h = current_time / 60;
	min = current_time % 60;
	if (h < 10)	cout << "0";
	cout << h << ":";
	if (min < 10)	cout << "0";
	cout << min;
	cout << " - Diner " << diner_idx + 1 << " finishes. Diner " << diner_idx + 1 << " leaves the restaurant." << endl;
	fh = h;
	fmin = min;
	mutexTable.lock();
	table_queue.push(table);
	mutexTable.unlock();
	sem_post(&semaphore_dinningtable);
	synchronize_mutex.unlock();
	pthread_exit(NULL);
}

int main() {
	sem_init(&semaphore_machine, 0, 3);
	in >> numberOfdiner >> numberOftable >> numberOfCook_behavior;
	resource["burger"] = 1;
	resource["fries"] = 1;
	resource["coke"] = 1;
	vector<Diner> info = vector<Diner>(numberOfdiner);
	vector<pthread_t> thread_pool = vector<pthread_t>(numberOfdiner);
	vector<diner_parameter> thread_data;
	sem_init(&semaphore_dinningtable, 0, numberOftable);
	sem_init(&semaphore_Cook_behavior, 0, numberOfCook_behavior);
	for (int i = 1; i <= numberOfCook_behavior; i++)
		Cook_behavior_queue.push(i);

	for (int i = 1; i <= numberOftable; i++) {
		table_queue.push(i);
	}
	beginning_time = time((time_t*)NULL);
	for (int i = 0; i < numberOfdiner; i++)
	{
		in >> info[i].arrive_time >> info[i].num_burger >> info[i].num_fries\
			>> info[i].is_coke;
		diner_parameter temp = { info[i], i };
		thread_data.push_back(temp);
	}
	for (int i = 0; i < numberOfdiner; i++)
	{
		int rc = pthread_create(&thread_pool[i], NULL, Diner_behavior, (void*)&thread_data[i]);
		if (rc) {
			cout << "cannot create the thread" << endl;
			exit(-1);
		}
	}
	for(int i = 0; i < numberOfdiner; i++)
	{
			pthread_join(thread_pool[i], NULL);
	}
	if (fh < 10)	cout << "0";
	cout << fh << ":";
	if (fmin < 10)	cout << "0";
	cout << fmin;
	cout << " - The last diner leaves the restaurant." << endl;
	return 0;
}
