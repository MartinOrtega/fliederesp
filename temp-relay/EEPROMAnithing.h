#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

template <class T> int EEPROM_writeAnything(int address, const T& value)
{
    const byte* valuePointer = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++){
          EEPROM.write(address++, *valuePointer++);
    }
    EEPROM.commit();
    return i;
}

template <class T> int EEPROM_readAnything(int address, T& value)
{
    byte* valuePointer = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *valuePointer++ = EEPROM.read(address++);
    return i;
}

const char* EEPROM_readCharArray(int initAddress, int endAddress)
{
  String str;
  for (int i = initAddress; i < endAddress; ++i)
    {
      str += char(EEPROM.read(i));
    }
  return str.c_str();
} 
