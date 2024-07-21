/*
 * -----------------------------------------------------------------------------
 * Project       : ring_buffer
 * File          : ring_buffer.h
 * Author        : nktsb
 * Created on    : 12.03.2024
 * GitHub        : https://github.com/nktsb/ring_buffer
 * -----------------------------------------------------------------------------
 * Copyright (c) 2024 nktsb
 * All rights reserved.
 * -----------------------------------------------------------------------------
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
    extern "C" {
#endif

// Structure representing the ring buffer
typedef struct
{
    unsigned length;       // Total length of the buffer
    unsigned input;        // Index for the next input position
    unsigned output;       // Index for the next output position
    void *data;           // Pointer to the buffer data
    size_t element_size;  // Size of each element in the buffer (in bytes)
} ring_buffer_st;

// Enumeration for ring buffer error codes
typedef enum {
    ERR_RING_BUFF_NULL = -3, // Error: buffer is NULL
    ERR_RING_BUFF_EMPTY,      // Error: buffer is empty
    ERR_RING_BUFF_FULL,       // Error: buffer is full
    RING_BUFF_OK              // Operation successful
} ring_buff_err_t;

/**
 * @brief Initializes a ring buffer.
 *
 * @param element_size Size of each element in the buffer (in bytes).
 * @param length Total number of elements the buffer can hold.
 * @return Pointer to the initialized ring buffer, or NULL on failure.
 */
ring_buffer_st * ringBufferInit(size_t element_size, unsigned length);

/**
 * @brief Deinitializes the ring buffer and frees allocated memory.
 *
 * @param buffer Pointer to the ring buffer to deinitialize.
 * @return Error code indicating the result of the operation.
 */
ring_buff_err_t ringBufferDeinit(ring_buffer_st *buffer);

/**
 * @brief Adds a single element to the ring buffer.
 *
 * @param buffer Pointer to the ring buffer.
 * @param data Pointer to the data to be added.
 * @return Error code indicating the result of the operation.
 */
ring_buff_err_t ringBufferPutSymbol(ring_buffer_st* buffer, void *data);

/**
 * @brief Retrieves a single element from the ring buffer.
 *
 * @param buffer Pointer to the ring buffer.
 * @param data Pointer to the location where the retrieved data will be stored.
 * @return Error code indicating the result of the operation.
 */
ring_buff_err_t ringBufferGetSymbol(ring_buffer_st* buffer, void *data);

/**
 * @brief Adds multiple elements to the ring buffer.
 *
 * @param buffer Pointer to the ring buffer.
 * @param data Pointer to the data to be added.
 * @param data_len Number of elements to add.
 * @return Error code indicating the result of the operation.
 */
ring_buff_err_t ringBufferPutData(ring_buffer_st *buffer, void *data, unsigned data_len);

/**
 * @brief Retrieves multiple elements from the ring buffer.
 *
 * @param buffer Pointer to the ring buffer.
 * @param data Pointer to the location where the retrieved data will be stored.
 * @param data_len Number of elements to retrieve.
 * @return Error code indicating the result of the operation.
 */
ring_buff_err_t ringBufferGetData(ring_buffer_st *buffer, void *data, unsigned data_len);

/**
 * @brief Gets the number of available elements in the ring buffer.
 *
 * @param buffer Pointer to the ring buffer.
 * @return Number of available elements.
 */
int ringBufferGetAvail(ring_buffer_st *buffer);

/**
 * @brief Clears the ring buffer, resetting input and output indices.
 *
 * @param buffer Pointer to the ring buffer.
 * @return Error code indicating the result of the operation.
 */
ring_buff_err_t ringBufferClear(ring_buffer_st *buffer);

#ifdef __cplusplus
    }
#endif

#endif /* RING_BUFFER_H_ */