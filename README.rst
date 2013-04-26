ujson4c
=============
A more user friendly layer for decoding JSON in C/C++ based on the ultra fast UltraJSON library

============
Usage
============
Copy all of the files from /src and /3rdparty into a folder of choice in your own project. #include ujdecode.h read more about the API in ujdecode.h

.. code-block:: 
    UJObject obj;
    void *state;
    const char input[] = "{\"name\": \"John Doe\", \"age\": 31, \"number\": 1337.0, \"address\": { \"city\": \"Uppsala\"} }";
    size_t cbInput = sizeof(input) - 1;
    const wchar_t *personKeys[4] = { L"name", L"age", L"number", L"address"};
    
    UJObject personObjs[4];
    obj = UJDecode(input, cbInput, NULL, &state);
    
    if (UJObjectUnpack(obj, 4, "SNNO", personKeys, personObjs) == 4)
    {
        const wchar_t *addressKeys[1] = { L"city" };
        UJObject addressObjs[1];
    
        const wchar_t *name = UJGetString(personObjs[0], NULL);
        int age = UJGetNumericAsInteger(personObjs[1]);
        double number = UJGetNumericAsDouble(personObjs[2]);
    
        if (UJObjectUnpack(personObjs[3], 1, "S", addressKeys, addressObjs) == 1)
        {
            const wchar_t *city = UJGetString(addressObjs[0], NULL);
        }
    }
    UJFree(state);
        