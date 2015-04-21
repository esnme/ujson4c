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

#include "ujdecode.h"
#include "ultrajson.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

typedef struct __Item
{
	int type;
} Item;

typedef struct __StringItem
{
	Item item;
	UJString str;
} StringItem;

typedef struct __KeyPair
{
	StringItem *name;
	Item *value;
	struct __KeyPair *next;
} KeyPair;

typedef struct __ObjectItem
{
	Item item;
	KeyPair *head;
	KeyPair *tail;
} ObjectItem;

typedef struct __ArrayEntry
{
	Item *item;
	struct __ArrayEntry *next;
} ArrayEntry;

typedef struct __ArrayItem
{
	Item item;
	ArrayEntry *head;
	ArrayEntry *tail;
} ArrayItem; 

typedef struct __LongValue
{
	Item item;
	long value;
} LongValue;

typedef struct __LongLongValue
{
	Item item;
	long long value;
} LongLongValue;

typedef struct __DoubleValue
{
	Item item;
	double value;
} DoubleValue;

typedef struct __NullValue
{
	Item item;
} NullValue;

typedef struct __FalseValue
{
	Item item;
} FalseValue;

typedef struct __TrueValue
{
	Item item;
} TrueValue;

typedef struct __HeapSlab
{
	unsigned char *start;
	unsigned char *offset;
	unsigned char *end;
	size_t size;
	char owned;
	struct __HeapSlab *next;
} HeapSlab;

struct DecoderState
{
	HeapSlab *heap;
	const char *error;
	void *(*malloc)(size_t cbSize);
	void (*free)(void *ptr);
};


static void *alloc(struct DecoderState *ds, size_t cbSize)
{
	unsigned char *ret;

	if (ds->heap->offset + cbSize > ds->heap->end)
	{
		size_t newSize = ds->heap->size * 2;
		HeapSlab *newSlab;

		while (newSize < (cbSize + sizeof (HeapSlab)))
			newSize *= 2;

		newSlab = (HeapSlab *) ds->malloc(newSize);
		newSlab->start = (unsigned char *) (newSlab + 1);
		newSlab->end = (unsigned char *) newSlab + newSize;
		newSlab->size = newSize;
		newSlab->offset = newSlab->start;
		newSlab->owned = 1;

		newSlab->next = ds->heap;
		ds->heap = newSlab;
	}


	ret = ds->heap->offset;
	ds->heap->offset += cbSize;

	return ret;
}

static JSOBJ newString(void* context, wchar_t *start, wchar_t *end)
{
	struct DecoderState *ds = context;
	size_t len;
	StringItem *si = (StringItem *) alloc(ds, sizeof(StringItem) + (end - start + 1) * sizeof(wchar_t));
	len = end - start;

	si->item.type = UJT_String;
	si->str.ptr = (wchar_t *) (si + 1);
	si->str.cchLen = len;

	if (len < 4)
	{
		wchar_t *dst = si->str.ptr;
		wchar_t *end = dst + len;

		while (dst < end)
		{
			*(dst++) = *(start++);
		}
	}
	else
	{
		memcpy (si->str.ptr, start, len * sizeof(wchar_t));
	}
	si->str.ptr[len] = '\0';
	return (JSOBJ) si;
}

static void objectAddKey(void* context, JSOBJ obj, JSOBJ name, JSOBJ value)
{
	struct DecoderState *ds = context;
	ObjectItem *oi = (ObjectItem *) obj;
	KeyPair *kp = (KeyPair *) alloc(ds, sizeof(KeyPair));

	kp->next = NULL;

	kp->name = (StringItem *) name;
	kp->value = (Item *) value;

	if (oi->tail)
	{
		oi->tail->next = kp;
	}
	else
	{
		oi->head = kp;
	}
	oi->tail = kp;
}

static void arrayAddItem(void* context, JSOBJ obj, JSOBJ value)
{
	struct DecoderState *ds = context;
	ArrayItem *ai = (ArrayItem *) obj;
	ArrayEntry *ae = (ArrayEntry *) alloc(ds, sizeof(ArrayEntry));

	ae->next = NULL;
	ae->item = (Item *) value;

	if (ai->tail)
	{
		ai->tail->next = ae;
	}
	else
	{
		ai->head = ae;
	}
	ai->tail = ae;

}

static JSOBJ newTrue(void* context)
{
	struct DecoderState *ds = context;
	TrueValue *tv = (TrueValue *) alloc(ds, sizeof(TrueValue *));
	tv->item.type = UJT_True;
	return (JSOBJ) tv;
}

static JSOBJ newFalse(void *context)
{
	struct DecoderState *ds = context;
	FalseValue *fv = (FalseValue *) alloc(ds, sizeof(FalseValue *));
	fv->item.type = UJT_False;
	return (JSOBJ) fv;
}

static JSOBJ newNull(void *context)
{
	struct DecoderState *ds = context;
	NullValue *nv = (NullValue *) alloc(ds, sizeof(NullValue *));
	nv->item.type = UJT_Null;
	return (JSOBJ) nv;
}

static JSOBJ newObject(void *context)
{
	struct DecoderState *ds = context;
	ObjectItem *oi = (ObjectItem *) alloc(ds, sizeof(ObjectItem));
	oi->item.type = UJT_Object;
	oi->head = NULL;
	oi->tail = NULL;

	return (JSOBJ) oi;
}

static JSOBJ newArray(void *context)
{
	struct DecoderState *ds = context;
	ArrayItem *ai = (ArrayItem *) alloc(ds, sizeof(ArrayItem));
	ai->head = NULL;
	ai->tail = NULL;
	ai->item.type = UJT_Array;
	return (JSOBJ) ai;
}

static JSOBJ newInt(void *context, JSINT32 value)
{
	struct DecoderState *ds = context;
	LongValue *lv = (LongValue *) alloc(ds, sizeof(LongValue));
	lv->item.type = UJT_Long;
	lv->value = (long) value;
	return (JSOBJ) lv;
}

static JSOBJ newLong(void *context, JSINT64 value)
{
	struct DecoderState *ds = context;
	LongLongValue *llv = (LongLongValue *) alloc(ds, sizeof(LongLongValue));
	llv->item.type = UJT_LongLong;
	llv->value = (long long) value;
	return (JSOBJ) llv;
}

static JSOBJ newDouble(void *context, double value)
{
	struct DecoderState *ds = context;
	DoubleValue *dv = (DoubleValue *) alloc(ds, sizeof(DoubleValue));
	dv->item.type = UJT_Double;
	dv->value = (double) value;
	return (JSOBJ) dv;
}

static void releaseObject(void *context, JSOBJ obj)
{
	struct DecoderState *ds = context;
	//NOTE: Fix for C4100 warning in L4 MSVC
	ds = NULL;
	obj = NULL;
}

static double GetDouble(UJObject obj)
{
	return ((DoubleValue *) obj)->value;
}

static long GetLong(UJObject obj)
{
	return ((LongValue *) obj)->value;
}

static long long GetLongLong(UJObject obj)
{
	return ((LongLongValue *) obj)->value;
}

void UJFree(void *state)
{
	struct DecoderState *ds = (struct DecoderState *) state;

	HeapSlab *slab = ds->heap;
	HeapSlab *next;
	while (slab)
	{
		next = slab->next;

		if (slab->owned)
		{
			ds->free(slab);
		}

		slab = next;
	}
}

int UJIsNull(UJObject obj)
{
	if (((Item *) obj)->type == UJT_Null)
	{
		return 1;
	}

	return 0;
}

int UJIsTrue(UJObject obj)
{
	if (((Item *) obj)->type == UJT_True)
	{
		return 1;
	}
	
	return 0;
}

int UJIsFalse(UJObject obj)
{
	if (((Item *) obj)->type == UJT_False)
	{
		return 1;
	}
	

	return 0;
}

int UJIsLong(UJObject obj)
{
	if (((Item *) obj)->type == UJT_Long)
	{
		return 1;
	}

	return 0;
}

int UJIsLongLong(UJObject obj)
{
	if (((Item *) obj)->type == UJT_LongLong)
	{
		return 1;
	}

	return 0;
}

int UJIsInteger(UJObject *obj)
{
	if (((Item *) obj)->type == UJT_LongLong ||
		((Item *) obj)->type == UJT_Long)
	{
		return 1;
	}

	return 0;
}

int UJIsDouble(UJObject obj)
{
	if (((Item *) obj)->type == UJT_Double)
	{
		return 1;
	}

	return 0;
}

int UJIsString(UJObject obj)
{
	if (((Item *) obj)->type == UJT_String)
	{
		return 1;
	}

	return 0;
}

int UJIsArray(UJObject obj)
{
	if (((Item *) obj)->type == UJT_Array)
	{
		return 1;
	}

	return 0;
}

int UJIsObject(UJObject obj)
{
	if (((Item *) obj)->type == UJT_Object)
	{
		return 1;
	}

	return 0;
}

void *UJBeginArray(UJObject arrObj)
{
	switch ( ((Item *) arrObj)->type)
	{
	case UJT_Array: return ((ObjectItem *) arrObj)->head;
	default: break;
	}

	return NULL;
}

int UJIterArray(void **iter, UJObject *outObj)
{
	ArrayEntry *ae = (ArrayEntry *) *iter;

	if (ae == NULL)
	{
		return 0;
	}

	*iter = ae->next;
	*outObj = ae->item;

	return 1;
}

void *UJBeginObject(UJObject objObj)
{
	switch ( ((Item *) objObj)->type)
	{
	case UJT_Object: return ((ObjectItem *) objObj)->head;
	default: break;
	}

	return NULL;
}

int UJIterObject(void **iter, UJString *outKey, UJObject *outValue)
{
	KeyPair *kp;

	if (*iter == NULL)
	{
		return 0;
	}

	kp = (KeyPair *) *iter;

	if (kp == NULL)
	{
		return 0;
	}

	*outKey = ((StringItem *) kp->name)->str;
	*outValue = kp->value;
	*iter = kp->next;
	return 1;
}

long long UJNumericLongLong(UJObject obj)
{
	switch ( ((Item *) obj)->type)
	{
	case UJT_Long: return (long long) GetLong(obj);
	case UJT_LongLong: return (long long) GetLongLong(obj);
	case UJT_Double: return (long long) GetDouble(obj);
	default: break;
	}

	return 0;
}

int UJNumericInt(UJObject obj)
{
	switch ( ((Item *) obj)->type)
	{
	case UJT_Long: return (int) GetLong(obj);
	case UJT_LongLong: return (int) GetLongLong(obj);
	case UJT_Double: return (int) GetDouble(obj);
	default: break;
	}

	return 0;
}

double UJNumericFloat(UJObject obj)
{
	switch ( ((Item *) obj)->type)
	{
	case UJT_Long: return (double) GetLong(obj);
	case UJT_LongLong: return (double) GetLongLong(obj);
	case UJT_Double: return (double) GetDouble(obj);
	default: break;
	}

	return 0.0;
}

const wchar_t *UJReadString(UJObject obj, size_t *cchOutBuffer)
{
	switch ( ((Item *) obj)->type)
	{
	case UJT_String:
		if (cchOutBuffer)
			*cchOutBuffer = ( (StringItem *) obj)->str.cchLen;
		return ( (StringItem *) obj)->str.ptr;

	default:
		break;
	}

	if (cchOutBuffer)
		*cchOutBuffer = 0;
	return L"";
}

const char *UJGetError(void *state)
{
	if (state == NULL)
		return NULL;

	return ( (struct DecoderState *) state)->error;
}

int UJGetType(UJObject obj)
{
	return ((Item *) obj)->type;
}

static int checkType(int ki, const char *format, UJObject obj)
{
	int c = (unsigned char) format[ki];
	int type = UJGetType(obj);
	int allowNull = 0;

	switch (c)
	{
	case 'b':
		allowNull = 1;
	case 'B':
		switch (type)
		{
			case UJT_Null:
				if (!allowNull)
					return 0;
			case UJT_True:
			case UJT_False:
				return 1;

			default:
				return 0;
		}
		break;

	case 'n':
		allowNull = 1;
	case 'N':
		switch (type)
		{
			case UJT_Null:
				if (!allowNull)
					return 0;
			case UJT_Long:
			case UJT_LongLong:
			case UJT_Double:
				return 1;

			default:
				return 0;
		}
		break;

	case 's':
		allowNull = 1;
	case 'S':
		switch (type)
		{
			case UJT_Null:
				if (!allowNull)
					return 0;
			case UJT_String:
				return 1;

			default:
				return 0;
		}
		break;

	case 'a':
		allowNull = 1;
	case 'A':
		switch (type)
		{
			case UJT_Null:
				if (!allowNull)
					return 0;
			case UJT_Array:
				return 1;

			default:
				return 0;
		}
		break;

	case 'o':
		allowNull = 1;
	case 'O':
		switch (type)
		{
			case UJT_Null:
				if (!allowNull)
					return 0;
			case UJT_Object:
				return 1;
			default:
				return 0;
		}

		break;

	case 'u':
		allowNull = 1;
	case 'U':
		return 1;
		break;
	}

	return 0;
}

int UJObjectUnpack(UJObject objObj, int keys, const char *format, const wchar_t **_keyNames, ...)
{
	void *iter;
	UJString key;
	UJObject value;
	int found = 0;
	int ki;
	int ks = 0;
	const wchar_t *keyNames[64];
  va_list args;
  UJObject *outValues[64];


  if (!UJIsObject(objObj))
	{
		return 0;
	}
  
	iter = UJBeginObject(objObj);

	if (keys > 64)
	{
		return -1;
	}

	va_start(args, _keyNames);
	for (ki = 0; ki < keys; ki ++)
	{
		keyNames[ki] = _keyNames[ki];
		outValues[ki] = va_arg(args, UJObject *);
	}
	va_end(args);
	
	while (UJIterObject(&iter, &key, &value))
	{
		for (ki = ks; ki < keys; ki ++)
		{
			const wchar_t *kn = keyNames[ki];

			if (kn == NULL)
			{
				continue;
			}

			if (wcscmp(key.ptr, kn) != 0)
			{
				continue;
			}

			if (!checkType(ki, format, value))
			{
				continue;
			}

			found ++;

			if (outValues[ki] != NULL)
			{
				*outValues[ki] = value;
			}
			keyNames[ki] = NULL;

			if (ki == ks)
			{
				ks ++;
			}
		}
	}

	return found;
}

UJObject UJDecode(const char *input, size_t cbInput, UJHeapFuncs *hf, void **outState)
{
	UJObject ret;
	struct DecoderState *ds;
	void *initialHeap;
	size_t cbInitialHeap;
	HeapSlab *slab;

	JSONObjectDecoder decoder = {
		newString,
		objectAddKey,
		arrayAddItem,
		newTrue,
		newFalse,
		newNull,
		newObject,
		newArray,
		newInt,
		newLong,
		newDouble,
		releaseObject,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		0, 
		NULL
	};

	if (hf == NULL)
	{
		decoder.malloc = malloc;
		decoder.free = free;
		decoder.realloc = realloc;
		cbInitialHeap = 16384;
		initialHeap = malloc(cbInitialHeap);
	}
	else
	{
		decoder.malloc = hf->malloc;
		decoder.free = hf->free;
		decoder.realloc = hf->realloc;
		initialHeap = hf->initalHeap;
		cbInitialHeap = hf->cbInitialHeap;
	
		if (cbInitialHeap < sizeof(HeapSlab) + sizeof(struct DecoderState))
		{
			return NULL;
		}
	}

	*outState = NULL;

	slab = (HeapSlab * ) initialHeap;
	slab->start = (unsigned char *) (slab + 1);
	slab->offset = slab->start;
	slab->end = (unsigned char *) initialHeap + cbInitialHeap;
	slab->size = cbInitialHeap;
	slab->owned = hf == NULL ? 1 : 0;
	slab->next = NULL;

	ds = (struct DecoderState *) slab->offset;
	slab->offset += sizeof(struct DecoderState);
	*outState = (void *) ds;

	ds->heap = slab;
	
	ds->malloc = decoder.malloc;
	ds->free = decoder.free;
	ds->error = NULL; 

	decoder.prv = (void *) ds;

	ret = (UJObject) JSON_DecodeObject(&decoder, input, cbInput);

	if (ret == NULL)
	{
		ds->error = decoder.errorStr;
	}

	return ret;
}
