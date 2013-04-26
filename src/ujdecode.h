/*
ujson4c decoder helper 1.0
Developed by ESN, an Electronic Arts Inc. studio. 
Copyright (c) 2013, Electronic Arts Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of ESN, Electronic Arts Inc. nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ELECTRONIC ARTS INC. BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Uses UltraJSON library:
Copyright (c) 2013, Electronic Arts Inc.
All rights reserved.
www.github.com/esnme/ultrajson
*/

#pragma once

#ifdef __cplusplus 
extern "C" {
#endif

enum UJTypes
{
	UJT_Null,
	UJT_True,
	UJT_False,
	UJT_Long,
	UJT_LongLong,
	UJT_Double,
	UJT_String,
	UJT_Array,
	UJT_Object
};

#include <wchar.h>
typedef void * UJObject;

typedef struct __UJString
{
	wchar_t *ptr;
	size_t cchLen;
} UJString;

typedef struct __UJHeapFuncs
{
	void *initalHeap;
	size_t cbInitialHeap;
	void *(*malloc)(size_t cbSize);
	void (*free)(void *ptr);
	void *(*realloc)(void *ptr, size_t cbSize);
} UJHeapFuncs;

/*
===============================================================================
Decodes an input text octet stream into a JSON object structure

Arguments:
  input - JSON data to decode in ANSI or UTF-8 format
  cbInput - Length of input in bytes
  hf - Heap functions, see UJHeapFuncs. Optional may be NULL
  outState - Outputs the decoder state.

Not about heap functions:
Declare a UJHeapFuncs structure on the stack if you want to manage your own heap
and pass it as the hf argument to UJDecode.
  initalHeap    - Pointer to a buffer for the initial heap handled by the caller. 
                  Preferably stored on the stack. MUST not be smaller than 1024 bytes
  cbInitialHeap - Size of the initial heap in bytes
  malloc        - Pointer to malloc function  
  free          - Pointer to free function
  realloc       - Pointer to realloc function  
  
Returns a JSON object structure representation or NULL in case of error.
Use UJGetError get obtain error message.

Example usage:

const char *input;
size_t cbInput;
void *state;

obj = UJDecode(input, cbInput, NULL, &state);

if (obj == NULL)
  printf ("Error: %s\n", UJGetError(state));

  ...Poke around in returned obj...

UJFree(state);
===============================================================================
*/
UJObject UJDecode(const char *input, size_t cbInput, UJHeapFuncs *hf, void **outState);

/*
===============================================================================
Called to free the decoder state
===============================================================================
*/
void UJFree(void *state);

/*
===============================================================================
Check if object is of certain type 
===============================================================================
*/
int UJIsNull(UJObject obj);
int UJIsTrue(UJObject obj);
int UJIsFalse(UJObject obj);
int UJIsLong(UJObject obj);
int UJIsLongLong(UJObject obj);
int UJIsInteger(UJObject *obj);
int UJIsDouble(UJObject obj);
int UJIsString(UJObject obj);
int UJIsArray(UJObject obj);
int UJIsObject(UJObject obj);

/*
===============================================================================
See UJTypes enum for possible return values
===============================================================================
*/
int UJGetType(UJObject obj);

/*
===============================================================================
Called to initiate the iterator of an array object
May return NULL in case array is empty
===============================================================================
*/
void *UJBeginArray(UJObject arrObj);

/*
===============================================================================
Iterates an array object

Arguments:
iter   - Anonymous iterator
outObj - Object in array

Usage:
Get initial iterator from call to UJBeginArray. 
Call this function until it returns 0
===============================================================================
*/
int UJIterArray(void **iter, UJObject *outObj);

/*
===============================================================================
Called to initiate the iterator of an Object (key-value structure)
May return NULL in case Object is empty

===============================================================================
*/
void *UJBeginObject(UJObject objObj);

/*
===============================================================================
Iterates an Object

Arguments:
iter     - Anonymous iterator
outKey   - Key name
outValue - Value object

Usage:
Get initial iterator from call to UJBeginObject
Call this function until it returns 0
===============================================================================
*/
int UJIterObject(void **iter, UJString *outKey, UJObject *outValue);

/*

/*
===============================================================================
Unpacks an Object by matching the key name with the requested format 

Each key name needs to be matched by the character in the format string 
representing the desired type of the value for that key
B - Boolean 
N - Numeric 
S - String
A - Array
O - Object
U - Unknown/any

Use lower case to accept JSON Null in place of the expected value.

Arguments:
objObj     - The object to unpack (JSON Object)
keys       - Number of keys to match. Keys can not exceed 64.
format     - Specified the expected types for the key value. 
keyNames   - An array of key names
outObjects - Output value objects

Return value:
Returns number of key pairs matched or -1 on error
===============================================================================
*/
int UJObjectUnpack(UJObject objObj, int keys, const char *format, const wchar_t **keyNames, UJObject *outObjects);

/*
===============================================================================
Returns the value of a double/decimal object as double 
Check for UJIsDouble before calling 
===============================================================================
*/
double UJGetDouble(UJObject obj);

/*
===============================================================================
Returns the value of a double, long and long long value as a double. 
If value is not any of these 0.0 is returned.
===============================================================================
*/
double UJGetNumericAsDouble(UJObject obj);

/*
===============================================================================
Returns the value of a double, long or long long value as an integer.
If value is not any of these types 0 is returned.

Truncation may arrise depending on machine word sizes and presence of decimals 
when converting double to int.
Check for UJIsDouble, UJIsLong or UJIsLongLong before calling 
===============================================================================
*/
int UJGetNumericAsInteger(UJObject obj);

/*
===============================================================================
Returns the value of an integer decoded as long (32-bit on most platforms).
Check for UJIsLong before calling 
===============================================================================
*/
long UJGetLong(UJObject obj);

/*
===============================================================================
Returns the value of an integer decoded as long long (64-bit) as a long long value.
Check for UJIsLongLong before calling 
===============================================================================
*/
long long UJGetLongLong(UJObject obj);

/*
===============================================================================
Returns the value of a string value as a wide character string pointer. Caller must NOT free returned pointer.
cchOutBuffer contains the character length of the returned string. 
Check for UJIsString before calling
===============================================================================
*/
wchar_t *UJGetString(UJObject obj, size_t *cchOutBuffer);

/*
===============================================================================
Returns last error message if any as a string or NULL. 
Caller must NOT free returned pointer.
===============================================================================
*/
const char *UJGetError(void *state);

#ifdef __cplusplus 
}
#endif