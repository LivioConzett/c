

#include <stdio.h>
#include <math.h>

#include "vector_u16.h"

/**
 * \brief get the Greatest Common Denominator of two numbers
 * \param one first number
 * \param two second number
 * \return gcd of the two numbers
 */
uint16_t v_gcd(uint16_t one, uint16_t two){

    uint16_t result;

    if(one < two) result = one;
    else result = two;

    while(result > 0){
        if(one % result == 0 && two % result == 0){
            break;
        }
        result --;
    }
    return result;
}


/**
 * See header
 */
void v_print(const vector_u16_t* vector, bool newline){
    printf("%d : %d", vector->x, vector->y);
    if(newline) printf("\n");
}


/**
 * See header
 */
void v_add(vector_u16_t* one, vector_u16_t* two){
    one->x += two->x;
    one->y += two->y;
}

/**
 * See header
 */
void v_subtract(vector_u16_t* one, vector_u16_t* two){
    one->x -= two->x;
    one->y -= two->y;
}

/**
 * See header
 */
void v_multiply(vector_u16_t* vector, uint16_t multiplier){
    vector->x *= multiplier;
    vector->y *= multiplier;
}

/**
 * See header
 */
void v_divide(vector_u16_t* vector, uint16_t divisor){
    vector->x /= divisor;
    vector->y /= divisor;
}

/**
 * See header
 */
uint16_t v_get_higher(vector_u16_t* vector){

    if(vector->x > vector->y) return vector->x;

    return vector->y;
}

/**
 * See header
 */
uint16_t v_get_lower(vector_u16_t* vector){

    if(vector->x < vector->y) return vector->x;

    return vector->y;
}

/**
 * See header
 */
bool v_in_bound(vector_u16_t* vector, vector_u16_t* bounds){

    if(vector->x < 0 || vector->x >= bounds->x) return 0;
    if(vector->y < 0 || vector->y >= bounds->y) return 0;
    return 1;
}

/**
 * See header
 */
void v_flip(vector_u16_t* vector){
    v_multiply(vector,-1);
}

/**
 * See header
 */
bool v_equal(vector_u16_t* one, vector_u16_t* two){
    return one->x == two->x && one->y == two->y;
}

/**
 * See header
 */
double v_get_magnitude(vector_u16_t* vector){
    return sqrt(pow(vector->x,2) + pow(vector->y, 2));
}

/**
 * See header
 */
vector_u16_t v_get_unit_vector(vector_u16_t* vector){
    vector_u16_t result;

    uint32_t magnitude = v_get_magnitude(vector);

    result.x = vector->x / magnitude;
    result.y = vector->y / magnitude;

    return result;
}

/**
 * See header
 */
vector_u16_t v_get_direction(vector_u16_t* vector){

    int denominator = v_gcd(vector->x, vector->y);

    vector_u16_t result = *vector;

    v_divide(&result, denominator);

    return result;
}

