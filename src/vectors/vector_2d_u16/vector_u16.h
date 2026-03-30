
#ifndef VECTOR_u16_H
#define VECTOR_u16_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t x;
    uint16_t y;
} vector_u16_t;


const vector_u16_t  N_V16 = {0,-1};
const vector_u16_t NE_V16 = {1,-1};
const vector_u16_t  E_V16 = {1,0};
const vector_u16_t SE_V16 = {1,1};
const vector_u16_t  S_V16 = {0,1};
const vector_u16_t SW_V16 = {-1,1};
const vector_u16_t  W_V16 = {-1,0};
const vector_u16_t NW_V16 = {-1,-1};

const vector_u16_t NESW_V16[] = {
    N_V16,
    E_V16,
    S_V16,
    W_V16
}; 

/**
 * \brief print the vector
 * \param vector vector to print
 * \param newline print a new line or not
 */
void v16_print(const vector_u16_t* vector, bool newline);

/**
 * \brief add the second vector to the first
 * \param one first vector
 * \param two second vector
 */
void v16_add(vector_u16_t* one, vector_u16_t* two);

/**
 * \brief subtract the second vector from the first
 * \param one first vector
 * \param two second vector
 */
void v16_subtract(vector_u16_t* one, vector_u16_t* two);

/**
 * \brief multiply the vector by a number
 * \param vector vector to multiply
 * \param multiplier number to multiply by
 */
void v16_multiply(vector_u16_t* vector, uint16_t multiplier);

/**
 * \brief divide the vector by a number
 * \param vector vector to divide
 * \param divisor number to divide by
 */
void v16_divide(vector_u16_t* vector, uint16_t divisor);

/**
 * \brief get the higher of the two numbers in the vector
 * \param vector vector to get number from
 * \return the higher of the two numbers in the vector
 */
uint16_t v16_get_higher(vector_u16_t* vector);

/**
 * \brief get the lower of the two numbers in the vector
 * \param vector vector to get number from
 * \return the lower of the two numbers in the vector
 */
uint16_t v16_get_lower(vector_u16_t* vector);

/**
 * \brief check if the vector is within some bounds
 * \param vector vector to check
 * \param bounds the bounds to check if the vector is in
 * \return 1 if vector is in bounds, else 0
 */
bool v16_in_bound(vector_u16_t* vector, vector_u16_t* bounds);

/**
 * \brief flip the vector
 * \param vector vector to flip
 */
void v16_flip(vector_u16_t* vector);

/**
 * \brief check if two vectors are the same
 * \param one first vector
 * \param two second vector
 * \return 1 if both vectors are the same, else 0
 */
bool v16_equal(vector_u16_t* one, vector_u16_t* two);

/**
 * \brief get the magnitude of a vector
 * \param vector vector to get magnitude of
 * \return magnitude of vector
 */
double v16_get_magnitude(vector_u16_t* vector);

/**
 * \brief get the unit vector of a vector
 * \param vector vector to get unit vector from
 * \return unit vector
 */
vector_u16_t v16_get_unit_vector(vector_u16_t* vector);

/**
 * \brief get the direction of the vector in a grid system
 * \param vector vector to get the direction from
 * \return vector of the direction
 */
vector_u16_t v16_get_direction(vector_u16_t* vector);


#endif 
