#include <iostream>
#include <random>
#include "TrafficLight.h"
#include "TrafficObject.h"
#include <chrono>
#include <thread>

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> ulck(_mutex);
    _condition.wait(ulck, [this]{return !_queue.empty();});
    T newMessage = std::move(_queue.back());

    // Consider using _queue.clear() as it will make the performance better. NOT using _queue.pop_back()
    // message queue has to be flushed out during every call using the clear() function.
    // At peripheral intersections Traffic is less. As number of changes in traffic light at these intersections is more compared to number of vehicles approaching, there will be accumulation of Traffic light messages in _queue.
    // By the time new vehicle approaches at any of these peripheral intersections it would be receiving out some older traffic light msg. That's the reason it seems vehicles are crossing red at these intersection, in fact they are crossing based on some previous green signal in that _queue.
    // But once new traffic light msg arrives, all the older msgs are redundant. So we have to clear _queue at send (as soon as new msg arrives).
    // For central intersection, _queue clear/ no clear won't be a problem, as traffic is huge there it will have enough receives to keep it empty for new msg.
    _queue.clear();

    return newMessage;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lck(_mutex);
    _queue.emplace_back(std::move(msg));
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    while (true)
    { 
      auto curr_phase = _queue.receive();
      if (curr_phase == TrafficLightPhase::green) 
        return; 
     }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    // https://knowledge.udacity.com/questions/395593
      threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases,this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // We would recommend referring to the implementation of sleep_for() used in the while loop with a 1 ms condition for each cycle within the drive() function in the vehicle.cpp file.
    // Don't forget to implement both parts of FP.2

    // set-up random number generator
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> cycleDurationRange(4000, 6000);

    // set cycle duration
    double cycleDuration = cycleDurationRange(mt);
  
    // initialize cycle start and end time
    std::chrono::time_point<std::chrono::system_clock> cycleTimeStart, cycleTimeEnd;
    // get cycle start time
    cycleTimeStart = std::chrono::high_resolution_clock::now();

    // infinite loop
    // Implemented infinite loop to that measures time between two cycles and togles the traffic light phases
    while (true) 
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // get cycel end time
        cycleTimeEnd = std::chrono::high_resolution_clock::now();

        // determine time between two loop cycles
        double cycleTime = std::chrono::duration_cast<std::chrono::milliseconds>(cycleTimeEnd - cycleTimeStart).count();

        if (cycleTime >= cycleDuration) {
            // toggle current traffic light phase
            (_currentPhase == TrafficLightPhase::green) ? _currentPhase = TrafficLightPhase::red : _currentPhase = TrafficLightPhase::green;
            
            // send upate to the message _gueue
            TrafficLightPhase update = TrafficLight::getCurrentPhase();
            _queue.send(std::move(update));

            // set cycle duration
            cycleDuration = cycleDurationRange(mt);
            // update cycle start time
            cycleTimeStart = std::chrono::high_resolution_clock::now();
        }
    }
}

