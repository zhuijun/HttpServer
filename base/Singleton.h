#pragma once

template <typename T>
class Singleton
{
public:
	Singleton() {};
	virtual ~Singleton(){};
	
	static T& Instance() 
	{ 
		static T inst; 
		return inst;
	}
};
