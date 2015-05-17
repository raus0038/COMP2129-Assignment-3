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

struct matrix_add {
    uint32_t* matrix;
    uint32_t scalar;
    uint32_t tid;
};
struct matrix_scalar_mul {
    
    uint32_t* result;
    const uint32_t* matrix;
    uint32_t tid;
    uint32_t scalar;

};

struct matrix_trace {
   const  uint32_t* matrix;
    uint32_t scalar;
    uint32_t tid;
    

};

struct matrix_addition {
    
    const uint32_t* matrix_a;
    const uint32_t* matrix_b;
    uint32_t*  result;
    uint32_t tid;

};

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

static void *uniform_worker(void* arg) {
    
    struct matrix_add *matrix = (struct matrix_add *) arg;
    
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;

    for(size_t i = start; i < end; i++) {
        matrix->matrix[i] = matrix->scalar;
    }

    return NULL;

}

uint32_t* uniform_matrix(uint32_t value) {

    uint32_t* result = new_matrix();

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
            .scalar = value
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, uniform_worker, m_add + i) != 0) {
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

struct matrix_clone {
    uint32_t tid;
    const uint32_t* toClone;
    uint32_t* result;
};

static void* clone_worker(void* arg) {

    struct matrix_clone* matrix = (struct matrix_clone*) arg;
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;
    

    for(size_t i = start; i < end;i++) {
       matrix->result[i] = matrix->toClone[i];
    }

    return NULL;

}


uint32_t* cloned(const uint32_t* matrix) {

    pthread_t thread_id[g_nthreads];
    uint32_t* result = new_matrix();

    struct matrix_clone *m_add = malloc(sizeof(struct matrix_clone) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return result;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_clone) {
            .toClone = matrix,
            .tid = i,
            .result = result
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, clone_worker, m_add + i) != 0) {
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



/*static void *scalar_worker(void *arg) {
    struct matrix_scalar_mul *matrix = (struct matrix_scalar_mul *) arg;
    
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;

    for(size_t i = start; i < end; i++) {
        matrix->result[i] = matrix->matrix[i] + matrix->scalar;
    }

    return NULL;
}

static void *multiply_worker(void *arg) {
    
    struct matrix_scalar_mul *matrix = (struct matrix_scalar_mul *) arg;
    
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;

    for(size_t i = start; i < end; i++) {
        matrix->result[i] = matrix->matrix[i] * matrix->scalar;
    }

    return NULL;


}*/

/**
 * Returns new matrix with scalar added to each element
 */
uint32_t* scalar_add(const uint32_t* matrix, uint32_t scalar) {


    uint32_t* result = cloned(matrix);

   /* 
    pthread_t thread_id[g_nthreads];

    struct matrix_scalar_mul  *m_add = malloc(sizeof(struct matrix_scalar_mul) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return result;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_scalar_mul) {
            .matrix = matrix,
            .result = result,
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
    
*/
    for(size_t i = 0; i < g_elements; i++) {
        result[i] = matrix[i] + scalar;
    }
    return result;
    
    /*
        to do

        1 0        2 1
        0 1 + 1 => 1 2

        1 2        5 6
        3 4 + 4 => 7 8
    */
}

/**
 * Returns new matrix with scalar multiplied to each element
 */

uint32_t* scalar_mul(const uint32_t* matrix, uint32_t scalar) {

    uint32_t* result = cloned(matrix);

    
    /*pthread_t thread_id[g_nthreads];

    struct matrix_scalar_mul  *m_add = malloc(sizeof(struct matrix_scalar_mul) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return result;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_scalar_mul) {
            .matrix = matrix,
            .result = result,
            .tid = i,
            .scalar = scalar
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, multiply_worker, m_add + i) != 0) {
            perror("Thread creation failed");
            return result;
        }
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_join(thread_id[i], NULL) != 0) {
            return result;
        }
    }
    free(m_add);*/
    

    for(size_t i = 0; i < g_elements; i++) {
        result[i] = matrix[i] * scalar;
    }
    return result;
    /*
        to do

        1 0        2 0
        0 1 x 2 => 0 2

        1 2        2 4
        3 4 x 2 => 6 8
    */

}

/**
 * Returns new matrix with elements added at the same index
 */

void* matrix_addition_worker (void* arg) {
    
    struct matrix_addition *matrix = (struct matrix_addition *) arg;
    
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;

    for(size_t i = start; i < end; i++) {
        matrix->result[i] = matrix->matrix_a[i] +  matrix->matrix_b[i];
    }

    return NULL;
    

}
uint32_t* matrix_add(const uint32_t* matrix_a, const uint32_t* matrix_b) {
    
    uint32_t* result = new_matrix();

    
    pthread_t thread_id[g_nthreads];

    struct matrix_addition  *m_add = malloc(sizeof(struct matrix_addition) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return result;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_addition) {
            .matrix_a = matrix_a,
            .matrix_b = matrix_b,
            .result = result,
            .tid = i
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, matrix_addition_worker, m_add + i) != 0) {
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

        1 0   0 1    1 1
        0 1 + 1 0 => 1 1

        1 2   4 4    5 6
        3 4 + 4 4 => 7 8
    */

}

/**
 * Returns new matrix, multiplying the two matrices together
 */


struct matrix_mul {
    const uint32_t* matrix_a;
    const uint32_t* matrix_b;
    uint32_t* result;
    uint32_t tid;
};


static void* mul_worker(void* arg) {
        
        struct matrix_mul *mul_data = arg;
        int row_count = g_width / g_nthreads;
        int row = mul_data->tid * (g_width / g_nthreads) ;
        
        for(int k = row; k < row + row_count; k++) {        
        for(int i = 0; i < g_width; i++) {
            int sum = 0;
            for(int j = 0; j < g_width; j++) {
                sum += mul_data->matrix_a[k * g_width + j] * mul_data->matrix_b[j * g_width + i];
                mul_data->result[k *g_width + i] = sum;
            }
        }
        }

        return NULL;

}
uint32_t* matrix_mul(const uint32_t* matrix_a, const uint32_t* matrix_b) {
    
    uint32_t* result = new_matrix();

    
    pthread_t thread_id[g_nthreads];

    struct matrix_mul *m_add = malloc(sizeof(struct matrix_mul) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return result;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_mul) {
            .result = result,
            .matrix_a = matrix_a,
            .matrix_b = matrix_b,
            .tid = i,
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, mul_worker, m_add + i) != 0) {
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
    
    for(int i = 1; i < exponent; i++) {
       result= matrix_mul(result, matrix); 
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

/*static void* sum_worker(void * arg) {
    
    
    struct matrix_add *matrix = (struct matrix_add *) arg;
    
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;

    for(size_t i = start; i < end; i++) {
      matrix->scalar += matrix->matrix[i];
    }

    return NULL;


}
*/

uint32_t get_sum(const uint32_t* matrix) {

   // pthread_t thread_id[g_nthreads];
    uint32_t sum = 0;
    
    /*struct matrix_add *m_add = malloc(sizeof(struct matrix_add) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return 0;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_add) {
            .matrix = cloned(matrix),
            .tid = i,
            .scalar = 0
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, sum_worker, m_add + i) != 0) {
            perror("Thread creation failed");
            return 0;
        }
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_join(thread_id[i], NULL) != 0) {
            return 0;
        }
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        sum += m_add[i].scalar;
    }

    free(m_add);*/


   for(size_t i = 0; i < g_elements; i++) {
        sum += matrix[i];
   }    

    return sum;
    
 }

    /*
        to do

        1 2
        2 1 => 6

        1 1
        1 1 => 4
    */

/**
 * Returns the trace of the matrix
 */

void* trace_worker(void* arg) {

    struct matrix_trace *matrix = (struct matrix_trace *) arg;
    
    const size_t start = matrix->tid * g_nthreads / g_width;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_width : (matrix->tid + 1) * g_nthreads / g_width;
     

    for(size_t i = start; i < end; i++) {

       matrix->scalar += matrix->matrix[i * g_width + i];
    }

    return NULL;
    

}

uint32_t get_trace(const uint32_t* matrix) {

    uint32_t trace = 0
        ;
    pthread_t thread_id[g_nthreads];
    
    struct matrix_trace *m_add = malloc(sizeof(struct matrix_trace) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return 0;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_trace) {
            .matrix = matrix,
            .tid = i,
            .scalar = 0
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, trace_worker, m_add + i) != 0) {
            perror("Thread creation failed");
            return 0;
        }
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_join(thread_id[i], NULL) != 0) {
            return 0;
        }
    }
    
    for(size_t i = 0; i < g_nthreads; i++) {
        trace += m_add[i].scalar;
    }

    free(m_add);
    

    return trace;


}

/**
 * Returns the smallest value in the matrix
 */

static void* min_worker(void * arg) {
    
    
    struct matrix_add *matrix = (struct matrix_add *) arg;
    
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;
     
    uint32_t minimum = UINT32_MAX;

    for(size_t i = start; i < end; i++) {
       if(matrix->matrix[i] < minimum) {
             minimum = matrix->matrix[i];
             matrix->scalar = matrix->matrix[i];
       }
    }

    return NULL;


}

uint32_t get_minimum(const uint32_t* matrix) {

    pthread_t thread_id[g_nthreads];
    
    struct matrix_trace *m_add = malloc(sizeof(struct matrix_trace) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return 0;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_trace) {
            .matrix = matrix,
            .tid = i,
            .scalar = 0
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, min_worker, m_add + i) != 0) {
            perror("Thread creation failed");
            return 0;
        }
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_join(thread_id[i], NULL) != 0) {
            return 0;
        }
    }
    
    uint32_t minimum = UINT32_MAX;


    for(size_t i = 0; i < g_nthreads; i++) {
        if(m_add[i].scalar < minimum) {
            minimum = m_add[i].scalar;
        }
    }

    free(m_add);
    

    return minimum;
    
    /*
        to do

        1 2
        3 4 => 1

        4 3
        2 1 => 1
    */

}

/**
 * Returns the largest value in the matrix
 */

struct matrix_max {
    uint32_t tid;
    uint32_t scalar;
    const uint32_t* matrix;

};
static void* max_worker(void * arg) {
    
    
    struct matrix_max *matrix = (struct matrix_max *) arg;
    
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;
     
    uint32_t max = 0;

    for(size_t i = start; i < end; i++) {
       if(matrix->matrix[i] > max) {
             max = matrix->matrix[i];
             matrix->scalar = max;
       }
    }

    return NULL;


}

uint32_t get_maximum(const uint32_t* matrix) {

    pthread_t thread_id[g_nthreads];
    
    struct matrix_max *m_add = malloc(sizeof(struct matrix_max) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return 0;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_max) {
            .matrix = matrix,
            .tid = i,
            .scalar = 0
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, max_worker, m_add + i) != 0) {
            perror("Thread creation failed");
            return 0;
        }
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_join(thread_id[i], NULL) != 0) {
            return 0;
        }
    }
    
    uint32_t max = 0;


    for(size_t i = 0; i < g_nthreads; i++) {
        if(m_add[i].scalar > max) {
            max = m_add[i].scalar;
        }
    }

    free(m_add);
    

    return max;

    /*
        to do

        1 2
        3 4 => 4

        4 3
        2 1 => 4
    */

}

/**
 * Returns the frequency of the value in the matrix
 */

struct matrix_freq {
    
    const uint32_t* matrix;
    uint32_t tid;
    uint32_t scalar;
    uint32_t count;

};

void* frequency_worker(void* arg) {
        
    struct matrix_freq* matrix = (struct matrix_freq*) arg;
 
    const size_t start = matrix->tid * g_elements / g_nthreads;
    const size_t end = matrix->tid == g_nthreads - 1 ? g_elements : (matrix->tid + 1) * g_elements / g_nthreads;
     

    uint32_t count = 0;
    for(size_t i = start; i < end; i++) {
       if(matrix->matrix[i] == matrix->scalar) {
             count++;
             matrix->count = count;
       }
    }

    return NULL;
    

}


uint32_t get_frequency(const uint32_t* matrix, uint32_t value) {

    pthread_t thread_id[g_nthreads];
    
    struct matrix_freq  *m_add = malloc(sizeof(struct matrix_freq) * g_nthreads);
    if(!m_add) {
        perror("malloc");
         return 0;
     }

    for(int i = 0; i < g_nthreads; i++) {
        m_add[i] = (struct matrix_freq) {
            .matrix = matrix,
            .tid = i,
            .scalar = value,
            .count = 0
        };
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_create(thread_id + i, NULL, frequency_worker, m_add + i) != 0) {
            perror("Thread creation failed");
            return 0;
        }
    }

    for(size_t i = 0; i < g_nthreads; i++) {
        if(pthread_join(thread_id[i], NULL) != 0) {
            return 0;
        }
    }
    
    uint32_t count = 0;


    for(size_t i = 0; i < g_nthreads; i++) {
            count += m_add[i].count;
    }

    free(m_add);
    

    return count;


}







