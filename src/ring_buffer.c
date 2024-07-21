/*
 * -----------------------------------------------------------------------------
 * Project       : ring_buffer
 * File          : ring_buffer.c
 * Author        : nktsb
 * Created on    : 12.03.2024
 * GitHub        : https://github.com/nktsb/ring_buffer
 * -----------------------------------------------------------------------------
 * Copyright (c) 2024 nktsb
 * All rights reserved.
 * -----------------------------------------------------------------------------
 */

#include "ring_buffer.h"

ring_buffer_st * ringBufferInit(size_t element_size, unsigned length)
{
	ring_buffer_st *buffer = malloc(sizeof(ring_buffer_st));
	if(buffer == NULL) return NULL;

	memset((void*)buffer, 0, sizeof(ring_buffer_st));

	buffer->data = malloc(length * element_size);
	if(buffer->data == NULL)
	{
		free(buffer);
		return NULL;
	}

	memset((void*)buffer->data, 0, length * element_size);

	buffer->length = length;
	buffer->element_size = element_size;
  	ringBufferClear(buffer);

	return buffer;
}

ring_buff_err_t ringBufferDeinit(ring_buffer_st *buffer)
{
	if(buffer == NULL) return ERR_RING_BUFF_NULL;

	ringBufferClear(buffer);
	free(buffer->data);
	free(buffer);

	return RING_BUFF_OK;
}

ring_buff_err_t ringBufferClear(ring_buffer_st* buffer)
{
	if(buffer == NULL) return ERR_RING_BUFF_NULL;

	buffer->input = 0;
	buffer->output = 0;

	return RING_BUFF_OK;
}

int ringBufferGetAvail(ring_buffer_st *buffer)
{
	if(buffer == NULL) return -1;

	return (buffer->input >= buffer->output)? 
			buffer->input - buffer->output : ((buffer->length - buffer->output) + buffer->input);
}

ring_buff_err_t ringBufferPutSymbol(ring_buffer_st* buffer, void *data)
{
	if(buffer == NULL) return ERR_RING_BUFF_NULL;
	if(buffer->length == ringBufferGetAvail(buffer)) return ERR_RING_BUFF_FULL;

	void *last_data_ptr = (uint8_t*)buffer->data + (buffer->input * buffer->element_size); 
	memcpy(last_data_ptr, data, buffer->element_size);

	buffer->input += 1;
	buffer->input %= buffer->length;

	return RING_BUFF_OK;
}

ring_buff_err_t ringBufferPutData(ring_buffer_st *buffer, void *data, unsigned data_len)
{
	if(buffer == NULL) return ERR_RING_BUFF_NULL;
	if((buffer->length - ringBufferGetAvail(buffer)) < data_len) return ERR_RING_BUFF_FULL;

	while(data_len)
	{
		unsigned round_max_len = (buffer->length - buffer->input);
		unsigned round_len = (data_len > round_max_len)? round_max_len: data_len;
		
		memcpy((void*)((uint8_t*)buffer->data + (buffer->input * buffer->element_size)), (void*)data, (round_len * buffer->element_size));
		data = (uint8_t*)data + (round_len * buffer->element_size);
		data_len -= round_len;

		buffer->input += round_len;
		buffer->input %= buffer->length;
	}
	return RING_BUFF_OK;
}

ring_buff_err_t ringBufferGetSymbol(ring_buffer_st* buffer, void *data)
{
	if(buffer == NULL) return ERR_RING_BUFF_NULL;
	if(ringBufferGetAvail(buffer) == 0) return ERR_RING_BUFF_EMPTY;

	void *last_data_ptr = (uint8_t*)buffer->data + (buffer->output * buffer->element_size); 
	memcpy(data, last_data_ptr, buffer->element_size);

	buffer->output += 1;
	buffer->output %= buffer->length;

	return RING_BUFF_OK;
}

ring_buff_err_t ringBufferGetData(ring_buffer_st *buffer, void *data, unsigned data_len)
{
	if (buffer == NULL) return ERR_RING_BUFF_NULL;
	if(ringBufferGetAvail(buffer) < data_len) return ERR_RING_BUFF_EMPTY;

	while(data_len)
	{
		unsigned round_max_len = (buffer->length - buffer->output);
		unsigned round_len = (data_len > round_max_len)? round_max_len: data_len;
		
		memcpy((void*)data, (void*)((uint8_t*)buffer->data + (buffer->output * buffer->element_size)), (round_len * buffer->element_size));
		data = (uint8_t*)data + (round_len * buffer->element_size);
		data_len -= round_len;

		buffer->output += round_len;
		buffer->output %= buffer->length;
	}
	return RING_BUFF_OK;
}
