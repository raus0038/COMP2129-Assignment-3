#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <inttypes.h>

#include "matrix.h"

static uint32_t g_seed = 0;

static ssize_t g_width = 0;
static ssize_t g_height = 0;
static ssize_t g_elements = 0;

static ssize_t g_nthreads = 1;

////////////////////////////////
///     UTILITY FUNCTIONS    ///
////////////////////////////////

/**
 * Returns pseudorandom number determined by the seed
 */
uint32_t fast_rand(void) {

    g_seed = (214013 * g_seed + 2531011);
    return (g_seed >> 16) & 0x7FFF;
}

/**
 * Sets the seed used when generating pseudorandom numbers
 */
void set_seed(uint32_t seed) {

    g_seed = seed;
}

/**
 * Sets the number of threads available
 */
void set_nthreads(ssize_t count) {

    g_nthreads = count;
}

/**
 * Sets the dimensions of the matrix
 */
void set_dimensions(ssize_t order) {

    g_width = order;
    g_height = order;

    g_elements = g_width * g_height;
}

/**
 * Displays given matrix
 */
void display(const uint32_t* matrix) {

    for (ssize_t y = 0; y < g_height; y++) {
        for (ssize_t x = 0; x < g_width; x++) {
            if (x > 0) printf(" ");
            printf("%" PRIu32, matrix[y * g_width + x]);
        }

        printf("\n");
    }
}

/**
 * Displays given matrix row
 */
void display_row(const uint32_t* matrix, ssize_t row) {

    for (ssize_t x = 0; x < g_width; x++) {
        if (x > 0) printf(" ");
        printf("%" PRIu32, matrix[row * g_width + x]);
    }

    printf("\n");
}

/**
 * Displays given matrix column
 */
void display_column(const uint32_t* matrix, ssize_t column) {

    for (ssize_t y = 0; y < g_height; y++) {
        printf("%" PRIu32 "\n", matrix[y * g_width + column]);
    }
}

/**
 * Displays the value stored at the given element index
 */
void display_element(const uint32_t* matrix, ssize_t row, ssize_t column) {

    printf("%" PRIu32 "\n", matrix[row * g_width + column]);
}

////////////////////////////////
///   MATRIX INITALISATIONS  ///
////////////////////////////////

/**
 * Returns new matrix with all elements set to zero
 */
uint32_t* new_matrix(void) {

    return calloc(g_elements, sizeof(uint32_t));
}

/**
 * Returns new identity matrix
 */
uint32_t* identity_matrix(void) {

    uint32_t* matrix = new_matrix();

    for (ssize_t i = 0; i < g_width; i++) {
        matrix[i * g_width + i] = 1;
    }

    return matrix;
}

/**
 * Returns new matrix with elements generated at random using given seed
 */
uint32_t* random_matrix(uint32_t seed) {

    uint32_t* matrix = new_matrix();

    set_seed(seed);

    for (ssize_t i = 0; i < g_elements; i++) {
        matrix[i] = fast_rand();
    }

    return matrix;
}

/**
 * Returns new matrix with all elements set to given value
 */
uint32_t* uniform_matrix(uint32_t value) {

    uint32_t* matrix = new_matrix();

    for (ssize_t i = 0; i < g_elements; i++) {
        matrix[i] = value;
    }

    return matrix;
}

/**
 * Returns new matrix with elements in sequence from given start and step
 */
uint32_t* sequence_matrix(uint32_t start, uint32_t step) {

    uint32_t* matrix = new_matrix();
    uint32_t current = start;

    for (ssize_t i = 0; i < g_elements; i++) {
        matrix[i] = current;
        current += step;
    }

    return matrix;
}

////////////////////////////////
///     MATRIX OPERATIONS    ///
////////////////////////////////

/**
 * Returns new matrix with elements cloned from given matrix
 */
uint32_t* cloned(const uint32_t* matrix) {

    uint32_t* result = new_matrix();

    for (ssize_t i = 0; i < g_elements; i++) {
        result[i] = matrix[i];
    }

    return result;
}

/**
 * Returns new matrix with elements ordered in reverse
 */
uint32_t* reversed(const uint32_t* matrix) {

    uint32_t* result = new_matrix();

    for (ssize_t i = 0; i < g_elements; i++) {
        result[i] = matrix[g_elements - 1 - i];
    }

    return result;
}

/**
 * Returns new transposed matrix
 */
uint32_t* transposed(const uint32_t* matrix) {

    uint32_t* result = new_matrix();

    for (ssize_t y = 0; y < g_height; y++) {
        for (ssize_t x = 0; x < g_width; x++) {
            result[x * g_width + y] = matrix[y * g_width + x];
        }
    }

    return result;
}

struct matrix_add {
    uint32_t* matrix;
    uint32_t scalar;
    uint32_t tid;
};


static void *scalar_worker(void *arg) {
    struct matrix_add *matrix = (struct matrix_add *) arg;
    
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;

    for(size_t i = start; i < end; i++) {
        matrix->matrix[i] += matrix->scalar;
    }

    return NULL;

}

/**
 * Returns new matrix with scalar added to each element
 */
uint32_t* scalar_add(const uint32_t* matrix, uint32_t scalar) {

    uint32_t* result = new_matrix();

     result = cloned(matrix); 
    
    pthread_t thread_id[g_nthreads];

    struct matrix_add *m_add = malloc(sizeof(struct matrix_add) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return result;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_add) {
            .matrix = result,
            .tid = i,
            .scalar = scalar
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, scalar_worker, m_add + i) != 0) {
            perror("Thread creation failed");
            return result;
        }
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_join(thread_id[i], NULL) != 0) {
            return result;
        }
    }

     
    
    free(m_add);


    return result;


    /*
        to do

        1 0        2 1
        0 1 + 1 => 1 2

        1 2        5 6
        3 4 + 4 => 7 8
    */

   // for (ssize_t i = 0; i < g_elements; i++) {
    //    result[i] = matrix[i] + scalar;
  //  }

    
}

/**
 * Returns new matrix with scalar multiplied to each element
 */
uint32_t* scalar_mul(const uint32_t* matrix, uint32_t scalar) {

    uint32_t* result = new_matrix();

    /*
        to do

        1 0        2 0
        0 1 x 2 => 0 2

        1 2        2 4
        3 4 x 2 => 6 8
    */

        for (ssize_t i = 0; i < g_elements; i++) {
        result[i] = matrix[i] * scalar;
   	 	}

    return result;
}

/**
 * Returns new matrix with elements added at the same index
 */
uint32_t* matrix_add(const uint32_t* matrix_a, const uint32_t* matrix_b) {

    uint32_t* result = new_matrix();

    for (ssize_t i = 0; i < g_elements; i++) {
        result[i] = matrix_a[i] + matrix_b[i];
   	 	}

    /*
        to do

        1 0   0 1    1 1
        0 1 + 1 0 => 1 1

        1 2   4 4    5 6
        3 4 + 4 4 => 7 8
    */

    return result;
}

/**
 * Returns new matrix, multiplying the two matrices together
 */
uint32_t* matrix_mul(const uint32_t* matrix_a, const uint32_t* matrix_b) {
	
    uint32_t* result = new_matrix();


    for(int j = 0; j < g_width; j++) {
        for(int k = 0; k < g_width;k++) {
            int sum = 0;
            for(int l = 0; l < g_width; l++) {
                sum += matrix_a[j * g_width + l] * matrix_b[l * g_width + k];
                result[j * g_width + k] = sum;
            }
        }
    }

    /*
        to do

        1 2   1 0    1 2
        3 4 x 0 1 => 3 4

        1 2   5 6    19 22
        3 4 x 7 8 => 43 50
    */


    return result;
}

/**
 * Returns new matrix, powering the matrix to the exponent
 */
uint32_t* matrix_pow(const uint32_t* matrix, uint32_t exponent) {

    uint32_t* result = new_matrix();

    if(exponent == 0) {
      result = identity_matrix();
      return result;
    }

    if(exponent == 1) {
      result = cloned(matrix);
      return result;
    }

    result = cloned(matrix);
    uint32_t* matrix_clone  =  new_matrix();
    
    
    for(int i = 1; i < exponent; i++) {
       matrix_clone = cloned(result);
        for(int j = 0; j < g_width; j++) {
            for(int k = 0; k < g_width;k++) {
                int sum = 0;
                for(int l = 0; l < g_width; l++) {
                    sum += matrix[j * g_width + l] * matrix_clone[l * g_width + k];
                    result[j * g_width + k] = sum;
                }
            }
        }
    }



    /*
        to do

        1 2        1 0
        3 4 ^ 0 => 0 1

        1 2        1 2
        3 4 ^ 1 => 3 4

        1 2        199 290
        3 4 ^ 4 => 435 634
    */

    return result;
}

////////////////////////////////
///       COMPUTATIONS       ///
////////////////////////////////

/**
 * Returns the sum of all elements
 */
uint32_t get_sum(const uint32_t* matrix) {

	int sum = 0;

	for (ssize_t i = 0; i < g_elements; i++) {
        sum += matrix[i];
   	 }

   	 return sum;

    /*
        to do

        1 2
        2 1 => 6

        1 1
        1 1 => 4
    */
}

/**
 * Returns the trace of the matrix
 */
uint32_t get_trace(const uint32_t* matrix) {
	
	int row_index = 0;
	int row = -1;
	uint32_t trace = 0;
	for (ssize_t i = 0; i < g_elements; i++) {

		if(i % g_width == 0) {
			row_index = 0;
			row++;
		}

		if(row == row_index) {
			trace += matrix[i];
		}



		row_index++;

   	 }
	

    /*
        to do

        1 0
        0 1 => 2

        2 1
        1 2 => 4
    */

    return trace;
}

/**
 * Returns the smallest value in the matrix
 */
uint32_t get_minimum(const uint32_t* matrix) {

	uint32_t minimum = UINT32_MAX;

	for (ssize_t i = 0; i < g_elements; i++) {
        if(matrix[i] < minimum) {
        	minimum = matrix[i];
        }
   	 }

    /*
        to do

        1 2
        3 4 => 1

        4 3
        2 1 => 1
    */

    return minimum;
}

/**
 * Returns the largest value in the matrix
 */
uint32_t get_maximum(const uint32_t* matrix) {

	uint32_t max = 0;

	for (ssize_t i = 0; i < g_elements; i++) {
        if(matrix[i] > max) {
        	max = matrix[i];
        }
   	 }

    /*
        to do

        1 2
        3 4 => 4

        4 3
        2 1 => 4
    */

    return max;
}

/**
 * Returns the frequency of the value in the matrix
 */
uint32_t get_frequency(const uint32_t* matrix, uint32_t value) {

	int count = 0;

	for (ssize_t i = 0; i < g_elements; i++) {
        if(matrix[i] == value) {
        	count++;
        }
   	 }

    /*
        to do

        1 1
        1 1 :: 1 => 4

        1 0
        0 1 :: 2 => 0
    */

    return count;
}
