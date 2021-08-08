#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <ctime>
#include <thread>
#include <future>

/* Implementation of class "MessageQueue" */
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_messages.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_messages.back());
    _messages.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
    _messages.push_back(std::move(msg));
    _cond.notify_one(); // notify client after pushing new Vehicle into vector
}

/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    TrafficLightPhase_MsgQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}


void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (TrafficLightPhase_MsgQueue->receive() == TrafficLightPhase::green) 
        { 
            return; 
        }
    }
}


TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}


// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
    
    /* Init our random generation between 4 and 6 seconds */
	auto cycleDuration = rand()%3 + 4;

    auto start = std::chrono::high_resolution_clock::now();
  
    while(1)
    {
        // Sleep to avoid high CPU load
    	std::this_thread::sleep_for(std::chrono::milliseconds(1));


        auto finish = std::chrono::high_resolution_clock::now();
        double difference = std::chrono::duration<double, std::milli>(finish-start).count();

        //std::cout<<"difference: " <<difference<<"\n";

        if (difference >= cycleDuration) {

        	if (_currentPhase == red)
			{
				_currentPhase = green;
			}
			else
			{
				_currentPhase = red;
			} 
          
            /* Send an update to the message queue and wait for it to be sent */
			auto msg = _currentPhase;
			auto t = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, TrafficLightPhase_MsgQueue, std::move(msg));
			t.wait();

			/* Reset stop watch for next cycle */
			start = std::chrono::high_resolution_clock::now();

			/* Randomly choose the cycle duration for the next cycle */
            cycleDuration = rand()%3 + 4;
        }
    }

}

