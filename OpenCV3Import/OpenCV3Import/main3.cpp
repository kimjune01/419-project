#include <iostream>
#include <thread>

//This function will be called from a thread

void call_from_thread(int &test) {
     std::cout << "Hello, World " << test << std::endl;
 }


int main() {
    
     int myint = 1;
    
     //Launch a thread
     std::thread t1(call_from_thread, std::ref(myint));

     //Join the thread with the main thread
     t1.join();

     return 0;
 }
