/*
ujson4c decoder helper 1.0
Copyright (c) 2013, Electronic Arts Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the Electronic Arts Inc. nor the
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
Copyright (c) 2011-2013, ESN Social Software AB and Jonas Tarnstrom
All rights reserved.
www.github.com/esnme/ultrajson
*/

#include <stdio.h>
#include "ujdecode.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

void prefixLine(int level)
{
	int index;
	for (index = 0; index < level; index ++)
	{
		fputc(' ', stdout);
		fputc(' ', stdout);
	}
}

void dumpObject(int level, void *state, UJObject obj)
{
	switch (UJGetType(obj))
	{
	case UJT_Null:
		{
			prefixLine(level);
			fprintf (stdout, "NULL\n");
			break;
		}
	case UJT_True:
		{
			prefixLine(level);
			fprintf (stdout, "True\n");
			break;
		}
	case UJT_False:
		{
			prefixLine(level);
			fprintf (stdout, "False\n");
			break;
		}
	case UJT_Long:
		{
			prefixLine(level);
			fprintf (stdout, "%ld\n", UJGetLong(obj));
			break;
		}
	case UJT_LongLong:
		{
			prefixLine(level);
			fprintf (stdout, "%lld\n", UJGetLongLong(obj));
			break;
		}
	case UJT_Double:
		{
			prefixLine(level);
			fprintf (stdout, "%f\n", UJGetDouble(obj));
			break;
		}
	case UJT_String:
		{
			size_t len;
			fwprintf (stdout, L"%s\n", UJGetString(obj, &len));
			break;
		}
	case UJT_Array:
		{
			void *iter = NULL;
			UJObject objiter = NULL;

			fputc('[', stdout);

			iter = UJBeginArray(obj);

			while(UJIterArray(&iter, &objiter))
			{
				dumpObject(level + 1, state, objiter);
			}

			fputc(']', stdout);

			break;
		}
	case UJT_Object:
		{
			void *iter = NULL;
			UJObject objiter = NULL;
			UJString key;

			prefixLine(level);
			fputc('{', stdout);

			iter = UJBeginObject(obj);

			while(UJIterObject(&iter, &key, &objiter))
			{
				fwprintf (stdout, L"%s: ", key.ptr);
				dumpObject(level + 1, state, objiter);
			}

			fputc('}', stdout);
			break;
		}
	}

}

#ifdef __BENCHMARK__
int main ()
{
	const char *input;
	size_t cbInput;
	void *state;
	FILE *file;
	char buffer[32768];
	time_t tsNow;
	int count;
	int once = 1;
	size_t bytesDecoded;

	UJHeapFuncs hf;
	hf.cbInitialHeap = sizeof(buffer);
	hf.initalHeap = buffer;
	hf.free = free;
	hf.malloc = malloc;
	hf.realloc = realloc;

	file = fopen("./sample.json", "rb");
	input = (char *) malloc(1024 * 1024);
	cbInput = fread ( (void *) input, 1, 1024 * 1024, file);
	fclose(file);
	
	tsNow = time(0);
	count = 0;
	bytesDecoded = 0;

	for (;;)
	{
		UJObject obj;

		obj = UJDecode(input, cbInput, NULL, &state);

		if (once)
		{
			dumpObject(0, state, obj);
			once = 0;
		}

		UJFree(state);

		count ++;
		bytesDecoded += cbInput;

		if (tsNow != time(0))
		{
			float bps = (float) bytesDecoded;
			fprintf (stderr, "Count %d, MBps: %f\n", count, bps / 1000000.0f);
			
			count = 0;
			bytesDecoded = 0;
			tsNow = time(0);
		}
	}
}
#endif