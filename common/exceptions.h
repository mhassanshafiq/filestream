#pragma once
#include <exception>
#include <string>
#include<iostream>

#define TRY try
#define CATCH_STD catch(const std::exception& e) 

#define PRINT_STD std::cout<< e.what()<<std::endl; 

#define CATCH catch(...) 
#define PRINT_EX std::cout << "some unhandled exception at '"<<__LINE__<< "', in '"<<__FILE__<<"'"<<std::endl ; 

#define THROW throw(e);
#define EXIT exit(0);