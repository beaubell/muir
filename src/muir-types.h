#ifndef MUIR_TYPES_H
#define MUIR_TYPES_H

#define NDEBUG 1
#define BOOST_DISABLE_ASSERTS TRUE
#include "boost/multi_array.hpp"

typedef boost::multi_array<unsigned int , 2> Muir2DArrayUI;
typedef boost::multi_array<unsigned int , 3> Muir3DArrayUI;
typedef boost::multi_array<unsigned int , 4> Muir4DArrayUI;

typedef boost::multi_array<float , 2> Muir2DArrayF;
typedef boost::multi_array<float , 3> Muir3DArrayF;
typedef boost::multi_array<float , 4> Muir4DArrayF;

typedef boost::multi_array<double , 2> Muir2DArrayD;
typedef boost::multi_array<double , 3> Muir3DArrayD;
typedef boost::multi_array<double , 4> Muir4DArrayD;

typedef std::vector<int> PhaseCodeT;

#endif // #ifndef MUIR_TYPES_H
