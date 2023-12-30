#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>


// a state machine is a collection of states
// each state has a list of transitions
class State
{
public:
	State(std::string name) :name(name) {};



	~State();

	std::unordered_map<std::string, State*> transitions;
	std::string name;


};

class StateMachine
{
public:
	StateMachine() {};
	~StateMachine() {};

	void add_state(State* state) {
		states.push_back(state);
	};



	void add_transition(State* from, State* to, std::string name) {
		from->transitions[name] = to;
	};
	void add_transition(std::string from, std::string to, std::string name) {
		for (int i = 0; i < states.size(); i++) {
			if (states[i]->name == from) {
				for (int j = 0; j < states.size(); j++) {
					if (states[j]->name == to) {
						states[i]->transitions[name] = states[j];
					}
				}
			}
		}
	};
	void add_transition(std::string from, State* to, std::string name) {
		for (int i = 0; i < states.size(); i++) {
			if (states[i]->name == from) {
				states[i]->transitions[name] = to;
			}
		}
	};
	void add_transition(State* from, std::string to, std::string name) {
		for (int i = 0; i < states.size(); i++) {
			if (states[i]->name == to) {
				from->transitions[name] = states[i];
			}
		}
	};

	void set_current_state(State* state) {
		currentState = state;
	}
	void set_current_state(std::string name) {
		for (int i = 0; i < states.size(); i++) {
			if (states[i]->name == name) {
				currentState = states[i];
				return;
			}
		}
		std::cout << "state not found" << std::endl;
	}

	std::string input; // the input to the state machine, this is the name of the transition, can only be one transition at a time
	std::vector<State*> states; // all the states in the state machine
	State* currentState; // the current state of the state machine
	void set_input(std::string input) {
		this->input = input;
	}


	State* get_current_state() {
		return currentState;
	}

	void update() {// according to the input, update new state
		if(currentState->transitions.count(input) != 0)
			currentState = currentState->transitions[input];
	}

	void print_state() {
		std::cout<< "current state:" << currentState->name << std::endl;
	}
};


class RTRTStateMachine : public StateMachine
{
public:
	RTRTStateMachine() {
		State* idle = new State("idle");
		State* ray_tracing = new State("ray_tracing");
		State* displaying = new State("displaying");



		add_state(idle);
		add_state(ray_tracing);
		add_state(displaying);
		
		add_transition(idle, ray_tracing, "start_RT");
		add_transition(ray_tracing, displaying, "finish_RT");
		add_transition(displaying, idle, "stop_display");

		set_current_state(idle);
	}
	~RTRTStateMachine() {};

	void input_start_RT() {
		set_input("start_RT");
	}
	void input_finish_RT() {
		set_input("finish_RT");
	}
	void input_stop_display() {
		set_input("stop_display");
	}





};




#endif